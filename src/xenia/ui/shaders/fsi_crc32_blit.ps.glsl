/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#version 460
// For XeSL.
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_GOOGLE_include_directive : require

#include "crc32.xesli"

layout(std140, push_constant) uniform XeCRC32Constants {
  uint buffer_width;
  uint buffer_samples;
} xe_crc32_constants;

layout(set=0, binding=0) restrict readonly buffer XeCRC32Buffer {
  uint xe_crc32_values[];
};

layout(location=0) out vec4 xe_crc32_result;

void main() {
  uvec2 xe_pixel_coord = uvec2(gl_FragCoord.xy);
  uint xe_buffer_offset =
      xe_crc32_constants.buffer_samples *
      (xe_pixel_coord.x + xe_pixel_coord.y * xe_crc32_constants.buffer_width);
  uint xe_result = ~0u;
  uint xe_sample_index;
  for (xe_sample_index = 0u;
       xe_sample_index < xe_crc32_constants.buffer_samples; ++xe_sample_index) {
    uint xe_sample = xe_crc32_values[xe_buffer_offset + xe_sample_index];
    xe_result = XeCRC32AddByte(xe_result, xe_sample & 0xFFu);
    xe_result = XeCRC32AddByte(xe_result, (xe_sample >> 8u) & 0xFFu);
    xe_result = XeCRC32AddByte(xe_result, (xe_sample >> 16u) & 0xFFu);
    xe_result = XeCRC32AddByte(xe_result, xe_sample >> 24u);
  }
  xe_crc32_result =
      vec4((uvec4(xe_result) >> uvec4(0u, 8u, 16u, 24u)) & 0xFFu) *
      (1.0 / 255.0);
}
