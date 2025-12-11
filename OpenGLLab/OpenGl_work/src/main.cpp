#pragma region Includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include <stb_image.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "shader_m.h"
#include "camera.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Player.h"
#include "Background.h"
#pragma endregion

#pragma region Forward Declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

GLFWwindow* initGLFWAndCreateWindow(int width, int height, const char* title);
bool initGLAD();
void setupCallbacks(GLFWwindow* window);
void initImGui(GLFWwindow* window);
void shutdownImGui();
void setupShadowMap(unsigned int& depthMapFBO, unsigned int& depthMap);
unsigned int loadCubemap(const std::vector<std::string>& faces);
void setupSkyboxData(unsigned int& skyboxVAO, unsigned int& skyboxVBO);
unsigned int initSkybox(Shader& skyboxShader, unsigned int& skyboxVAO, unsigned int& skyboxVBO);
glm::mat4 computeLightSpaceMatrix();
void buildImGuiUI();
void drawScene(Shader& skinnedShader, unsigned int shadowMap, const glm::mat4& lightSpaceMatrix);
#pragma endregion

#pragma region Global Variables

// 화면 해상도
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// 카메라 설정
// 위치: (2.510, -0.518, -0.152), 위쪽 방향: (0, 1, 0), Yaw: -179.9도, Pitch: -8도
Camera camera(glm::vec3(2.510f, -0.518f, -0.152f), glm::vec3(0.0f, 1.0f, 0.0f), -179.9f, -8.0f);

// 마우스 입력 처리용 변수
float lastX = SCR_WIDTH / 2.0f;  // 이전 마우스 X 좌표
float lastY = SCR_HEIGHT / 2.0f; // 이전 마우스 Y 좌표
bool firstMouse = true;          // 첫 마우스 입력인지 여부

// 시간 관리
float deltaTime = 0.0f;   // 이전 프레임과의 시간 차이 (초 단위)
float lastFrame = 0.0f;   // 이전 프레임의 시간

// UI 모드 토글
// true: UI 표시 및 마우스 커서 활성화, false: UI 숨김 및 마우스 커서 비활성화
bool g_UiMode = false;

// 조명 설정
glm::vec3 gLightPos(1.2f, 1.0f, 2.0f);      // 광원 위치
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);    // 광원 색상 (흰색)
float gAmbientStrength  = 0.526f;            // 환경광 강도
float gDiffuseStrength  = 0.716f;            // 난반사 강도
float gSpecularStrength = 2.0f;              // 정반사 강도

// 그림자 맵 설정
const unsigned int SHADOW_WIDTH  = 1024;    // 그림자 맵 너비
const unsigned int SHADOW_HEIGHT = 1024;    // 그림자 맵 높이
unsigned int gDepthMapFBO = 0;              // 그림자 맵 프레임버퍼
unsigned int gDepthMap    = 0;              // 그림자 맵 텍스처

// 게임 오브젝트 포인터
Enemy* gEnemyPtr = nullptr;        // 적 캐릭터 포인터
Player* gPlayerPtr = nullptr;      // 플레이어 캐릭터 포인터
Background* gBackgroundPtr = nullptr;  // 배경 포인터
#pragma endregion

