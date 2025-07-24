#include <gtest/gtest.h>

#include <algorithm>
#include <barrier>
#include <numeric>
#include <unordered_set>

#include "../Exceptions.h"
#include "../Order.h"
#include "../Orderbook.h"

void CheckOrderbookValidity(OrderbookPointer &orderbook) {
  std::scoped_lock orderbookLock{orderbook->orderbookMutex_};

  for (const auto &[orderId, order] : orderbook->orders_) {
    ASSERT_EQ(order->orderId_, orderId)
        << "OrderId " << orderId << " maps to order with OrderId "
        << order->orderId_ << " in orders_";
  }

  for (const auto &[price, orders] : orderbook->bids_) {
    ASSERT_EQ(orders.empty(), false)
        << "Bid price level $" << price
        << "is empty. Empty price levelis should not exist";
  }

  for (const auto &[price, orders] : orderbook->asks_) {
    ASSERT_EQ(orders.empty(), false)
        << "Ask price level $" << price
        << "is empty. Empty price levels should not exist";
  }

  for (const auto &[price, orders] : orderbook->bids_) {
    for (const auto &order : orders) {
      ASSERT_EQ(order->side_, Side::Buy)
          << "Order " << order->orderId_
          << " is a sell order, but is on the bids_ data structure";
    }
  }

  for (const auto &[price, orders] : orderbook->asks_) {
    for (const auto &order : orders) {
      ASSERT_EQ(order->side_, Side::Sell)
          << "Order " << order->orderId_
          << " is a buy order, but is on the asks_ data structure";
    }
  }

  for (const auto &[orderId, order] : orderbook->orders_) {
    if (order->side_ == Side::Buy) {
      ASSERT_EQ(orderbook->bids_.contains(order->price_), true)
          << "Bid with orderId " << order->orderId_ << "has a price of $"
          << order->price_ << ", but no bid level exists with that price";
    }
  }

  for (const auto &[orderId, order] : orderbook->orders_) {
    if (order->side_ == Side::Sell) {
      ASSERT_EQ(orderbook->asks_.contains(order->price_), true)
          << "Ask with orderId " << order->orderId_ << "has a price of $"
          << order->price_ << ", but no ask level exists with that price";
    }
  }

  for (const auto &[orderId, order] : orderbook->orders_) {
    if (order->side_ == Side::Buy) {
      const auto &orders = orderbook->bids_.at(order->price_);
      const auto &orderIt = std::find(orders.begin(), orders.end(), order);
      ASSERT_NE(orderIt, orders.end())
          << "Bid level with price $" << order->price_
          << " exists, but bid with order id " << order->orderId_
          << " and price " << order->price_
          << "does not exist at that bid level";
    }
  }

  for (const auto &[orderId, order] : orderbook->orders_) {
    if (order->side_ == Side::Sell) {
      const auto &orders = orderbook->asks_.at(order->price_);
      const auto &orderIt = std::find(orders.begin(), orders.end(), order);
      ASSERT_NE(orderIt, orders.end())
          << "Ask level with price $" << order->price_
          << " exists, but ask with order id " << order->orderId_
          << " and price " << order->price_
          << "does not exist at that ask level";
    }
  }

  for (const auto &[price, orders] : orderbook->bids_) {
    for (const auto &order : orders) {
      ASSERT_EQ(orderbook->orders_.contains(order->orderId_), true)
          << "Order " << order->orderId_
          << "exists in bids_ but does not exist in orders_";
    }
  }

  for (const auto &[price, orders] : orderbook->asks_) {
    for (const auto &order : orders) {
      ASSERT_EQ(orderbook->orders_.contains(order->orderId_), true)
          << "Order " << order->orderId_
          << "exists in asks_ but does not exist in orders_";
    }
  }

  for (const auto &[price, orders] : orderbook->bids_) {
    for (const auto &order : orders) {
      const auto &orderFromOrders = orderbook->orders_.at(order->orderId_);
      ASSERT_EQ(order, orderFromOrders)
          << "Duplicate order exists. Existing order: "
          << orderFromOrders->ToString()
          << "\nDuplicate order: " << order->ToString()
          << "\nDuplicate order found in bids_ at price level $" << price;
    }
  }

  for (const auto &[price, orders] : orderbook->asks_) {
    for (const auto &order : orders) {
      const auto &orderFromOrders = orderbook->orders_.at(order->orderId_);
      ASSERT_EQ(order, orderFromOrders)
          << "Duplicate order exists. Existing order: "
          << orderFromOrders->ToString()
          << "\nDuplicate order: " << order->ToString()
          << "\nDuplicate order found in asks_ at price level $" << price;
    }
  }

  // if else required since bids_ and asks_ are technically different types
  if (orderbook->bids_.size() > orderbook->asks_.size()) {
    for (const auto &[price, _] : orderbook->bids_) {
      ASSERT_EQ(orderbook->asks_.contains(price), false)
          << "Price level $" << price
          << "exists on both bids_ and asks_. A price level should only exist "
             "on one side, not both";
    }
  } else {
    for (const auto &[price, _] : orderbook->asks_) {
      ASSERT_EQ(orderbook->bids_.contains(price), false)
          << "Price level $" << price
          << "exists on both bids_ and asks_. A price level should only exist "
             "on one side, not both";
    }
  }

  for (const auto &[price, orders] : orderbook->bids_) {
    ASSERT_EQ(orderbook->data_.contains(price), true)
        << "Bid price level $" << price
        << " exists but no entry exists in data_ with that price";
  }

  for (const auto &[price, orders] : orderbook->asks_) {
    ASSERT_EQ(orderbook->data_.contains(price), true)
        << "Ask price level $" << price
        << " exists but no entry exists in data_ with that price";
  }

  for (const auto &[price, levelData] : orderbook->data_) {
    ASSERT_EQ(
        orderbook->bids_.contains(price) || orderbook->asks_.contains(price),
        true)
        << "data_ contains an entry for price $" << price
        << " but that price level does not exist in bids_ or asks_";
  }

  for (const auto &[price, orders] : orderbook->bids_) {
    const auto &levelData = orderbook->data_.at(price);
    ASSERT_EQ(levelData.count_, orders.size())
        << orders.size() << " orders exist on bids price level $" << price
        << " but levelData count for that price is " << levelData.count_;
  }

  for (const auto &[price, orders] : orderbook->asks_) {
    const auto &levelData = orderbook->data_.at(price);
    ASSERT_EQ(levelData.count_, orders.size())
        << orders.size() << " orders exist on asks_ price level $" << price
        << " but levelData count for that price is " << levelData.count_;
  }

  for (const auto &[price, orders] : orderbook->bids_) {
    const auto &levelData = orderbook->data_.at(price);
    Quantity levelQuantity =
        std::accumulate(orders.begin(), orders.end(), Quantity{0},
                        [](Quantity sum, const OrderPointer &order) {
                          return sum + order->remainingQuantity_;
                        });
    ASSERT_EQ(levelData.quantity_, levelQuantity)
        << "Cumulative quantity of orders at bid price level $" << price
        << " is " << levelQuantity
        << ", but levelData quantity for that price is " << levelData.quantity_;
  }

  for (const auto &[price, orders] : orderbook->asks_) {
    const auto &levelData = orderbook->data_.at(price);
    Quantity levelQuantity =
        std::accumulate(orders.begin(), orders.end(), Quantity{0},
                        [](Quantity sum, const OrderPointer &order) {
                          return sum + order->remainingQuantity_;
                        });
    ASSERT_EQ(levelData.quantity_, levelQuantity)
        << "Cumulative quantity of orders at ask price level $" << price
        << " is " << levelQuantity
        << ", but levelData quantity for that price is " << levelData.quantity_;
  }
}

