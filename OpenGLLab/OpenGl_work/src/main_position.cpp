#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>
#include <iostream>
#include <cmath>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "shader_m.h"	// 셰이더 유틸리티 클래스


#pragma region 전역 함수
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
#pragma endregion

#pragma region 전역변수

static bool gUsePerspective = true;
static bool gPrevP = false;


//카메라 설정
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);   // 시작 위치
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // 정면 방향 (Z-방향)
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);  // 위쪽 벡터


//프레임
float deltaTime = 0.0f; // 현재 프레임과 마지막 프레임 사이의 시간
float lastFrame = 0.0f; // 마지막 프레임 시간


//카메라 회전
float yaw = -90.0f; // Y축 회전 (기본 -Z방향)
float pitch = 0.0f;  // X축 회전
float lastX = 960.0f / 2.0f; // 창 크기 절반
float lastY = 600.0f / 2.0f;
bool firstMouse = true;


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
#pragma endregion

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // 위로 갈수록 Y값 감소하므로 반대
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.02f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // pitch 범위 제한
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // yaw, pitch로 Front 벡터 갱신
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}


// 키 처리 함수 분리
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // --- 투영 모드 토글 (P키) ---
    bool pNow = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pNow && !gPrevP) gUsePerspective = !gUsePerspective;
    gPrevP = pNow;

    // --- 카메라 이동 처리 ---
    float cameraSpeed = 2.5f * deltaTime; // 프레임 보정 속도

    // 전/후 이동 (W/S)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;

    // 좌/우 이동 (A/D)
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp)); // 오른쪽 벡터
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= cameraRight * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += cameraRight * cameraSpeed;
}



static void CheckShaderCompile(GLuint shader, const char* name)
{
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLint len = 0; glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetShaderInfoLog(shader, len, &len, log.data());
		fprintf(stderr, "[Shader Compile Error] %s\n%s\n", name, log.c_str());
	}
}

static void CheckProgramLink(GLuint prog)
{
	GLint success = 0; glGetProgramiv(prog, GL_LINK_STATUS, &success);
	if (!success) {
		GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
		std::string log(len, '\0');
		glGetProgramInfoLog(prog, len, &len, log.data());
		fprintf(stderr, "[Program Link Error]\n%s\n", log.c_str());
	}
}

GLuint CreateShaderProgram(const char* vs, const char* fs)
{
	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vs, nullptr);
	glCompileShader(v);
	CheckShaderCompile(v, "Vertex");

	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &fs, nullptr);
	glCompileShader(f);
	CheckShaderCompile(f, "Fragment");

	GLuint p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p);
	CheckProgramLink(p);

	glDeleteShader(v);
	glDeleteShader(f);

	return p;
}

////////////////////////////////////////////////////////////////////////
static std::string ReadFile(const char* path) {
	std::ifstream f(path, std::ios::binary);
	if (!f) {
		fprintf(stderr, "Failed to open %s", path);
			return {};
	}
	std::ostringstream ss; ss << f.rdbuf();
	return ss.str();
}

static GLuint CreateShaderProgramFromFiles(const char* vsPath, const char* fsPath) {
	std::string vsCode = ReadFile(vsPath);
	std::string fsCode = ReadFile(fsPath);
	if (vsCode.empty() || fsCode.empty()) {
		fprintf(stderr, "Shader source empty: %s or %s", vsPath, fsPath);
			return 0;
	}
	const char* vsSrc = vsCode.c_str();
	const char* fsSrc = fsCode.c_str();

	GLuint v = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v, 1, &vsSrc, nullptr);
	glCompileShader(v); CheckShaderCompile(v, "Vertex");

	GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f, 1, &fsSrc, nullptr);
	glCompileShader(f); CheckShaderCompile(f, "Fragment");

	GLuint p = glCreateProgram();
	glAttachShader(p, v);
	glAttachShader(p, f);
	glLinkProgram(p); CheckProgramLink(p);

	glDeleteShader(v); glDeleteShader(f);
	return p;
}


////////////////////////////////////////////////////////////////////////

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

