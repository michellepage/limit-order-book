#include "bookSide.h"

namespace obe {

BookSide::BookSide( Side side )
    : m_side( side ),
      m_levels( side == Side::buy
                    ? std::function<bool( int64_t, int64_t )>( std::greater<int64_t>() )
                    : std::function<bool( int64_t, int64_t )>( std::less<int64_t>() ) )
{
}

const PriceLevel* BookSide::bestLevel() const
{
    if ( m_levels.empty() )
        return nullptr;
    return &m_levels.begin()->second; // comparator keeps the best price at begin()
}

void BookSide::addOrder( const Order& order )
{
    // try_emplace constructs PriceLevel(order.price) only if the key is missing
    auto [it, inserted] = m_levels.try_emplace( order.price, order.price );
    it->second.addOrder( order );
    m_order_prices[order.id] = order.price;
}

bool BookSide::cancelOrder( uint64_t order_id )
{
    auto idx = m_order_prices.find( order_id );
    if ( idx == m_order_prices.end() )
        return false;

    auto level = m_levels.find( idx->second );
    if ( level == m_levels.end() || !level->second.cancelOrder( order_id ) )
        return false;

    if ( level->second.isEmpty() )
        m_levels.erase( level );
    m_order_prices.erase( idx );
    return true;
}

std::vector<Trade> BookSide::matchBest( uint64_t incoming_order_id, uint32_t& incoming_quantity,
                                        uint64_t timestamp )
{
    if ( m_levels.empty() )
        return {};

    auto best = m_levels.begin();
    std::vector<uint64_t> filled;
    std::vector<Trade> trades = best->second.match( incoming_order_id, incoming_quantity, timestamp, &filled );

    for ( uint64_t id : filled )
        m_order_prices.erase( id );

    if ( best->second.isEmpty() )
        m_levels.erase( best );

    return trades;
}

} // namespace obe