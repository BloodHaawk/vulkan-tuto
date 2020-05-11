include_directories(${vulkan_tuto_SOURCE_DIR}/include)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

set(
  SOURCES
  main.cpp
  application.cpp
)

add_executable(vulkan_tuto ${SOURCES})
target_link_libraries(vulkan_tuto glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)
