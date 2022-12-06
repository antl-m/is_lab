#include "MakeOrderWindow.h"

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

MakeOrderWindow::MakeOrderWindow(
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

  m_SignalConnections.AddConnection(m_Products->TableChangedSignal, this, &MakeOrderWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Customers->TableChangedSignal, this, &MakeOrderWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Categories->TableChangedSignal, this, &MakeOrderWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Orders->TableChangedSignal, this, &MakeOrderWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Inventories->TableChangedSignal, this, &MakeOrderWindow::UpdateData);
  m_SignalConnections.AddConnection(m_Warehouses->TableChangedSignal, this, &MakeOrderWindow::UpdateData);
}

MakeOrderWindow::~MakeOrderWindow()
{
  m_Conn->terminateStatement(m_MakeOrderStmt);

  m_SignalConnections.Disconnect();
}

void MakeOrderWindow::OnUIRender()
{
  ImGui::Begin("Make order");

  if (ImGui::IsWindowAppearing())
    m_NeedUpdate = true;

  if (m_NeedUpdate)
  {
    UpdateData();
    m_NeedUpdate = false;
  }

  if (m_CustomerData.has_value())
    RenderProductsWindow();
  else
    RenderLoginWindow();

  RenderErrorWindow();
  ImGui::End();
}

void MakeOrderWindow::OpenErrorWindow(
    const std::string_view _ErrorMessage
  )
{
  m_ErrorMessage = _ErrorMessage;
  m_IsError = true;
  ImGui::OpenPopup("Error");
}

void MakeOrderWindow::RenderErrorWindow()
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

void MakeOrderWindow::CloseErrorWindow()
{
  m_IsError = false;
}

void MakeOrderWindow::UpdateData()
{
  m_ProductQuantitiesCache.clear();
}

void MakeOrderWindow::RenderProductEntry(
    const int _ProductID,
    const std::string & _ProductName,
    const std::string & _Description,
    const float _Cost,
    const float _Price,
    const int _CategoryID
  )
{
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.15f, 0.2f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.16f, 0.25f, 0.35f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.31f, 0.47f, 0.65f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.46f, 0.55f, 0.62f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.57f, 0.68f, 0.77f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.37f, 0.71f, 0.97f, 1.0f));

  ImGui::BeginChild(_ProductID, ImVec2(-1, 300), true);
  const auto WindowPadding = ImGui::GetStyle().WindowPadding;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  ImGui::BeginChild("DescriptionBlock", ImVec2(-200, -1));

  ImGui::PushFont(GetFontL());
  ImGui::TextUnformatted(_ProductName.c_str());
  ImGui::PopFont();
  ImGui::PushFont(GetFontS());
  ImGui::TextDisabled(m_Categories->GetCategoryName(_CategoryID).data());
  ImGui::PopFont();
  ImGui::TextUnformatted(_Description.c_str());

  std::set<int> Warehouses;
  for (const auto & [WarehouseId, WarehouseName, CountryId] : m_Warehouses->GetTable())
    if (m_CustomerData->CountryID == CountryId)
      Warehouses.insert(WarehouseId);

  int AvailableCount = 0;
  int FastDeliveryThreshold = 0;
  for (const auto & [ProductId, WarehouseId, Quantity] : m_Inventories->GetTable())
  {
    if (ProductId != _ProductID)
      continue;

    AvailableCount += Quantity;

    if (Warehouses.find(WarehouseId) != Warehouses.end())
      FastDeliveryThreshold += Quantity;
  }

  ImGui::TextDisabled("\nAvailable: %d\nFast delivery up to %d items", AvailableCount, FastDeliveryThreshold);

  ImGui::EndChild();
  ImGui::SameLine();
  ImGui::BeginChild("PriceBlock", ImVec2(-1, -1));

  ImGui::PushFont(GetFontL());
  int q = m_ProductQuantitiesCache[_ProductID];
  ImGui::Text("%.2f", _Price * std::max(1, q));
  ImGui::SetNextItemWidth(200 - WindowPadding.x);
  ImGui::InputInt("##Quantity", &q);
  m_ProductQuantitiesCache[_ProductID] = q = std::clamp(q, 0, AvailableCount);
  ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFFFFFF);
  ImGui::BeginDisabled(q < 1);
  if (ImGui::Button("Buy", ImVec2(-1, -1)))
  {
    m_Orders->Create(m_CustomerData->ID, EOrderStatus::CREATED, oci::Date::getSystemDate(m_Env), _ProductID, q);
    m_Inventories->Decrease(_ProductID, m_CustomerData->CountryID, q);
  }
  ImGui::EndDisabled();
  ImGui::PopStyleColor();

  ImGui::PopFont();

  ImGui::EndChild();

  ImGui::PopStyleVar();
  ImGui::EndChild();

  ImGui::PopStyleColor(6);
}

void MakeOrderWindow::RenderProductsWindow()
{
  ImGui::Text(
      "Name: %s %s\nEmail: %s\nAddress: %s",
      m_CustomerData->FirstName.c_str(),
      m_CustomerData->LastName.c_str(),
      m_CustomerData->Email.c_str(),
      m_CustomerData->Address.c_str()
    );

  if (ImGui::Button("logout"))
    return m_CustomerData.reset();

  ImGui::Separator();

  ImGui::BeginChild("ProductsList", ImVec2(-1, -1), true);

  for (const auto & [ID, Name, Description, Cost, Price, Category] : m_Products->GetTable())
    RenderProductEntry(ID, Name, Description, Cost, Price, Category);

  ImGui::EndChild();
}

void MakeOrderWindow::RenderLoginWindow()
{
  ImGui::PushFont(GetFontL());
  ImGui::TextUnformatted("Enter your data\nto login into your account");

  if (ImGui::InputText("First name", m_FirstNameBuffer.data(), m_FirstNameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    ImGui::SetKeyboardFocusHere();

  if (ImGui::InputText("Last name", m_LastNameBuffer.data(), m_LastNameBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue))
    ImGui::SetKeyboardFocusHere();

  const bool Accept = ImGui::InputText("Email", m_EmailBuffer.data(), m_EmailBuffer.size(), ImGuiInputTextFlags_EnterReturnsTrue);

  if (ImGui::Button("Log in") || Accept)
  {
    for (const auto & [Id, FirstName, LastName, Address, Email, Country] : m_Customers->GetTable())
    {
      if (FirstName != m_FirstNameBuffer.data() ||
          LastName != m_LastNameBuffer.data() ||
          Email != m_EmailBuffer.data())
        continue;

      m_CustomerData = CustomerData{ Id, FirstName, LastName, Address, Email, Country };
      break;
    }

    if (!m_CustomerData.has_value())
      OpenErrorWindow("Invalid login data");
  }

  ImGui::PopFont();
}