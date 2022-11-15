#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <imgui.h>
#include <vector>
#include <tuple>
#include <functional>
#include <signals/Connection.h>

class ProductCategoriesTableWindow;

class ProductsTableWindow
  : public IWindow
{
public:

  ProductsTableWindow(
      oci::Environment * _Env,
      oci::Connection * _Conn,
      ProductCategoriesTableWindow * _Categories
    );

  ~ProductsTableWindow();

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
      const char * _ProductName,
      const char * _Description,
      float _Cost,
      float _Price,
      int _CategoryId
    );

  void Delete(
      int _ProductID
    );

private:

  oci::Environment * m_Env = nullptr;
  oci::Connection * m_Conn = nullptr;

  oci::Statement * m_CreateStmt = nullptr;
  oci::Statement * m_DeleteStmt = nullptr;
  oci::Statement * m_UpdateStmt = nullptr;

  int m_ProductId = 0;
  std::vector<char> m_ProductNameBuffer = std::vector<char>(255 + 1, '\0');
  std::vector<char> m_DescriptionBuffer = std::vector<char>(2000 + 1, '\0');
  float m_Cost = 0;
  float m_Price = 0;
  int m_CategoryId = 0;

  bool m_IsCreating = false;
  bool m_IsDeleting = false;
  bool m_IsError = false;
  bool m_NeedUpdate = true;

  std::string m_ErrorMessage;

  Table<int, std::string, std::string, float, float, int> m_Table;
  ProductCategoriesTableWindow * m_Categories = nullptr;
  sig::CConnection<> m_SignalConnection;
};