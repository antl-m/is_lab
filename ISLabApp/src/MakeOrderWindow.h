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

struct CustomerData
{
  int ID;
  std::string FirstName;
  std::string LastName;
  std::string Address;
  std::string Email;
  std::string CountryID;
};

class MakeOrderWindow
  : public IWindow
{
public:

  MakeOrderWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn,
      ProductsTableWindow * _Products,
      CustomersTableWindow * _Customers,
      ProductCategoriesTableWindow * _Categories,
      OrdersTableWindow * _Orders,
      InventoriesTableWindow * _Inventories,
      WarehousesTableWindow * _Warehouses
    );

  ~MakeOrderWindow();

  void OnUIRender() override;

  void OpenErrorWindow(
      const std::string_view _ErrorMessage
    );
  void RenderErrorWindow();
  void CloseErrorWindow();

  void UpdateData();

  void RenderProductEntry(
      const int _ProductID,
      const std::string & _ProductName,
      const std::string & _Description,
      const float _Cost,
      const float _Price,
      const int _CategoryID
    );
  void RenderProductsWindow();
  void RenderLoginWindow();

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

  std::vector<char> m_FirstNameBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_LastNameBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_EmailBuffer = std::vector<char>(255 + 1, '\0');
  std::optional<CustomerData> m_CustomerData;
  std::unordered_map<int, int> m_ProductQuantitiesCache;
};