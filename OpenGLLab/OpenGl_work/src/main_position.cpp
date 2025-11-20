#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <filesystem>
#include "stb_image.h"
#include "shader_m.h"
#include "camera.h"

// ImGui 헤더
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

using namespace std;

//전역 변수
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 마우스 입력 관련 변수
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
bool firstMouse = true;


glm::vec3 gLightPos(1.2f, 1.0f, 2.0f);       // 광원 위치
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);     // 기본: 흰색 빛

float gAmbientStrength = 0.1f;  // 주변광 계수
float gDiffuseStrength = 1.0f;  // 난반사 계수
float gSpecularStrength = 0.5f;  // 정반사 계수

// UI 모드 토글을 위한 전역 변수
bool g_UiMode = false;

// 함수 선언
void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

int main()
{
    // OpenGL 및 창 초기화
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(960, 600, "Camera Control", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    // 콜백 등록
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 마우스 이동/스크롤 콜백 등록
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // ImGui를 위해 키보드/마우스 '버튼' 콜백도 등록
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // 초기엔 FPS 모드로 시작 (커서 숨김)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSwapInterval(1); // V-sync 켜기

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n"; return -1;
    }

    glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화

    // ===================================================
    // ImGui 초기화
    // ===================================================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // ImGui를 GLFW와 OpenGL 3.3 버전에 맞게 설정
    // 'false'로 설정 -> 우리가 콜백을 수동으로 등록(key_callback 등)해서 전달하겠다는 의미
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330"); // GLSL 버전에 맞게

    // 셰이더 (사용자 제공 vertex/fragment에 맞춰 이름은 basic 로 둠)
    Shader ourShader("shaders/basic.vert", "shaders/basic.frag");

    // ===================================================
    // 정점 데이터 (position(3) + normal(3) + texcoords(2)) — 총 stride = 8 floats
    // 36 vertices (6 faces * 2 triangles * 3 vertices)
    // ===================================================
    float vertices[] = {
        // positions          // normals           // texcoords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f, 0.0f,   1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f, 0.0f,   1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f, 0.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f, 0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f, 0.0f,   1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f, 0.0f,   1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f, 0.0f,   0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f, 0.0f,   0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f, 0.0f,   0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f, 0.0f,   1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,   0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,   1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,   0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,   0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f,   0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f, 0.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f, 0.0f,   1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f, 0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f, 0.0f,   0.0f, 1.0f
    };

    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 위치 (layout = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 노말 (layout = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // 텍스처 좌표 (layout = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //텍스처 로드
    unsigned int texture1;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    string path = std::filesystem::current_path().string() + "/assets/container.jpg";
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // 셰이더 기본 유니폼(한번만 설정해도 되는 값)
    ourShader.use();
    ourShader.setInt("material.diffuse", 0);
    ourShader.setFloat("material.shininess", 32.0f);

    // 렌더 루프
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input (processInput은 g_UiMode일 때 멈추도록 수정됨)
        processInput(window);

        // ===================================================
        // ImGui 새 프레임 시작 (순서 중요!)
        // ===================================================
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ===================================================
        // ImGui UI 코드
        // ===================================================
        if (g_UiMode) { // UI 모드일 때만 창을 그림
            ImGui::Begin("Camera & Light Control");

            // camera.Position (vec3) 조작
            ImGui::DragFloat3("Position", glm::value_ptr(camera.Position), 0.01f);

            // camera.Yaw/Pitch (float) 조작
            ImGui::DragFloat("Yaw", &camera.Yaw, 0.5f);
            ImGui::DragFloat("Pitch", &camera.Pitch, 0.5f, -89.0f, 89.0f); // min/max로 제한

            // camera.Zoom (FOV) 조작
            ImGui::DragFloat("Zoom (FOV)", &camera.Zoom, 0.1f, 1.0f, 90.0f);

            // Light controls
            ImGui::Separator();
            ImGui::Text("Light Controls");
            ImGui::DragFloat3("Light Pos", glm::value_ptr(gLightPos), 0.1f);
            ImGui::ColorEdit3("Light Color", glm::value_ptr(gLightColor));
            ImGui::DragFloat("Ambient", &gAmbientStrength, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Diffuse", &gDiffuseStrength, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Specular", &gSpecularStrength, 0.01f, 0.0f, 1.0f);

            // 리셋 버튼
            if (ImGui::Button("Reset Camera")) {
                // Camera 객체를 기본값으로 새로 생성해서 덮어씀
                camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
            }

            ImGui::End();
        }

        // 중요!! Yaw/Pitch 값이 ImGui로 변경되었을 경우 내부 벡터 재계산
        camera.ProcessMouseMovement(0.0f, 0.0f, true);

        // (기존 3D 렌더링 코드)
        glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 텍스처 바인딩
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);

        ourShader.use();

        // ------------------------------
        // Light / Material uniform 전송
        // ------------------------------
        ourShader.setVec3("light.position", gLightPos);
        ourShader.setVec3("light.ambient", gLightColor * gAmbientStrength);
        ourShader.setVec3("light.diffuse", gLightColor * gDiffuseStrength);
        ourShader.setVec3("light.specular", gLightColor * gSpecularStrength);

        ourShader.setVec3("viewPos", camera.Position);

        // Projection / View
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            100.0f
        );
        ourShader.setMat4("projection", projection);

        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        // render boxes
        glBindVertexArray(vao);
        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            ourShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ===================================================
        // ImGui 렌더링 (3D 씬 그린 직후, 스왑 버퍼 직전)
        // ===================================================
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 정리
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture1);

    // ===================================================
    // ImGui 종료
    // ===================================================
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // UI 모드일 때는 카메라 이동 막기
    if (g_UiMode)
        return;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    // UI 모드일 때는 카메라 회전 막기
    if (g_UiMode)
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // UI 모드일 때는 ImGui가 먼저 처리하도록
    if (g_UiMode)
        return;

    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // ImGui에 키 이벤트 전달
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    // Tab 키로 UI 모드 토글
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        g_UiMode = !g_UiMode;

        if (g_UiMode)
        {
            // UI 모드: 커서 보이기
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            firstMouse = true; // 다시 FPS 모드로 돌아갈 때 점프 방지
        }
        else
        {
            // FPS 모드: 커서 숨기기
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // ImGui에 마우스 버튼 이벤트 전달
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}