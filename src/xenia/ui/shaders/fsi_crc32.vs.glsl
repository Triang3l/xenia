/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#version 460

layout(location=0) in vec2 xe_in_position;
layout(location=1) in uint xe_in_primitive_id;
layout(location=0) out uint xe_out_primitive_id;

void main() {
  gl_Position = vec4(xe_in_position, 0.0, 1.0);
  xe_out_primitive_id = xe_in_primitive_id;
}
