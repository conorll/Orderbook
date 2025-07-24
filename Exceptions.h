#pragma once

#include <format>
#include <stdexcept>
#include <string>

#include "Usings.h"

// expection messages are stored as member variables to avoid constructing a new
// string each time what() is called

class DuplicateOrderIdException : public std::exception {
 public:
  DuplicateOrderIdException(OrderId orderId)
      : orderId_(orderId),
        message_(std::format("Duplicate OrderId detected: {}", orderId)) {}

  const char* what() const noexcept override { return message_.c_str(); }

 private:
  OrderId orderId_;
  std::string message_;
};

class OrderNotFoundException : public std::exception {
 public:
  OrderNotFoundException(OrderId orderId)
      : orderId_(orderId),
        message_(std::format("Order not found: {}", orderId)) {}

  const char* what() const noexcept override { return message_.c_str(); }

 private:
  OrderId orderId_;
  std::string message_;
};