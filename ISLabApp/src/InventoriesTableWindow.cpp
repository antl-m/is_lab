#include "InventoriesTableWindow.h"

#include "WarehousesTableWindow.h"
#include "ProductsTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>
#include <set>

InventoriesTableWindow::InventoriesTableWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn,
    WarehousesTableWindow * _Warehouses,
    ProductsTableWindow * _Products
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn },
    m_Warehouses{ _Warehouses },
    m_Products{ _Products }
{
  m_CreateStmt = m_Conn->createStatement("INSERT INTO inventories(product_id,warehouse_id,quantity) VALUES(:1,:2,:3)");
  m_DeleteStmt = m_Conn->createStatement("DELETE FROM inventories WHERE product_id = :1 AND warehouse_id = :2");
  m_DecreaseStmt = m_Conn->createStatement("UPDATE inventories SET quantity = :1 WHERE product_id = :2 AND warehouse_id = :3");
  m_UpdateStmt = m_Conn->createStatement("SELECT * FROM inventories");

  m_SignalConnections.AddConnection(m_Warehouses->TableChangedSignal, this, &InventoriesTableWindow::UpdateTable);
  m_SignalConnections.AddConnection(m_Products->TableChangedSignal, this, &InventoriesTableWindow::UpdateTable);
}

InventoriesTableWindow::~InventoriesTableWindow()
{
  m_Conn->terminateStatement(m_CreateStmt);
  m_Conn->terminateStatement(m_DeleteStmt);
  m_Conn->terminateStatement(m_DecreaseStmt);
  m_Conn->terminateStatement(m_UpdateStmt);

  m_SignalConnections.Disconnect();
}