#pragma region Main
int main()
{
	// ========== 초기화 단계 ==========
	
	// 1. GLFW 초기화 및 윈도우 생성
	// OpenGL 컨텍스트를 만들기 위한 윈도우를 생성합니다.
	GLFWwindow* window = initGLFWAndCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LightingMaps");
	if (!window) return -1;
	
	// 2. GLAD 초기화
	// OpenGL 함수 포인터를 로드합니다.
	if (!initGLAD()) { glfwTerminate(); return -1; }
	
	// 3. 콜백 함수 설정
	// 키보드, 마우스, 창 크기 변경 등의 이벤트를 처리할 함수들을 등록합니다.
	setupCallbacks(window);
	
	// 4. ImGui 초기화
	// 디버깅용 UI 라이브러리를 초기화합니다.
	initImGui(window);

	// ========== 셰이더 로드 ==========
	
	// 그림자 맵 생성용 셰이더 (정적 모델용)
	Shader depthShader("shaders/shadow_depth.vs", "shaders/shadow_depth.fs");
	
	// 메인 렌더링용 셰이더 (스켈레탈 애니메이션 지원)
	Shader skinnedShader("shaders/animated_model.vs", "shaders/animated_model.fs");
	
	// 그림자 맵 생성용 셰이더 (애니메이션 모델용)
	Shader depthAnimShader("shaders/shadow_depth_anim.vs", "shaders/shadow_depth.fs");
	
	// 스카이박스 렌더링용 셰이더
	Shader skyboxShader("shaders/skybox.vs", "shaders/skybox.fs");

	// 깊이 테스트 활성화 (물체가 앞뒤로 올바르게 그려지도록)
	glEnable(GL_DEPTH_TEST);

	// ========== 스카이박스 초기화 ==========
	
	// 스카이박스 VAO, VBO 생성 및 HDR 큐브맵 텍스처 로드
	unsigned int skyboxVAO = 0, skyboxVBO = 0;
	unsigned int cubemapTexture = initSkybox(skyboxShader, skyboxVAO, skyboxVBO);

	// ========== 게임 오브젝트 초기화 ==========
	
	// 적 캐릭터 생성 및 초기화
	// 모델과 애니메이션을 로드하고 기본 상태로 설정합니다.
	Enemy enemy;
	if (!enemy.Init()) { std::cerr << "Failed to init Enemy" << std::endl; return -1; }
	gEnemyPtr = &enemy;

	// 플레이어 캐릭터 생성 및 초기화
	// 모델과 애니메이션을 로드하고 기본 상태로 설정합니다.
	Player player;
	if (!player.Init()) { std::cerr << "Failed to init Player" << std::endl; return -1; }
	gPlayerPtr = &player;

	// 플레이어와 적의 상호작용 설정
	player.SetAttackTarget(&enemy);  // 플레이어의 공격 대상 = 적
	enemy.SetPunchTarget(&player.Status());  // 적의 펀치 대상 = 플레이어
	// 플레이어의 액션이 끝나면 적이 스턴 상태가 되도록 콜백 설정
	player.SetOnActionFinished([&enemy]() { enemy.TriggerStunSequence(); });
	std::cout << "[Init] Enemy and Player initialized" << std::endl;

	// 배경 모델 로드 및 초기화
	Background background;
	if (!background.Init("assets/BackGround/sNOWlaNDSCAPE.fbx")) {
		std::cerr << "Failed to init Background" << std::endl;
		return -1;
	}
	// 배경 위치, 회전, 크기 설정
	background.SetTransform(glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(-90.0f, 0.0f, 0.0f), 10.0f);
	gBackgroundPtr = &background;

	// ========== 그림자 맵 초기화 ==========
	
	// 그림자를 만들기 위한 깊이 맵 프레임버퍼와 텍스처 생성
	setupShadowMap(gDepthMapFBO, gDepthMap);

	// ========== 메인 게임 루프 ==========
	
	while (!glfwWindowShouldClose(window))
	{
		// 델타 타임 계산 (이전 프레임과의 시간 차이)
		// 애니메이션과 물리 시뮬레이션에 사용됩니다.
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// 입력 처리 (ESC 키로 종료 등)
		processInput(window);

		// ImGui 프레임 시작
		// UI를 그리기 전에 프레임을 초기화합니다.
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		buildImGuiUI();  // HP 바, 버튼 등 UI 요소 생성

		// 게임 로직 업데이트
		// 각 캐릭터의 애니메이션 타이머와 상태를 업데이트합니다.
		enemy.UpdateActions(deltaTime);
		player.UpdateActions(deltaTime);

		// ========== Pass 1: 그림자 맵 생성 ==========
		// 광원의 시점에서 장면을 렌더링하여 깊이 정보만 저장합니다.
		
		// 광원의 변환 행렬 계산 (광원 위치에서 장면을 바라보는 행렬)
		glm::mat4 lightSpaceMatrix = computeLightSpaceMatrix();
		
		// 그림자 맵 해상도로 뷰포트 설정
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		
		// 그림자 맵 프레임버퍼에 바인딩
		glBindFramebuffer(GL_FRAMEBUFFER, gDepthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);  // 깊이 버퍼만 클리어 (색상은 필요 없음)

		// 정적 모델용 깊이 셰이더 설정
		depthShader.use();
		depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
		
		// 애니메이션 모델용 깊이 셰이더 설정
		depthAnimShader.use();
		depthAnimShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

		// 각 오브젝트를 그림자 맵에 렌더링 (깊이만 기록)
		background.RenderDepth(depthAnimShader, 30.0f);
		enemy.RenderDepth(depthAnimShader, glm::vec3(0.0f, -1.75f, 0.0f), 0.002f);
		player.RenderDepth(depthAnimShader, glm::vec3(2.0f, -1.75f, 0.0f), 0.002f);

		// 기본 프레임버퍼로 복원
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// ========== Pass 2: 메인 씬 렌더링 ==========
		// 카메라의 시점에서 조명과 그림자가 적용된 장면을 렌더링합니다.
		
		// 화면 해상도로 뷰포트 복원
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		
		// 화면 클리어 (어두운 청록색 배경)
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 메인 씬 렌더링 (배경, 플레이어, 적)
		// 조명, 그림자, 텍스처가 모두 적용됩니다.
		drawScene(skinnedShader, gDepthMap, lightSpaceMatrix);

		// ========== 스카이박스 렌더링 ==========
		// 배경 하늘을 그립니다. 항상 카메라 뒤에서 렌더링됩니다.
		
		// 깊이 테스트를 변경 (스카이박스는 항상 배경으로)
		glDepthFunc(GL_LEQUAL);
		
		skyboxShader.use();
		// 카메라의 회전만 사용 (이동은 무시)
		glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		skyboxShader.setMat4("view", view);
		skyboxShader.setMat4("projection", projection);
		
		// 스카이박스 그리기
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);  // 큐브는 36개 정점 (6면 × 2삼각형 × 3정점)
		glBindVertexArray(0);
		
		// 깊이 테스트를 원래대로 복원
		glDepthFunc(GL_LESS);

		// ImGui 렌더링
		// UI 요소들을 화면에 그립니다.
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// 더블 버퍼링: 백 버퍼와 프론트 버퍼를 교체
		// 백 버퍼에 그린 내용이 화면에 표시됩니다.
		glfwSwapBuffers(window);
		
		// 이벤트 처리 (키보드, 마우스 입력 등)
		glfwPollEvents();
	}

	// ========== 정리 ==========
	
	// 스카이박스 리소스 해제
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVBO);
	
	// ImGui 정리
	shutdownImGui();
	
	// 윈도우 및 GLFW 정리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
