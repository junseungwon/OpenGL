#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>





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
int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(800, 600, "FirstOpenGL with GLAD", nullptr, nullptr);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	// GLAD 초기화
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	// 렌더 루프
#pragma region 랜더루프
	// NDC 좌표 (반시계 방향)
	float vertices[] = {
	-0.5f, -0.5f, 0.0f, // 왼쪽 아래
	 0.5f, -0.5f, 0.0f, // 오른쪽 아래
	 0.0f,  0.5f, 0.0f // 위쪽
	};
	unsigned int VBO, VAO;


	// 1) VAO 생성 및 바인딩
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);


	// 2) VBO 생성 및 데이터 업로드
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// vertices 배열을 GPU로 복사
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


	// 3) 정점 속성 포인터 설정 (위치 속성만 있는 경우)
	// layout(location = 0)에 vec3 입력을 연결
	// 인자: (인덱스, 크기, 타입, 정규화 여부, stride, offset)
	glVertexAttribPointer(
		0, // attribute index (location=0)
		3, // vec3 → 3개 구성요소
		GL_FLOAT, // 각 구성요소 타입
		GL_FALSE, // 정규화 불필요
		3 * sizeof(float), // stride: 한 정점에서 다음 정점까지 바이트 간격
		(void*)0 // 시작 오프셋
	);
	// 해당 속성 활성화
	glEnableVertexAttribArray(0);


	// (선택) 정리
	// glBindBuffer(GL_ARRAY_BUFFER, 0);
	// glBindVertexArray(0);

#pragma endregion



	// (가정) GLFW로 창/컨텍스트 생성 완료, GLAD 초기화 완료

//	GLuint program = CreateShaderProgramFromFiles("shaders/simple.vert", "shaders/simple.frag");

	// 교체: (인라인 셰이더 생성)
	const char* vtxSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
void main(){ gl_Position = vec4(aPos, 1.0); })";

	const char* fragSrc = R"(#version 330 core
out vec4 FragColor;
uniform vec4 uColor;             // ← 선언
void main(){ FragColor = uColor; }   // ← 사용
)";

	// 1) 컴파일 헬퍼 (람다)
	auto compileShader = [](GLenum type, const char* src) {
		GLuint sh = glCreateShader(type);
		glShaderSource(sh, 1, &src, nullptr);
		glCompileShader(sh);
		GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
		if (!ok) {
			char log[512]; glGetShaderInfoLog(sh, 512, nullptr, log);
			std::cerr << "Shader compile error: " << log << std::endl;
		}
		return sh;
		};

	// 2) 컴파일 & 링크
	GLuint vtx = compileShader(GL_VERTEX_SHADER, vtxSrc);
	GLuint frg = compileShader(GL_FRAGMENT_SHADER, fragSrc);

	GLuint program = glCreateProgram();
	glAttachShader(program, vtx);
	glAttachShader(program, frg);
	glLinkProgram(program);

	GLint linked = 0; glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked) {
		char log[512]; glGetProgramInfoLog(program, 512, nullptr, log);
		std::cerr << "Program link error: " << log << std::endl;
	}
	glDeleteShader(vtx);
	glDeleteShader(frg);

	GLint colorLoc = glGetUniformLocation(program, "uColor");
	std::cout << "dasd";
	// 렌더 루프 안쪽
	while (!glfwWindowShouldClose(window)) {
		float t = (float)glfwGetTime();               // 경과 시간(초)
		float g = 0.5f * std::sin(t) + 0.5f;          // 0~1 사이로 변환
		glUseProgram(program);                           // (중요) 활성화 먼저
		glUniform4f(colorLoc, 0.0f, g, 1.0f - g, 1.0f); // 파랑<->초록 계열 변화
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}