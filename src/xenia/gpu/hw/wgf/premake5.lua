project_root = "../../../../.."
include(project_root.."/tools/build")

-- Common Xenos emulation functionality for host implementations on top of
-- Direct3D 10+.
-- Using the name WGF (Windows Graphics Foundation, another name of DirectX
-- graphics APIs introduced since Windows Vista) to clearly distinguish the host
-- functionality from Direct3D 9 on the Xbox 360.

group("src")
project("xenia-gpu-hw-wgf")
  uuid("b4539cf4-4445-4995-9373-07800a57d8c9")
  kind("StaticLib")
  language("C++")
  links({
    "xenia-base",
    "xenia-gpu",
    "xenia-gpu-hw",
  })
  local_platform_files()
