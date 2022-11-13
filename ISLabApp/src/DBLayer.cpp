#include "DBLayer.h"

#include "CountriesTableWindow.h"

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

  m_Windows.emplace_back(std::make_unique<CountriesTableWindow>(m_Env, m_Conn));
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
