#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>

class ProductCategoriesTableWindow
  : public IWindow
{
public:

  ProductCategoriesTableWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn
    );

  ~ProductCategoriesTableWindow();

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
      const char * _CategoryName
    );

  void Delete(
      int _CategoryID
    );

  const Table<int, std::string> & GetTable() const;
  std::string_view GetCategoryName(
      const int _CategoryID
    );

public:

  sig::CSignal<> TableChangedSignal;

private:

  oci::Environment * m_Env = nullptr;
  oci::Connection * m_Conn = nullptr;

  oci::Statement * m_CreateStmt = nullptr;
  oci::Statement * m_DeleteStmt = nullptr;
  oci::Statement * m_UpdateStmt = nullptr;

  int m_CategoryId = 0;
  std::vector<char> m_CategoryNameBuffer = std::vector<char>(255 + 1, '\0');

  bool m_IsCreating = false;
  bool m_IsDeleting = false;
  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  Table<int, std::string> m_Table;
};