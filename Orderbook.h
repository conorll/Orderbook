#pragma once

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <map>
#include <mutex>
#include <numeric>
#include <thread>
#include <unordered_map>

#include "Exceptions.h"
#include "LevelData.h"
#include "Order.h"
#include "OrderModify.h"
#include "Trade.h"
#include "concepts/Params.h"

template <ValidParams Params>
class Orderbook;

template <ValidParams Params>
class Orderbook {
  using Types = typename Params::Types;

  using Price = typename Types::Price;
  using Quantity = typename Types::Quantity;
  using OrderId = typename Types::OrderId;

  using OrderIds = typename std::vector<OrderId>;

  using Trades = std::vector<Trade<Types>>;

  using Containers = typename Params::Containers;

  using OrderMap = Containers::OrderMap;
  using BidLevels = Containers::BidLevels;
  using AskLevels = Containers::AskLevels;
  using LevelInfo = Containers::LevelInfo;

  OrderMap orders_;
  BidLevels bids_;
  AskLevels asks_;
  LevelInfo data_;
  mutable std::mutex orderbookMutex_;

  Trades AddOrderInternal(OrderPointer<Types> order) {
    if (orders_.contains(order->orderId_))
      throw DuplicateOrderIdException<Types>(order->orderId_);

    if (order->orderType_ == OrderType::Market) {
      if (order->side_ == Side::Buy) {
        if (asks_.empty()) return {};
        const auto& [worstAsk, _] = *asks_.rbegin();
        order->ToGoodTillCancel(worstAsk);
      } else {
        if (bids_.empty()) return {};
        const auto& [worstBid, _] = *bids_.rbegin();
        order->ToGoodTillCancel(worstBid);
      }
    }

    if (order->orderType_ == OrderType::FillAndKill &&
        !CanMatch(order->side_, order->price_))
      return {};

    if (order->orderType_ == OrderType::FillOrKill &&
        !CanFullyFill(order->side_, order->price_, order->initialQuantity_))
      return {};

    if (order->side_ == Side::Buy) {
      bids_[order->price_].push_back(order);
    } else {
      asks_[order->price_].push_back(order);
    }

    orders_.insert({order->orderId_, order});

    OnOrderAdded(order);

    return MatchOrders();
  }

  void CancelOrders(OrderIds orderIds) {
    std::scoped_lock orderbookLock{orderbookMutex_};

    for (const auto& orderId : orderIds) CancelOrderInternal(orderId);
  }
  void CancelOrderInternal(OrderId orderId) {
    if (!orders_.contains(orderId))
      throw OrderNotFoundException<Types>(orderId);

    const auto order = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->side_ == Side::Sell) {
      auto price = order->price_;
      auto& orders = asks_.at(price);

      auto it = std::find(orders.begin(), orders.end(), order);
      orders.erase(it);

      if (orders.empty()) asks_.erase(price);
    } else {
      auto price = order->price_;
      auto& orders = bids_.at(price);

      auto it = std::find(orders.begin(), orders.end(), order);
      orders.erase(it);

      if (orders.empty()) bids_.erase(price);
    }

