#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>
#include <signals/Connection.h>

class CountriesTableWindow;

class CustomersTableWindow
  : public IWindow
{
public:

  CustomersTableWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn,
      CountriesTableWindow * _Countries
    );

  ~CustomersTableWindow();

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
      const char * _FirstName,
      const char * _LastName,
      const char * _Address,
      const char * _Email,
      const char * _CountryId
    );

  void Delete(
      int _CustomerID
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

  int m_CustomerId = 0;
  std::vector<char> m_FirstNameBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_LastNameBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_AddressBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_EmailBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_CountryIdBuffer = std::vector<char>(2 + 1, '\0');

  bool m_IsCreating = false;
  bool m_IsDeleting = false;
  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  Table<int, std::string, std::string, std::string, std::string, std::string> m_Table;
  CountriesTableWindow * m_Countries = nullptr;
  sig::CConnection<> m_SignalConnection;
};