void InventoriesTableWindow::OnUIRender()
{
  ImGui::Begin("Inventories");

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

void InventoriesTableWindow::OpenCreateWindow()
{
  m_IsCreating = true;
  ImGui::OpenPopup("Create");
}

void InventoriesTableWindow::RenderCreateWindow()
{
  if (ImGui::BeginPopupModal("Create", &m_IsCreating, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Product ID", m_Products->GetTable(), m_ProductId);
    DropDown<0>("Warehouse ID", m_Warehouses->GetTable(), m_WarehouseId);
    ImGui::InputInt("Quantity", &m_Quantity);

    m_Quantity = std::clamp(m_Quantity, 0, std::numeric_limits<int>::max());

    if (ButtonCentered("OK"))
    {
      Create(
          m_ProductId,
          m_WarehouseId,
          m_Quantity
        );
      UpdateTable();
      CloseCreateWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void InventoriesTableWindow::CloseCreateWindow()
{
  m_IsCreating = false;
}

void InventoriesTableWindow::OpenDeleteWindow()
{
  m_IsDeleting = true;
  ImGui::OpenPopup("Delete");
}

void InventoriesTableWindow::RenderDeleteWindow()
{
  if (ImGui::BeginPopupModal("Delete", &m_IsDeleting, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Product ID", m_Table, m_ProductId);

    Table<int, int, int> Copy;
    for (const auto & [ProductId, WarehouseId, Quantity] : m_Table)
      if (ProductId == m_ProductId)
        Copy.emplace_back(ProductId, WarehouseId, Quantity);

    DropDown<1>("Warehouse ID", Copy, m_WarehouseId);

    if (ButtonCentered("OK"))
    {
      Delete(m_ProductId, m_WarehouseId);
      UpdateTable();
      CloseDeleteWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void InventoriesTableWindow::CloseDeleteWindow()
{
  m_IsDeleting = false;
}

void InventoriesTableWindow::OpenErrorWindow()
{
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void InventoriesTableWindow::RenderErrorWindow()
{
  if (m_IsError)
    ImGui::OpenPopup("Error");

  if (ImGui::BeginPopupModal("Error", &m_IsError, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted(m_ErrorMessage.c_str());

    if (ButtonCentered("OK"))
      CloseErrorWindow();

    ImGui::EndPopup();
  }
}

void InventoriesTableWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void InventoriesTableWindow::RenderTable()
{
  constexpr auto TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;

  if (ImGui::BeginTable("InventoriesTable", 3, TableFlags, ImVec2(-1, -1)))
  {
    ImGui::TableSetupColumn("Product ID");
    ImGui::TableSetupColumn("Warehouse ID");
    ImGui::TableSetupColumn("Quantity");
    ImGui::TableHeadersRow();

    auto * SortSpecs = ImGui::TableGetSortSpecs();
    if (SortSpecs->SpecsDirty)
    {
      SortTable(SortSpecs->Specs[0].ColumnIndex, SortSpecs->Specs[0].SortDirection);
      SortSpecs->SpecsDirty = false;
    }

    for (const auto & [ProductId, WarehouseId, Quantity] : m_Table)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%d", ProductId);
      ImGui::TableNextColumn();
      ImGui::Text("%d", WarehouseId);
      ImGui::TableNextColumn();
      ImGui::Text("%d", Quantity);
    }

    ImGui::EndTable();
  }
}

void InventoriesTableWindow::UpdateTable()
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
          Result->getInt(2),
          Result->getInt(3)
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
    Copy(std::get<1>(m_Table.front()), m_WarehouseId);
    Copy(std::get<2>(m_Table.front()), m_Quantity);
  }
}

void InventoriesTableWindow::SortTable(
    int _ColIdx,
    ImGuiSortDirection _SortDir
  )
{
  std::sort(
      m_Table.begin(), m_Table.end(),
      PredicateImpl<decltype(m_Table)>{ _ColIdx, _SortDir }
    );
}

void InventoriesTableWindow::Create(
    const int _ProductId,
    const int _WarehouseId,
    const int _Quantity
  )
{
  try
  {
    m_CreateStmt->setInt(1, _ProductId);
    m_CreateStmt->setInt(2, _WarehouseId);
    m_CreateStmt->setInt(3, _Quantity);
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

void InventoriesTableWindow::Delete(
    const int _ProductId,
    const int _WarehouseId
  )
{
  try
  {
    m_DeleteStmt->setInt(1, _ProductId);
    m_DeleteStmt->setInt(2, _WarehouseId);
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

void InventoriesTableWindow::Decrease(
    const int _ProductId,
    const std::string & _CountryId,
    const int _Quantity
  )
{
  std::set<int> CountryWarehouses;
  for (const auto & [WarehouseId, WarehouseName, CountryId] : m_Warehouses->GetTable())
    if (CountryId == _CountryId)
      CountryWarehouses.insert(WarehouseId);

  struct DecreaseEntry
  {
    int WarehouseId;
    int Quantity;
  };
  std::vector<DecreaseEntry> ToDecrease;

  for (const auto & [ProductId, WarehouseId, Quantity] : m_Table)
    if (ProductId == _ProductId)
      ToDecrease.emplace_back(DecreaseEntry{ WarehouseId, Quantity });

  std::sort(ToDecrease.begin(), ToDecrease.end(), [&](const DecreaseEntry & lhs, const DecreaseEntry & rhs){
      const bool IsInCountryL = CountryWarehouses.find(lhs.WarehouseId) != CountryWarehouses.end();
      const bool IsInCountryR = CountryWarehouses.find(rhs.WarehouseId) != CountryWarehouses.end();

      if (IsInCountryL != IsInCountryR)
        return IsInCountryL;

      if (lhs.Quantity != rhs.Quantity)
        return lhs.Quantity > rhs.Quantity;

      return lhs.WarehouseId < rhs.WarehouseId;
    });

  int Left = _Quantity;
  for (auto & Entry : ToDecrease)
  {
    if (Left > 0)
    {
      const int Available = Entry.Quantity;
      Entry.Quantity = std::max(0, Available - Left);
      Left = std::max(0, Left - Available);
    }
    else break;
  }

  try
  {
    for (const auto & Entry : ToDecrease)
    {
      m_DecreaseStmt->setInt(1, Entry.Quantity);
      m_DecreaseStmt->setInt(2, _ProductId);
      m_DecreaseStmt->setInt(3, Entry.WarehouseId);
      m_DecreaseStmt->executeUpdate();
    }
    m_Conn->commit();
    UpdateTable();
    TableChangedSignal.Emit();
  }
  catch (const oci::SQLException & ex)
  {
    m_Conn->cancel();
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}

const Table<int, int, int> & InventoriesTableWindow::GetTable() const
{
  return m_Table;
}