    OnOrderCancelled(order);
  }

  void OnOrderCancelled(OrderPointer<Types> order) {
    UpdateLevelData(order->price_, order->remainingQuantity_,
                    LevelData<Types>::Action::Remove);
  }

  void OnOrderAdded(OrderPointer<Types> order) {
    UpdateLevelData(order->price_, order->initialQuantity_,
                    LevelData<Types>::Action::Add);
  }

  void OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled) {
    UpdateLevelData(price, quantity,
                    isFullyFilled ? LevelData<Types>::Action::Remove
                                  : LevelData<Types>::Action::Match);
  }

  void UpdateLevelData(Price price, Quantity quantity,
                       LevelData<Types>::Action action) {
    auto& data = data_[price];

    data.count_ += action == LevelData<Types>::Action::Remove ? -1
                   : action == LevelData<Types>::Action::Add  ? 1
                                                              : 0;
    if (action == LevelData<Types>::Action::Add) {
      data.quantity_ += quantity;
    } else {
      data.quantity_ -= quantity;
    }

    if (data.count_ == 0) data_.erase(price);
  }

  bool CanFullyFill(Side side, Price price, Quantity quantity) const {
    if (!CanMatch(side, price)) return false;

    Quantity available = 0;

    for (const auto& [levelPrice, levelData] : data_) {
      if (side == Side::Buy && price >= levelPrice)
        available += levelData.quantity_;
      if (side == Side::Sell && price <= levelPrice)
        available += levelData.quantity_;

      if (quantity <= available) return true;
    }

    return false;
  }

  bool CanMatch(Side side, Price price) const {
    if (side == Side::Buy) {
      if (asks_.empty()) return false;

      const auto& [bestAsk, _] = *asks_.begin();
      return price >= bestAsk;
    } else {
      if (bids_.empty()) return false;

      const auto& [bestBid, _] = *bids_.begin();
      return price <= bestBid;
    }
  }

  Trades MatchOrders() {
    Trades trades;
    trades.reserve(orders_.size());

    while (true) {
      if (bids_.empty() || asks_.empty()) break;

      auto& [bidPrice, bids] = *bids_.begin();
      auto& [askPrice, asks] = *asks_.begin();

      if (bidPrice < askPrice) break;

      while (!bids.empty() && !asks.empty()) {
        auto bid = bids.front();
        auto ask = asks.front();

        Quantity quantity =
            std::min(bid->remainingQuantity_, ask->remainingQuantity_);

        bid->Fill(quantity);
        ask->Fill(quantity);

        if (bid->IsFilled()) {
          bids.pop_front();
          orders_.erase(bid->orderId_);
        }

        if (ask->IsFilled()) {
          asks.pop_front();
          orders_.erase(ask->orderId_);
        }

        trades.push_back(Trade<Types>{
            TradeInfo<Types>{bid->orderId_, bid->price_, quantity},
            TradeInfo<Types>{ask->orderId_, ask->price_, quantity}});

        OnOrderMatched(bid->price_, quantity, bid->IsFilled());
        OnOrderMatched(ask->price_, quantity, ask->IsFilled());
      }

      if (bids.empty()) bids_.erase(bidPrice);

      if (asks.empty()) asks_.erase(askPrice);
    }

    if (!bids_.empty()) {
      auto& [_, bids] = *bids_.begin();
      auto& order = bids.front();
      if (order->orderType_ == OrderType::FillAndKill)
        CancelOrderInternal(order->orderId_);
    }

    if (!asks_.empty()) {
      auto& [_, asks] = *asks_.begin();
      auto& order = asks.front();
      if (order->orderType_ == OrderType::FillAndKill)
        CancelOrderInternal(order->orderId_);
    }

    return trades;
  }

 public:
  Trades AddOrder(OrderPointer<Types> order) {
    std::scoped_lock orderbookLock{orderbookMutex_};
    return AddOrderInternal(order);
  }

  void CancelOrder(OrderId orderId) {
    std::scoped_lock orderbookLock{orderbookMutex_};

    CancelOrderInternal(orderId);
  }

  Trades ModifyOrder(OrderModify<Types> orderModify) {
    std::scoped_lock orderbookLock{orderbookMutex_};

    if (!orders_.contains(orderModify.GetOrderId()))
      throw OrderNotFoundException<Types>(orderModify.GetOrderId());

    const auto& existingOrder = orders_.at(orderModify.GetOrderId());
    OrderType orderType = existingOrder->orderType_;

    CancelOrderInternal(orderModify.GetOrderId());
    return AddOrderInternal(orderModify.ToOrderPointer(orderType));
  }

  std::string ToString() {
    std::stringstream ss;
    std::scoped_lock lock{orderbookMutex_};
    ss << "Bid levels: ";
    for (const auto& [price, orders] : bids_) {
      ss << "$" << price << ": [";
      for (size_t i = 0; i < orders.size(); ++i) {
        ss << orders[i]->orderId_;
        if (i < orders.size() - 1) ss << ", ";
      }
      ss << "] ";
    }
    ss << "Ask levels: ";
    for (const auto& [price, orders] : asks_) {
      ss << "($" << price << ": [";
      for (size_t i = 0; i < orders.size(); ++i) {
        ss << orders[i]->orderId_;
        if (i < orders.size() - 1) ss << ", ";
      }
      ss << "]) ";
    }
    return ss.str();
  }
};
