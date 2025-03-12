#ifndef COINBASE_TICKER_HPP
#define COINBASE_TICKER_HPP

#include <curl.h>

#include <optional>

namespace coinbase {

// represents subscription to a "ticker" chanel for a single ticker on coinbase
struct TickerSubscription {
    CURL *curl = nullptr;
};

// establishes websocket connection and subscribes to the ticker.
// returns nullopt on error.
std::optional<TickerSubscription> subscribe_to_ticker(const char *ticker_name);

// unsubscribes and closes the socket.
void destroy_subscription(const TickerSubscription &s);

// normally this is not a good idea, but it's fine for a small task like this.
using Price = float;

struct Ticker {
    uint64_t seq_no = 0;
    uint64_t timestamp_us = 0;
    Price bid = 0;
    Price ask = 0;
    Price price = 0;
};

// returns next available ticker from the subscription, doesn't block.
// {Ticker, err, retry}
// Ticker - new ticker on success
// err - true if error, false otherwise
// retry - true if we can retry, false if everything is futile
std::tuple<Ticker, bool, bool> get_next_ticker(const TickerSubscription &sub);

}

#endif //COINBASE_TICKER_HPP
