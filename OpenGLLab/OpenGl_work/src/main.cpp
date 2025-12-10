#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "shader_m.h"	// 셰이더 유틸리티 클래스
#include "camera.h"    // 카메라 클래스

// ImGui 헤더
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


#include "model.h"

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
void setupShadowMap(unsigned int& depthMapFBO, unsigned int& depthMap);
void setupCubeData(unsigned int& VBO, unsigned int& cubeVAO, unsigned int& lightVAO);
unsigned int loadTexture2D(const char* path);
unsigned int loadCubemap(const std::vector<std::string>& faces);
void setupSkyboxData(unsigned int& skyboxVAO, unsigned int& skyboxVBO);
glm::mat4 computeLightSpaceMatrix();
void renderSceneGeometry(Shader& shader, unsigned int cubeVAO, Model* model);

void buildImGuiUI();
void drawScene(Shader& lightingShader,
    Shader& lightCubeShader,
    unsigned int cubeVAO,
    unsigned int lightVAO,
    unsigned int diffuseMap,
    unsigned int specularMap,
    Model* model,
    unsigned int shadowMap,
    const glm::mat4& lightSpaceMatrix);

// settings
const unsigned int SCR_WIDTH  = 800;
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

float gAmbientStrength  = 0.1f;
float gDiffuseStrength  = 1.0f;
float gSpecularStrength = 0.5f;



// shadow mapping용 해상도 및 FBO/텍스처 전역 변수 선언
const unsigned int SHADOW_WIDTH  = 1024;
const unsigned int SHADOW_HEIGHT = 1024;
unsigned int gDepthMapFBO = 0;
unsigned int gDepthMap    = 0;

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
	Shader depthShader("shaders/shadow_depth.vs", "shaders/shadow_depth.fs");
	Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.fs");

	// 6. 깊이버퍼 사용
	glEnable(GL_DEPTH_TEST);

	// 7. 정점 데이터, VAO/VBO, 광원용 VAO 설정
	unsigned int VBO = 0, cubeVAO = 0, lightVAO = 0;
	setupCubeData(VBO, cubeVAO, lightVAO);

	// Skybox VAO/VBO 및 큐브맵 텍스처
	unsigned int skyboxVAO = 0, skyboxVBO = 0;
	setupSkyboxData(skyboxVAO, skyboxVBO);
	std::vector<std::string> skyboxFaces = {
		"assets/skybox/right.hdr",
		"assets/skybox/left.hdr",
		"assets/skybox/top.hdr",
		"assets/skybox/bottom.hdr",
		"assets/skybox/front.hdr",
		"assets/skybox/back.hdr"
	};
	unsigned int cubemapTexture = loadCubemap(skyboxFaces);
	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);

	// 8. 텍스처 로드 (diffuse, specular)
	unsigned int diffuseMap  = loadTexture2D("assets/container2.png");
	unsigned int specularMap = loadTexture2D("assets/container2_specular.png");

	if (diffuseMap == 0 || specularMap == 0) {
		std::cerr << "Texture load failed. Check assets paths.\n";
	}

	// 재질 텍스처 유닛 연결
	lightingShader.use();
	lightingShader.setInt("material.diffuse", 0);
	lightingShader.setInt("material.specular", 1);



	// === 여기서부터 모델 로딩 추가 코드임 ===
    // 프로젝트 기준 경로에 맞게 수정 가능함
    Model nanosuit("assets/models/Eemy/Model_04.fbx");

	// 9. Shadow map FBO 및 텍스처 설정
	setupShadowMap(gDepthMapFBO, gDepthMap);

	// 렌더 루프
while (!glfwWindowShouldClose(window)) {

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    buildImGuiUI();

    // 카메라 내부 벡터 재계산 트릭
    camera.ProcessMouseMovement(0.0f, 0.0f, true);

    // 0. 빛 시점 행렬 계산
    glm::mat4 lightSpaceMatrix = computeLightSpaceMatrix();

    // 1패스: shadow map용 깊이 렌더링 패스 수행함
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, gDepthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // 큐브 + 모델 기하만 그리는 공용 함수 호출함
    renderSceneGeometry(depthShader, cubeVAO, &nanosuit);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ===== 2패스: 실제 화면 렌더링 패스 수행함 =====
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawScene(lightingShader, lightCubeShader,
        cubeVAO, lightVAO,
        diffuseMap, specularMap,
        &nanosuit,
        gDepthMap,           // shadow map 텍스처
        lightSpaceMatrix);   // 빛 시점 행렬

	// Skybox 렌더링 (항상 마지막, 깊이 함수 변경)
	glDepthFunc(GL_LEQUAL);
	skyboxShader.use();
	glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // 위치 이동 제거
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	skyboxShader.setMat4("view", view);
	skyboxShader.setMat4("projection", projection);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);

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
// Skybox 설정 함수
// ==========================================

void setupSkyboxData(unsigned int& skyboxVAO, unsigned int& skyboxVBO)
{
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);
}