template <typename T>
bool HasDuplicates(const std::vector<T> &vec) {
  std::unordered_set<T> seen;
  for (const T &item : vec) {
    if (seen.count(item)) {
      return true;
    }
    seen.insert(item);
  }
  return false;
}

// template cannot be used since pointer must be dereferenced for comparison
bool IsSubsequence(const std::vector<OrderPointer> &sequence,
                   const OrderPointers &subsequence) {
  auto it = sequence.begin();
  for (const auto &val : subsequence) {
    it = std::find_if(
        it, sequence.end(),
        [&val](const OrderPointer &candidate) { return *candidate == *val; });
    if (it == sequence.end()) return false;
    ++it;
  }
  return true;
}

void CheckOrdersMatch(OrderbookPointer &orderbook,
                      std::vector<OrderPointer> &orders) {
  if (HasDuplicates(orders))
    throw std::logic_error("Order vector contains duplicate entries");

  std::scoped_lock orderbookLock{orderbook->orderbookMutex_};

  for (const auto &order : orders) {
    ASSERT_EQ(orderbook->orders_.contains(order->orderId_), true)
        << "Orderbook is missing expected order: " << order->ToString();
  }

  std::unordered_set<OrderId> ordersSet;
  for (const auto &order : orders) ordersSet.insert(order->orderId_);

  for (const auto &[orderId, order] : orderbook->orders_) {
    ASSERT_EQ(ordersSet.contains(orderId), true)
        << "Orderbook contains unexpected order: " << order->ToString();
  }

  for (const auto &order : orders) {
    const auto &orderInOrderbook = orderbook->orders_.at(order->orderId_);
    ASSERT_EQ(*order, *orderInOrderbook)
        << "Order " << orderInOrderbook->orderId_
        << "has incorrect values. Expected order: " << order->ToString()
        << "\nActual order: " << orderInOrderbook->ToString();
  }

  for (const auto &[price, levelOrders] : orderbook->bids_) {
    if (IsSubsequence(orders, levelOrders)) continue;
    std::string levelIds{};
    for (const auto &order : levelOrders)
      levelIds += std::to_string(order->orderId_) + " ";

    ASSERT_EQ(IsSubsequence(orders, levelOrders), true)
        << "Order of orders at bids price level $" << price
        << " does not match order of orders in vector. Order Ids at bid price "
           "level $"
        << price << ": " << levelIds;
  }

  for (const auto &[price, levelOrders] : orderbook->asks_) {
    if (IsSubsequence(orders, levelOrders)) continue;
    std::string levelIds{};
    for (const auto &order : levelOrders)
      levelIds += std::to_string(order->orderId_) + " ";

    ASSERT_EQ(IsSubsequence(orders, levelOrders), true)
        << "Order of orders at asks price level $" << price
        << " does not match order of orders in vector. Orders Ids at ask price "
           "level $"
        << price << ": " << levelIds;
  }
}

