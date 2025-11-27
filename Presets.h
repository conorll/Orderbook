#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>

#include "Orderbook.h"
#include "concepts/Containers.h"
#include "concepts/Params.h"

struct DefaultTypes {
  using Price = uint64_t;
  using Quantity = uint64_t;
  using OrderId = u_int64_t;
};

using DefaultOrderPointers = std::list<OrderPointer<DefaultTypes>>;
struct DefaultContainers {
  using Types = DefaultTypes;
  using OrderMap = std::unordered_map<Types::OrderId, OrderPointer<Types>>;
  using AskLevels =
      std::map<Types::Price, DefaultOrderPointers, std::less<Types::Price>>;
  using BidLevels =
      std::map<Types::Price, DefaultOrderPointers, std::greater<Types::Price>>;
  using LevelInfo = std::unordered_map<Types::Price, LevelData<Types>>;
};

struct DefaultParams {
  using Types = DefaultTypes;
  using Containers = DefaultContainers;
};

using OrderPointersDeque = std::deque<OrderPointer<DefaultTypes>>;
struct ContainersDeque {
  using Types = DefaultTypes;
  using OrderMap = std::unordered_map<Types::OrderId, OrderPointer<Types>>;
  using AskLevels =
      std::map<Types::Price, OrderPointersDeque, std::less<Types::Price>>;
  using BidLevels =
      std::map<Types::Price, OrderPointersDeque, std::greater<Types::Price>>;
  using LevelInfo = std::unordered_map<Types::Price, LevelData<Types>>;
};

struct ParamsDeque {
  using Types = DefaultTypes;
  using Containers = ContainersDeque;
};