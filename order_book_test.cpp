#include "order_book.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <stdexcept>

namespace {
void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void check_trades(const std::vector<Trade>& actual, const std::vector<Trade>& expected) {
    check(actual.size() == expected.size(), "unexpected fill count");
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto& a = actual[i];
        const auto& b = expected[i];
        check(a.maker == b.maker && a.taker == b.taker && a.price == b.price && a.quantity == b.quantity,
              "unexpected fill");
    }
}

void price_then_time() {
    OrderBook book;
    book.submit({1, Side::sell, 102, 4});
    book.submit({2, Side::sell, 100, 3});
    book.submit({3, Side::sell, 100, 5});
    check_trades(book.submit({4, Side::buy, 102, 9}), {{2, 4, 100, 3}, {3, 4, 100, 5}, {1, 4, 102, 1}});
    check(book.remaining(1) == 3u && book.size() == 1, "resting quantity after partial fill");
    check(book.cancel(1) && !book.cancel(1) && !book.best_price(Side::sell), "cancel clears empty level");

    book.submit({5, Side::buy, 97, 5});
    book.submit({6, Side::buy, 99, 2});
    check_trades(book.submit({7, Side::sell, 98, 4}), {{6, 7, 99, 2}});
    check(book.best_price(Side::buy) == 97 && book.best_price(Side::sell) == 98, "remainder rests at limit");
    check(book.remaining(7) == 2u, "taker remainder");
}

void invalid_order_leaves_book_intact() {
    OrderBook book;
    book.submit({1, Side::buy, 100, 5});
    for (auto order : {Order{1, Side::sell, 99, 2}, Order{0, Side::sell, 99, 2},
                       Order{2, Side::sell, 0, 2}, Order{2, Side::sell, 99, 0}}) {
        try {
            book.submit(order);
            throw std::runtime_error("invalid order accepted");
        } catch (const std::invalid_argument&) {
            check(book.size() == 1 && book.remaining(1) == 5u, "invalid order changed the book");
        }
    }
}

std::vector<Trade> linear_match(std::list<Order>& live, Order taker) {
    std::vector<Trade> fills;
    while (taker.quantity) {
        auto best = live.end();
        for (auto maker = live.begin(); maker != live.end(); ++maker) {
            if (maker->side == taker.side) {
                continue;
            }
            const bool cross = taker.side == Side::buy ? maker->price <= taker.price : maker->price >= taker.price;
            if (cross && (best == live.end() || (taker.side == Side::buy ? maker->price < best->price
                                                                       : maker->price > best->price))) {
                best = maker;
            }
        }
        if (best == live.end()) {
            live.push_back(taker);
            break;
        }
        const auto quantity = std::min(taker.quantity, best->quantity);
        fills.push_back({best->id, taker.id, best->price, quantity});
        taker.quantity -= quantity;
        best->quantity -= quantity;
        if (!best->quantity) {
            live.erase(best);
        }
    }
    return fills;
}

void differential_replay() {
    OrderBook book;
    std::list<Order> live;
    std::mt19937 random(17);
    for (OrderId id = 1; id <= 2500; ++id) {
        if (id > 1 && random() % 4 == 0) {
            const auto cancelled = 1 + random() % (id - 1);
            const auto position = std::find_if(live.begin(), live.end(), [cancelled](const Order& order) {
                return order.id == cancelled;
            });
            check(book.cancel(cancelled) == (position != live.end()), "cancellation disagrees with reference");
            if (position != live.end()) {
                live.erase(position);
            }
        } else {
            const Order order{id, random() % 2 ? Side::buy : Side::sell,
                              static_cast<Price>(95 + random() % 11), static_cast<Quantity>(1 + random() % 10)};
            check_trades(book.submit(order), linear_match(live, order));
        }
        check(book.size() == live.size(), "resting count disagrees with reference");
        std::optional<Price> expected_bid, expected_ask;
        for (const auto& order : live) {
            check(book.remaining(order.id) == order.quantity, "resting quantity disagrees with reference");
            if (order.side == Side::buy && (!expected_bid || order.price > *expected_bid)) {
                expected_bid = order.price;
            }
            if (order.side == Side::sell && (!expected_ask || order.price < *expected_ask)) {
                expected_ask = order.price;
            }
        }
        const auto bid = book.best_price(Side::buy);
        const auto ask = book.best_price(Side::sell);
        check(bid == expected_bid && ask == expected_ask, "best price disagrees with reference");
        check(!bid || !ask || *bid < *ask, "resting book is crossed");
    }
}
}

int main() {
    try {
        price_then_time();
        invalid_order_leaves_book_intact();
        differential_replay();
        std::cout << "Price-time, cancellation, invalid input, and seeded replay passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