bool DoOrdersMatch(OrderbookPointer &orderbook,
                   std::vector<OrderPointer> &orders) {
  if (HasDuplicates(orders))
    throw std::logic_error("Order vector contains duplicate entries");

  std::scoped_lock orderbookLock{orderbook->orderbookMutex_};

  for (const auto &order : orders)
    if (!orderbook->orders_.contains(order->orderId_)) return false;

  std::unordered_set<OrderId> ordersSet;
  for (const auto &order : orders) ordersSet.insert(order->orderId_);

  for (const auto &[orderId, _] : orderbook->orders_)
    if (!ordersSet.contains(orderId)) return false;

  for (const auto &order : orders) {
    const auto &orderInOrderbook = orderbook->orders_.at(order->orderId_);
    if (*order != *orderInOrderbook) return false;
  }

  for (const auto &[price, levelOrders] : orderbook->bids_)
    if (!IsSubsequence(orders, levelOrders)) return false;

  for (const auto &[price, levelOrders] : orderbook->asks_)
    if (!IsSubsequence(orders, levelOrders)) return false;

  return true;
}

OrderPointer createPartiallyFilledOrder(OrderType orderType, OrderId orderId,
                                        Side side, Price price,
                                        Quantity initialQuantity,
                                        Quantity remainingQuantity) {
  auto order =
      std::make_shared<Order>(orderType, orderId, side, price, initialQuantity);
  order->remainingQuantity_ = remainingQuantity;

  return order;
}

std::string OrdersToString(const std::vector<OrderPointer> &orders) {
  std::stringstream ss;
  ss << "[";
  for (size_t i = 0; i < orders.size(); ++i) {
    if (orders[i]) {
      ss << orders[i]->ToString();
      if (i < orders.size() - 1) ss << ", ";
    }
  }
  ss << "]";
  return ss.str();
}