#pragma endregion

#pragma region Initialization

// GLFW 초기화 및 윈도우 생성
// OpenGL을 사용하기 위한 윈도우를 생성하고 컨텍스트를 설정합니다.
GLFWwindow* initGLFWAndCreateWindow(int width, int height, const char* title)
{
	// GLFW 라이브러리 초기화
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return nullptr;
	}
	
	// OpenGL 버전 설정 (3.3)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// 코어 프로파일 사용 (레거시 함수 제거)
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// 윈도우 생성
	GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return nullptr;
	}
	
	// 이 윈도우를 현재 OpenGL 컨텍스트로 설정
	glfwMakeContextCurrent(window);
	return window;
}

// GLAD 초기화
// OpenGL 함수 포인터를 로드합니다.
// OpenGL 함수들은 런타임에 동적으로 로드되어야 합니다.
bool initGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return false;
	}
	return true;
}

// 콜백 함수 설정
// 윈도우 이벤트(키보드, 마우스, 창 크기 변경 등)를 처리할 함수들을 등록합니다.
void setupCallbacks(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  // 창 크기 변경
	glfwSetCursorPosCallback(window, mouse_callback);                   // 마우스 이동
	glfwSetScrollCallback(window, scroll_callback);                     // 마우스 휠
	glfwSetKeyCallback(window, key_callback);                           // 키보드 입력
	glfwSetMouseButtonCallback(window, mouse_button_callback);          // 마우스 버튼
	// 마우스 커서를 숨김 (FPS 스타일 카메라 제어)
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

// ImGui 초기화
// 디버깅용 UI 라이브러리를 초기화합니다.
void initImGui(GLFWwindow* window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// GLFW와 OpenGL 3.3용 ImGui 백엔드 초기화
	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init("#version 330");
}

// ImGui 정리
// 프로그램 종료 시 ImGui 리소스를 해제합니다.
void shutdownImGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
#pragma endregion

#pragma region Skybox

// 스카이박스 정점 데이터 설정
// 큐브의 6면을 구성하는 정점들을 GPU 버퍼에 업로드합니다.
void setupSkyboxData(unsigned int& skyboxVAO, unsigned int& skyboxVBO)
{
	// 큐브의 정점 데이터 (6면, 각 면 2개 삼각형, 총 36개 정점)
	// 정점은 -1 ~ 1 범위의 정규화된 좌표입니다.
	float skyboxVertices[] = {
		-1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
	};

	// VAO, VBO 생성
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	
	// VAO 바인딩 (이후 설정은 이 VAO에 저장됨)
	glBindVertexArray(skyboxVAO);
	
	// VBO에 정점 데이터 업로드
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	
	// 정점 속성 설정 (위치만 사용, layout = 0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	
	// 바인딩 해제
	glBindVertexArray(0);
}

// 큐브맵 텍스처 로드
// 6개의 이미지 파일을 읽어서 큐브맵 텍스처를 생성합니다.
// HDR 이미지도 지원합니다.
unsigned int loadCubemap(const std::vector<std::string>& faces)
{
	// HDR 파일인지 확인하는 람다 함수
	auto isHDR = [](const std::string& path) {
		auto lower = path;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return lower.rfind(".hdr") != std::string::npos || lower.rfind(".exr") != std::string::npos;
	};

	// 큐브맵 텍스처 생성
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width = 0, height = 0, nrChannels = 0;
	// 스카이박스는 Y축을 뒤집지 않음
	stbi_set_flip_vertically_on_load(false);

	// 6면의 이미지를 각각 로드
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		if (isHDR(faces[i])) {
			// HDR 이미지 로드 (32비트 부동소수점)
			float* data = stbi_loadf(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data) {
				GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
				GLenum internalFormat = (nrChannels == 4) ? GL_RGBA16F : GL_RGB16F;  // HDR용 16비트 부동소수점
				// 큐브맵의 각 면에 이미지 데이터 설정
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, GL_FLOAT, data);
				stbi_image_free(data);
			} else {
				std::cout << "Failed to load HDR cubemap: " << faces[i] << std::endl;
			}
		} else {
			// 일반 이미지 로드 (8비트 정수)
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data) {
				GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
				// 큐브맵의 각 면에 이미지 데이터 설정
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
			} else {
				std::cout << "Failed to load cubemap: " << faces[i] << std::endl;
			}
		}
	}

	// 텍스처 파라미터 설정
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // 축소 필터
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // 확대 필터
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // 경계 처리
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

