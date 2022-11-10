#include "DBLayer.h"

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
}

void DBLayer::OnDetach()
{
  m_Env->terminateConnection(m_Conn);
  oci::Environment::terminateEnvironment(m_Env);
}

void DBLayer::OnUIRender()
{
  ImGui::Begin("Main window");

  ImGui::InputText("Country ID", m_CountryId.data(), m_CountryId.size());
  ImGui::InputText("Country Name", m_CountryName.data(), m_CountryName.size());

  if (ImGui::Button("Add"))
  {
    std::string Query = std::string{"insert into COUNTRIES (COUNTRY_ID, COUNTRY_NAME) values ('"}
                          .append(m_CountryId.c_str()).append("', '").append(m_CountryName.c_str())
                          .append("')");

    auto * Statement = m_Conn->createStatement(Query);
    try
    {
      auto * Result = Statement->executeQuery();
      m_Conn->commit();
      Statement->closeResultSet(Result);
    }
    catch (const oci::SQLException & ex)
    {
      m_ExceptionMessage = "Code: ";
      m_ExceptionMessage.append(std::to_string(ex.getErrorCode())).append("; Message: ").append(ex.what());
    }
    m_Conn->terminateStatement(Statement);
  }

  if (ImGui::Button("Get countries"))
  {
    m_Data.clear();
    auto * Statement = m_Conn->createStatement("select * from countries");
    auto * Result = Statement->executeQuery();
    Result->next();

    while (Result->status() == oci::ResultSet::DATA_AVAILABLE)
    {
      m_Data.append(Result->getString(1)).append("\t").append(Result->getString(2)).append("\n");
      Result->next();
    }

    Statement->closeResultSet(Result);
    m_Conn->terminateStatement(Statement);
  }

  ImGui::Text("Data :\n%s", m_Data.c_str());
  ImGui::Text("Exception : %s", m_ExceptionMessage.c_str());
  ImGui::End();
}