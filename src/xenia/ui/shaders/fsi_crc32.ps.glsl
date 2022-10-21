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
// For the shader.
#extension GL_ARB_fragment_shader_interlock : require

#include "crc32.xesli"

layout(std140, push_constant) uniform XeCRC32Constants {
  uint buffer_width;
  uint buffer_samples;
} xe_crc32_constants;

layout(set=0, binding=0) restrict coherent buffer XeCRC32Buffer {
  uint xe_crc32_values[];
};

layout(location=0) flat in uint xe_primitive_id;

layout(sample_interlock_ordered) in;

void main() {
  uvec2 xe_pixel_coord = uvec2(gl_FragCoord.xy);
  uint xe_buffer_offset =
      xe_crc32_constants.buffer_samples *
      (xe_pixel_coord.x + xe_pixel_coord.y * xe_crc32_constants.buffer_width);
  uint xe_samples_remaining = uint(gl_SampleMaskIn[0]);
  beginInvocationInterlockARB();
  while (xe_samples_remaining != 0u) {
    uint xe_sample_index = uint(findLSB(xe_samples_remaining));
    xe_samples_remaining &= ~(1u << xe_sample_index);
    uint xe_sample_address = xe_buffer_offset + xe_sample_index;
    uint xe_sample_value = xe_crc32_values[xe_sample_address];
    xe_sample_value = XeCRC32AddByte(xe_sample_value, xe_primitive_id & 0xFFu);
    xe_sample_value =
        XeCRC32AddByte(xe_sample_value, (xe_primitive_id >> 8u) & 0xFFu);
    xe_sample_value =
        XeCRC32AddByte(xe_sample_value, (xe_primitive_id >> 16u) & 0xFFu);
    xe_sample_value = XeCRC32AddByte(xe_sample_value, xe_primitive_id >> 24u);
    xe_sample_value = XeCRC32AddByte(xe_sample_value, xe_sample_index);
    xe_sample_value = XeCRC32AddByte(xe_sample_value, xe_pixel_coord.x & 0xFFu);
    xe_sample_value = XeCRC32AddByte(xe_sample_value, xe_pixel_coord.x >> 8u);
    xe_sample_value = XeCRC32AddByte(xe_sample_value, xe_pixel_coord.y & 0xFFu);
    xe_sample_value = XeCRC32AddByte(xe_sample_value, xe_pixel_coord.y >> 8u);
    xe_crc32_values[xe_sample_address] = xe_sample_value;
  }
  endInvocationInterlockARB();
}
