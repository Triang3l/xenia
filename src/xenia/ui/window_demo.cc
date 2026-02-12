/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <cstring>

#include "third_party/imgui/imgui.h"
#include "xenia/base/clock.h"
#include "xenia/base/logging.h"
#include "xenia/base/profiling.h"
#include "xenia/base/threading.h"
#include "xenia/ui/imgui_dialog.h"
#include "xenia/ui/imgui_drawer.h"
#include "xenia/ui/ui_event.h"
#include "xenia/ui/virtual_key.h"
#include "xenia/ui/window.h"
#include "xenia/ui/window_demo.h"

namespace xe {
namespace ui {

WindowDemoApp::~WindowDemoApp() { Profiler::Shutdown(); }

bool WindowDemoApp::OnInitialize() {
  Profiler::Initialize();
  Profiler::ThreadEnter("Main");

  enum : size_t {
    kZOrderImGui,
    kZOrderProfiler,
    kZOrderWindowDemoInput,
  };

  // Create the window.
  window_ = xe::ui::Window::Create(app_context(), GetName(), 1920, 1200);
  window_->AddListener(&window_listener_);
  window_->AddInputListener(&window_listener_, kZOrderWindowDemoInput);

  // Main menu.
  auto main_menu = MenuItem::Create(MenuItem::Type::kNormal);
  auto file_menu = MenuItem::Create(MenuItem::Type::kPopup, "&File");
  {
    file_menu->AddChild(
        MenuItem::Create(MenuItem::Type::kString, "&Close", "Alt+F4",
                         [this]() { window_->RequestClose(); }));
  }
  main_menu->AddChild(std::move(file_menu));
  auto debug_menu = MenuItem::Create(MenuItem::Type::kPopup, "&Debug");
  {
    debug_menu->AddChild(MenuItem::Create(MenuItem::Type::kString,
                                          "Toggle Profiler &Display", "F3",
                                          []() { Profiler::ToggleDisplay(); }));
    debug_menu->AddChild(MenuItem::Create(MenuItem::Type::kString,
                                          "&Pause/Resume Profiler", "`",
                                          []() { Profiler::TogglePause(); }));
  }
  main_menu->AddChild(std::move(debug_menu));
  window_->SetMainMenu(std::move(main_menu));

  // Open the window once it's configured.
  if (!window_->Open()) {
    XELOGE("Failed to open the main window");
    return false;
  }

  // Setup drawing to the window.

  imgui_drawer_ = std::make_unique<ImGuiDrawer>(window_.get(), kZOrderImGui);
  demo_dialog_ = std::make_unique<WindowDemoDialog>(imgui_drawer_.get());

  Profiler::SetUserIO(kZOrderProfiler, window_.get());

  return true;
}

void WindowDemoApp::WindowDemoWindowListener::OnClosing(UIEvent& e) {
  app_context_.QuitFromUIThread();
}

void WindowDemoApp::WindowDemoWindowListener::OnKeyDown(KeyEvent& e) {
  switch (e.virtual_key()) {
    case VirtualKey::kF3:
      Profiler::ToggleDisplay();
      break;
    default:
      return;
  }
  e.set_handled(true);
}

void WindowDemoApp::WindowDemoDialog::OnDraw(ImGuiIO& io) {
  ImGui::ShowDemoWindow();
  ImGui::ShowMetricsWindow();
}

}  // namespace ui
}  // namespace xe
