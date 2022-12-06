#include "AdminWindow.h"

#include "ProductsTableWindow.h"
#include "CustomersTableWindow.h"
#include "ProductCategoriesTableWindow.h"
#include "OrdersTableWindow.h"
#include "InventoriesTableWindow.h"
#include "WarehousesTableWindow.h"

#include <imgui.h>
#include <algorithm>
#include <map>
#include <set>

AdminWindow::AdminWindow(
    oci::Environment * _Env,
    oci::Connection * _Conn,
    ProductsTableWindow * _Products,
    CustomersTableWindow * _Customers,
    ProductCategoriesTableWindow * _Categories,
    OrdersTableWindow * _Orders,
    InventoriesTableWindow * _Inventories,
    WarehousesTableWindow * _Warehouses
  ) :
    m_Env{ _Env },
    m_Conn{ _Conn },
    m_Products{ _Products },
    m_Customers{ _Customers },
    m_Categories{ _Categories },
    m_Orders{ _Orders },
    m_Inventories{ _Inventories },
    m_Warehouses{ _Warehouses }
{
  m_MakeOrderStmt = m_Conn->createStatement("SELECT * FROM inventories");

  m_SignalConnections.AddConnection(m_Products->TableChangedSignal, this, &AdminWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Customers->TableChangedSignal, this, &AdminWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Categories->TableChangedSignal, this, &AdminWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Orders->TableChangedSignal, this, &AdminWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Inventories->TableChangedSignal, this, &AdminWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Warehouses->TableChangedSignal, this, &AdminWindow::UpdateData);

  for (const auto & Status : ORDER_STATUS_LIST)
    m_StatusFilter[Status] = true;
}

AdminWindow::~AdminWindow()
{
  m_Conn->terminateStatement(m_MakeOrderStmt);

  m_SignalConnections.Disconnect();
}

void AdminWindow::OnUIRender()
{
  ImGui::Begin("Admin panel");

  if (ImGui::IsWindowAppearing())
    m_NeedUpdate = true;

  if (m_NeedUpdate)
  {
    UpdateData();
    m_NeedUpdate = false;
  }

  if (ImGui::BeginTabBar("##Tabs"))
  {
    if (ImGui::BeginTabItem("Orders"))
    {
      RenderAdminPanel();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Charts"))
    {
      RenderCharts();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }


  RenderErrorWindow();
  ImGui::End();
}

void AdminWindow::OpenErrorWindow(
    const std::string_view _ErrorMessage
  )
{
  m_ErrorMessage = _ErrorMessage;
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void AdminWindow::RenderErrorWindow()
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

void AdminWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void AdminWindow::UpdateData()
{
  m_OrderEntries.clear();

  std::unordered_map<int, OrderCustomerData> Customers;
  std::unordered_map<int, OrderProductData> Products;

  for (const auto & [CustomerId, FirstName, LastName, Address, Email, Country] : m_Customers->GetTable())
    Customers.emplace(CustomerId, OrderCustomerData{ CustomerId, FirstName, LastName, Address });

  for (const auto & [ProductId, Name, Description, Cost, Price, Category] : m_Products->GetTable())
    Products.emplace(ProductId, OrderProductData{ ProductId, Name, Cost, Price });

  for (const auto & [OrderId, CustomerId, OrderStatus, Date, ProductId, Quantity] : m_Orders->GetTable())
    m_OrderEntries.emplace_back(OrderEntry{
        OrderId,
        Quantity,
        OrderStatus,
        Date,
        Customers.at(CustomerId),
        Products.at(ProductId)
      });
}

void AdminWindow::RenderOrderEntry(
    OrderEntry & _Order
  )
{
  ImGui::PushID(_Order.OrderId);
  ImGui::BeginGroup();
  if (ImGui::BeginTable("OrderData", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX))
  {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Order #");
    ImGui::TableNextColumn();
    ImGui::Text("%d", _Order.OrderId);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Date");
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_Order.Date.toText("DD-MM-RR").c_str());

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Customer");
    ImGui::TableNextColumn();
    ImGui::Text("%s %s", _Order.Customer.FirstName.c_str(), _Order.Customer.LastName.c_str());

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Product");
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(_Order.Product.ProductName.c_str());

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Quantity");
    ImGui::TableNextColumn();
    ImGui::Text("%f", _Order.Quantity);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Price");
    ImGui::TableNextColumn();
    ImGui::Text("%.2f", _Order.Product.Price);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Cost");
    ImGui::TableNextColumn();
    ImGui::Text("%.2f", _Order.Product.Cost);

    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted("Status");
    ImGui::TableNextColumn();
    if (DropDownOrderStatus("##Status", _Order.Status))
      m_Orders->UpdateStatus(_Order.OrderId, _Order.Status);

    ImGui::EndTable();
  }
  ImGui::EndGroup();
  ImGui::GetWindowDrawList()->AddRect(
      ImGui::GetItemRectMin(),
      ImGui::GetItemRectMax(),
      IM_COL32(255, 255, 255, 255)
    );
  ImGui::PopID();
}

void AdminWindow::RenderAdminPanel()
{
  if (ImGui::Button("Filters"))
    ImGui::OpenPopup("FiltersPopup");

  if (ImGui::BeginPopup("FiltersPopup"))
  {
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Order statuses");
    ImGui::Separator();
    for (const auto & [Status, String]: ORDER_STATUS_TO_STRING)
      ImGui::MenuItem(String.c_str(), nullptr, &m_StatusFilter[Status]);
    ImGui::EndGroup();

    ImGui::EndPopup();
  }

  ImGui::Separator();

  ImGui::BeginChild("ProductsList", ImVec2(-1, -1), true);

  for (std::size_t i = 0; i < m_OrderEntries.size(); ++i)
    if (IsFilterSuitable(m_OrderEntries.at(i)))
      RenderOrderEntry(m_OrderEntries.at(i));

  ImGui::EndChild();
}

bool AdminWindow::IsFilterSuitable(
    const OrderEntry & _Order
  ) const
{
  return m_StatusFilter.at(_Order.Status);
}
