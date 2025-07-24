#pragma once

#include <exception>
#include <format>
#include <list>

#include "Constants.h"
#include "OrderType.h"
#include "Side.h"
#include "Usings.h"

class Order {
  friend class Orderbook;

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

 private:
  OrderType orderType_;
  OrderId orderId_;
  Side side_;
  Price price_;
  Quantity initialQuantity_;
  Quantity remainingQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;
