#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

enum class Side { buy, sell };

using OrderId = std::uint64_t;
using Price = std::int64_t;
using Quantity = std::uint32_t;

struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity quantity;
};

struct Trade {
    OrderId maker;
    OrderId taker;
    Price price;
    Quantity quantity;
};

class OrderBook {
public:
    OrderBook() = default;
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    std::vector<Trade> submit(Order order);
    bool cancel(OrderId id);
    std::optional<Price> best_price(Side side) const;
    std::optional<Quantity> remaining(OrderId id) const;
    std::size_t size() const { return open_.size(); }

private:
    using Queue = std::list<Order>;
    using Levels = std::map<Price, Queue>;

    struct Location {
        Side side;
        Levels::iterator level;
        Queue::iterator order;
    };

    using Index = std::unordered_map<OrderId, Location>;

    Levels bids_;
    Levels asks_;
    Index open_;

    Levels& levels(Side side) { return side == Side::buy ? bids_ : asks_; }
    void rest(Order order);
    void erase(Index::iterator position);
};
