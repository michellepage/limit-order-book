#include <gtest/gtest.h>

#include "matchingEngine.h"

using namespace obe;

namespace {
Order limitOrder( uint64_t id, Side side, int64_t price, uint32_t qty )
{
    return Order{ id, side, OrderType::limit, price, qty, 0, id };
}

Order marketOrder( uint64_t id, Side side, uint32_t qty )
{
    return Order{ id, side, OrderType::market, 0, qty, 0, id };
}
}

TEST( matchingEngineValidation, rejectsZeroQuantity )
{
    MatchingEngine engine;
    auto result = engine.submitOrder( limitOrder( 1, Side::buy, 10000, 0 ) );

    EXPECT_FALSE( result.accepted );
    EXPECT_TRUE( result.trades.empty() );
    EXPECT_EQ( engine.bestBid(), nullptr );
}

TEST( matchingEngineValidation, rejectsNonPositiveLimitPrice )
{
    MatchingEngine engine;

    EXPECT_FALSE( engine.submitOrder( limitOrder( 1, Side::buy, 0, 10 ) ).accepted );
    EXPECT_FALSE( engine.submitOrder( limitOrder( 2, Side::sell, -5, 10 ) ).accepted );
    EXPECT_EQ( engine.bestBid(), nullptr );
    EXPECT_EQ( engine.bestAsk(), nullptr );
}

TEST( matchingEngineValidation, rejectsDuplicateRestingId )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::buy, 9900, 10 ) );
    auto result = engine.submitOrder( limitOrder( 1, Side::sell, 10100, 10 ) );

    EXPECT_FALSE( result.accepted );
    EXPECT_EQ( engine.bestAsk(), nullptr );
    EXPECT_EQ( engine.bestBid()->getTotalQuantity(), 10u ); // original untouched
}

TEST( matchingEngineValidation, idCanBeReusedAfterCancel )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::buy, 9900, 10 ) );
    engine.cancelOrder( 1 );

    EXPECT_TRUE( engine.submitOrder( limitOrder( 1, Side::buy, 9900, 10 ) ).accepted );
}

TEST( matchingEngineResting, nonCrossingLimitOrderRests )
{
    MatchingEngine engine;
    auto result = engine.submitOrder( limitOrder( 1, Side::buy, 9900, 100 ) );

    EXPECT_TRUE( result.accepted );
    EXPECT_TRUE( result.trades.empty() );
    ASSERT_NE( engine.bestBid(), nullptr );
    EXPECT_EQ( engine.bestBid()->getPrice(), 9900 );
    EXPECT_EQ( engine.bestBid()->getTotalQuantity(), 100u );
}

TEST( matchingEngineResting, buyBelowBestAskDoesNotCross )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10100, 100 ) );
    auto result = engine.submitOrder( limitOrder( 2, Side::buy, 10000, 100 ) );

    EXPECT_TRUE( result.trades.empty() );
    EXPECT_EQ( engine.bestBid()->getPrice(), 10000 );
    EXPECT_EQ( engine.bestAsk()->getPrice(), 10100 );
}

TEST( matchingEngineCrossing, aggressiveBuyTradesAtRestingPrice )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10000, 100 ) );
    auto result = engine.submitOrder( limitOrder( 2, Side::buy, 10100, 50 ) );

    ASSERT_EQ( result.trades.size(), 1u );
    EXPECT_EQ( result.trades[0].price, 10000 ); // resting price, not the 10100 limit
    EXPECT_EQ( result.trades[0].quantity, 50u );
    EXPECT_EQ( result.trades[0].buy_order_id, 2u );
    EXPECT_EQ( result.trades[0].sell_order_id, 1u );
    EXPECT_EQ( engine.bestAsk()->getTotalQuantity(), 50u );
    EXPECT_EQ( engine.bestBid(), nullptr ); // fully filled, nothing rests
}

TEST( matchingEngineCrossing, aggressiveSellTradesAtRestingPrice )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::buy, 10000, 100 ) );
    auto result = engine.submitOrder( limitOrder( 2, Side::sell, 9900, 40 ) );

    ASSERT_EQ( result.trades.size(), 1u );
    EXPECT_EQ( result.trades[0].price, 10000 );
    EXPECT_EQ( result.trades[0].buy_order_id, 1u );
    EXPECT_EQ( result.trades[0].sell_order_id, 2u );
    EXPECT_EQ( engine.bestBid()->getTotalQuantity(), 60u );
}

