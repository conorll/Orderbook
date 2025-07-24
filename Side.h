#pragma once
#include <iostream>

enum class Side { Buy, Sell };

inline std::ostream& operator<<(std::ostream& os, Side side) {
  switch (side) {
    case Side::Buy:
      os << "Buy";
      break;
    case Side::Sell:
      os << "Sell";
      break;
    default:
      throw std::logic_error("Attempted to print invalid orderSide");
  }
  return os;
}