TEST(OrderbookTest, Add) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Sell, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Sell, 100, 6));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Sell, 100, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Sell, 100, 6));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, AddDuplicateOrderId) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Sell, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Sell, 100, 6));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  for (const auto &order : orders) orderbook->AddOrder(order);

  EXPECT_THROW(orderbook->AddOrder(std::make_shared<Order>(
                   OrderType::GoodTillCancel, 1, Side::Buy, 98, 20)),
               DuplicateOrderIdException);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Sell, 100, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Sell, 100, 6));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, Modify) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100, 6));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Buy, 99, 8));

  for (const auto &order : orders) orderbook->AddOrder(order);

  orderbook->ModifyOrder(OrderModify(2, Side::Sell, 101, 7));

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Buy, 100, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Sell, 101, 7));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Buy, 99, 8));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, ModifyNonExistingOrder) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100, 6));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Buy, 99, 8));

  for (const auto &order : orders) orderbook->AddOrder(order);

  EXPECT_THROW(orderbook->ModifyOrder(OrderModify(4, Side::Sell, 101, 7)),
               OrderNotFoundException);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Buy, 100, 10));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100, 6));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Buy, 99, 8));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, Cancel) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Sell, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Sell, 100, 6));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  for (const auto &order : orders) orderbook->AddOrder(order);

  orderbook->CancelOrder(1);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Sell, 100, 6));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, CancelNonExistingOrder) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Sell, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Sell, 100, 6));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  for (const auto &order : orders) orderbook->AddOrder(order);

  EXPECT_THROW(orderbook->CancelOrder(5), OrderNotFoundException);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Sell, 100, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Sell, 100, 6));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 8));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, FillAndKill_AggressorConstrained) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 101, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 102, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Buy, 102, 50));
  orders.push_back(
      std::make_shared<Order>(OrderType::FillAndKill, 4, Side::Sell, 100, 25));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Buy, 101, 10));
  expectedOrders.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 3, Side::Buy, 102, 50, 35));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, FillAndKill_TakerConstrained) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 99, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 101, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Buy, 103, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 4,
                                           Side::Buy, 102, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 5, Side::Buy, 98, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::FillAndKill, 6, Side::Sell, 100, 50));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 99, 10));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 5, Side::Buy, 98, 10));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, FillAndKill_Miss) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 100, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::FillAndKill, 3, Side::Sell, 101, 25));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Buy, 100, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Buy, 100, 10));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, FillOrKill_Hit) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 101, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 102, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Buy, 102, 50));
  orders.push_back(
      std::make_shared<Order>(OrderType::FillOrKill, 4, Side::Sell, 100, 25));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Buy, 101, 10));
  expectedOrders.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 3, Side::Buy, 102, 50, 35));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, FillOrKill_Miss) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 99, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 101, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Buy, 103, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 4,
                                           Side::Buy, 102, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 5, Side::Buy, 98, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::FillOrKill, 6, Side::Sell, 100, 50));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 99, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Buy, 101, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                                   Side::Buy, 103, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 4,
                                                   Side::Buy, 102, 10));
  expectedOrders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 5, Side::Buy, 98, 10));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, GoodTillCancel_AggressorConstrained) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 101, 50));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Sell, 100, 20));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Buy, 100, 10));
  expectedOrders.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 2, Side::Buy, 101, 50, 30));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, GoodTillCancel_TakerConstrained) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 101, 20));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Sell, 100, 50));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 3, Side::Sell, 100, 50, 20));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, GoodTillCancel_Multiple) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 101, 50));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Sell, 100, 20));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 4,
                                           Side::Sell, 99, 31));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 5, Side::Sell, 98, 5));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 1, Side::Buy, 100, 10, 4));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, GoodTillCancel_Miss) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                           Side::Sell, 102, 20));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 101, 50));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 4,
                                           Side::Sell, 103, 31));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                                   Side::Buy, 100, 10));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 3,
                                                   Side::Sell, 102, 20));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                                   Side::Buy, 101, 50));
  expectedOrders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 4,
                                                   Side::Sell, 103, 31));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, Market_TakerContrained) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 50));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 101, 10));
  orders.push_back(std::make_shared<Order>(3, Side::Sell, 20));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  expectedOrders.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 1, Side::Buy, 100, 50, 40));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, Market_AggressorConstrained) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 101, 10));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 2, Side::Buy, 100, 5));
  orders.push_back(std::make_shared<Order>(3, Side::Sell, 50));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  // Market order converts to GoodTillCancel Order
  expectedOrders.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 3, Side::Sell, 100, 50, 35));

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, Market_EmptyOrderbook) {
  auto orderbook = std::make_shared<Orderbook>();

  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(1, Side::Sell, 50));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderPointer> expectedOrders;

  CheckOrderbookValidity(orderbook);
  CheckOrdersMatch(orderbook, expectedOrders);
}

