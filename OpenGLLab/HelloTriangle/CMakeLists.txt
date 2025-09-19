cmake_minimum_required(VERSION 3.10)
project(HelloTriangle LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 상위 폴더의 external 경로 계산 (프로젝트 루트 바로 위에 external/가 있다고 가정)
get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../external" ABSOLUTE)
set(GLFW_DIR "${EXTERNAL_DIR}/glfw-3.4")
set(GLAD_DIR "${EXTERNAL_DIR}/glad")

# GLFW 빌드 (외부 소스를 서브디렉터리로 추가)
add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw_build)

# GLAD 정적 라이브러리
add_library(glad ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# 실행 파일
add_executable(${PROJECT_NAME} src/main.cpp)

# 포함 경로
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLFW_DIR}/include
)

# 링크
if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad opengl32)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad)
endif()

# ── 셰이더 상대경로 지원: 빌드 후 shaders/를 실행 파일 폴더로 복사 ──
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders")

# (권장) VS 디버거 작업 디렉터리를 실행 파일 폴더로 고정
if (MSVC)
  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
               "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif()