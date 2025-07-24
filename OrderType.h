#pragma once

enum class OrderType { GoodTillCancel, FillAndKill, FillOrKill, Market };

inline std::ostream& operator<<(std::ostream& os, OrderType orderType) {
  switch (orderType) {
    case OrderType::GoodTillCancel:
      os << "GoodTillCancel";
      break;
    case OrderType::FillAndKill:
      os << "FillAndKill";
      break;
    case OrderType::FillOrKill:
      os << "FillOrKill";
      break;
    case OrderType::Market:
      os << "Market";
      break;
    default:
      throw std::logic_error("Attempted to print invalid orderType");
  }
  return os;
}