#pragma once

template <ValidTypes Types>
struct LevelData {
  using Quantity = typename Types::Quantity;

  Quantity quantity_{};
  Quantity count_{};

  enum class Action {
    Add,
    Remove,
    Match,
  };
};