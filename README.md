# Limit order book

A C++17 matching engine for one instrument. It matches the best price first, then matches orders at that price in arrival order. Prices are integer ticks and trades execute at the resting order's price. An unmatched remainder joins its side of the book.

For example, two asks at 100 for 3 and 5 units, followed by a buy for 6 at 100, produce fills of 3 and 3. The second ask keeps its place with 2 units left.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Run `build/replay` on single-config builds or `build/Release/replay.exe` with Visual Studio. Each run processes 192,000 messages across 32 price levels and checks for 64,000 fills and 32,000 successful cancels. The median of five runs on an i9-13900H with MSVC x64 Release was **6.3 million messages/s**. The timer covers the single-threaded in-process loop, including its allocations.

Ordered maps hold the price levels. A FIFO list at each level and a hash index by order ID give expected constant-time cancellation. Posting a remainder adds a logarithmic lookup by price. The submit path first plans the fills and allocates the remainder, then applies the fills. The test suite compares 2,500 seeded add/cancel events with a simple linear-scan matcher.
