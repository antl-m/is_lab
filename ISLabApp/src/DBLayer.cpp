#include "DBLayer.h"

#include "CountriesTableWindow.h"
#include "WarehousesTableWindow.h"
#include "ProductCategoriesTableWindow.h"
#include "ProductsTableWindow.h"

#include <imgui.h>
#include <string_view>

namespace
{

constexpr std::string_view USER_NAME      = "demouser";
constexpr std::string_view PASSWORD       = "demouser";
constexpr std::string_view CONNECT_STRING = "gdn-nt15:1521/XEPDB1";

} // namespace

void DBLayer::OnAttach()
{
  m_Env = oci::Environment::createEnvironment();
  m_Conn = m_Env->createConnection(USER_NAME.data(), PASSWORD.data(), CONNECT_STRING.data());

  auto Countries = std::make_unique<CountriesTableWindow>(m_Env, m_Conn);
  auto Warehouses = std::make_unique<WarehousesTableWindow>(m_Env, m_Conn, Countries.get());
  auto Categories = std::make_unique<ProductCategoriesTableWindow>(m_Env, m_Conn);
  auto Products = std::make_unique<ProductsTableWindow>(m_Env, m_Conn, Categories.get());

  m_Windows.emplace_back(std::move(Countries));
  m_Windows.emplace_back(std::move(Warehouses));
  m_Windows.emplace_back(std::move(Categories));
  m_Windows.emplace_back(std::move(Products));
}

void DBLayer::OnDetach()
{
  m_Windows.clear();

  m_Env->terminateConnection(m_Conn);
  oci::Environment::terminateEnvironment(m_Env);
}

void DBLayer::OnUIRender()
{
  for (auto & Window : m_Windows)
    Window->OnUIRender();
}
