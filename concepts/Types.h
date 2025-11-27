#pragma once
#include <concepts>

template <typename T>
concept Price =
    std::totally_ordered<T> && std::regular<T> && requires(T a, T b) {
      a + b;
      a - b;
      a * b;
      a / b;
    };

template <typename T>
concept Quantity = (std::unsigned_integral<T> || std::floating_point<T>) &&
                   requires(T a, T b) {
                     a + b;
                     a - b;
                   };

template <typename T>
concept OrderId = std::integral<T> && !std::same_as<T, bool>;

template <typename T>
concept ValidTypes =
    requires {
      typename T::Price;
      typename T::Quantity;
      typename T::OrderId;
    } && Price<typename T::Price> && Quantity<typename T::Quantity> &&
    OrderId<typename T::OrderId>;