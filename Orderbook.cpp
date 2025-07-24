#include "Orderbook.h"

#include <chrono>
#include <ctime>
#include <numeric>

void Orderbook::CancelOrders(OrderIds orderIds) {
  std::scoped_lock ordersLock{ordersMutex_};

  for (const auto& orderId : orderIds) CancelOrderInternal(orderId);
}

void Orderbook::CancelOrderInternal(OrderId orderId) {
  if (!orders_.contains(orderId)) return;

  const auto [order, iterator] = orders_.at(orderId);
  orders_.erase(orderId);

  if (order->side_ == Side::Sell) {
    auto price = order->price_;
    auto& orders = asks_.at(price);
    orders.erase(iterator);
    if (orders.empty()) asks_.erase(price);
  } else {
    auto price = order->price_;
    auto& orders = bids_.at(price);
    orders.erase(iterator);
    if (orders.empty()) bids_.erase(price);
  }

  OnOrderCancelled(order);
}

void Orderbook::OnOrderCancelled(OrderPointer order) {
  UpdateLevelData(order->price_, order->remainingQuantity_,
                  LevelData::Action::Remove);
}

void Orderbook::OnOrderAdded(OrderPointer order) {
  UpdateLevelData(order->price_, order->initialQuantity_,
                  LevelData::Action::Add);
}

void Orderbook::OnOrderMatched(Price price, Quantity quantity,
                               bool isFullyFilled) {
  UpdateLevelData(
      price, quantity,
      isFullyFilled ? LevelData::Action::Remove : LevelData::Action::Match);
}

void Orderbook::UpdateLevelData(Price price, Quantity quantity,
                                LevelData::Action action) {
  auto& data = data_[price];

  data.count_ += action == LevelData::Action::Remove ? -1
                 : action == LevelData::Action::Add  ? 1
                                                     : 0;
  if (action == LevelData::Action::Remove ||
      action == LevelData::Action::Match) {
    data.quantity_ -= quantity;
  } else {
    data.quantity_ += quantity;
  }

  if (data.count_ == 0) data_.erase(price);
}

bool Orderbook::CanFullyFill(Side side, Price price, Quantity quantity) const {
  if (!CanMatch(side, price)) return false;

  std::optional<Price> threshold;

  if (side == Side::Buy) {
    const auto [askPrice, _] = *asks_.begin();
    threshold = askPrice;
  } else {
    const auto [bidPrice, _] = *bids_.begin();
    threshold = bidPrice;
  }

  for (const auto& [levelPrice, levelData] : data_) {
    if (threshold.has_value() &&
            (side == Side::Buy && threshold.value() > levelPrice) ||
        (side == Side::Sell && threshold.value() < levelPrice))
      continue;

    if ((side == Side::Buy && levelPrice > price) ||
        (side == Side::Sell && levelPrice < price))
      continue;

    if (quantity <= levelData.quantity_) return true;

    quantity -= levelData.quantity_;
  }

  return false;
}

bool Orderbook::CanMatch(Side side, Price price) const {
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

Trades Orderbook::MatchOrders() {
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

      trades.push_back(Trade{TradeInfo{bid->orderId_, bid->price_, quantity},
                             TradeInfo{ask->orderId_, ask->price_, quantity}});

      OnOrderMatched(bid->price_, quantity, bid->IsFilled());
      OnOrderMatched(ask->price_, quantity, ask->IsFilled());
    }

    if (bids.empty()) {
      bids_.erase(bidPrice);
      data_.erase(bidPrice);
    }

    if (asks.empty()) {
      asks_.erase(askPrice);
      data_.erase(askPrice);
    }
  }

  if (!bids_.empty()) {
    auto& [_, bids] = *bids_.begin();
    auto& order = bids.front();
    if (order->orderType_ == OrderType::FillAndKill)
      CancelOrder(order->orderId_);
  }

  if (!asks_.empty()) {
    auto& [_, asks] = *asks_.begin();
    auto& order = asks.front();
    if (order->orderType_ == OrderType::FillAndKill)
      CancelOrder(order->orderId_);
  }

  return trades;
}

Trades Orderbook::AddOrder(OrderPointer order) {
  std::scoped_lock ordersLock{ordersMutex_};

  if (orders_.contains(order->orderId_)) return {};

  if (order->orderType_ == OrderType::Market) {
    if (order->side_ == Side::Buy && !asks_.empty()) {
      const auto& [worstAsk, _] = *asks_.rbegin();
      order->ToGoodTillCancel(worstAsk);
    } else if (order->side_ == Side::Sell && !bids_.empty()) {
      const auto& [worstBid, _] = *bids_.rbegin();
      order->ToGoodTillCancel(worstBid);
    } else
      return {};
  }

  if (order->orderType_ == OrderType::FillAndKill &&
      !CanMatch(order->side_, order->price_))
    return {};

  if (order->orderType_ == OrderType::FillOrKill &&
      !CanFullyFill(order->side_, order->price_, order->initialQuantity_))
    return {};

  OrderPointers::iterator iterator;

  if (order->side_ == Side::Buy) {
    auto& orders = bids_[order->price_];
    orders.push_back(order);
    iterator = std::prev(orders.end());
  } else {
    auto& orders = asks_[order->price_];
    orders.push_back(order);
    iterator = std::prev(orders.end());
  }

  orders_.insert({order->orderId_, OrderEntry{order, iterator}});

  OnOrderAdded(order);

  return MatchOrders();
}

void Orderbook::CancelOrder(OrderId orderId) {
  std::scoped_lock ordersLock{ordersMutex_};

  CancelOrderInternal(orderId);
}

Trades Orderbook::ModifyOrder(OrderModify order) {
  OrderType orderType;

  {
    std::scoped_lock ordersLock{ordersMutex_};

    if (!orders_.contains(order.GetOrderId())) return {};

    const auto& [existingOrder, _] = orders_.at(order.GetOrderId());
    orderType = existingOrder->orderType_;
  }

  CancelOrder(order.GetOrderId());
  return AddOrder(order.ToOrderPointer(orderType));
}

std::size_t Orderbook::Size() const {
  std::scoped_lock ordersLock{ordersMutex_};
  return orders_.size();
}

OrderbookLevelInfos Orderbook::GetOrderInfos() const {
  LevelInfos bidInfos, askInfos;
  bidInfos.reserve(orders_.size());
  askInfos.reserve(orders_.size());

  auto CreateLevelInfos = [](Price price, const OrderPointers& orders) {
    return LevelInfo{
        price,
        std::accumulate(orders.begin(), orders.end(), (Quantity)0,
                        [](Quantity runningSum, const OrderPointer& order) {
                          return runningSum + order->remainingQuantity_;
                        })};
  };

  for (const auto& [price, orders] : bids_)
    bidInfos.push_back(CreateLevelInfos(price, orders));

  for (const auto& [price, orders] : asks_)
    askInfos.push_back(CreateLevelInfos(price, orders));

  return OrderbookLevelInfos{bidInfos, askInfos};
}
