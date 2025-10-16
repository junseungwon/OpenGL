cmake_minimum_required(VERSION 3.10)
project(TextureDemo LANGUAGES C CXX)

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

# ── stb_image 구현 OBJECT 라이브러리 ──
add_library(stb_image_obj OBJECT src/stb_image_impl.cpp)
target_include_directories(stb_image_obj PUBLIC ${CMAKE_SOURCE_DIR}/include)


add_executable(TextureMix src/main_mix.cpp)
target_include_directories(TextureMix PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GLFW_DIR}/include
)
if (WIN32)
  target_link_libraries(TextureMix PRIVATE glfw glad opengl32 stb_image_obj)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(TextureMix PRIVATE glfw glad OpenGL::GL stb_image_obj)
endif()

# ── 빌드 후 assets/shaders 복사 ──
foreach(tgt IN ITEMS TextureMix TextureMix)
  add_custom_command(TARGET ${tgt} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${tgt}>/assets)
  add_custom_command(TARGET ${tgt} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:${tgt}>/shaders)
endforeach()

# ── (권장) VS 디버거 작업 디렉터리 고정 ──
if (MSVC)
  set_property(TARGET TextureMix PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
               "$<TARGET_FILE_DIR:TextureSingle>")
  set_property(TARGET TextureMix PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
               "$<TARGET_FILE_DIR:TextureMix>")
endif()