#include <gtest/gtest.h>

#include "orderTypes.h"

TEST( orderTest, remainingQuantityNoFills )
{
    obe::Order order{ 1, obe::Side::buy, obe::OrderType::limit, 10000, 500, 0, 1000 };

    EXPECT_EQ( order.remaining_quantity(), 500u );
    EXPECT_FALSE( order.is_filled() );
}

TEST( orderTest, remainingQuantityAfterPartialFill )
{
    obe::Order order{ 1, obe::Side::buy, obe::OrderType::limit, 10000, 500, 0, 1000 };
    order.filled_quantity += 200;

    EXPECT_EQ( order.remaining_quantity(), 300u );
    EXPECT_FALSE( order.is_filled() );
}

TEST( orderTest, isFilledWhenFullyMatched )
{
    obe::Order order{ 1, obe::Side::buy, obe::OrderType::limit, 10000, 500, 0, 1000 };
    order.filled_quantity = 500;

    EXPECT_EQ( order.remaining_quantity(), 0u );
    EXPECT_TRUE( order.is_filled() );
}

TEST( orderTest, equalityComparesAllFields )
{
    obe::Order a{ 1, obe::Side::buy, obe::OrderType::limit, 10000, 500, 0, 1000 };
    obe::Order b = a;

    EXPECT_TRUE( a == b );
    b.filled_quantity = 1;
    EXPECT_FALSE( a == b );
}