#pragma once

struct IWindow
{
  virtual ~IWindow() = default;

  virtual void OnUIRender() = 0;
};
