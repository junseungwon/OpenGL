cmake_minimum_required(VERSION 3.10)
project(FirstOpenGL LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GLFW_DIR "C:/MJC/OpenGLLab/external/glfw-3.4")
set(GLAD_DIR "C:/MJC/OpenGLLab/external/glad")
add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw_build)
add_library(glad ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)
add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE
 ${CMAKE_CURRENT_SOURCE_DIR}/include
 ${GLFW_DIR}/include
)
if (WIN32)
 target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad opengl32)
else()
 target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad)
endif()