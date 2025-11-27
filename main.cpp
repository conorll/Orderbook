#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Orderbook.h"
#include "Presets.h"
#include "concepts/Containers.h"
#include "concepts/Params.h"

using OrderbookT = Orderbook<ParamsDeque>;
using Types = ParamsDeque::Types;

int main() {
  OrderbookT orderbook;

  std::vector<OrderPointer<Types>> orders;

  orders.push_back(std::make_shared<Order<Types>>(OrderType::GoodTillCancel, 1,
                                                  Side::Buy, 101, 10));
  orders.push_back(std::make_shared<Order<Types>>(OrderType::GoodTillCancel, 2,
                                                  Side::Buy, 102, 10));
  orders.push_back(std::make_shared<Order<Types>>(OrderType::GoodTillCancel, 3,
                                                  Side::Buy, 102, 50));
  orders.push_back(std::make_shared<Order<Types>>(OrderType::FillOrKill, 4,
                                                  Side::Sell, 100, 25));

  for (const auto& order : orders) orderbook.AddOrder(order);
}
