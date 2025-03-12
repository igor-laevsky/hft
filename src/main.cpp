#include "defer.hpp"
#include "coinbase_ticker.hpp"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

#include <curl.h>

#include <iostream>
#include <cassert>
#include <iomanip>
#include <thread>

using namespace utils;
using namespace rapidjson;
using namespace coinbase;

__attribute__((always_inline))
Price next_ema_5sec(Price prev_ema, Price new_price, uint64_t dt_us) {
    // this really shouldn't be part of the loop, but fine for now
    if (prev_ema == 0) [[unlikely]]
        return new_price;

    // 5 seconds moving average interval
    constexpr uint64_t sampling_interval_us = 5 * 1e6;

    // dt/T ~== 1-e**(-dt/T)
    const Price a = (Price)dt_us / sampling_interval_us;
    return prev_ema + a * (new_price - prev_ema);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ticker> <count>" << std::endl;
        exit(1);
    }

    const auto max_entries = (uint64_t)std::atoi(argv[2]);

    // connect to coinbase
    auto ticker_sub = [&argv]() {
        auto res = coinbase::subscribe_to_ticker(argv[1]);
        if (!res) {
            std::cerr << "failed to subscribe to the ticker";
            exit(1);
        }
        return *res;
    }();
    Defer cancel([&ticker_sub]() { destroy_subscription(ticker_sub); });

    // table header
    std::cout << "timestamp_us,bid,ask,price,mid_ema,price_ema\n";

    uint64_t last_seq_no = 0;
    uint64_t last_timestamp_us = 0;

    Price ema_mid = 0;
    Price ema_price = 0;

    struct {
        uint64_t retries;
        uint64_t critical_errors;
        uint64_t dropped_out_of_order;
        uint64_t dropped_dt0;
        uint64_t handled;
    } stats = {};

    while (stats.handled < max_entries) {
        const auto [ticker, err, retry] = coinbase::get_next_ticker(ticker_sub);

        if (err && retry) [[likely]] {
            stats.retries++;
            // busy loop is not the best idea to poll a socket, but works for now
            std::this_thread::yield();
            continue;
        }
        if (err) [[unlikely]] {
            stats.critical_errors++;
            break;
        }

        // check for out of order updates, really don't want them.
        // note that we might miss some updates but that's fine.
        if (ticker.seq_no < last_seq_no) [[unlikely]] {
            stats.dropped_out_of_order++;
            continue;
        }
        last_seq_no = ticker.seq_no;

        int64_t dt_us = ticker.timestamp_us - last_timestamp_us;
        assert(dt_us >= 0); // can't move back in time
        if (dt_us == 0) {
            // this means we can't represent this timestep with a microsecond
            // precision. Given that 5 seconds window is much larger than 1us
            // we can drop this sample without losing too much precision.
            stats.dropped_dt0++;
            continue;
        }

        ema_mid = next_ema_5sec(ema_mid, (ticker.bid + ticker.ask) / 2, dt_us);
        ema_price = next_ema_5sec(ema_price, ticker.price, dt_us);

        std::cout << std::setprecision(9) <<
            ticker.timestamp_us << ',' <<
            ticker.bid << ',' <<
            ticker.ask << ',' <<
            ticker.price << ',' <<
            ema_mid << ',' <<
            ema_price << '\n';

        last_timestamp_us = ticker.timestamp_us;
        stats.handled++;
    }

    std::cout << "Exit stats:\n";
    std::cout << "Retries: " << stats.retries << '\n';
    std::cout << "Critical errors: " << stats.critical_errors << '\n';
    std::cout << "Dropped ooo: " << stats.dropped_out_of_order << '\n';
    std::cout << "Dropped dt0: " << stats.dropped_dt0 << '\n';
    std::cout << "Handled: " << stats.handled << '\n';

    return stats.critical_errors > 0;
}

#if defined(__APPLE__) && defined(__MACH__)
// better errors on macos
void std::__libcpp_verbose_abort(char const* format, ...) {
  va_list list;
  va_start(list, format);
  std::vfprintf(stderr, format, list);
  va_end(list);

  std::abort();
}
#endif