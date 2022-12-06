#pragma once

#include <unordered_map>
#include <string>

#define ORDER_STATUS_VALUES \
  ENUM_VALUE(CREATED),      \
  ENUM_VALUE(RECIEVED),     \
  ENUM_VALUE(IN_TRANSIT),   \
  ENUM_VALUE(DELIVERED),    \
  ENUM_VALUE(DISCARDED)

#define ENUM_VALUE(x) x
enum class EOrderStatus
{
  ORDER_STATUS_VALUES
};
#undef ENUM_VALUE

#define ENUM_VALUE(x) { #x, EOrderStatus::x }
inline const std::unordered_map<std::string, EOrderStatus> STRING_TO_ORDER_STATUS {
    ORDER_STATUS_VALUES
  };
#undef ENUM_VALUE

#define ENUM_VALUE(x) { EOrderStatus::x, #x }
inline const std::unordered_map<EOrderStatus, std::string> ORDER_STATUS_TO_STRING {
    ORDER_STATUS_VALUES
  };
#undef ENUM_VALUE

#define ENUM_VALUE(x) EOrderStatus::x
inline const std::vector<EOrderStatus> ORDER_STATUS_LIST {
    ORDER_STATUS_VALUES
  };
#undef ENUM_VALUE

#undef ORDER_STATUS_VALUES
