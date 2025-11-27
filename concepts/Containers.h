#pragma once

#include <concepts>
#include <list>
#include <memory>

#include "../LevelData.h"
#include "Types.h"

template <ValidTypes Types>
class Order;
template <ValidTypes Types>
using OrderPointer = std::shared_ptr<Order<Types>>;

template <typename T, typename Types>
concept OrderMap = requires(T m, Types::OrderId id, OrderPointer<Types> ptr) {
  { m.contains(id) } -> std::same_as<bool>;
  { m.insert({id, ptr}) };
  { m.at(id) } -> std::same_as<OrderPointer<Types>&>;
  { m.erase(id) };
  { m.size() } -> std::same_as<std::size_t>;
};

template <typename T, typename Types>
concept PriceLevels =
    requires(T levels, Types::Price price, OrderPointer<Types> ptr) {
      { levels.empty() } -> std::same_as<bool>;
      { levels.begin() } -> std::same_as<typename T::iterator>;
      { levels.rbegin() } -> std::same_as<typename T::reverse_iterator>;
      levels.erase(price);

      requires requires(decltype(levels[price])& container) {
        container.push_back(ptr);
        container.pop_front();
        { container.front() } -> std::same_as<OrderPointer<Types>&>;
        { container.empty() } -> std::same_as<bool>;
        container.erase(container.begin());
      };
    };

template <typename T, typename Types>
concept LevelInfo =
    requires(T m, Types::Price price, typename Types::Quantity qty) {
      { m[price] };
      m.erase(price);

      requires std::ranges::range<T>;
    };

template <typename T>
concept ValidContainers =
    requires {
      typename T::Types;
      typename T::OrderMap;
      typename T::BidLevels;
      typename T::AskLevels;
      typename T::LevelInfo;
    } && ValidTypes<typename T::Types> &&
    OrderMap<typename T::OrderMap, typename T::Types> &&
    PriceLevels<typename T::BidLevels, typename T::Types> &&
    PriceLevels<typename T::AskLevels, typename T::Types> &&
    LevelInfo<typename T::LevelInfo, typename T::Types>;