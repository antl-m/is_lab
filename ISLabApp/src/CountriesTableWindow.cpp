#include "CountriesTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>

CountriesTableWindow::CountriesTableWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn }
{
  m_CreateStmt = m_Conn->createStatement("INSERT INTO countries VALUES(:1,:2)");
  m_DeleteStmt = m_Conn->createStatement("DELETE FROM countries WHERE country_id = :1");
  m_UpdateStmt = m_Conn->createStatement("SELECT * FROM countries");
}

CountriesTableWindow::~CountriesTableWindow()
{
  m_Conn->terminateStatement(m_CreateStmt);
  m_Conn->terminateStatement(m_DeleteStmt);
  m_Conn->terminateStatement(m_UpdateStmt);
}

void CountriesTableWindow::OnUIRender()
{
  ImGui::Begin("Countries");

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

void CountriesTableWindow::OpenCreateWindow()
{
  m_IsCreatingNewCountry = true;
  ImGui::OpenPopup("Create new country");
}

void CountriesTableWindow::RenderCreateWindow()
{
  if (ImGui::BeginPopupModal("Create new country", &m_IsCreatingNewCountry, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::InputText("Country ID", m_CountryIdBuffer.data(), m_CountryIdBuffer.size());
    ImGui::InputText("Country name", m_CountryNameBuffer.data(), m_CountryNameBuffer.size());

    if (ButtonCentered("OK"))
    {
      CreateCountry(m_CountryIdBuffer.data(), m_CountryNameBuffer.data());
      UpdateTable();
      CloseCreateWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void CountriesTableWindow::CloseCreateWindow()
{
  m_IsCreatingNewCountry = false;
}

void CountriesTableWindow::OpenDeleteWindow()
{
  m_IsDeletingCountry = true;
  ImGui::OpenPopup("Delete country");
}

void CountriesTableWindow::RenderDeleteWindow()
{
  if (ImGui::BeginPopupModal("Delete country", &m_IsDeletingCountry, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Country ID", m_CountriesTable, m_CountryIdBuffer);

    if (ButtonCentered("OK"))
    {
      DeleteCountry(m_CountryIdBuffer.data());
      UpdateTable();
      CloseDeleteWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void CountriesTableWindow::CloseDeleteWindow()
{
  m_IsDeletingCountry = false;
}

void CountriesTableWindow::OpenErrorWindow()
{
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void CountriesTableWindow::RenderErrorWindow()
{
  if (ImGui::BeginPopupModal("Error", &m_IsError, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::TextUnformatted(m_ErrorMessage.c_str());

    if (ButtonCentered("OK"))
      CloseErrorWindow();

    ImGui::EndPopup();
  }
}

void CountriesTableWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void CountriesTableWindow::RenderTable()
{
  constexpr auto TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;

  if (ImGui::BeginTable("CountriesTable", 2, TableFlags, ImVec2(-1, -1)))
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

    for (const auto & [ID, Name] : m_CountriesTable)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(ID.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(Name.c_str());
    }

    ImGui::EndTable();
  }
}

void CountriesTableWindow::UpdateTable()
{
  m_CountriesTable.clear();

  try
  {
    auto * Result = m_UpdateStmt->executeQuery();
    Result->next();

    while (Result->status() == oci::ResultSet::DATA_AVAILABLE)
    {
      m_CountriesTable.emplace_back(Result->getString(1), Result->getString(2));
      Result->next();
    }
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }

  if (!m_CountriesTable.empty())
  {
    Copy(std::get<0>(m_CountriesTable.front()), m_CountryIdBuffer);
    Copy(std::get<1>(m_CountriesTable.front()), m_CountryNameBuffer);
  }
}

void CountriesTableWindow::SortTable(
    int _ColIdx,
    ImGuiSortDirection _SortDir
  )
{
  std::sort(
      m_CountriesTable.begin(), m_CountriesTable.end(),
      PredicateImpl<std::string, std::string>{ _ColIdx, _SortDir }
    );
}

void CountriesTableWindow::CreateCountry(
    const char * _ID,
    const char * _Name
  )
{
  try
  {
    m_CreateStmt->setString(1, _ID);
    m_CreateStmt->setString(2, _Name);
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

void CountriesTableWindow::DeleteCountry(
    const char * _ID
  )
{
  try
  {
    m_DeleteStmt->setString(1, _ID);
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

const Table<std::string, std::string> & CountriesTableWindow::GetTable() const
{
  return m_CountriesTable;
}