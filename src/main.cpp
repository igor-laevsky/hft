#include "defer.hpp"
#include "coinbase_ticker.hpp"

#include <curl.h>

#include <iostream>
#include <cassert>
#include <thread>

using namespace utils;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ticker>" << std::endl;
        exit(1);
    }

    auto ticker = [&argv]() {
        auto res = coinbase::subscribe_to_ticker(argv[1]);
        if (!res) {
            std::cerr << "failed to subscribe to ticker";
            exit(1);
        }
        return *res;
    }();
    Defer cancel([&ticker]() { destroy_subscription(ticker); });

    auto err = CURLE_OK;

    while (true) {
        size_t rlen;
        const curl_ws_frame *meta;
        char buffer[4096] = {};

        err = curl_ws_recv(ticker.curl, buffer, sizeof(buffer), &rlen, &meta);
        if (err == CURLE_AGAIN) {
            std::this_thread::yield();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        if (err != CURLE_OK) {
            std::cerr << "failed ws recv " << curl_easy_strerror(err);
            exit(1);
        }
        std::cout << buffer;
    }

    return 0;
}

#if defined(__APPLE__) && defined(__MACH__)
// better errors on mac
void std::__libcpp_verbose_abort(char const* format, ...) {
  va_list list;
  va_start(list, format);
  std::vfprintf(stderr, format, list);
  va_end(list);

  std::abort();
}
#endif