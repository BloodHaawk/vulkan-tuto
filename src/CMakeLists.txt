include_directories(${vulkan_tuto_SOURCE_DIR}/include)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

set(
  SOURCES
  main.cpp
  application.cpp
)

execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_SOURCE_DIR}/shaders" "${CMAKE_BINARY_DIR}/shaders")

add_executable(vulkan_tuto ${SOURCES})
target_link_libraries(vulkan_tuto glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)

add_custom_command(TARGET vulkan_tuto PRE_BUILD
                   COMMAND "glslc"
                           "../shaders/vertex.vert"
                           "-o"
                           "../shaders/vertex.spv"
)
add_custom_command(TARGET vulkan_tuto PRE_BUILD
                   COMMAND "glslc"
                           "../shaders/fragment.frag"
                           "-o"
                           "../shaders/fragment.spv"
)
