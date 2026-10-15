#include "matchingEngine.h"

#include <iterator>
#include <utility>

namespace obe {

bool MatchingEngine::isValid( const Order& order )
{
    if ( order.quantity == 0 || order.filled_quantity >= order.quantity )
        return false;
    if ( order.type == OrderType::limit && order.price <= 0 )
        return false;
    return true;
}

bool MatchingEngine::crosses( const Order& incoming, const PriceLevel& level )
{
    if ( incoming.type == OrderType::market )
        return true;   // market orders accept any price

    return incoming.side == Side::buy
        ? level.getPrice() <= incoming.price  // buy: best ask must be at or below my limit
        : level.getPrice() >= incoming.price; // sell: best bid must be at or above my limit
}

SubmitResult MatchingEngine::submitOrder( const Order& order )
{
    if ( !isValid( order ) )
        return SubmitResult{ false, {} };

    if ( m_bids.contains( order.id ) || m_asks.contains( order.id ) )
        return SubmitResult{ false, {} };

    BookSide& opposite = ( order.side == Side::buy ) ? m_asks : m_bids;
    BookSide& own      = ( order.side == Side::buy ) ? m_bids : m_asks;

    uint32_t remaining = order.remaining_quantity();
    std::vector<Trade> trades;

    // BookSide::matchBest handles one level per call, so the price check lives here.
    while ( remaining > 0 )
    {
        const PriceLevel* best = opposite.bestLevel();
        if ( best == nullptr || !crosses( order, *best ) )
            break;

        std::vector<Trade> fills = opposite.matchBest( order.id, remaining, order.timestamp );
        trades.insert( trades.end(),
                       std::make_move_iterator( fills.begin() ),
                       std::make_move_iterator( fills.end() ) );
    }

    // Limit remainders rest in the book; market remainders are discarded.
    if ( remaining > 0 && order.type == OrderType::limit )
    {
        Order resting = order;
        resting.filled_quantity = order.quantity - remaining;
        own.addOrder( resting );
    }

    return SubmitResult{ true, std::move( trades ) };
}

bool MatchingEngine::cancelOrder( uint64_t order_id )
{
    // A miss on BookSide is O(1) (hash lookup), so trying both sides is cheap.
    return m_bids.cancelOrder( order_id ) || m_asks.cancelOrder( order_id );
}

} // namespace obe