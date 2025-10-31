#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include "stb_image.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#pragma region 전역변수

static bool gUsePerspective = true;
static bool gPrevP = false;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);   // 시작 위치
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);  // 정면 방향 (Z-방향)
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);  // 위쪽 벡터

float deltaTime = 0.0f; // 현재 프레임과 마지막 프레임 사이의 시간
float lastFrame = 0.0f; // 마지막 프레임 시간

float yaw = -90.0f; // Y축 회전 (기본 -Z방향)
float pitch = 0.0f;  // X축 회전
float lastX = 960.0f / 2.0f; // 창 크기 절반
float lastY = 600.0f / 2.0f;
bool firstMouse = true;
#pragma endregion

 //전역 함수
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
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
    glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화 (중요)


    // --- 프레임 시간 계산 ---
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // 2) 셰이더
    GLuint prog = CreateShaderProgramFromFiles("shaders/transform.vert", "shaders/transform.frag");

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

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // 색(1)
    // 텍스처 파라미터 & 업로드
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);
    int w, h, nc; unsigned char* data = stbi_load("assets/container.jpg", &w, &h, &nc, 0);
    if (data) {
        GLenum fmt = (nc == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    // Uniform 위치
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    GLint locModel = glGetUniformLocation(prog, "uModel");
    GLint locView = glGetUniformLocation(prog, "uView");
    GLint locProj = glGetUniformLocation(prog, "uProj");

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
        //model = glm::rotate(model, t * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Y축 회전
        //model = glm::rotate(model, t * glm::radians(17.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // X축 회전


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

        // 업로드(열우선: 전치 필요 없음 -> GL_FALSE)
        glUseProgram(prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);

        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // 창 제목에 모드 표시
        std::string title = std::string("Coordinate Systems - ") + (gUsePerspective ? "Perspective(P to toggle)" : "Orthographic(P to toggle)");
        glfwSetWindowTitle(window, title.c_str());
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


