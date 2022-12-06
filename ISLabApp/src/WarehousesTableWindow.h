#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>
#include <signals/Connection.h>

class CountriesTableWindow;

class WarehousesTableWindow
  : public IWindow
{
public:

  WarehousesTableWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn,
      CountriesTableWindow * _Countries
    );

  ~WarehousesTableWindow();

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
      const char * _WarehouseName,
      const char * _CountryID
    );

  void Delete(
      int _WarehouseID
    );

  const auto & GetTable() const
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
  oci::Statement * m_UpdateStmt = nullptr;

  int m_WarehouseId = 0;
  std::vector<char> m_WarehouseNameBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_CountryIdBuffer = std::vector<char>(2 + 1, '\0');

  bool m_IsCreating = false;
  bool m_IsDeleting = false;
  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  Table<int, std::string, std::string> m_Table;
  CountriesTableWindow * m_Countries = nullptr;
  sig::CMultiConnection m_SignalConnections;
};