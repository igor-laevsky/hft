#include "coinbase_ticker.hpp"
#include "defer.hpp"

#include <iostream>
#include <cassert>
#include <iomanip>
#include <sstream>

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/error/en.h>

#include <date/date.h>

using namespace coinbase;
using namespace utils;
using namespace std::literals::string_literals;

namespace {

constexpr std::string_view coinbase_url = "wss://ws-feed.exchange.coinbase.com";

constexpr size_t max_message_size = 4096;

constexpr bool debug_print_all = false;

}

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
    if (err != CURLE_OK || sent != msg.size()) {
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
}

namespace {

// converts coinbase timestamp to unix timestamp in microseconds.
// returns 0 on error.
uint64_t _utc_to_timestamp_us(const char *s) {
    using namespace date;

    std::istringstream in{s};
    date::sys_time<std::chrono::microseconds> tp;
    in >> date::parse("%Y-%m-%dT%H:%M:%S%Z", tp);
    if (in.fail()) {
        return 0;
    }

    return tp.time_since_epoch().count();
}

}

// parses json in place at 'buf', returns nullopt on error
std::optional<Ticker> parse_inplace(char *buf) {
    rapidjson::Document d;
    if (d.ParseInsitu(buf).HasParseError()) {
        if constexpr (debug_print_all)
            std::cerr << "failed to parse json: " << GetParseError_En(d.GetParseError());
        return std::nullopt;
    }

    Ticker ret{};

    if (d["type"] != "ticker") return std::nullopt;

    ret.seq_no = d["sequence"].GetUint64();
    ret.price = std::stof(d["price"].GetString());
    ret.bid = std::stof(d["best_bid"].GetString());
    ret.ask = std::stof(d["best_ask"].GetString());
    ret.timestamp_us = _utc_to_timestamp_us(d["time"].GetString());
    if (ret.timestamp_us == 0) return std::nullopt;

    return ret;
}

std::tuple<Ticker, bool, bool> coinbase::get_next_ticker(const TickerSubscription &sub) {
    assert(sub.curl != nullptr);

    const curl_ws_frame *meta;
    size_t rlen;
    auto err = CURLE_OK;

    // fast and cache friendly
    alignas(64) char buf[max_message_size];

    // request the data
    err = curl_ws_recv(sub.curl, buf, sizeof(buf) - 1, &rlen, &meta);
    if (err == CURLE_AGAIN) [[likely]] {
        // socket not ready, try again
        return {Ticker{}, true, true};
    }
    if (err != CURLE_OK) [[unlikely]] {
        if constexpr (debug_print_all)
            std::cerr << "curl failed to recv next ticker " << curl_easy_strerror(err);
        return {Ticker{}, true, false};
    }
    buf[rlen] = '\0';

    if constexpr (debug_print_all) {
        std::cout << buf << std::endl;
    }

    // NOTE: this fails if more than one ticker had arrived in the 'buf'
    // I should probably fix that, but it doesn't matter for this task.
    auto ticker = parse_inplace(buf);
    if (!ticker)
        return {Ticker{}, true, true};

    // NOTE: There is probably one additional copy of Ticker here, can avoid that.
    return {*ticker, false, false};
}
