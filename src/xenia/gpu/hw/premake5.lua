project_root = "../../../.."
include(project_root.."/tools/build")

group("src")
project("xenia-gpu-hw")
  uuid("a32ff13d-8a1e-4e42-b293-eab8540e7997")
  kind("StaticLib")
  language("C++")
  links({
    "xenia-base",
    "xenia-gpu",
  })
  local_platform_files()
