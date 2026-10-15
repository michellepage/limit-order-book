#pragma once

#include "bookSide.h"
#include "orderTypes.h"

#include <cstdint>
#include <vector>

namespace obe {

struct SubmitResult
{
    bool accepted;             // false if the order failed validation
    std::vector<Trade> trades; // trades produced, in execution order
};

class MatchingEngine
{
    private:
        BookSide m_bids;
        BookSide m_asks;

        static bool isValid( const Order& order );
        static bool crosses( const Order& incoming, const PriceLevel& level );

    public:
        MatchingEngine() : m_bids( Side::buy ), m_asks( Side::sell ) {}

        SubmitResult submitOrder( const Order& order );

        bool cancelOrder( uint64_t order_id );

        const PriceLevel* bestBid() const { return m_bids.bestLevel(); }
        const PriceLevel* bestAsk() const { return m_asks.bestLevel(); }
};

} // namespace obe