#include "order_book.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>

std::vector<Trade> OrderBook::submit(Order order) {
    if (order.id == 0 || order.price <= 0 || order.quantity == 0) {
        throw std::invalid_argument("id, price and quantity must be positive");
    }
    if (order.side != Side::buy && order.side != Side::sell) {
        throw std::invalid_argument("invalid side");
    }
    if (open_.find(order.id) != open_.end()) {
        throw std::invalid_argument("order id is already open");
    }

    const bool buy = order.side == Side::buy;
    const auto& opposite = buy ? asks_ : bids_;
    std::vector<Trade> trades;
    auto level = buy ? opposite.begin() : opposite.end();
    while (order.quantity && (buy ? level != opposite.end() : level != opposite.begin())) {
        if (!buy) {
            --level;
        }
        const auto price = level->first;
        if (buy ? price > order.price : price < order.price) {
            break;
        }
        for (const auto& maker : level->second) {
            const auto filled = std::min(order.quantity, maker.quantity);
            trades.push_back({maker.id, order.id, price, filled});
            order.quantity -= filled;
            if (order.quantity == 0) {
                break;
            }
        }
        if (buy) {
            ++level;
        }
    }

    // Allocate the remainder before applying fills, so allocation failure cannot half-fill a trade.
    if (order.quantity) {
        rest(order);
    }
    for (const auto& trade : trades) {
        const auto maker = open_.find(trade.maker);
        auto& remaining = maker->second.order->quantity;
        remaining -= trade.quantity;
        if (remaining == 0) {
            erase(maker);
        }
    }
    return trades;
}

void OrderBook::rest(Order order) {
    auto& book = levels(order.side);
    const auto level = book.try_emplace(order.price).first;
    auto& queue = level->second;
    try {
        queue.push_back(order);
        try {
            open_.emplace(order.id, Location{order.side, level, std::prev(queue.end())});
        } catch (...) {
            queue.pop_back();
            throw;
        }
    } catch (...) {
        if (queue.empty()) {
            book.erase(level);
        }
        throw;
    }
}

void OrderBook::erase(Index::iterator position) {
    const auto location = position->second;
    auto& queue = location.level->second;
    queue.erase(location.order);
    if (queue.empty()) {
        levels(location.side).erase(location.level);
    }
    open_.erase(position);
}

bool OrderBook::cancel(OrderId id) {
    const auto position = open_.find(id);
    if (position == open_.end()) {
        return false;
    }
    erase(position);
    return true;
}

std::optional<Price> OrderBook::best_price(Side side) const {
    const auto& book = side == Side::buy ? bids_ : asks_;
    if (book.empty()) {
        return std::nullopt;
    }
    return side == Side::buy ? book.rbegin()->first : book.begin()->first;
}

std::optional<Quantity> OrderBook::remaining(OrderId id) const {
    const auto position = open_.find(id);
    if (position == open_.end()) {
        return std::nullopt;
    }
    return position->second.order->quantity;
}
