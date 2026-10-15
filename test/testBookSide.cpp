#include <gtest/gtest.h>

#include "bookSide.h"

using namespace obe;

namespace {
Order makeOrder( uint64_t id, Side side, int64_t price, uint32_t qty )
{
    return Order{ id, side, OrderType::limit, price, qty, 0, id };
}
}

TEST( bookSideTest, startsEmpty )
{
    BookSide bids( Side::buy );
    EXPECT_TRUE( bids.isEmpty() );
    EXPECT_EQ( bids.bestLevel(), nullptr );
}

TEST( bookSideTest, bidBestLevelIsHighestPrice )
{
    BookSide bids( Side::buy );
    bids.addOrder( makeOrder( 1, Side::buy, 9900, 10 ) );
    bids.addOrder( makeOrder( 2, Side::buy, 10100, 10 ) );
    bids.addOrder( makeOrder( 3, Side::buy, 10000, 10 ) );

    ASSERT_NE( bids.bestLevel(), nullptr );
    EXPECT_EQ( bids.bestLevel()->getPrice(), 10100 );
}

TEST( bookSideTest, askBestLevelIsLowestPrice )
{
    BookSide asks( Side::sell );
    asks.addOrder( makeOrder( 1, Side::sell, 10100, 10 ) );
    asks.addOrder( makeOrder( 2, Side::sell, 9900, 10 ) );
    asks.addOrder( makeOrder( 3, Side::sell, 10000, 10 ) );

    EXPECT_EQ( asks.bestLevel()->getPrice(), 9900 );
}

TEST( bookSideTest, ordersAtSamePriceShareOneLevel )
{
    BookSide bids( Side::buy );
    bids.addOrder( makeOrder( 1, Side::buy, 10000, 10 ) );
    bids.addOrder( makeOrder( 2, Side::buy, 10000, 25 ) );

    EXPECT_EQ( bids.levelCount(), 1u );
    EXPECT_EQ( bids.bestLevel()->getTotalQuantity(), 35u );
}

TEST( bookSideTest, cancelByIdWithoutKnowingPrice )
{
    BookSide bids( Side::buy );
    bids.addOrder( makeOrder( 1, Side::buy, 10000, 10 ) );
    bids.addOrder( makeOrder( 2, Side::buy, 10000, 25 ) );

    EXPECT_TRUE( bids.cancelOrder( 1 ) );
    EXPECT_EQ( bids.bestLevel()->getTotalQuantity(), 25u );
}

TEST( bookSideTest, cancelLastOrderErasesLevel )
{
    BookSide bids( Side::buy );
    bids.addOrder( makeOrder( 1, Side::buy, 10100, 10 ) );
    bids.addOrder( makeOrder( 2, Side::buy, 10000, 10 ) );

    EXPECT_TRUE( bids.cancelOrder( 1 ) );
    EXPECT_EQ( bids.levelCount(), 1u );
    EXPECT_EQ( bids.bestLevel()->getPrice(), 10000 ); // best falls through to next level
}

TEST( bookSideTest, cancelUnknownIdReturnsFalse )
{
    BookSide bids( Side::buy );
    bids.addOrder( makeOrder( 1, Side::buy, 10000, 10 ) );

    EXPECT_FALSE( bids.cancelOrder( 999 ) );
    EXPECT_EQ( bids.levelCount(), 1u );
}

TEST( bookSideTest, matchBestPartialFillKeepsLevel )
{
    BookSide asks( Side::sell );
    asks.addOrder( makeOrder( 1, Side::sell, 10000, 100 ) );

    uint32_t incoming = 40;
    auto trades = asks.matchBest( 99, incoming, 5000 );

    ASSERT_EQ( trades.size(), 1u );
    EXPECT_EQ( trades[0].quantity, 40u );
    EXPECT_EQ( incoming, 0u );
    EXPECT_EQ( asks.bestLevel()->getTotalQuantity(), 60u );
}

TEST( bookSideTest, matchBestFullFillErasesLevelAndIndex )
{
    BookSide asks( Side::sell );
    asks.addOrder( makeOrder( 1, Side::sell, 10000, 100 ) );
    asks.addOrder( makeOrder( 2, Side::sell, 10100, 50 ) );

    uint32_t incoming = 100;
    asks.matchBest( 99, incoming, 5000 );

    EXPECT_EQ( incoming, 0u );
    EXPECT_EQ( asks.levelCount(), 1u );
    EXPECT_EQ( asks.bestLevel()->getPrice(), 10100 );
    EXPECT_FALSE( asks.cancelOrder( 1 ) ); // filled order must not linger in the index
}

TEST( bookSideTest, matchBestOnEmptySideDoesNothing )
{
    BookSide asks( Side::sell );
    uint32_t incoming = 10;
    auto trades = asks.matchBest( 99, incoming, 5000 );

    EXPECT_TRUE( trades.empty() );
    EXPECT_EQ( incoming, 10u );
}