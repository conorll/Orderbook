#pragma once

#include "concepts/Types.h"

template <ValidTypes Types>
struct TradeInfo {
  using Price = typename Types::Price;
  using Quantity = typename Types::Quantity;
  using OrderId = typename Types::OrderId;

  OrderId orderId_;
  Price price_;
  Quantity quantity_;
};