cmake_minimum_required(VERSION 3.0.0)
project(vulkan_test VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Weffc++ -Wno-sign-compare -Wconversion -Wsign-conversion -pedantic-errors")

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME}
${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

target_link_libraries(${PROJECT_NAME} glfw ${GLFW_LIBRARIES} Vulkan::Vulkan)
