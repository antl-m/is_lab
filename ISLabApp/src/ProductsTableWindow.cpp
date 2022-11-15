#include "ProductsTableWindow.h"

#include "ProductCategoriesTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>

ProductsTableWindow::ProductsTableWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn,
    ProductCategoriesTableWindow * _Categories
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn },
    m_Categories{ _Categories }
{
  m_CreateStmt = m_Conn->createStatement("INSERT INTO products(product_name, description, cost, price, category_id) VALUES(:1,:2,:3,:4,:5)");
  m_DeleteStmt = m_Conn->createStatement("DELETE FROM products WHERE product_id = :1");
  m_UpdateStmt = m_Conn->createStatement("SELECT * FROM products");

  m_SignalConnection.Attach(m_Categories->TableChangedSignal, this, &ProductsTableWindow::UpdateTable);
  m_SignalConnection.Connect();
}

ProductsTableWindow::~ProductsTableWindow()
{
  m_Conn->terminateStatement(m_CreateStmt);
  m_Conn->terminateStatement(m_DeleteStmt);
  m_Conn->terminateStatement(m_UpdateStmt);

  if (m_SignalConnection.IsConnected())
    m_SignalConnection.Disconnect();
}

void ProductsTableWindow::OnUIRender()
{
  ImGui::Begin("Products");

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

void ProductsTableWindow::OpenCreateWindow()
{
  m_IsCreating = true;
  ImGui::OpenPopup("Create");
}

void ProductsTableWindow::RenderCreateWindow()
{
  if (ImGui::BeginPopupModal("Create", &m_IsCreating, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::InputText("Product name", m_ProductNameBuffer.data(), m_ProductNameBuffer.size());
    ImGui::InputTextMultiline("Description", m_DescriptionBuffer.data(), m_DescriptionBuffer.size());
    ImGui::InputFloat("Cost", &m_Cost, 0, 0, "%.2f");
    ImGui::InputFloat("Price", &m_Price, 0, 0, "%.2f");

    DropDown<0>("Category ID", m_Categories->GetTable(), m_CategoryId);

    if (ButtonCentered("OK"))
    {
      Create(m_ProductNameBuffer.data(), m_DescriptionBuffer.data(), m_Cost, m_Price, m_CategoryId);
      UpdateTable();
      CloseCreateWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void ProductsTableWindow::CloseCreateWindow()
{
  m_IsCreating = false;
}

void ProductsTableWindow::OpenDeleteWindow()
{
  m_IsDeleting = true;
  ImGui::OpenPopup("Delete");
}

void ProductsTableWindow::RenderDeleteWindow()
{
  if (ImGui::BeginPopupModal("Delete", &m_IsDeleting, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Product ID", m_Table, m_ProductId);

    if (ButtonCentered("OK"))
    {
      Delete(m_ProductId);
      UpdateTable();
      CloseDeleteWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void ProductsTableWindow::CloseDeleteWindow()
{
  m_IsDeleting = false;
}

void ProductsTableWindow::OpenErrorWindow()
{
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void ProductsTableWindow::RenderErrorWindow()
{
  if (ImGui::BeginPopupModal("Error", &m_IsError, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted(m_ErrorMessage.c_str());

    if (ButtonCentered("OK"))
      CloseErrorWindow();

    ImGui::EndPopup();
  }
}

void ProductsTableWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void ProductsTableWindow::RenderTable()
{
  constexpr auto TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;

  if (ImGui::BeginTable("ProductsTable", 6, TableFlags, ImVec2(-1, -1)))
  {
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Description");
    ImGui::TableSetupColumn("Cost");
    ImGui::TableSetupColumn("Price");
    ImGui::TableSetupColumn("Category ID");
    ImGui::TableHeadersRow();

    auto * SortSpecs = ImGui::TableGetSortSpecs();
    if (SortSpecs->SpecsDirty)
    {
      SortTable(SortSpecs->Specs[0].ColumnIndex, SortSpecs->Specs[0].SortDirection);
      SortSpecs->SpecsDirty = false;
    }

    for (const auto & [Id, Name, Description, Cost, Price, CategoryId] : m_Table)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%d", Id);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(Name.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(Description.c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%.2f", Cost);
      ImGui::TableNextColumn();
      ImGui::Text("%.2f", Price);
      ImGui::TableNextColumn();
      ImGui::Text("%d", CategoryId);
    }

    ImGui::EndTable();
  }
}

void ProductsTableWindow::UpdateTable()
{
  m_Table.clear();

  try
  {
    auto * Result = m_UpdateStmt->executeQuery();
    Result->next();

    while (Result->status() == oci::ResultSet::DATA_AVAILABLE)
    {
      m_Table.emplace_back(
          Result->getInt(1),
          Result->getString(2),
          Result->getString(3),
          Result->getFloat(4),
          Result->getFloat(5),
          Result->getInt(6)
        );
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
    Copy(std::get<0>(m_Table.front()), m_ProductId);
    Copy(std::get<1>(m_Table.front()), m_ProductNameBuffer);
    Copy(std::get<2>(m_Table.front()), m_DescriptionBuffer);
    Copy(std::get<3>(m_Table.front()), m_Cost);
    Copy(std::get<4>(m_Table.front()), m_Price);
    Copy(std::get<5>(m_Table.front()), m_CategoryId);
  }
}

void ProductsTableWindow::SortTable(
    int _ColIdx,
    ImGuiSortDirection _SortDir
  )
{
  std::sort(
      m_Table.begin(), m_Table.end(),
      PredicateImpl<decltype(m_Table)>{ _ColIdx, _SortDir }
    );
}

void ProductsTableWindow::Create(
    const char * _ProductName,
    const char * _Description,
    float _Cost,
    float _Price,
    int _CategoryId
  )
{
  try
  {
    m_CreateStmt->setString(1, _ProductName);
    m_CreateStmt->setString(2, _Description);
    m_CreateStmt->setFloat(3, _Cost);
    m_CreateStmt->setFloat(4, _Price);
    m_CreateStmt->setInt(5, _CategoryId);
    m_CreateStmt->executeUpdate();
    m_Conn->commit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}

void ProductsTableWindow::Delete(
    int _ProductID
  )
{
  try
  {
    m_DeleteStmt->setInt(1, _ProductID);
    m_DeleteStmt->executeUpdate();
    m_Conn->commit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}