#include "DBLayer.h"

#include <Walnut/Application.h>
#include <Walnut/EntryPoint.h>
#include <Walnut/Image.h>

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
  Walnut::ApplicationSpecification spec;
  spec.Name = "IS lab work";

  Walnut::Application * app = new Walnut::Application(spec);
  app->PushLayer<DBLayer>();
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