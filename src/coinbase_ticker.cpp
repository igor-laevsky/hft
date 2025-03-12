#include "coinbase_ticker.hpp"
#include "defer.hpp"

#include <iostream>
#include <cassert>

using namespace coinbase;
using namespace utils;
using namespace std::literals::string_literals;

constexpr std::string_view coinbase_url = "wss://ws-feed.exchange.coinbase.com";

std::optional<TickerSubscription> coinbase::subscribe_to_ticker(const char *ticker_name) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "failed to initialize curl";
        return std::nullopt;
    }

    auto err = CURLE_OK;
    err = curl_easy_setopt(curl, CURLOPT_URL, coinbase_url.data());
    if (err != CURLE_OK) {
        std::cerr << "failed setopt " << curl_easy_strerror(err);
        curl_easy_cleanup(curl);
        return std::nullopt;
    }

    err = curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L); // ws specific
    if (err != CURLE_OK) {
        std::cerr << "failed setopt connect only " << curl_easy_strerror(err);
        curl_easy_cleanup(curl);
        return std::nullopt;
    }

    err = curl_easy_perform(curl);
    if (err != CURLE_OK) {
        std::cerr << "failed to establish ws connection " << curl_easy_strerror(err);
        curl_easy_cleanup(curl);
        return std::nullopt;
    }

    size_t sent = 0;
    std::string msg =
        R"({ "type": "subscribe", "product_ids": [")"s + ticker_name + R"("], "channels": ["ticker"] })";

    err =
        curl_ws_send(curl, msg.data(), msg.size(),
                     &sent, 0, CURLWS_TEXT);
    if (err != CURLE_OK) {
        std::cerr << "failed ws subscription " << curl_easy_strerror(err);
        return std::nullopt;
    }

    return TickerSubscription{curl};
}

void coinbase::destroy_subscription(const TickerSubscription &s) {
    assert(s.curl != nullptr);
    size_t sent = 0;
    curl_ws_send(s.curl, "", 0, &sent, 0, CURLWS_CLOSE);
    curl_easy_cleanup(s.curl);
    return;
}
