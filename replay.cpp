#include "order_book.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>

int main() {
    constexpr int cycles = 250;
    constexpr int depth = 256;
    constexpr int messages = cycles * depth * 3;
    std::array<double, 5> seconds{};
    std::size_t fills = 0;
    std::size_t cancelled = 0;

    for (auto& elapsed : seconds) {
        OrderBook book;
        fills = 0;
        cancelled = 0;
        OrderId next = 1;
        const auto start = std::chrono::steady_clock::now();
        for (int cycle = 0; cycle < cycles; ++cycle) {
            const auto first_ask = next;
            for (int i = 0; i < depth; ++i) {
                book.submit({next++, Side::sell, 100 + i % 32, 2});
            }
            for (int i = 0; i < depth; ++i) {
                fills += book.submit({next++, Side::buy, 131, 1}).size();
            }
            for (int i = 0; i < depth; ++i) {
                cancelled += book.cancel(first_ask + i);
            }
        }
        elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (book.size() != 0 || fills != cycles * depth || cancelled != cycles * depth / 2) {
            std::cerr << "Unexpected replay totals.\n";
            return 1;
        }
    }
    std::sort(seconds.begin(), seconds.end());
    std::cout << messages << " messages/run, " << fills << " fills, " << cancelled
              << " successful cancels\n" << std::fixed << std::setprecision(0)
              << messages / seconds[seconds.size() / 2] << " messages/s (median of five runs)\n";
}
