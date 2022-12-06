#include "CustomersTableWindow.h"

#include "CountriesTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>

CustomersTableWindow::CustomersTableWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn,
    CountriesTableWindow * _Countries
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn },
    m_Countries{ _Countries }
{
  m_CreateStmt = m_Conn->createStatement("INSERT INTO customers(first_name, last_name, address, email, country_id) VALUES(:1,:2,:3,:4,:5)");
  m_DeleteStmt = m_Conn->createStatement("DELETE FROM customers WHERE customer_id = :1");
  m_UpdateStmt = m_Conn->createStatement("SELECT * FROM customers");

  m_SignalConnection.Attach(m_Countries->TableChangedSignal, this, &CustomersTableWindow::UpdateTable);
  m_SignalConnection.Connect();
  m_Countries->TableChangedSignal.Connect(&TableChangedSignal, &sig::CSignal<>::Emit);
}

CustomersTableWindow::~CustomersTableWindow()
{
  m_Conn->terminateStatement(m_CreateStmt);
  m_Conn->terminateStatement(m_DeleteStmt);
  m_Conn->terminateStatement(m_UpdateStmt);

  if (m_SignalConnection.IsConnected())
  {
    m_SignalConnection.Disconnect();
    m_Countries->TableChangedSignal.Disconnect(&TableChangedSignal, &sig::CSignal<>::Emit);
  }
}

void CustomersTableWindow::OnUIRender()
{
  ImGui::Begin("Customers");

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

void CustomersTableWindow::OpenCreateWindow()
{
  m_IsCreating = true;
  ImGui::OpenPopup("Create");
}

void CustomersTableWindow::RenderCreateWindow()
{
  if (ImGui::BeginPopupModal("Create", &m_IsCreating, ImGuiWindowFlags_AlwaysAutoResize))
  {
    ImGui::InputText("First name", m_FirstNameBuffer.data(), m_FirstNameBuffer.size());
    ImGui::InputText("Last name", m_LastNameBuffer.data(), m_LastNameBuffer.size());
    ImGui::InputText("Address", m_AddressBuffer.data(), m_AddressBuffer.size());
    ImGui::InputText("Email", m_EmailBuffer.data(), m_EmailBuffer.size());

    DropDown<0>("Country ID", m_Countries->GetTable(), m_CountryIdBuffer);

    if (ButtonCentered("OK"))
    {
      Create(
          m_FirstNameBuffer.data(),
          m_LastNameBuffer.data(),
          m_AddressBuffer.data(),
          m_EmailBuffer.data(),
          m_CountryIdBuffer.data()
        );
      UpdateTable();
      CloseCreateWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void CustomersTableWindow::CloseCreateWindow()
{
  m_IsCreating = false;
}

void CustomersTableWindow::OpenDeleteWindow()
{
  m_IsDeleting = true;
  ImGui::OpenPopup("Delete");
}

void CustomersTableWindow::RenderDeleteWindow()
{
  if (ImGui::BeginPopupModal("Delete", &m_IsDeleting, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Customer ID", m_Table, m_CustomerId);

    if (ButtonCentered("OK"))
    {
      Delete(m_CustomerId);
      UpdateTable();
      CloseDeleteWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void CustomersTableWindow::CloseDeleteWindow()
{
  m_IsDeleting = false;
}

void CustomersTableWindow::OpenErrorWindow()
{
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void CustomersTableWindow::RenderErrorWindow()
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

void CustomersTableWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void CustomersTableWindow::RenderTable()
{
  constexpr auto TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;

  if (ImGui::BeginTable("CustomersTable", 6, TableFlags, ImVec2(-1, -1)))
  {
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("First name");
    ImGui::TableSetupColumn("Last name");
    ImGui::TableSetupColumn("Address");
    ImGui::TableSetupColumn("Email");
    ImGui::TableSetupColumn("Country ID");
    ImGui::TableHeadersRow();

    auto * SortSpecs = ImGui::TableGetSortSpecs();
    if (SortSpecs->SpecsDirty)
    {
      SortTable(SortSpecs->Specs[0].ColumnIndex, SortSpecs->Specs[0].SortDirection);
      SortSpecs->SpecsDirty = false;
    }

    for (const auto & [Id, FirstName, LastName, Address, Email, CountryId] : m_Table)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%d", Id);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(FirstName.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(LastName.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(Address.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(Email.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(CountryId.c_str());
    }

    ImGui::EndTable();
  }
}

void CustomersTableWindow::UpdateTable()
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
          Result->getString(4),
          Result->getString(5),
          Result->getString(6)
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
    Copy(std::get<0>(m_Table.front()), m_CustomerId);
    Copy(std::get<1>(m_Table.front()), m_FirstNameBuffer);
    Copy(std::get<2>(m_Table.front()), m_LastNameBuffer);
    Copy(std::get<3>(m_Table.front()), m_AddressBuffer);
    Copy(std::get<4>(m_Table.front()), m_EmailBuffer);
    Copy(std::get<5>(m_Table.front()), m_CountryIdBuffer);
  }
}

void CustomersTableWindow::SortTable(
    int _ColIdx,
    ImGuiSortDirection _SortDir
  )
{
  std::sort(
      m_Table.begin(), m_Table.end(),
      PredicateImpl<decltype(m_Table)>{ _ColIdx, _SortDir }
    );
}

void CustomersTableWindow::Create(
    const char * _FirstName,
    const char * _LastName,
    const char * _Address,
    const char * _Email,
    const char * _CountryId
  )
{
  try
  {
    m_CreateStmt->setString(1, _FirstName);
    m_CreateStmt->setString(2, _LastName);
    m_CreateStmt->setString(3, _Address);
    m_CreateStmt->setString(4, _Email);
    m_CreateStmt->setString(5, _CountryId);
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

void CustomersTableWindow::Delete(
    int _CustomerID
  )
{
  try
  {
    m_DeleteStmt->setInt(1, _CustomerID);
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