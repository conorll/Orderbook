#pragma once

#include <format>
#include <stdexcept>
#include <string>

#include "concepts/Types.h"

// expection messages are stored as member variables to avoid constructing a new
// string each time what() is called

template <ValidTypes Types>
class DuplicateOrderIdException : public std::exception {
  using Price = typename Types::Price;
  using Quantity = typename Types::Quantity;
  using OrderId = typename Types::OrderId;

 public:
  DuplicateOrderIdException(OrderId orderId)
      : orderId_(orderId),
        message_(std::format("Duplicate OrderId detected: {}", orderId)) {}

  const char* what() const noexcept override { return message_.c_str(); }

 private:
  OrderId orderId_;
  std::string message_;
};

template <ValidTypes Types>
class OrderNotFoundException : public std::exception {
  using Price = typename Types::Price;
  using Quantity = typename Types::Quantity;
  using OrderId = typename Types::OrderId;

 public:
  OrderNotFoundException(OrderId orderId)
      : orderId_(orderId),
        message_(std::format("Order not found: {}", orderId)) {}

  const char* what() const noexcept override { return message_.c_str(); }

 private:
  OrderId orderId_;
  std::string message_;
};