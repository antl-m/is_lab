#include "ProductCategoriesTableWindow.h"

#include "CountriesTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>

ProductCategoriesTableWindow::ProductCategoriesTableWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn }
{
  m_CreateStmt = m_Conn->createStatement("INSERT INTO product_categories(category_name) VALUES(:1)");
  m_DeleteStmt = m_Conn->createStatement("DELETE FROM product_categories WHERE category_id = :1");
  m_UpdateStmt = m_Conn->createStatement("SELECT * FROM product_categories");
}

ProductCategoriesTableWindow::~ProductCategoriesTableWindow()
{
  m_Conn->terminateStatement(m_CreateStmt);
  m_Conn->terminateStatement(m_DeleteStmt);
  m_Conn->terminateStatement(m_UpdateStmt);
}

void ProductCategoriesTableWindow::OnUIRender()
{
  ImGui::Begin("Product categories");

  if (ImGui::IsWindowAppearing())
    m_NeedUpdate = true;

  if (m_NeedUpdate)
  {
    UpdateTable();
    m_NeedUpdate = false;
  }

  if (ImGui::Button("Create"))
    OpenCreateWindow();
  RenderCreateWindow();

  ImGui::SameLine();

  if (ImGui::Button("Delete"))
    OpenDeleteWindow();
  RenderDeleteWindow();

  RenderTable();
  RenderErrorWindow();

  ImGui::End();
}

void ProductCategoriesTableWindow::OpenCreateWindow()
{
  m_IsCreating = true;
  ImGui::OpenPopup("Create");
}

void ProductCategoriesTableWindow::RenderCreateWindow()
{
  if (ImGui::BeginPopupModal("Create", &m_IsCreating, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::InputText("Category name", m_CategoryNameBuffer.data(), m_CategoryNameBuffer.size());

    if (ButtonCentered("OK"))
    {
      Create(m_CategoryNameBuffer.data());
      UpdateTable();
      CloseCreateWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void ProductCategoriesTableWindow::CloseCreateWindow()
{
  m_IsCreating = false;
}

void ProductCategoriesTableWindow::OpenDeleteWindow()
{
  m_IsDeleting = true;
  ImGui::OpenPopup("Delete");
}

void ProductCategoriesTableWindow::RenderDeleteWindow()
{
  if (ImGui::BeginPopupModal("Delete", &m_IsDeleting, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Category ID", m_Table, m_CategoryId);

    if (ButtonCentered("OK"))
    {
      Delete(m_CategoryId);
      UpdateTable();
      CloseDeleteWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void ProductCategoriesTableWindow::CloseDeleteWindow()
{
  m_IsDeleting = false;
}

void ProductCategoriesTableWindow::OpenErrorWindow()
{
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void ProductCategoriesTableWindow::RenderErrorWindow()
{
  if (ImGui::BeginPopupModal("Error", &m_IsError, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted(m_ErrorMessage.c_str());

    if (ButtonCentered("OK"))
      CloseErrorWindow();

    ImGui::EndPopup();
  }
}

void ProductCategoriesTableWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void ProductCategoriesTableWindow::RenderTable()
{
  constexpr auto TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;

  if (ImGui::BeginTable("ProductCategoriesTable", 2, TableFlags, ImVec2(-1, -1)))
  {
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("Name");
    ImGui::TableHeadersRow();

    auto * SortSpecs = ImGui::TableGetSortSpecs();
    if (SortSpecs->SpecsDirty)
    {
      SortTable(SortSpecs->Specs[0].ColumnIndex, SortSpecs->Specs[0].SortDirection);
      SortSpecs->SpecsDirty = false;
    }

    for (const auto & [CategoryID, CategoryName] : m_Table)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%d", CategoryID);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(CategoryName.c_str());
    }

    ImGui::EndTable();
  }
}

void ProductCategoriesTableWindow::UpdateTable()
{
  m_Table.clear();

  try
  {
    auto * Result = m_UpdateStmt->executeQuery();
    Result->next();

    while (Result->status() == oci::ResultSet::DATA_AVAILABLE)
    {
      m_Table.emplace_back(Result->getInt(1), Result->getString(2));
      Result->next();
    }
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }

  if (!m_Table.empty())
  {
    Copy(std::get<0>(m_Table.front()), m_CategoryId);
    Copy(std::get<1>(m_Table.front()), m_CategoryNameBuffer);
  }
}

void ProductCategoriesTableWindow::SortTable(
    int _ColIdx,
    ImGuiSortDirection _SortDir
  )
{
  std::sort(
      m_Table.begin(), m_Table.end(),
      PredicateImpl<int, std::string>{ _ColIdx, _SortDir }
    );
}

void ProductCategoriesTableWindow::Create(
    const char * _CategoryName
  )
{
  try
  {
    m_CreateStmt->setString(1, _CategoryName);
    m_CreateStmt->executeUpdate();
    m_Conn->commit();
    TableChangedSignal.Emit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}

void ProductCategoriesTableWindow::Delete(
    int _CategoryID
  )
{
  try
  {
    m_DeleteStmt->setInt(1, _CategoryID);
    m_DeleteStmt->executeUpdate();
    m_Conn->commit();
    TableChangedSignal.Emit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}

const Table<int, std::string>& ProductCategoriesTableWindow::GetTable() const
{
  return m_Table;
}
