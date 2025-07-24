#pragma once

#include <deque>
#include <exception>
#include <format>
#include <sstream>

#include "Constants.h"
#include "OrderType.h"
#include "Side.h"
#include "Usings.h"

class Order;
class Orderbook;

using OrderPointer = std::shared_ptr<Order>;
using OrderbookPointer = std::shared_ptr<Orderbook>;
using OrderPointers = std::deque<OrderPointer>;

class Order {
  friend class Orderbook;
  friend void CheckOrderbookValidity(OrderbookPointer &orderbook);
  friend void CheckOrdersMatch(OrderbookPointer &orderbook,
                               std::vector<OrderPointer> &orders);
  friend OrderPointer createPartiallyFilledOrder(OrderType orderType,
                                                 OrderId orderId, Side side,
                                                 Price price,
                                                 Quantity initialQuantity,
                                                 Quantity remainingQuantity);
  friend bool DoOrdersMatch(OrderbookPointer &orderbook,
                            std::vector<OrderPointer> &orders);

 public:
  Order(OrderType orderType, OrderId orderId, Side side, Price price,
        Quantity quantity)
      : orderType_{orderType},
        orderId_{orderId},
        side_{side},
        price_{price},
        initialQuantity_{quantity},
        remainingQuantity_{quantity} {}

  Order(OrderId orderId, Side side, Quantity quantity)
      : Order(OrderType::Market, orderId, side, Constants::InvalidPrice,
              quantity) {}

  bool operator==(const Order &) const = default;
  bool IsFilled() const { return remainingQuantity_ == 0; }
  void Fill(Quantity quantity) {
    if (quantity > remainingQuantity_)
      throw std::logic_error(std::format(
          "Order ({}) cannot be filled for more than its remaining quantity.",
          orderId_));

    remainingQuantity_ -= quantity;
  }
  void ToGoodTillCancel(Price price) {
    if (orderType_ != OrderType::Market)
      throw std::logic_error(std::format(
          "Order ({}) cannot have its price adjusted, only market orders can.",
          orderId_));

    price_ = price;
    orderType_ = OrderType::GoodTillCancel;
  }

  std::string ToString() const {
    std::ostringstream oss;
    oss << "(type=" << orderType_ << ", id=" << orderId_ << ", side=" << side_
        << ", price="
        << (price_ == Constants::InvalidPrice ? "Market"
                                              : ("$" + std::to_string(price_)))
        << ", initialQty=" << initialQuantity_
        << ", remainingQty=" << remainingQuantity_ << ")";
    return oss.str();
  }

 private:
  OrderType orderType_;
  OrderId orderId_;
  Side side_;
  Price price_;
  Quantity initialQuantity_;
  Quantity remainingQuantity_;
};
