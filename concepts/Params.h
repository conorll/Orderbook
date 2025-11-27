#pragma once
#include "Containers.h"

template <typename T>
concept ValidParams = requires {
  typename T::Types;
  typename T::Containers;
} && ValidTypes<typename T::Types> && ValidContainers<typename T::Containers>;
