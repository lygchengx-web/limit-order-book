# Limit order book

C++17 matching engine for a single instrument. Orders trade at the best available price, then in arrival order at that price. Trades use the resting order's price. A partially filled order keeps its place in the queue.

## A fill across two prices

Order 1 arrives first, asking 102 for four units. Orders 2 and 3 then join at 100 for three and five units. An incoming buy for nine at 102 trades in this order.

| Resting order | Price (ticks) | Units filled | Units still resting |
| --- | ---: | ---: | ---: |
| 2 | 100 | 3 | 0 |
| 3 | 100 | 5 | 0 |
| 1 | 102 | 1 | 3 |

This is the first case in the [test suite](order_book_test.cpp). A second check compares 2,500 seeded add and cancel events against a linear-scan matcher, checking fills, remaining quantities and the best bid and ask after each event.

## Data structures and order updates

- A sorted map holds each side's price levels. A FIFO list at each price keeps arrival order even when a resting order is partially filled.
- An ID index stores the price-level and order iterators. Finding and removing an order takes expected constant time. Inserting a new price level takes logarithmic time in the number of levels.
- [`submit`](order_book.cpp#L7) collects the fills and allocates any remainder before changing the orders it fills. If allocation fails during that work, the existing book stays intact.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `./build/replay` with a single-config generator or `.\build\Release\replay.exe` in PowerShell with Visual Studio.

The [replay](replay.cpp) processes 192,000 synthetic messages across 32 prices and checks that they produce 64,000 fills, 32,000 successful cancels and an empty book. The recorded median of five runs on an i9-13900H with MSVC x64 Release was **6.3 million messages per second**. The timer covers the single-threaded, in-memory loop and its allocations.
