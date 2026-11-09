cmake_minimum_required(VERSION 3.16)
project(limit_order_book LANGUAGES CXX)

add_library(order_book order_book.cpp)
target_compile_features(order_book PUBLIC cxx_std_17)
set_target_properties(order_book PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(order_book PUBLIC /W4 /permissive-)
else()
    target_compile_options(order_book PUBLIC -Wall -Wextra -Wpedantic)
endif()

add_executable(order_book_test order_book_test.cpp)
target_link_libraries(order_book_test PRIVATE order_book)
add_executable(replay replay.cpp)
target_link_libraries(replay PRIVATE order_book)

enable_testing()
add_test(NAME order_book COMMAND order_book_test)
