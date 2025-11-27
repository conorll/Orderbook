#pragma once

#include <deque>
#include <exception>
#include <format>
#include <sstream>

#include "OrderType.h"
#include "Side.h"
#include "concepts/Params.h"

template <ValidTypes Types>
class Order;

template <ValidTypes Types>
using OrderPointer = std::shared_ptr<Order<Types>>;

template <ValidTypes Types>
class Order {
  template <ValidParams Params>
  friend class Orderbook;

  using Price = typename Types::Price;
  using Quantity = typename Types::Quantity;
  using OrderId = typename Types::OrderId;

  static constexpr Price MarketOrderPrice = Price{0};

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
      : Order(OrderType::Market, orderId, side, MarketOrderPrice, quantity) {}

  bool operator==(const Order&) const = default;
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
        << (orderType_ == OrderType::Market ? "Market"
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
