/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_UI_MICROPROFILE_DRAWER_H_
#define XENIA_UI_MICROPROFILE_DRAWER_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "xenia/ui/immediate_drawer.h"

namespace xe {
namespace ui {

class MicroprofileDrawer {
 public:
  enum class BoxType {
    kBar = 0,   // MicroProfileBoxTypeBar
    kFlat = 1,  // MicroProfileBoxTypeFlat
  };

  // Initially hidden.
  MicroprofileDrawer(ImmediateDrawer* immediate_drawer);

  void DrawBox(int x0, int y0, int x1, int y1, uint32_t color, BoxType type);
  void DrawLine2D(uint32_t count, float* vertices, uint32_t color);
  // The name DrawTextString collides with DrawText in Windows.
  void DrawTextString(int x, int y, uint32_t color, const char* text,
                      int text_length);

 protected:
  void SetupFont();

  void EndVertices();
  void Flush();

  ImmediateDrawer* immediate_drawer_;

  int vertex_count_ = 0;

  struct {
    uint16_t char_offsets[256];
  } font_description_ = {{0}};
};

}  // namespace ui
}  // namespace xe

#endif  // XENIA_UI_MICROPROFILE_DRAWER_H_
