cmake_minimum_required(VERSION 3.10)
project(Modeling LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 상위 폴더의 external 경로 계산 (프로젝트 루트 바로 위에 external/가 있다고 가정)
get_filename_component(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/../external" ABSOLUTE)
set(GLFW_DIR "${EXTERNAL_DIR}/glfw-3.4")
set(GLAD_DIR "${EXTERNAL_DIR}/glad")
set(GLM_DIR "${EXTERNAL_DIR}/glm")  # header-only
set(IMGUI_DIR "${EXTERNAL_DIR}/imgui")
set(ASSIMP_DIR "${EXTERNAL_DIR}/assimp")

# GLFW 빌드 (외부 소스를 서브디렉터리로 추가)
add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw_build)

# GLAD 정적 라이브러리
add_library(glad ${GLAD_DIR}/src/glad.c) 
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# 보통 빌드하면 lib 폴더 안에 .lib 또는 .a 파일이 생김
# Windows MSVC 기준 예시임 (Release x64)
set(ASSIMP_LIB_DIR "${ASSIMP_DIR}/build/lib/Release")
link_directories(${ASSIMP_LIB_DIR})

# ### ImGui 정적 라이브"imgui_lib" 생성 ###
add_library(imgui_lib STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp 
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
 )
# ImGui 라이브러리가 자신의 헤더 및 백엔드 헤더를 찾도록 설정
target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
# ImGui 백엔드는 glfw 및 glad 타겟에 의존함
target_link_libraries(imgui_lib PUBLIC glfw glad)
# ### ImGui 정적 라이브러리 끝 ###

# ── stb_image 구현 OBJECT 라이브러리 ──
add_library(stb_image_obj OBJECT src/stb_image_impl.cpp )
target_include_directories(stb_image_obj PUBLIC ${CMAKE_SOURCE_DIR}/include)


# 실행 파일
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/animation.cpp
    src/animator.cpp
    src/bone.cpp
)

# 포함 경로
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GLFW_DIR}/include
    ${GLM_DIR}
    ${ASSIMP_DIR}/include
    ${ASSIMP_DIR}/build/include  # 빌드된 config.h 등을 위해 필요
    # ${IMGUI_DIR}는 imgui_lib 링크 시 자동으로 상속됨 (PUBLIC)
)

# 링크
if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad opengl32 stb_image_obj imgui_lib 
  assimp-vc143-mt   # ← 실제 .lib 파일 이름에서 .lib 뺀 문자열로 수정
  )
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad stb_image_obj imgui_lib
  assimp-vc143-mt   # ← 실제 .lib 파일 이름에서 .lib 뺀 문자열로 수정
  )
endif()

# ── 셰이더 상대경로 지원: 빌드 후 shaders/를 실행 파일 폴더로 복사 ──
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders")

# ── 에셋 복사 추가 ──
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_CURRENT_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets")

          # ── Assimp DLL 복사 (Windows) ──
if (WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ASSIMP_DIR}/build/bin/Release/assimp-vc143-mt.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assimp-vc143-mt.dll")
endif()


# (권장) VS 디버거 작업 디렉터리를 실행 파일 폴더로 고정
if (MSVC)
  set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
               "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif()