TEST(OrderbookTest, Concurrent_Add) {
  auto orderbook = std::make_shared<Orderbook>();
  std::vector<std::thread> threads;
  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Sell, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Sell, 100, 6));

  std::barrier barrier(orders.size());

  for (const auto &order : orders) {
    threads.emplace_back([&orderbook, &order, &barrier]() {
      barrier.arrive_and_wait();
      orderbook->AddOrder(order);
    });
  }

  for (auto &thread : threads) thread.join();

  std::vector<OrderPointer> expectedOrders1;

  expectedOrders1.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    1, Side::Sell, 100, 10));
  expectedOrders1.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    2, Side::Sell, 100, 6));

  std::vector<OrderPointer> expectedOrders2;

  expectedOrders2.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    2, Side::Sell, 100, 6));
  expectedOrders2.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    1, Side::Sell, 100, 10));

  CheckOrderbookValidity(orderbook);

  ASSERT_EQ((DoOrdersMatch(orderbook, expectedOrders1) ||
             DoOrdersMatch(orderbook, expectedOrders2)),
            true)
      << "Orderbook does not match any expected state.\n"
      << "Expected state 1: " << OrdersToString(expectedOrders1) << "\n"
      << "Expected state 2: " << OrdersToString(expectedOrders2) << "\n"
      << "Actual orderbook: " << orderbook->ToString();
}

TEST(OrderbookTest, Concurrent_GoodTillCancel) {
  auto orderbook = std::make_shared<Orderbook>();
  std::vector<std::thread> threads;
  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 100, 20));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Buy, 100, 30));
  orders.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 3, Side::Sell, 99, 5));

  std::barrier barrier(orders.size());

  for (const auto &order : orders) {
    threads.emplace_back([&orderbook, &order, &barrier]() {
      barrier.arrive_and_wait();
      orderbook->AddOrder(order);
    });
  }

  for (auto &thread : threads) thread.join();

  std::vector<OrderPointer> expectedOrders1;

  expectedOrders1.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 1, Side::Buy, 100, 20, 15));
  expectedOrders1.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    2, Side::Buy, 100, 30));

  std::vector<OrderPointer> expectedOrders2;

  expectedOrders2.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 2, Side::Buy, 100, 30, 25));
  expectedOrders2.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    1, Side::Buy, 100, 20));

  CheckOrderbookValidity(orderbook);

  ASSERT_EQ((DoOrdersMatch(orderbook, expectedOrders1) ||
             DoOrdersMatch(orderbook, expectedOrders2)),
            true)
      << "Orderbook does not match any expected state.\n"
      << "Expected state 1: " << OrdersToString(expectedOrders1) << "\n"
      << "Expected state 2: " << OrdersToString(expectedOrders2) << "\n"
      << "Actual orderbook: " << orderbook->ToString();
}

TEST(OrderbookTest, Concurrent_FillAndKill) {
  auto orderbook = std::make_shared<Orderbook>();
  std::vector<std::thread> threads;
  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 101, 40));
  orders.push_back(
      std::make_shared<Order>(OrderType::FillAndKill, 2, Side::Sell, 99, 30));

  std::barrier barrier(orders.size());

  for (const auto &order : orders) {
    threads.emplace_back([&orderbook, &order, &barrier]() {
      barrier.arrive_and_wait();
      orderbook->AddOrder(order);
    });
  }

  for (auto &thread : threads) thread.join();

  std::vector<OrderPointer> expectedOrders1;

  expectedOrders1.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    1, Side::Buy, 101, 40));

  std::vector<OrderPointer> expectedOrders2;

  expectedOrders2.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 1, Side::Buy, 101, 40, 10));

  CheckOrderbookValidity(orderbook);

  ASSERT_EQ((DoOrdersMatch(orderbook, expectedOrders1) ||
             DoOrdersMatch(orderbook, expectedOrders2)),
            true)
      << "Orderbook does not match any expected state.\n"
      << "Expected state 1: " << OrdersToString(expectedOrders1) << "\n"
      << "Expected state 2: " << OrdersToString(expectedOrders2) << "\n"
      << "Actual orderbook: " << orderbook->ToString();
}

