cmake_minimum_required(VERSION 3.10.0)
project(OrderbookTest VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-std=c++2c)

include(CTest)
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(OrderbookTest OrderbookTest.cpp ../Orderbook.cpp)
target_link_libraries(OrderbookTest gtest_main)
add_test(NAME OrderbookTest COMMAND OrderbookTest)
