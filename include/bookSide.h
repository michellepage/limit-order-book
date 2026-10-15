#pragma once

#include "orderTypes.h"
#include "priceLevel.h"

#include <cstdint>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

namespace obe {

class BookSide
{
    private:
        using LevelMap = std::map<int64_t, PriceLevel, std::function<bool( int64_t, int64_t )>>;

        Side m_side;
        LevelMap m_levels;                                    // sorted best price first
        std::unordered_map<uint64_t, int64_t> m_order_prices; // order id -> price, for O(1) cancel lookup

    public:
        // side == Side::buy makes a bid side (highest price first)
        //  side == Side::sell makes an ask side (lowest first).
        explicit BookSide( Side side );

        bool isEmpty() const { return m_levels.empty(); }
        size_t levelCount() const { return m_levels.size(); }

        // Returns
        // - the best level (highest bid / lowest ask)
        // - nullptr if the side is emptycvx
        const PriceLevel* bestLevel() const;

        // Rests an order at its price
        // Creates the level if needed
        void addOrder( const Order& order );

        // Removes a resting order by ID, erasing its level if it becomes empty
        // Returns false if unknown
        bool cancelOrder( uint64_t order_id );

        // Returns
        // - true if an order with this ID is currently resting on this side
        bool contains( uint64_t order_id ) const { return m_order_prices.count( order_id ) > 0; }

        std::vector<Trade> matchBest( uint64_t incoming_order_id, uint32_t& incoming_quantity,
                                      uint64_t timestamp );
};

} // namespace obe