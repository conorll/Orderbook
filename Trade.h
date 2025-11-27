#pragma once

#include <vector>

#include "TradeInfo.h"

template <ValidTypes Types>
class Trade {
 public:
  Trade(const TradeInfo<Types>& bidTrade, const TradeInfo<Types>& askTrade)
      : bidTrade_{bidTrade}, askTrade_{askTrade} {}

  const TradeInfo<Types>& GetBidTrade() const { return bidTrade_; }
  const TradeInfo<Types>& GetAskTrade() const { return askTrade_; }

 private:
  TradeInfo<Types> bidTrade_;
  TradeInfo<Types> askTrade_;
};