int main()
{
    // 1) 창/컨텍스트
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(960, 600, "Coordinate Systems - MVP", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    // --- 마우스 시점 설정 ---
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 커서 숨기기 + 화면 중앙 고정
    glfwSetCursorPosCallback(window, mouse_callback); // 마우스 콜백 등록


    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n"; return -1;
    }

    // Shader는 컨텍스트/GLAD 이후에 생성해야 함
    Shader ourShader("shaders/coordinate_systems.vs", "shaders/coordinate_systems.fs");
    glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화 (중요)


    // --- 프레임 시간 계산 ---
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;


    // 3) 큐브 정점(위치 xyz + 색 rgb) 36개 (각 면 2삼각형*6면)
    float vertices[] = {
        // 뒤(-Z)
        -0.5f,-0.5f,-0.5f,  0.0f,0.0f,
         0.5f,-0.5f,-0.5f,  1.0f,0.0f,
         0.5f, 0.5f,-0.5f,  1.0f,1.0f,
         0.5f, 0.5f,-0.5f,  1.0f,1.0f,
        -0.5f, 0.5f,-0.5f,  0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,0.0f,

        // 앞(+Z)
        -0.5f,-0.5f, 0.5f,  0.0f,0.0f,
         0.5f,-0.5f, 0.5f,  1.0f,0.0f,
         0.5f, 0.5f, 0.5f,  1.0f,1.0f,
         0.5f, 0.5f, 0.5f,  1.0f,1.0f,
        -0.5f, 0.5f, 0.5f,  0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,  0.0f,0.0f,

        // 좌(-X)
        -0.5f, 0.5f, 0.5f,  1.0f,0.0f,
        -0.5f, 0.5f,-0.5f,  1.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,1.0f,
        -0.5f,-0.5f,-0.5f,  0.0f,1.0f,
        -0.5f,-0.5f, 0.5f,  0.0f,0.0f,
        -0.5f, 0.5f, 0.5f,  1.0f,0.0f,

        // 우(+X)
         0.5f, 0.5f, 0.5f,  1.0f,0.0f,
         0.5f, 0.5f,-0.5f,  1.0f,1.0f,
         0.5f,-0.5f,-0.5f,  0.0f,1.0f,
         0.5f,-0.5f,-0.5f,  0.0f,1.0f,
         0.5f,-0.5f, 0.5f,  0.0f,0.0f,
         0.5f, 0.5f, 0.5f,  1.0f,0.0f,

         // 하(-Y)
         -0.5f,-0.5f,-0.5f,  0.0f,1.0f,
          0.5f,-0.5f,-0.5f,  1.0f,1.0f,
          0.5f,-0.5f, 0.5f,  1.0f,0.0f,
          0.5f,-0.5f, 0.5f,  1.0f,0.0f,
         -0.5f,-0.5f, 0.5f,  0.0f,0.0f,
         -0.5f,-0.5f,-0.5f,  0.0f,1.0f,

         // 상(+Y)
         -0.5f, 0.5f,-0.5f,  0.0f,1.0f,
          0.5f, 0.5f,-0.5f,  1.0f,1.0f,
          0.5f, 0.5f, 0.5f,  1.0f,0.0f,
          0.5f, 0.5f, 0.5f,  1.0f,0.0f,
         -0.5f, 0.5f, 0.5f,  0.0f,0.0f,
         -0.5f, 0.5f,-0.5f,  0.0f,1.0f
    };


    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // layout(location=0) position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // layout(location=1) texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint texture;
    glGenTextures(1, &texture); // 텍스처 ID 생성
    // texture1: container.jpg (RGB)
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, nc;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("assets/textures/container.jpg", &w, &h, &nc, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    GLuint texture2;
    glGenTextures(1, &texture2); // 텍스처 ID 생성
    // texture2: awesomeface.png (RGBA)
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("assets/textures/awesomeface.png", &w, &h, &nc, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    // 샘플러 유닛 연결(한 번만)
    ourShader.use();
    ourShader.setInt("texture1", 0);
    ourShader.setInt("texture2", 1);


    // 카메라/투영 기본값
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.07f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Model: 큐브를 회전시켜 변환 흐름을 관찰 ---
        float t = (float)glfwGetTime();
        glm::mat4 model(1.0f);
        

        // --- View: 현재 카메라 위치에서 바라보기 ---
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        // --- Projection: P키로 원근/직교 전환 ---
        float aspect = (height == 0) ? 1.0f : (float)width / (float)height;
        glm::mat4 proj;
        if (gUsePerspective) {
            proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        }
        else {
            float s = 1.5f;
            proj = glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 100.0f);
        }



        // ----- 렌더링 루프 내부 -----
        ourShader.use();

        // MVP 행렬 전달
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", proj);

        // 텍스처 활성화
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // 창 제목에 모드 표시
        std::string title = std::string("Coordinate Systems - ") + (gUsePerspective ? "Perspective(P to toggle)" : "Orthographic(P to toggle)");
        glfwSetWindowTitle(window, title.c_str());
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


