#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>
#include <signals/Connection.h>

class WarehousesTableWindow;
class ProductsTableWindow;

class InventoriesTableWindow
  : public IWindow
{
public:

  InventoriesTableWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn,
      WarehousesTableWindow * _Warehouses,
      ProductsTableWindow * _Products
    );

  ~InventoriesTableWindow();

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
      const int _ProductId,
      const int _WarehouseId,
      const int _Quantity
    );

  void Delete(
      const int _ProductId,
      const int _WarehouseId
    );

  void Decrease(
      const int _ProductId,
      const std::string & _CountryId,
      const int _Quantity
    );

  const Table<int, int, int> & GetTable() const;

public:

  sig::CSignal<> TableChangedSignal;

private:

  oci::Environment * m_Env = nullptr;
  oci::Connection * m_Conn = nullptr;

  oci::Statement * m_CreateStmt = nullptr;
  oci::Statement * m_DeleteStmt = nullptr;
  oci::Statement * m_DecreaseStmt = nullptr;
  oci::Statement * m_UpdateStmt = nullptr;

  int m_ProductId = 0;
  int m_WarehouseId = 0;
  int m_Quantity = 0;

  bool m_IsCreating = false;
  bool m_IsDeleting = false;
  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  Table<int, int, int> m_Table;
  WarehousesTableWindow * m_Warehouses = nullptr;
  ProductsTableWindow * m_Products = nullptr;
  sig::CMultiConnection m_SignalConnections;
};