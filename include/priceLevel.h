#pragma once

#include "orderTypes.h"

#include <cstdint>
#include <deque>
#include <vector>

namespace obe {

class PriceLevel
{
    private:
        int64_t m_price;
        std::deque<Order> m_orders;
        uint32_t m_total_quantity = 0;

    public:
        explicit PriceLevel( int64_t price = 0 ) : m_price( price ) {}

        int64_t getPrice() const { return m_price; }
        uint32_t getTotalQuantity() const { return m_total_quantity; }
        bool isEmpty() const { return m_orders.empty(); }

        void addOrder( const Order& order );
        bool cancelOrder( uint64_t order_id );

        std::vector<Trade> match( uint64_t incoming_order_id, uint32_t& incoming_quantity,
                          uint64_t timestamp,
                          std::vector<uint64_t>* filled_order_ids = nullptr );

};

} // namespace obe