#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>

template<typename ... TArgs>
using Table = std::vector<std::tuple<TArgs...>>;

class CountriesTableWindow
  : public IWindow
{
public:

  CountriesTableWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn
    );

  ~CountriesTableWindow();

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

  void CreateCountry(
      const char * _ID,
      const char * _Name
    );

  void DeleteCountry(
      const char * _ID
    );

private:

  oci::Environment * m_Env = nullptr;
  oci::Connection * m_Conn = nullptr;

  oci::Statement * m_CreateStmt = nullptr;
  oci::Statement * m_DeleteStmt = nullptr;
  oci::Statement * m_UpdateStmt = nullptr;

  std::vector<char> m_CountryIdBuffer = std::vector<char>(2 + 1, '\0');
  std::vector<char> m_CountryNameBuffer = std::vector<char>(40 + 1, '\0');

  bool m_IsCreatingNewCountry = false;
  bool m_IsDeletingCountry = false;
  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  Table<std::string, std::string> m_CountriesTable;
};