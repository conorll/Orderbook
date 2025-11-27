#pragma once

#include "Order.h"

template <ValidTypes Types>
class OrderModify {
  using Price = typename Types::Price;
  using Quantity = typename Types::Quantity;
  using OrderId = typename Types::OrderId;

 public:
  OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
      : orderId_{orderId}, price_{price}, side_{side}, quantity_{quantity} {}

  OrderId GetOrderId() const { return orderId_; }
  Price GetPrice() const { return price_; }
  Side GetSide() const { return side_; }
  Quantity GetQuantity() const { return quantity_; }

  OrderPointer<Types> ToOrderPointer(OrderType type) const {
    return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(),
                                   GetQuantity());
  }

 private:
  OrderId orderId_;
  Price price_;
  Side side_;
  Quantity quantity_;
};
