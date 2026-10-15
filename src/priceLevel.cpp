#include "priceLevel.h"

#include <algorithm>

namespace obe {

void PriceLevel::addOrder( const Order& order )
{
    m_orders.push_back( order );
    m_total_quantity += order.remaining_quantity();
}

// Removes an order by ID.
// Returns:
// - true if found and removed
// - false if no such order
bool PriceLevel::cancelOrder( uint64_t order_id )
{
    auto it = std::find_if( m_orders.begin(), m_orders.end(),
                            [order_id]( const Order& o ) { return o.id == order_id; } );
    if ( it == m_orders.end() )
        return false;

    m_total_quantity -= it->remaining_quantity();
    m_orders.erase( it );
    return true;
}

/**
 * Matches an incoming order against resting orders at this level, oldest first.
 *
 * Trades execute at this level's price (the resting order's price), not the
 * incoming order's limit. Fully filled resting orders are removed.
 *
 * @param incoming_order_id  ID of the aggressing order, used in the produced Trades.
 * @param incoming_quantity  In/out: reduced by the quantity filled. If nonzero on
 *                           return, this level is empty and the caller should
 *                           continue at the next level.
 * @param timestamp          Timestamp stamped onto each produced Trade.
 * @return Trades produced, in fill order. Empty if the level had no orders.
 */
std::vector<Trade> PriceLevel::match( uint64_t incoming_order_id, uint32_t& incoming_quantity,
                                      uint64_t timestamp, std::vector<uint64_t>* filled_order_ids )
{
    std::vector<Trade> trades;

    while ( incoming_quantity > 0 && !m_orders.empty() )
    {
        Order& resting = m_orders.front();
        uint32_t fill = std::min( incoming_quantity, resting.remaining_quantity() );

        resting.filled_quantity += fill;
        incoming_quantity -= fill;
        m_total_quantity -= fill;

        // The resting order's side tells us which ID is the buyer.
        bool restingIsBuy = ( resting.side == Side::buy );
        trades.push_back( Trade{
            restingIsBuy ? resting.id : incoming_order_id,
            restingIsBuy ? incoming_order_id : resting.id,
            m_price, // trades execute at the resting order's price
            fill,
            timestamp } );

        if ( resting.is_filled() )
        {
            if ( filled_order_ids )
                filled_order_ids->push_back( resting.id );
            m_orders.pop_front();
        }
    }
    return trades;
}

} // namespace obe