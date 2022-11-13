#pragma once

#include "ISLabApp.h"
#include "IWindow.h"

#include <Walnut/Layer.h>
#include <string>
#include <vector>
#include <memory>

class DBLayer
  : public Walnut::Layer
{
public:

  virtual void OnAttach() override;
  virtual void OnDetach() override;
  virtual void OnUIRender() override;

private:

  oci::Environment * m_Env = nullptr;
  oci::Connection * m_Conn = nullptr;

  std::vector<std::unique_ptr<IWindow>> m_Windows;
};