TEST( matchingEngineCrossing, sweepsMultipleLevelsBestPriceFirst )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10100, 30 ) );
    engine.submitOrder( limitOrder( 2, Side::sell, 10000, 30 ) );
    auto result = engine.submitOrder( limitOrder( 3, Side::buy, 10100, 50 ) );

    ASSERT_EQ( result.trades.size(), 2u );
    EXPECT_EQ( result.trades[0].price, 10000 ); // best ask first
    EXPECT_EQ( result.trades[0].quantity, 30u );
    EXPECT_EQ( result.trades[1].price, 10100 );
    EXPECT_EQ( result.trades[1].quantity, 20u );
    EXPECT_EQ( engine.bestAsk()->getPrice(), 10100 );
    EXPECT_EQ( engine.bestAsk()->getTotalQuantity(), 10u );
}

TEST( matchingEngineCrossing, stopsSweepingAtLimitPrice )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10000, 30 ) );
    engine.submitOrder( limitOrder( 2, Side::sell, 10200, 30 ) );
    auto result = engine.submitOrder( limitOrder( 3, Side::buy, 10100, 50 ) );

    ASSERT_EQ( result.trades.size(), 1u ); // 10200 is beyond the limit
    EXPECT_EQ( result.trades[0].quantity, 30u );
    EXPECT_EQ( engine.bestAsk()->getPrice(), 10200 );
}

TEST( matchingEngineCrossing, partialFillRemainderRestsOnOwnSide )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10000, 30 ) );
    auto result = engine.submitOrder( limitOrder( 2, Side::buy, 10000, 50 ) );

    ASSERT_EQ( result.trades.size(), 1u );
    EXPECT_EQ( result.trades[0].quantity, 30u );
    EXPECT_EQ( engine.bestAsk(), nullptr );
    ASSERT_NE( engine.bestBid(), nullptr );
    EXPECT_EQ( engine.bestBid()->getPrice(), 10000 );
    EXPECT_EQ( engine.bestBid()->getTotalQuantity(), 20u ); // only the unfilled part rests
}

TEST( matchingEngineCrossing, respectsTimePriorityAtSamePrice )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10000, 30 ) );
    engine.submitOrder( limitOrder( 2, Side::sell, 10000, 30 ) );
    auto result = engine.submitOrder( limitOrder( 3, Side::buy, 10000, 40 ) );

    ASSERT_EQ( result.trades.size(), 2u );
    EXPECT_EQ( result.trades[0].sell_order_id, 1u ); // earlier order fills first
    EXPECT_EQ( result.trades[0].quantity, 30u );
    EXPECT_EQ( result.trades[1].sell_order_id, 2u );
    EXPECT_EQ( result.trades[1].quantity, 10u );
}

TEST( matchingEngineMarketOrders, sweepsAnyPrice )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10000, 30 ) );
    engine.submitOrder( limitOrder( 2, Side::sell, 99999, 30 ) );
    auto result = engine.submitOrder( marketOrder( 3, Side::buy, 50 ) );

    ASSERT_EQ( result.trades.size(), 2u );
    EXPECT_EQ( result.trades[1].price, 99999 );
    EXPECT_EQ( result.trades[1].quantity, 20u );
}

TEST( matchingEngineMarketOrders, remainderIsDiscarded )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10000, 30 ) );
    auto result = engine.submitOrder( marketOrder( 2, Side::buy, 50 ) );

    ASSERT_EQ( result.trades.size(), 1u );
    EXPECT_EQ( result.trades[0].quantity, 30u );
    EXPECT_EQ( engine.bestBid(), nullptr ); // nothing rests
    EXPECT_EQ( engine.bestAsk(), nullptr );
}

TEST( matchingEngineMarketOrders, emptyBookProducesNoTrades )
{
    MatchingEngine engine;
    auto result = engine.submitOrder( marketOrder( 1, Side::sell, 10 ) );

    EXPECT_TRUE( result.accepted );
    EXPECT_TRUE( result.trades.empty() );
    EXPECT_EQ( engine.bestBid(), nullptr );
}

TEST( matchingEngineCancel, removesRestingOrderOnEitherSide )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::buy, 9900, 10 ) );
    engine.submitOrder( limitOrder( 2, Side::sell, 10100, 10 ) );

    EXPECT_TRUE( engine.cancelOrder( 1 ) );
    EXPECT_TRUE( engine.cancelOrder( 2 ) );
    EXPECT_EQ( engine.bestBid(), nullptr );
    EXPECT_EQ( engine.bestAsk(), nullptr );
}

TEST( matchingEngineCancel, unknownOrFilledOrderReturnsFalse )
{
    MatchingEngine engine;
    engine.submitOrder( limitOrder( 1, Side::sell, 10000, 30 ) );
    engine.submitOrder( limitOrder( 2, Side::buy, 10000, 30 ) ); // fully fills order 1

    EXPECT_FALSE( engine.cancelOrder( 1 ) );
    EXPECT_FALSE( engine.cancelOrder( 999 ) );
}