unsigned int loadCubemap(const std::vector<std::string>& faces)
{
	auto isHDR = [](const std::string& path) {
		auto lower = path;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return lower.rfind(".hdr") != std::string::npos || lower.rfind(".exr") != std::string::npos;
	};

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width = 0, height = 0, nrChannels = 0;
	stbi_set_flip_vertically_on_load(false); // 큐브맵은 뒤집지 않음

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		if (isHDR(faces[i]))
		{
			float* data = stbi_loadf(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
				GLenum internalFormat = (nrChannels == 4) ? GL_RGBA16F : GL_RGB16F;
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, GL_FLOAT, data);
				stbi_image_free(data);
			}
			else
			{
				std::cout << "Failed to load HDR cubemap texture: " << faces[i] << std::endl;
			}
		}
		else
		{
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
			}
			else
			{
				std::cout << "Failed to load cubemap texture: " << faces[i] << std::endl;
			}
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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
	ImGui::SliderFloat("Ambient",  &gAmbientStrength,  0.0f, 1.0f);
	ImGui::SliderFloat("Diffuse",  &gDiffuseStrength,  0.0f, 2.0f);
	ImGui::SliderFloat("Specular", &gSpecularStrength, 0.0f, 2.0f);

	if (ImGui::Button("Reset Light")) {
		gLightPos         = glm::vec3(1.2f, 1.0f, 2.0f);
		gLightColor       = glm::vec3(1.0f, 1.0f, 1.0f);
		gAmbientStrength  = 0.1f;
		gDiffuseStrength  = 1.0f;
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
	unsigned int specularMap,
	Model* model,
	unsigned int shadowMap,
	const glm::mat4& lightSpaceMatrix)
{
	// 텍스처 바인딩
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, diffuseMap);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, specularMap);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	// 카메라 행렬
	glm::mat4 projection = glm::perspective(
		glm::radians(camera.Zoom),
		(float)SCR_WIDTH / (float)SCR_HEIGHT,
		0.1f, 100.0f
	);
	glm::mat4 view = camera.GetViewMatrix();

	// 조명 쉐이더 설정
	lightingShader.use();
	lightingShader.setMat4("projection", projection);
	lightingShader.setMat4("view", view);
	lightingShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

	lightingShader.setVec3("light.position", gLightPos);
	lightingShader.setVec3("viewPos", camera.Position);

	glm::vec3 diffuseColor  = gLightColor * gDiffuseStrength;
	glm::vec3 ambientColor  = diffuseColor * gAmbientStrength;
	glm::vec3 specularColor = gLightColor * gSpecularStrength;

	lightingShader.setVec3("light.ambient", ambientColor);
	lightingShader.setVec3("light.diffuse", diffuseColor);
	lightingShader.setVec3("light.specular", specularColor);

	lightingShader.setFloat("material.shininess", 32.0f);

	// 텍스처 유닛 연결
	lightingShader.setInt("material.diffuse", 0);
	lightingShader.setInt("material.specular", 1);
	lightingShader.setInt("shadowMap", 2);

	// 큐브 + 모델 실제 기하 렌더링
	renderSceneGeometry(lightingShader, cubeVAO, model);

	// 광원 큐브 렌더링
	lightCubeShader.use();
	lightCubeShader.setMat4("projection", projection);
	lightCubeShader.setMat4("view", view);

	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, gLightPos);
	lightModel = glm::scale(lightModel, glm::vec3(0.2f));
	lightCubeShader.setMat4("model", lightModel);

	glBindVertexArray(lightVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
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


//그림자 전용 함수

void setupShadowMap(unsigned int& depthMapFBO, unsigned int& depthMap)
{
    // FBO 생성
    glGenFramebuffers(1, &depthMapFBO);

    // 깊이 텍스처 생성
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    // 필터 / 래핑 설정
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // FBO에 깊이 텍스처 부착
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        depthMap,
        0
    );

    // 컬러 버퍼 비활성화
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0); 
    //사용자 정의 FBO 사용 종료
		//다시 윈도우 화면에 그리기 시작
}

void renderSceneGeometry(Shader& shader, unsigned int cubeVAO, Model* model)
{
    // 큐브 렌더링
    glBindVertexArray(cubeVAO);

    for (unsigned int i = 0; i < gCubeCount; i++)
    {
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, gCubePositions[i]);
        float angle = 20.0f * i;
        modelMat = glm::rotate(
            modelMat,
            glm::radians(angle),
            glm::vec3(1.0f, 0.3f, 0.5f)
        );
        shader.setMat4("model", modelMat);

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    glBindVertexArray(0);

    // Assimp로 로딩한 3D 모델 렌더링
    if (model)
    {
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -1.75f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.002f));

        shader.setMat4("model", modelMat);
        model->Draw(shader);
    }
}

// 빛 시점 행렬(light-space matrix) 계산 함수
glm::mat4 computeLightSpaceMatrix()
{
    // 방향광처럼 사용하기 위한 직교 투영 설정임
    float near_plane = 1.0f;
    float far_plane  = 25.0f;
    glm::mat4 lightProjection = glm::ortho(
        -10.0f, 10.0f,
        -10.0f, 10.0f,
        near_plane, far_plane
    );

    // 조명 위치에서 원점을 바라보는 view 행렬 설정임
    glm::mat4 lightView = glm::lookAt(
        gLightPos,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    return lightProjection * lightView;
}
