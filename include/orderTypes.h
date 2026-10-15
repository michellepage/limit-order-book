#pragma once

#include <cstdint>

namespace obe {

enum class Side { buy, sell };
enum class OrderType { limit, market };

struct Order
{
    uint64_t id;
    Side side;
    OrderType type;
    int64_t price;
    uint32_t quantity;
    uint32_t filled_quantity = 0;
    uint64_t timestamp;

    uint32_t remaining_quantity() const { return quantity - filled_quantity; }
    bool is_filled() const { return filled_quantity >= quantity; }

    bool operator==( const Order& other ) const
    {
        return id == other.id && side == other.side && type == other.type &&
               price == other.price && quantity == other.quantity &&
               filled_quantity == other.filled_quantity && timestamp == other.timestamp;
    }
};

struct Trade
{
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    int64_t price;
    uint32_t quantity;
    uint64_t timestamp;
};

} // namespace obe