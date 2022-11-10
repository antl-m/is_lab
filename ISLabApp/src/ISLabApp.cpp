#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"
#include "Walnut/Image.h"

#include "occi.h"

namespace oci = oracle::occi;

class ExampleLayer : public Walnut::Layer
{
public:

  ~ExampleLayer() = default;

  virtual void OnAttach() override
  {

    static const std::string userName = "demouser";
    static const std::string password = "demouser";
    static const std::string connectString = "gdn-nt15:1521/XEPDB1";

    oci::Environment * env = oci::Environment::createEnvironment();
    try
    {
      oci::Connection * conn = env->createConnection(
          userName, password, connectString
        );
      oci::Statement * stmt = conn->createStatement(
          "SELECT "
             "table_name, owner "
           "FROM "
             "user_tables"
        );
      oci::ResultSet * rs = stmt->executeQuery();
      rs->next();
      m_Data = rs->getString(1);

      stmt->closeResultSet(rs);
      conn->terminateStatement(stmt);
      env->terminateConnection(conn);
    }
    catch (oci::SQLException & exception)
    {
      m_ExceptionMessage = exception.what();
    }
    oci::Environment::terminateEnvironment(env);
  }

  virtual void OnUIRender() override
  {
    ImGui::Begin("Main window");
    ImGui::Button("Button");
    ImGui::Text("Data : %s", m_Data.c_str());
    ImGui::Text("Exception : %s", m_ExceptionMessage.c_str());
    ImGui::End();
  }

private:

  std::string m_Data;
  std::string m_ExceptionMessage;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
  Walnut::ApplicationSpecification spec;
  spec.Name = "IS lab work";

  Walnut::Application* app = new Walnut::Application(spec);
  app->PushLayer<ExampleLayer>();
  app->SetMenubarCallback([app]()
    {
      if (ImGui::BeginMenu("File"))
      {
        if (ImGui::MenuItem("Exit"))
        {
          app->Close();
        }
        ImGui::EndMenu();
      }
    });
  return app;
}