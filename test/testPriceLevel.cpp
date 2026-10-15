#include <gtest/gtest.h>

#include "priceLevel.h"

using namespace obe;

namespace {
// Helper so tests stay readable
// Price is fixed per level, so only id/side/qty vary.
Order makeOrder( uint64_t id, Side side, uint32_t qty, int64_t price = 10000 )
{
    return Order{ id, side, OrderType::limit, price, qty, 0, id };
}
}

TEST( priceLevelTest, startsEmpty )
{
    PriceLevel level( 10000 );
    EXPECT_TRUE( level.isEmpty() );
    EXPECT_EQ( level.getTotalQuantity(), 0u );
    EXPECT_EQ( level.getPrice(), 10000 );
}

TEST( priceLevelTest, addOrderUpdatesTotalQuantity )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::sell, 100 ) );
    level.addOrder( makeOrder( 2, Side::sell, 50 ) );

    EXPECT_FALSE( level.isEmpty() );
    EXPECT_EQ( level.getTotalQuantity(), 150u );
}

TEST( priceLevelTest, cancelRemovesOnlyThatOrder )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::sell, 100 ) );
    level.addOrder( makeOrder( 2, Side::sell, 50 ) );

    EXPECT_TRUE( level.cancelOrder( 1 ) );
    EXPECT_EQ( level.getTotalQuantity(), 50u );
    EXPECT_FALSE( level.isEmpty() );
}

TEST( priceLevelTest, cancelUnknownIdReturnsFalse )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::sell, 100 ) );

    EXPECT_FALSE( level.cancelOrder( 999 ) );
    EXPECT_EQ( level.getTotalQuantity(), 100u );
}

TEST( priceLevelTest, matchPartiallyFillsFrontOrder )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::sell, 100 ) );

    uint32_t incoming = 40;
    auto trades = level.match( 99, incoming, 5000 );

    ASSERT_EQ( trades.size(), 1u );
    EXPECT_EQ( trades[0].quantity, 40u );
    EXPECT_EQ( trades[0].price, 10000 );
    EXPECT_EQ( trades[0].sell_order_id, 1u );
    EXPECT_EQ( trades[0].buy_order_id, 99u );
    EXPECT_EQ( incoming, 0u );
    EXPECT_EQ( level.getTotalQuantity(), 60u );
    EXPECT_FALSE( level.isEmpty() );
}

TEST( priceLevelTest, matchRespectsFifoOrder )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::sell, 30 ) );
    level.addOrder( makeOrder( 2, Side::sell, 30 ) );

    uint32_t incoming = 40;
    auto trades = level.match( 99, incoming, 5000 );

    ASSERT_EQ( trades.size(), 2u );
    EXPECT_EQ( trades[0].sell_order_id, 1u ); // oldest fills first
    EXPECT_EQ( trades[0].quantity, 30u );
    EXPECT_EQ( trades[1].sell_order_id, 2u );
    EXPECT_EQ( trades[1].quantity, 10u );
    EXPECT_EQ( level.getTotalQuantity(), 20u );
}

TEST( priceLevelTest, matchExactQuantityEmptiesLevel )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::sell, 100 ) );

    uint32_t incoming = 100;
    level.match( 99, incoming, 5000 );

    EXPECT_EQ( incoming, 0u );
    EXPECT_TRUE( level.isEmpty() );
    EXPECT_EQ( level.getTotalQuantity(), 0u );
}

TEST( priceLevelTest, matchMoreThanAvailableReturnsRemainder )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::sell, 100 ) );

    uint32_t incoming = 150;
    auto trades = level.match( 99, incoming, 5000 );

    EXPECT_EQ( trades.size(), 1u );
    EXPECT_EQ( incoming, 50u ); // caller keeps matching at the next level
    EXPECT_TRUE( level.isEmpty() );
}

TEST( priceLevelTest, matchAgainstRestingBuyAssignsIdsCorrectly )
{
    PriceLevel level( 10000 );
    level.addOrder( makeOrder( 1, Side::buy, 100 ) );

    uint32_t incoming = 10;
    auto trades = level.match( 99, incoming, 5000 );

    ASSERT_EQ( trades.size(), 1u );
    EXPECT_EQ( trades[0].buy_order_id, 1u );
    EXPECT_EQ( trades[0].sell_order_id, 99u );
}