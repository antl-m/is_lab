#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>
#include <signals/Connection.h>

class CustomersTableWindow;
class ProductsTableWindow;

class OrdersTableWindow
  : public IWindow
{
public:

  OrdersTableWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn,
      CustomersTableWindow * _Customers,
      ProductsTableWindow * _Products
    );

  ~OrdersTableWindow();

  void OnUIRender() override;

  void OpenCreateWindow();
  void RenderCreateWindow();
  void CloseCreateWindow();

  void OpenDeleteWindow();
  void RenderDeleteWindow();
  void CloseDeleteWindow();

  void OpenErrorWindow();
  void RenderErrorWindow();
  void CloseErrorWindow();

  void RenderTable();
  void UpdateTable();

  void SortTable(
      int _ColIdx,
      ImGuiSortDirection _SortDir
    );

  void Create(
      int _CustomerId,
      EOrderStatus _Status,
      const oci::Date & _Date,
      int _ProductId,
      float _Quantity
    );

  void UpdateStatus(
      int _OrderId,
      EOrderStatus _NewStatus
    );

  void Delete(
      int _OrderID
    );

  const auto & GetTable()
  {
    return m_Table;
  }

public:

  sig::CSignal<> TableChangedSignal;

private:

  oci::Environment * m_Env = nullptr;
  oci::Connection * m_Conn = nullptr;

  oci::Statement * m_CreateStmt = nullptr;
  oci::Statement * m_DeleteStmt = nullptr;
  oci::Statement* m_UpdateStatusStmt = nullptr;
  oci::Statement * m_UpdateStmt = nullptr;

  int m_OrderId = 0;
  int m_CustomerId = 0;
  EOrderStatus m_OrderStatus = EOrderStatus::CREATED;
  int m_Year = 2022;
  int m_Month = 11;
  int m_Day = 14;
  int m_ProductId = 0;
  float m_Quantity = 0;

  bool m_IsCreating = false;
  bool m_IsDeleting = false;
  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  Table<int, int, EOrderStatus, oci::Date, int, float> m_Table;
  CustomersTableWindow * m_Customers = nullptr;
  ProductsTableWindow * m_Products = nullptr;
  sig::CMultiConnection m_SignalConnections;
};