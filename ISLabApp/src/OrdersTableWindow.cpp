#include "OrdersTableWindow.h"

#include "CustomersTableWindow.h"
#include "ProductsTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>

OrdersTableWindow::OrdersTableWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn,
    CustomersTableWindow * _Customers,
    ProductsTableWindow * _Products
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn },
    m_Customers{ _Customers },
    m_Products{ _Products }
{
  m_CreateStmt = m_Conn->createStatement("INSERT INTO orders(customer_id, status, order_date, product_id, quantity) VALUES(:1,:2,:3,:4,:5)");
  m_DeleteStmt = m_Conn->createStatement("DELETE FROM orders WHERE order_id = :1");
  m_UpdateStatusStmt = m_Conn->createStatement("UPDATE orders SET status = :1 WHERE order_id = :2");
  m_UpdateStmt = m_Conn->createStatement("SELECT * FROM orders");

  m_SignalConnections.AddConnection(m_Customers->TableChangedSignal, this, &OrdersTableWindow::UpdateTable);
  m_SignalConnections.AddConnection(m_Products->TableChangedSignal, this, &OrdersTableWindow::UpdateTable);
}

OrdersTableWindow::~OrdersTableWindow()
{
  m_Conn->terminateStatement(m_CreateStmt);
  m_Conn->terminateStatement(m_DeleteStmt);
  m_Conn->terminateStatement(m_UpdateStmt);

  m_SignalConnections.Disconnect();
}

void OrdersTableWindow::OnUIRender()
{
  ImGui::Begin("Orders");

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

void OrdersTableWindow::OpenCreateWindow()
{
  m_IsCreating = true;
  ImGui::OpenPopup("Create");
}

void OrdersTableWindow::RenderCreateWindow()
{
  if (ImGui::BeginPopupModal("Create", &m_IsCreating, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Customer ID", m_Customers->GetTable(), m_CustomerId);
    DropDownOrderStatus("Status", m_OrderStatus);

    ImGui::InputInt("Year", &m_Year);
    ImGui::InputInt("Month", &m_Month);
    ImGui::InputInt("Day", &m_Day);

    m_Year = std::clamp(m_Year, 2000, 2022);
    m_Month = std::clamp(m_Month, 0, 12);
    m_Day = std::clamp(m_Day, 0, 31);

    DropDown<0>("Product ID", m_Products->GetTable(), m_ProductId);

    ImGui::InputFloat("Quantity", &m_Quantity);

    if (ButtonCentered("OK"))
    {
      Create(
          m_CustomerId,
          m_OrderStatus,
          oci::Date(m_Env, m_Year, m_Month, m_Day),
          m_ProductId,
          m_Quantity
        );
      CloseCreateWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void OrdersTableWindow::CloseCreateWindow()
{
  m_IsCreating = false;
}

void OrdersTableWindow::OpenDeleteWindow()
{
  m_IsDeleting = true;
  ImGui::OpenPopup("Delete");
}

void OrdersTableWindow::RenderDeleteWindow()
{
  if (ImGui::BeginPopupModal("Delete", &m_IsDeleting, ImGuiWindowFlags_AlwaysAutoResize))
  {
    DropDown<0>("Order ID", m_Table, m_OrderId);

    if (ButtonCentered("OK"))
    {
      Delete(m_CustomerId);
      CloseDeleteWindow();
    }

    RenderErrorWindow();

    ImGui::EndPopup();
  }
}

void OrdersTableWindow::CloseDeleteWindow()
{
  m_IsDeleting = false;
}

void OrdersTableWindow::OpenErrorWindow()
{
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void OrdersTableWindow::RenderErrorWindow()
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

void OrdersTableWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void OrdersTableWindow::RenderTable()
{
  constexpr auto TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
                              ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable;

  if (ImGui::BeginTable("CustomersTable", 6, TableFlags, ImVec2(-1, -1)))
  {
    ImGui::TableSetupColumn("ID");
    ImGui::TableSetupColumn("Customer ID");
    ImGui::TableSetupColumn("Status");
    ImGui::TableSetupColumn("Date");
    ImGui::TableSetupColumn("Product ID");
    ImGui::TableSetupColumn("Quantity");
    ImGui::TableHeadersRow();

    auto * SortSpecs = ImGui::TableGetSortSpecs();
    if (SortSpecs->SpecsDirty)
    {
      SortTable(SortSpecs->Specs[0].ColumnIndex, SortSpecs->Specs[0].SortDirection);
      SortSpecs->SpecsDirty = false;
    }

    for (const auto & [Id, CustomerId, Status, Date, ProductId, Quantity] : m_Table)
    {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("%d", Id);
      ImGui::TableNextColumn();
      ImGui::Text("%d", CustomerId);
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(ORDER_STATUS_TO_STRING.at(Status).c_str());
      ImGui::TableNextColumn();

      const auto DateStr = Date.toText("DD-MM-RR");
      ImGui::TextUnformatted(DateStr.c_str());

      ImGui::TableNextColumn();
      ImGui::Text("%d", ProductId);
      ImGui::TableNextColumn();
      ImGui::Text("%.2f", Quantity);
    }

    ImGui::EndTable();
  }
}

void OrdersTableWindow::UpdateTable()
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
          STRING_TO_ORDER_STATUS.at(Result->getString(3)),
          Result->getDate(4),
          Result->getInt(5),
          Result->getFloat(6)
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
    Copy(std::get<0>(m_Table.front()), m_OrderId);
    Copy(std::get<1>(m_Table.front()), m_CustomerId);
    Copy(std::get<2>(m_Table.front()), m_OrderStatus);
    //Copy(std::get<3>(m_Table.front()), m_Date);
    Copy(std::get<4>(m_Table.front()), m_ProductId);
    Copy(std::get<5>(m_Table.front()), m_Quantity);
  }
}

void OrdersTableWindow::SortTable(
    int _ColIdx,
    ImGuiSortDirection _SortDir
  )
{
  std::sort(
      m_Table.begin(), m_Table.end(),
      PredicateImpl<decltype(m_Table)>{ _ColIdx, _SortDir }
    );
}

void OrdersTableWindow::Create(
    int _CustomerId,
    EOrderStatus _Status,
    const oci::Date & _Date,
    int _ProductId,
    float _Quantity
  )
{
  try
  {
    m_CreateStmt->setInt(1, _CustomerId);
    m_CreateStmt->setString(2, ORDER_STATUS_TO_STRING.at(_Status));
    m_CreateStmt->setDate(3, _Date);
    m_CreateStmt->setInt(4, _ProductId);
    m_CreateStmt->setFloat(5, _Quantity);
    m_CreateStmt->executeUpdate();
    m_Conn->commit();
    UpdateTable();
    TableChangedSignal.Emit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}

void OrdersTableWindow::UpdateStatus(
    int _OrderId,
    EOrderStatus _Status
  )
{
  try
  {
    m_UpdateStatusStmt->setString(1, ORDER_STATUS_TO_STRING.at(_Status));
    m_UpdateStatusStmt->setInt(2, _OrderId);
    m_UpdateStatusStmt->executeUpdate();
    m_Conn->commit();
    UpdateTable();
    TableChangedSignal.Emit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}

void OrdersTableWindow::Delete(
    int _OrderID
  )
{
  try
  {
    m_DeleteStmt->setInt(1, _OrderID);
    m_DeleteStmt->executeUpdate();
    m_Conn->commit();
    UpdateTable();
    TableChangedSignal.Emit();
  }
  catch (const oci::SQLException & ex)
  {
    m_ErrorMessage = ex.what();
    OpenErrorWindow();
  }
}