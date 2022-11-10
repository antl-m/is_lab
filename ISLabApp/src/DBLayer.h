#pragma once

#include "ISLabApp.h"

#include <Walnut/Layer.h>
#include <string>

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

  std::string m_Data;
  std::string m_ExceptionMessage;

  std::string m_CountryId = std::string(2 + 1, '\0');
  std::string m_CountryName = std::string(40 + 1, '\0');
};