// 스카이박스 초기화
// 정점 데이터와 큐브맵 텍스처를 설정하고 셰이더에 텍스처 유닛을 연결합니다.
unsigned int initSkybox(Shader& skyboxShader, unsigned int& skyboxVAO, unsigned int& skyboxVBO)
{
	// 스카이박스 정점 데이터 설정
	setupSkyboxData(skyboxVAO, skyboxVBO);

	// 6면의 이미지 파일 경로 (순서: 오른쪽, 왼쪽, 위, 아래, 앞, 뒤)
	std::vector<std::string> skyboxFaces = {
		"assets/skybox/right.hdr", "assets/skybox/left.hdr",
		"assets/skybox/top.hdr",   "assets/skybox/bottom.hdr",
		"assets/skybox/front.hdr", "assets/skybox/back.hdr"
	};

	// 큐브맵 텍스처 로드
	unsigned int cubemapTexture = loadCubemap(skyboxFaces);
	
	// 셰이더에 텍스처 유닛 번호 설정
	skyboxShader.use();
	skyboxShader.setInt("skybox", 0);  // GL_TEXTURE0 사용
	return cubemapTexture;
}
#pragma endregion

#pragma region ImGui

// ImGui UI 생성
// HP 바와 애니메이션 제어 버튼을 표시합니다.
// TAB 키를 눌러 UI 모드를 토글할 수 있습니다.
void buildImGuiUI()
{
	// UI 모드가 꺼져있으면 아무것도 그리지 않음
	if (!g_UiMode) return;

	// HP 바를 그리는 람다 함수
	auto DrawHpBars = []() {
		const float maxHp = 100.0f;
		// 현재 HP 가져오기
		int enemyHp = gEnemyPtr ? gEnemyPtr->Status().hp : 0;
		int playerHp = gPlayerPtr ? gPlayerPtr->Status().hp : 0;

		ImGui::Text("HP");
		// 적 HP 바 (빨간색)
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(200, 40, 40, 255));
		ImGui::ProgressBar(enemyHp / maxHp, ImVec2(-1, 0), ("Enemy: " + std::to_string(enemyHp)).c_str());
		ImGui::PopStyleColor();
		// 플레이어 HP 바 (기본 색상)
		ImGui::ProgressBar(playerHp / maxHp, ImVec2(-1, 0), ("Player: " + std::to_string(playerHp)).c_str());
		ImGui::Separator();
	};

	// 첫 번째 UI 창: "Animation"
	ImGui::Begin("Animation");
	DrawHpBars();

	// 플레이어가 없거나 애니메이터가 없으면 버튼 비활성화
	bool disabled = !gPlayerPtr || !gPlayerPtr->GetAnimator();
	if (disabled) ImGui::BeginDisabled();
	
	// 플레이어 애니메이션 제어 버튼
	if (ImGui::Button("Attack"))  { if (gPlayerPtr) gPlayerPtr->StartAttack(); }
	if (ImGui::Button("Slash"))   { if (gPlayerPtr) gPlayerPtr->StartSlash(); }
	if (ImGui::Button("Casting")) { if (gPlayerPtr) gPlayerPtr->StartCast(); }
	
	if (disabled) ImGui::EndDisabled();
	ImGui::End();

	// 두 번째 UI 창: "Animation (Alt)" (같은 기능, 다른 위치)
	ImGui::Begin("Animation (Alt)");
	DrawHpBars();

	if (disabled) ImGui::BeginDisabled();
	// ##alt는 버튼 ID를 구분하기 위한 접미사
	if (ImGui::Button("Attack##alt"))  { if (gPlayerPtr) gPlayerPtr->StartAttack(); }
	if (ImGui::Button("Slash##alt"))   { if (gPlayerPtr) gPlayerPtr->StartSlash(); }
	if (ImGui::Button("Casting##alt")) { if (gPlayerPtr) gPlayerPtr->StartCast(); }
	if (disabled) ImGui::EndDisabled();

	ImGui::End();
}
#pragma endregion

