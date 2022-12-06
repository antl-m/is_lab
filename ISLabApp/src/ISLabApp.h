#pragma once

#include "OrderStatus.h"

#include <occi.h>
#include <imgui.h>
#include <tuple>
#include <signals/Signal.h>

namespace oci = oracle::occi;

template<typename ... TArgs>
using Table = std::vector<std::tuple<TArgs...>>;

template<typename ... TArgs>
struct PredicateImpl
{
  int m_ColIdx;
  ImGuiSortDirection m_SortDir;

  bool operator()(
      const std::tuple<TArgs...> & _Lhs,
      const std::tuple<TArgs...> & _Rhs
    ) const
  {
    return Compare<sizeof...(TArgs) - 1>(_Lhs, _Rhs);
  }

  template<int MaxIdx>
  bool Compare(
      const std::tuple<TArgs...> & _Lhs,
      const std::tuple<TArgs...> & _Rhs
    ) const
  {
    if (m_ColIdx != MaxIdx)
    {
      if constexpr (MaxIdx == 0)
        return false;
      else
        return Compare<MaxIdx - 1>(_Lhs, _Rhs);
    }

    if (m_SortDir == ImGuiSortDirection_Ascending)
      return std::get<MaxIdx>(_Lhs) < std::get<MaxIdx>(_Rhs);
    else
      return std::get<MaxIdx>(_Lhs) > std::get<MaxIdx>(_Rhs);
  }
};

template<typename ... TArgs>
struct PredicateImpl<Table<TArgs...>> : PredicateImpl<TArgs...>
{};

template<typename ... SignalArguments>
struct MultiConnection
{

};

inline bool ButtonCentered(
    const char * _Label
  )
{
  const auto AvailSize = ImGui::GetContentRegionAvail();
  const auto ButtonWidth = ImGui::CalcTextSize(_Label).x + 2 * ImGui::GetStyle().FramePadding.x;
  const auto OldCursorX = ImGui::GetCursorPosX();

  ImGui::SetCursorPosX(OldCursorX + (AvailSize.x - ButtonWidth) / 2);
  const auto Result = ImGui::Button(_Label);
  ImGui::SetCursorPosX(OldCursorX);

  return Result;
}

inline std::string Stringify(
    const std::string & _Item
  )
{
  return _Item;
}

inline std::string Stringify(
    const int _Item
  )
{
  return std::to_string(_Item);
}

inline std::string Stringify(
    const std::vector<char> & _Item
  )
{
  return _Item.data();
}

inline std::string Stringify(
    EOrderStatus _OrderStatus
  )
{
  return ORDER_STATUS_TO_STRING.at(_OrderStatus);
}

inline void Copy(
    const std::string & _Str,
    std::vector<char> & _Buf
  )
{
  std::fill(_Buf.begin(), _Buf.end(), '\0');
  std::copy(_Str.begin(), _Str.end(), _Buf.begin());
}

inline void Copy(
    int _Val,
    int & _Dst
  )
{
  _Dst = _Val;
}

inline void Copy(
    float _Val,
    float & _Dst
  )
{
  _Dst = _Val;
}

inline void Copy(
    const oci::Date & _Val,
    oci::Date & _Dst
  )
{
  _Dst = _Val;
}

inline void Copy(
    EOrderStatus _Val,
    EOrderStatus & _Dst
  )
{
  _Dst = _Val;
}

template<int Idx, typename TCurrent, typename ... TArgs>
void DropDown(
    const char * _Label,
    const Table<TArgs...> & _Table,
    TCurrent & _CurrentItem
  )
{
  const auto CurrentItem = Stringify(_CurrentItem);

  if (ImGui::BeginCombo(_Label, CurrentItem.c_str()))
  {
    for (int i = 0; i < _Table.size(); ++i)
    {
      const auto Item = Stringify(std::get<Idx>(_Table[i]));
      bool IsSelected = (CurrentItem == Item);
      if (ImGui::Selectable(Item.c_str(), IsSelected))
        Copy(std::get<Idx>(_Table[i]), _CurrentItem);
      if (IsSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
}

inline bool DropDownOrderStatus(
    const char * _Label,
    EOrderStatus & _Current
  )
{
  bool Activated = false;
  if (ImGui::BeginCombo(_Label, ORDER_STATUS_TO_STRING.at(_Current).c_str()))
  {
    for (const auto OrderStatus : ORDER_STATUS_LIST)
    {
      if (ImGui::Selectable(ORDER_STATUS_TO_STRING.at(OrderStatus).c_str(), OrderStatus == _Current))
      {
        _Current = OrderStatus;
        Activated = true;
      }
      if (OrderStatus == _Current)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  return Activated;
}

inline ImFont * GetFontS() { return ImGui::GetIO().Fonts->Fonts[0]; }
inline ImFont * GetFontM() { return ImGui::GetIO().Fonts->Fonts[1]; }
inline ImFont * GetFontL() { return ImGui::GetIO().Fonts->Fonts[2]; }