TEST(OrderbookTest, Concurrent_FillOrKill) {
  auto orderbook = std::make_shared<Orderbook>();
  std::vector<std::thread> threads;
  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 101, 40));
  orders.push_back(
      std::make_shared<Order>(OrderType::FillOrKill, 2, Side::Sell, 99, 30));

  std::barrier barrier(orders.size());

  for (const auto &order : orders) {
    threads.emplace_back([&orderbook, &order, &barrier]() {
      barrier.arrive_and_wait();
      orderbook->AddOrder(order);
    });
  }

  for (auto &thread : threads) thread.join();

  std::vector<OrderPointer> expectedOrders1;

  expectedOrders1.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    1, Side::Buy, 101, 40));

  std::vector<OrderPointer> expectedOrders2;

  expectedOrders2.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 1, Side::Buy, 101, 40, 10));

  CheckOrderbookValidity(orderbook);

  ASSERT_EQ((DoOrdersMatch(orderbook, expectedOrders1) ||
             DoOrdersMatch(orderbook, expectedOrders2)),
            true)
      << "Orderbook does not match any expected state.\n"
      << "Expected state 1: " << OrdersToString(expectedOrders1) << "\n"
      << "Expected state 2: " << OrdersToString(expectedOrders2) << "\n"
      << "Actual orderbook: " << orderbook->ToString();
}

TEST(OrderbookTest, Concurrent_MarketOrder) {
  auto orderbook = std::make_shared<Orderbook>();
  std::vector<std::thread> threads;
  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Buy, 101, 20));
  orders.push_back(std::make_shared<Order>(2, Side::Sell, 50));

  std::barrier barrier(orders.size());

  for (const auto &order : orders) {
    threads.emplace_back([&orderbook, &order, &barrier]() {
      barrier.arrive_and_wait();
      orderbook->AddOrder(order);
    });
  }

  for (auto &thread : threads) thread.join();

  std::vector<OrderPointer> expectedOrders1;

  expectedOrders1.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    1, Side::Buy, 101, 20));

  std::vector<OrderPointer> expectedOrders2;

  expectedOrders2.push_back(createPartiallyFilledOrder(
      OrderType::GoodTillCancel, 2, Side::Sell, 101, 50, 30));

  CheckOrderbookValidity(orderbook);

  ASSERT_EQ((DoOrdersMatch(orderbook, expectedOrders1) ||
             DoOrdersMatch(orderbook, expectedOrders2)),
            true)
      << "Orderbook does not match any expected state.\n"
      << "Expected state 1: " << OrdersToString(expectedOrders1) << "\n"
      << "Expected state 2: " << OrdersToString(expectedOrders2) << "\n"
      << "Actual orderbook: " << orderbook->ToString();
}

TEST(OrderbookTest, Concurrent_Modify) {
  auto orderbook = std::make_shared<Orderbook>();
  std::vector<std::thread> threads;
  std::vector<OrderPointer> orders;

  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 1,
                                           Side::Sell, 100, 10));
  orders.push_back(std::make_shared<Order>(OrderType::GoodTillCancel, 2,
                                           Side::Sell, 100, 6));

  for (const auto &order : orders) orderbook->AddOrder(order);

  std::vector<OrderModify> orderModifies;

  orderModifies.push_back(OrderModify(1, Side::Buy, 99, 50));
  orderModifies.push_back(OrderModify(1, Side::Buy, 98, 20));

  std::barrier barrier(orderModifies.size());

  for (const auto &orderModify : orderModifies) {
    threads.emplace_back([&orderbook, &orderModify, &barrier]() {
      barrier.arrive_and_wait();
      orderbook->ModifyOrder(orderModify);
    });
  }

  for (auto &thread : threads) thread.join();

  std::vector<OrderPointer> expectedOrders1;

  expectedOrders1.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    2, Side::Sell, 100, 6));
  expectedOrders1.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 99, 50));

  std::vector<OrderPointer> expectedOrders2;

  expectedOrders2.push_back(std::make_shared<Order>(OrderType::GoodTillCancel,
                                                    2, Side::Sell, 100, 6));
  expectedOrders2.push_back(
      std::make_shared<Order>(OrderType::GoodTillCancel, 1, Side::Buy, 98, 20));

  CheckOrderbookValidity(orderbook);

  ASSERT_EQ((DoOrdersMatch(orderbook, expectedOrders1) ||
             DoOrdersMatch(orderbook, expectedOrders2)),
            true)
      << "Orderbook does not match any expected state.\n"
      << "Expected state 1: " << OrdersToString(expectedOrders1) << "\n"
      << "Expected state 2: " << OrdersToString(expectedOrders2) << "\n"
      << "Actual orderbook: " << orderbook->ToString();
}