#pragma region Scene Rendering

// 메인 씬 렌더링
// 조명과 그림자가 적용된 모든 오브젝트를 렌더링합니다.
void drawScene(Shader& skinnedShader, unsigned int shadowMap, const glm::mat4& lightSpaceMatrix)
{
	// 그림자 맵을 텍스처 유닛 2에 바인딩
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	// 투영 행렬 (원근 투영)
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	// 뷰 행렬 (카메라 변환)
	glm::mat4 view = camera.GetViewMatrix();

	// 조명 색상 계산
	// Blinn-Phong 조명 모델의 각 성분을 계산합니다.
	glm::vec3 diffuseColor  = gLightColor * gDiffuseStrength;   // 난반사 색상
	glm::vec3 ambientColor  = diffuseColor * gAmbientStrength;  // 환경광 색상
	glm::vec3 specularColor = gLightColor * gSpecularStrength;  // 정반사 색상

	// 셰이더 공통 설정을 하는 람다 함수
	// 각 오브젝트를 그리기 전에 공통 uniform 변수들을 설정합니다.
	auto setupShader = [&]() {
		skinnedShader.use();
		skinnedShader.setMat4("projection", projection);           // 투영 행렬
		skinnedShader.setMat4("view", view);                       // 뷰 행렬
		skinnedShader.setMat4("lightSpaceMatrix", lightSpaceMatrix); // 그림자 계산용 행렬
		skinnedShader.setVec3("light.position", gLightPos);       // 광원 위치
		skinnedShader.setVec3("viewPos", camera.Position);         // 카메라 위치 (정반사 계산용)
		skinnedShader.setVec3("light.ambient", ambientColor);      // 환경광 색상
		skinnedShader.setVec3("light.diffuse", diffuseColor);     // 난반사 색상
		skinnedShader.setVec3("light.specular", specularColor);    // 정반사 색상
		skinnedShader.setFloat("material.shininess", 32.0f);        // 반짝임 정도
		skinnedShader.setInt("shadowMap", 2);                      // 그림자 맵 텍스처 유닛
	};

	// 배경 렌더링
	if (gBackgroundPtr) {
		setupShader();
		skinnedShader.setInt("material.diffuse", 0);   // 디퓨즈 텍스처 유닛
		skinnedShader.setInt("material.specular", 1); // 스페큘러 텍스처 유닛
		gBackgroundPtr->Render(skinnedShader);
	}

	// 적 캐릭터 렌더링
	if (gEnemyPtr) {
		setupShader();
		skinnedShader.setFloat("shininess", 32.0f);
		skinnedShader.setInt("texture_diffuse1", 0);
		skinnedShader.setInt("texture_specular1", 1);
		// 위치: (0, -1, 0), 크기: 0.002, Y축 회전: 90도
		gEnemyPtr->Render(skinnedShader, glm::vec3(0.0f, -1.0f, 0.0f), 0.002f, 90.0f);
	}

	// 플레이어 캐릭터 렌더링
	if (gPlayerPtr) {
		setupShader();
		skinnedShader.setInt("material.diffuse", 0);
		skinnedShader.setInt("material.specular", 1);
		// 위치: (2, -1, 0), 크기: 0.002, Y축 회전: -90도
		gPlayerPtr->Render(skinnedShader, glm::vec3(2.0f, -1.0f, 0.0f), 0.002f, -90.0f);
	}
}
#pragma endregion

