#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>
#include <iostream>
#include <cmath>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "shader_m.h"	// 셰이더 유틸리티 클래스
#include "camera.h"    // 카메라 클래스

// ImGui 헤더
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// 콜백 함수 선언
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

// ====== 새로 추가할 헬퍼 함수들 선언 ======
GLFWwindow* initGLFWAndCreateWindow(int width, int height, const char* title);
bool initGLAD();
void setupCallbacks(GLFWwindow* window);
void initImGui(GLFWwindow* window);
void shutdownImGui();

void setupCubeData(unsigned int& VBO, unsigned int& cubeVAO, unsigned int& lightVAO);
unsigned int loadTexture2D(const char* path);

void buildImGuiUI();
void drawScene(Shader& lightingShader,
	Shader& lightCubeShader,
	unsigned int cubeVAO,
	unsigned int lightVAO,
	unsigned int diffuseMap,
	unsigned int specularMap);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// UI 모드 토글
bool g_UiMode = false;

// lighting parameters (ImGui로 조절)
glm::vec3 gLightPos(1.2f, 1.0f, 2.0f);
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

float gAmbientStrength = 0.1f;
float gDiffuseStrength = 1.0f;
float gSpecularStrength = 0.5f;

// 월드 공간에서 큐브 위치들
const glm::vec3 gCubePositions[] = {
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
const unsigned int gCubeCount = sizeof(gCubePositions) / sizeof(glm::vec3);

// ==========================================
// main
// ==========================================
int main() {

	// 1. GLFW + Window 생성
	GLFWwindow* window = initGLFWAndCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LightingMaps");
	if (!window) return -1;

	// 2. GLAD 초기화
	if (!initGLAD()) {
		glfwTerminate();
		return -1;
	}

	// 3. 콜백 등록 및 마우스 모드 설정
	setupCallbacks(window);

	// 4. ImGui 초기화
	initImGui(window);

	// 5. 쉐이더 생성
	Shader lightingShader("shaders/basic_lighting_tex.vs", "shaders/basic_lighting_tex.fs");
	Shader lightCubeShader("shaders/light_cube.vs", "shaders/light_cube.fs");

	// 6. 깊이버퍼 사용
	glEnable(GL_DEPTH_TEST);

	// 7. 정점 데이터, VAO/VBO, 광원용 VAO 설정
	unsigned int VBO = 0, cubeVAO = 0, lightVAO = 0;
	setupCubeData(VBO, cubeVAO, lightVAO);

	// 8. 텍스처 로드 (diffuse, specular)
	unsigned int diffuseMap = loadTexture2D("assets/Light.png");
	unsigned int specularMap = loadTexture2D("assets/Light.png");

	if (diffuseMap == 0 || specularMap == 0) {
		std::cerr << "Texture load failed. Check assets paths.\n";
	}

	// 재질 텍스처 유닛 연결
	lightingShader.use();
	lightingShader.setInt("material.diffuse", 0);
	lightingShader.setInt("material.specular", 1);

	// 렌더 루프
	while (!glfwWindowShouldClose(window)) {

		// per-frame time logic
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// 입력 처리
		processInput(window);

		// ImGui 새 프레임
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui UI 코드 (창 구성)
		buildImGuiUI();

		// 카메라 내부 벡터 재계산 트릭
		camera.ProcessMouseMovement(0.0f, 0.0f, true);

		// 화면/깊이버퍼 클리어
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 3D 씬(큐브 + 광원) 렌더링
		drawScene(lightingShader, lightCubeShader,
			cubeVAO, lightVAO,
			diffuseMap, specularMap);

		// ImGui 렌더링
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// 정리 작업
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	shutdownImGui();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

// ==========================================
// GLFW / GLAD / 콜백 / ImGui 초기화 함수들
// ==========================================

// GLFW 초기화 + Window 생성
GLFWwindow* initGLFWAndCreateWindow(int width, int height, const char* title)
{
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return nullptr;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	glfwMakeContextCurrent(window);
	return window;
}

// GLAD 초기화
bool initGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}
	return true;
}

// 콜백 등록 및 초기 입력 모드 설정
void setupCallbacks(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	// 초기엔 FPS 모드로 시작 (커서 숨김)
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

// ImGui 초기화
void initImGui(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// 우리가 콜백을 직접 전달할 것이므로 false
	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init("#version 330");
}

// ImGui 종료
void shutdownImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

// ==========================================
// 데이터 설정: 정점/VAO/VBO, 광원 VAO
// ==========================================

void setupCubeData(unsigned int& VBO, unsigned int& cubeVAO, unsigned int& lightVAO)
{
	// position + normal + texcoord (8 floats)
	float vertices[] = {
		// positions          // normals           // texcoords
		// 뒤쪽 면 (z = -0.5, normal = (0,0,-1))
		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,

		// 앞쪽 면 (z = +0.5, normal = (0,0,1))
		-0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

		// 왼쪽 면 (x = -0.5, normal = (-1,0,0))
		-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

		// 오른쪽 면 (x = +0.5, normal = (1,0,0))
		 0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

		 // 아래쪽 면 (y = -0.5, normal = (0,-1,0))
		 -0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,
		  0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,
		  0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
		  0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
		 -0.5f, -0.5f,  0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,
		 -0.5f, -0.5f, -0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,

		 // 위쪽 면 (y = +0.5, normal = (0,1,0))
		 -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
		  0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		  0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
		  0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
		 -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
		 -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
	};

	// 큐브용 VAO/VBO
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// position attribute (location = 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// normal attribute (location = 1)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// texcoord attribute (location = 2)
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// 광원용 VAO (position만 사용)
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// VAO 해제
	glBindVertexArray(0);
}

// ==========================================
// 텍스처 설정 함수
// ==========================================

unsigned int loadTexture2D(const char* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// wrapping / filtering 옵션
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // 이미지 상하 반전
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, format,
			width, height, 0, format,
			GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture: " << path << std::endl;
		textureID = 0; // 실패 표시
	}
	stbi_image_free(data);

	return textureID;
}

// ==========================================
// ImGui UI 구성 함수 (카메라 / 조명 설정창)
// ==========================================

void buildImGuiUI()
{
	if (!g_UiMode)
		return;

	ImGui::Begin("Camera Control");

	// 카메라 위치
	ImGui::DragFloat3("Position", glm::value_ptr(camera.Position), 0.01f);
	// Yaw / Pitch
	ImGui::DragFloat("Yaw", &camera.Yaw, 0.5f);
	ImGui::DragFloat("Pitch", &camera.Pitch, 0.5f, -89.0f, 89.0f);

	// FOV
	ImGui::DragFloat("Zoom (FOV)", &camera.Zoom, 0.1f, 1.0f, 90.0f);

	// 카메라 리셋
	if (ImGui::Button("Reset Camera")) {
		camera = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
	}

	ImGui::Separator();
	ImGui::Text("Lighting");

	// 광원 위치
	ImGui::DragFloat3("Light Pos", glm::value_ptr(gLightPos), 0.01f);

	// 광원 색상
	ImGui::ColorEdit3("Light Color", glm::value_ptr(gLightColor));

	// 각 계수
	ImGui::SliderFloat("Ambient", &gAmbientStrength, 0.0f, 1.0f);
	ImGui::SliderFloat("Diffuse", &gDiffuseStrength, 0.0f, 2.0f);
	ImGui::SliderFloat("Specular", &gSpecularStrength, 0.0f, 2.0f);

	if (ImGui::Button("Reset Light")) {
		gLightPos = glm::vec3(1.2f, 1.0f, 2.0f);
		gLightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		gAmbientStrength = 0.1f;
		gDiffuseStrength = 1.0f;
		gSpecularStrength = 0.5f;
	}

	ImGui::End();
}

// ==========================================
// 씬 렌더링 함수 (큐브 + 광원)
// ==========================================

void drawScene(Shader& lightingShader,
	Shader& lightCubeShader,
	unsigned int cubeVAO,
	unsigned int lightVAO,
	unsigned int diffuseMap,
	unsigned int specularMap)
{
	// 텍스처 바인딩
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularMap);

	// 카메라 변환 행렬
	glm::mat4 projection = glm::perspective(
		glm::radians(camera.Zoom),
		(float)SCR_WIDTH / (float)SCR_HEIGHT,
		0.1f, 100.0f
	);
	glm::mat4 view = camera.GetViewMatrix();

	// ===== 조명 쉐이더 설정 =====
	lightingShader.use();
	lightingShader.setMat4("projection", projection);
	lightingShader.setMat4("view", view);

	// 조명/재질 유니폼 설정
	lightingShader.setVec3("light.position", gLightPos);
	lightingShader.setVec3("viewPos", camera.Position);

	glm::vec3 diffuseColor = gLightColor * gDiffuseStrength;
	glm::vec3 ambientColor = diffuseColor * gAmbientStrength;
	glm::vec3 specularColor = gLightColor * gSpecularStrength;

	lightingShader.setVec3("light.ambient", ambientColor);
	lightingShader.setVec3("light.diffuse", diffuseColor);
	lightingShader.setVec3("light.specular", specularColor);

	lightingShader.setFloat("material.shininess", 32.0f);

	// 큐브 그리기
	glBindVertexArray(cubeVAO);

	for (unsigned int i = 0; i < gCubeCount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, gCubePositions[i]);
		float angle = 20.0f * i;
		model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
		lightingShader.setMat4("model", model);

		glDrawArrays(GL_TRIANGLES, 0, 36);
	}

	// ===== 광원 큐브 렌더링 =====
	lightCubeShader.use();
	lightCubeShader.setMat4("projection", projection);
	lightCubeShader.setMat4("view", view);

	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, gLightPos);
	lightModel = glm::scale(lightModel, glm::vec3(0.2f));
	lightCubeShader.setMat4("model", lightModel);

	glBindVertexArray(lightVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

// ==========================================
// 기존 콜백/입력 함수들 (내용은 그대로 유지)
// ==========================================

void processInput(GLFWwindow* window)
{
	// UI 모드이면 카메라 이동(WASD) 및 종료(ESC) 키 입력을 막음
	if (g_UiMode)
		return;
	// ImGui가 키보드를 사용 중이면 막음
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

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

// 마우스 이동 콜백
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	// UI 모드이면 회전 멈춤
	if (g_UiMode)
	{
		firstMouse = true; // UI 모드에서 나올 때 튐 방지
		return;
	}

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // y는 아래에서 위로 증가하므로 반대

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// 스크롤 콜백
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset); // ImGui에 전달

	// ImGui가 마우스를 캡처 중이면 카메라 줌 막음
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// 키보드 콜백 (UI 모드 토글 포함)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods); // ImGui에 전달

	if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
	{
		g_UiMode = !g_UiMode;

		if (g_UiMode) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	}
}

// 마우스 버튼 콜백
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods); // ImGui에 전달
}
