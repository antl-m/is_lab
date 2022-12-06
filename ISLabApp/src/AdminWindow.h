#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>
#include <signals/Connection.h>
#include <optional>

class ProductsTableWindow;
class CustomersTableWindow;
class ProductCategoriesTableWindow;
class OrdersTableWindow;
class InventoriesTableWindow;
class WarehousesTableWindow;

struct OrderCustomerData
{
  int Id;
  std::string FirstName;
  std::string LastName;
  std::string Address;
};

struct OrderProductData
{
  int Id;
  std::string ProductName;
  float Cost;
  float Price;
};

struct OrderEntry
{
  int OrderId;
  float Quantity;
  EOrderStatus Status;
  oci::Date Date;
  OrderCustomerData Customer;
  OrderProductData Product;
};

class AdminWindow
  : public IWindow
{
public:

  AdminWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn,
      ProductsTableWindow * _Products,
      CustomersTableWindow * _Customers,
      ProductCategoriesTableWindow * _Categories,
      OrdersTableWindow * _Orders,
      InventoriesTableWindow * _Inventories,
      WarehousesTableWindow * _Warehouses
    );

  ~AdminWindow();

  void OnUIRender() override;

  void OpenErrorWindow(
      const std::string_view _ErrorMessage
    );
  void RenderErrorWindow();
  void CloseErrorWindow();

  void UpdateData();

  void RenderOrderEntry(
      OrderEntry & _Order
    );

  void RenderAdminPanel();

  bool IsFilterSuitable(
      const OrderEntry & _Order
    ) const;

  void RenderCharts();

private:

  oci::Environment * m_Env = nullptr;
  oci::Connection * m_Conn = nullptr;

  oci::Statement * m_MakeOrderStmt = nullptr;

  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  ProductsTableWindow * m_Products = nullptr;
  CustomersTableWindow * m_Customers = nullptr;
  ProductCategoriesTableWindow * m_Categories = nullptr;
  OrdersTableWindow * m_Orders = nullptr;
  InventoriesTableWindow * m_Inventories = nullptr;
  WarehousesTableWindow * m_Warehouses = nullptr;

  sig::CMultiConnection m_SignalConnections;

  std::vector<OrderEntry> m_OrderEntries;
  std::unordered_map<EOrderStatus, bool> m_StatusFilter;
};