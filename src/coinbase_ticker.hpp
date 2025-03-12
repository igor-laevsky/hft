#ifndef COINBASE_TICKER_HPP
#define COINBASE_TICKER_HPP

#include <curl.h>

#include <optional>

namespace coinbase {

// represents subscription to a "ticker" chanel for single ticker on coinbase
struct TickerSubscription {
    CURL *curl = nullptr;
};

std::optional<TickerSubscription> subscribe_to_ticker(const char *ticker_name);

void destroy_subscription(const TickerSubscription &s);

}

#endif //COINBASE_TICKER_HPP
