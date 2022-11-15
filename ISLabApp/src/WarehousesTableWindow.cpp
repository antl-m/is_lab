#include "WarehousesTableWindow.h"

#include "CountriesTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>

WarehousesTableWindow::WarehousesTableWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn,
    CountriesTableWindow * _Countries
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn },
    m_Countries{ _Countries }
{
  m_CreateStmt = m_Conn->createStatement("INSERT INTO warehouses(warehouse_name, country_id) VALUES(:1,:2)");
  m_DeleteStmt = m_Conn->createStatement("DELETE FROM warehouses WHERE warehouse_id = :1");
  m_UpdateStmt = m_Conn->createStatement("SELECT * FROM warehouses");

  m_SignalConnection.Attach(m_Countries->TableChangedSignal, this, &WarehousesTableWindow::UpdateTable);
  m_SignalConnection.Connect();
}

WarehousesTableWindow::~WarehousesTableWindow()
{
  m_Conn->terminateStatement(m_CreateStmt);
  m_Conn->terminateStatement(m_DeleteStmt);
  m_Conn->terminateStatement(m_UpdateStmt);

  if (m_SignalConnection.IsConnected())
    m_SignalConnection.Disconnect();
}

void WarehousesTableWindow::OnUIRender()
{
  ImGui::Begin("Warehouses");

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

void WarehousesTableWindow::OpenCreateWindow()
{
  m_IsCreating = true;
  ImGui::OpenPopup("Create");
}

void WarehousesTableWindow::RenderCreateWindow()
{
  if (ImGui::BeginPopupModal("Create", &m_IsCreating, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::InputText("Warehouse name", m_WarehouseNameBuffer.data(), m_WarehouseNameBuffer.size());

    DropDown<0>("Country ID", m_Countries->GetTable(), m_CountryIdBuffer);

    if (ButtonCentered("OK"))
    {
      Create(m_WarehouseNameBuffer.data(), m_CountryIdBuffer.data());
      UpdateTable();
      CloseCreateWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void WarehousesTableWindow::CloseCreateWindow()
{
  m_IsCreating = false;
}

void WarehousesTableWindow::OpenDeleteWindow()
{
  m_IsDeleting = true;
  ImGui::OpenPopup("Delete");
}

void WarehousesTableWindow::RenderDeleteWindow()
{
  if (ImGui::BeginPopupModal("Delete", &m_IsDeleting, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Warehouse ID", m_Table, m_WarehouseId);

    if (ButtonCentered("OK"))
    {
      Delete(m_WarehouseId);
      UpdateTable();
      CloseDeleteWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void WarehousesTableWindow::CloseDeleteWindow()
{
  m_IsDeleting = false;
}

void WarehousesTableWindow::OpenErrorWindow()
{
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void WarehousesTableWindow::RenderErrorWindow()
{
  if (ImGui::BeginPopupModal("Error", &m_IsError, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted(m_ErrorMessage.c_str());

    if (ButtonCentered("OK"))
      CloseErrorWindow();

    ImGui::EndPopup();
  }
}

void WarehousesTableWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void WarehousesTableWindow::RenderTable()
{
  constexpr auto TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;

  if (ImGui::BeginTable("WarehousesTable", 3, TableFlags, ImVec2(-1, -1)))
  {
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Country ID");
    ImGui::TableHeadersRow();

    auto * SortSpecs = ImGui::TableGetSortSpecs();
    if (SortSpecs->SpecsDirty)
    {
      SortTable(SortSpecs->Specs[0].ColumnIndex, SortSpecs->Specs[0].SortDirection);
      SortSpecs->SpecsDirty = false;
    }

    for (const auto & [WarehouseID, WarehouseName, CountryID] : m_Table)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%d", WarehouseID);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(WarehouseName.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(CountryID.c_str());
    }

    ImGui::EndTable();
  }
}

void WarehousesTableWindow::UpdateTable()
{
  m_Table.clear();

  try
  {
    auto * Result = m_UpdateStmt->executeQuery();
    Result->next();

    while (Result->status() == oci::ResultSet::DATA_AVAILABLE)
    {
      m_Table.emplace_back(Result->getInt(1), Result->getString(2), Result->getString(3));
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
    Copy(std::get<0>(m_Table.front()), m_WarehouseId);
    Copy(std::get<1>(m_Table.front()), m_WarehouseNameBuffer);
    Copy(std::get<2>(m_Table.front()), m_CountryIdBuffer);
  }
}

void WarehousesTableWindow::SortTable(
    int _ColIdx,
    ImGuiSortDirection _SortDir
  )
{
  std::sort(
      m_Table.begin(), m_Table.end(),
      PredicateImpl<int, std::string, std::string>{ _ColIdx, _SortDir }
    );
}

void WarehousesTableWindow::Create(
    const char * _WarehouseName,
    const char * _CountryID
  )
{
  try
  {
    m_CreateStmt->setString(1, _WarehouseName);
    m_CreateStmt->setString(2, _CountryID);
    m_CreateStmt->executeUpdate();
    m_Conn->commit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}

void WarehousesTableWindow::Delete(
    int _WarehouseID
  )
{
  try
  {
    m_DeleteStmt->setInt(1, _WarehouseID);
    m_DeleteStmt->executeUpdate();
    m_Conn->commit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}