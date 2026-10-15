# Limit Order Book & Matching Engine

This is a single-instrument limit order book and matching engine written in C++17. Enforces strict price-time priority, supports limit and market orders, partial fills, and cancellation by order ID. Built as a library with a GoogleTest suite.
- WIP: incorporating a Google Benchmark harness

## Implementation

Two red-black trees (one for bids and one for asks) maintain limit price priority implemented with `std::map`. The map is keyed by integer tick price.
- Bids are sorted by `std::greater`
- Asks are sorted by `std::less`

Every price level uses a double-ended queue `std::deque` with objects of type `Order` to sort by time priority.

Both the bid and ask side use a cancellation directory implemented with `std::unordered_map` that maps each order ID to its price.

Cached depth:
- Each price level keeps a running total of its resting quantity
- Total is updated on every add, cancel, and fill
- The depth at any price is O(1)

#### Time Complexity
n = # distinct price levels, k = # orders at a given level
- Best bid / best ask: O(1)
- Add order: O(log(n)) to find / create level, then O(1) to append
- Cancel Order: O(1) ID lookup, O(log(n)) to find level, O(k) to scan within it
- Fill against a level: O(1) per fill, amortized O(1) for erasing an emptied level

## Features

- Price-time priority algorithm (FIFO) for matching orders.
- Order types: limit (unfilled remainder rests) and market (unfilled remainder is discarded).
- Partial fills across multiple price levels that stop at the incoming limit price.
- Trades execute at the resting order's price. An aggressive order can get a better price than its limit.
- Input validation. Rejects zero quantity, non-positive limit prices, and duplicate IDs of currently resting orders.

## Testing

- GoogleTest suites for orders, price levels, book side, and matching engine (validation, resting, crossing, market orders, cancel).
- Covers FIFO ordering, partial and full fills, multi-level sweeps, stale-index cleanup after fills, and ID reuse after cancel.

## Getting Started

### Installation
```bash
git clone https://github.com/michellepage/limit-order-book
```

### Building and Running Tests

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```
## References

- WK Selph, "How to Build a Fast Limit Order Book" (2011)
- https://github.com/github/gitignore