#pragma region Callbacks

// 입력 처리
// 매 프레임 호출되어 키보드 입력을 처리합니다.
void processInput(GLFWwindow* window)
{
	// ESC 키를 누르면 프로그램 종료
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// 프레임버퍼 크기 변경 콜백
// 윈도우 크기가 변경되면 뷰포트를 조정합니다.
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// 마우스 이동 콜백
// 현재는 사용하지 않지만, FPS 스타일 카메라 제어에 사용할 수 있습니다.
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {}

// 마우스 휠 콜백
// 현재는 사용하지 않지만, 줌 기능에 사용할 수 있습니다.
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {}

// 키보드 입력 콜백
// 키보드 입력을 처리합니다.
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// ImGui에 키 입력 전달 (UI 입력 처리)
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

	// TAB 키로 UI 모드 토글
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
		g_UiMode = !g_UiMode;
		// UI 모드일 때는 커서 표시, 아니면 숨김
		glfwSetInputMode(window, GLFW_CURSOR, g_UiMode ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
}

// 마우스 버튼 콜백
// 마우스 버튼 입력을 ImGui에 전달합니다.
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}
#pragma endregion

#pragma region Shadow Map

// 그림자 맵 설정
// 그림자를 만들기 위한 깊이 맵 프레임버퍼와 텍스처를 생성합니다.
// Shadow Mapping 기법을 사용합니다.
void setupShadowMap(unsigned int& depthMapFBO, unsigned int& depthMap)
{
	// 프레임버퍼 생성 (깊이 정보만 저장)
	glGenFramebuffers(1, &depthMapFBO);
	
	// 깊이 텍스처 생성
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	// 깊이 정보만 저장하는 텍스처 (색상 정보는 필요 없음)
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	// 텍스처 파라미터 설정
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // 최근접 필터
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);  // 경계 처리
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// 경계 색상을 흰색으로 설정 (그림자가 아닌 영역)
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// 프레임버퍼에 깊이 텍스처 연결
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	// 색상 버퍼는 사용하지 않음 (깊이만 필요)
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// 광원 공간 변환 행렬 계산
// 광원의 시점에서 장면을 바라보는 변환 행렬을 계산합니다.
// 이 행렬은 그림자 맵 생성과 그림자 판정에 사용됩니다.
glm::mat4 computeLightSpaceMatrix()
{
	float near_plane = 1.0f;   // 근거리 평면
	float far_plane  = 25.0f;  // 원거리 평면
	
	// 직교 투영 행렬 (광원은 평행광처럼 동작)
	// -10 ~ 10 범위의 영역을 투영
	glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
	
	// 뷰 행렬 (광원 위치에서 원점을 바라봄)
	glm::mat4 lightView = glm::lookAt(gLightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	
	// 최종 변환 행렬 = 투영 × 뷰
	return lightProjection * lightView;
}
#pragma endregion
