#pragma once

#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "Order.h"
#include "OrderModify.h"
#include "Trade.h"
#include "Usings.h"

class Orderbook;

class Orderbook {
  friend void CheckOrderbookValidity(OrderbookPointer &orderbook);
  friend void CheckOrdersMatch(OrderbookPointer &orderbook,
                               std::vector<OrderPointer> &orders);
  friend bool DoOrdersMatch(OrderbookPointer &orderbook,
                            std::vector<OrderPointer> &orders);

 private:
  struct LevelData {
    Quantity quantity_{};
    Quantity count_{};

    enum class Action {
      Add,
      Remove,
      Match,
    };
  };

  std::unordered_map<Price, LevelData> data_;
  std::map<Price, OrderPointers, std::greater<Price>> bids_;
  std::map<Price, OrderPointers, std::less<Price>> asks_;
  std::unordered_map<OrderId, OrderPointer> orders_;
  mutable std::mutex orderbookMutex_;

  Trades AddOrderInternal(OrderPointer order);

  void CancelOrders(OrderIds orderIds);
  void CancelOrderInternal(OrderId orderId);

  void OnOrderCancelled(OrderPointer order);
  void OnOrderAdded(OrderPointer order);
  void OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled);
  void UpdateLevelData(Price price, Quantity quantity,
                       LevelData::Action action);

  bool CanFullyFill(Side side, Price price, Quantity quantity) const;
  bool CanMatch(Side side, Price price) const;
  Trades MatchOrders();

 public:
  Trades AddOrder(OrderPointer order);
  void CancelOrder(OrderId orderId);
  Trades ModifyOrder(OrderModify order);

  std::string ToString();
};
