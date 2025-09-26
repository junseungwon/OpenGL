#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;


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
	GLFWwindow* window = glfwCreateWindow(800, 600, "Hello with GLAD", nullptr, nullptr);
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
	float quadVertices[] = {
		// A
-0.8f, -0.2f, 0.0f,
-0.2f, -0.2f, 0.0f,
-0.5f,  0.4f, 0.0f,
// B
 0.2f, -0.2f, 0.0f,
 0.8f, -0.2f, 0.0f,
 0.5f,  0.4f, 0.0f
	};

	float colored[] = {
	 // A
    -0.8f,-0.2f,0.0f,   1.0f,0.7f,0.0f,
    -0.2f,-0.2f,0.0f,   0.6f,0.0f,0.0f,
    -0.5f, 0.4f,0.0f,   0.1f,0.0f,0.0f,
    // B
     0.2f,-0.2f,0.0f,   1.0f,0.7f,0.0f,
     0.8f,-0.2f,0.0f,   0.6f,0.0f,0.0f,
     0.5f, 0.4f,0.0f,	0.1f,0.0f,0.0f,
	};

	unsigned int indices[] = {
		 0,1,2,  3,4,5
	};

	unsigned int VBO2, VAO2, EBO;

	glGenVertexArrays(1, &VAO2);
	glBindVertexArray(VAO2);

	// VBO
	glGenBuffers(1, &VBO2);
	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colored), colored, GL_STATIC_DRAW);

	// 위치 속성
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// 색상 속성: offset 3 * sizeof(float)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// EBO (VAO가 바인딩된 상태에서!)
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


#pragma region Shader 프로그램
	// 1) 셰이더 소스 (실습 편의를 위해 여기선 문자열로 바로 넣었음)
	const char* vtxSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
void main(){ gl_Position = vec4(aPos, 1.0); })";

	const char* fragSrc = R"(#version 330 core
out vec4 FragColor;
void main(){ FragColor = vec4(1.0, 0.5, 0.2, 1.0); })";

	// 2) 컴파일 헬퍼
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

	// 3) 컴파일
	GLuint vtx = compileShader(GL_VERTEX_SHADER, vtxSrc);
	GLuint frg = compileShader(GL_FRAGMENT_SHADER, fragSrc);

	// 4) 링크 (프로그램 만들기)
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vtx);
	glAttachShader(prog, frg);
	glLinkProgram(prog);
	GLint linked = 0; glGetProgramiv(prog, GL_LINK_STATUS, &linked);
	if (!linked) {
		char log[512]; glGetProgramInfoLog(prog, 512, nullptr, log);
		std::cerr << "Program link error: " << log << std::endl;
	}
	// (최적화) 개별 셰이더는 링크 후 삭제 가능
	glDeleteShader(vtx);
	glDeleteShader(frg);

	// 5) 사용
	glUseProgram(prog);



#pragma endregion


	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);   // 선 모드
	// (가정) GLFW로 창/컨텍스트 생성 완료, GLAD 초기화 완료
	GLuint program = CreateShaderProgramFromFiles("shaders/simple.vert", "shaders/simple.frag");
	GLint colorLoc = glGetUniformLocation(prog, "uColor");
		cout << "OpenGL ";
	while (!glfwWindowShouldClose(window))
	{

		float t = (float)glfwGetTime();               // 경과 시간(초)
		float g = 0.5f * std::sin(t) + 0.5f;

		// 1) 화면 초기화(배경색)
		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 2) 그리기

		glUseProgram(program);
		glUniform4f(colorLoc, 0.0f, g, 1.0f - g, 1.0f); // 파랑<->초록 계열 변화
		glBindVertexArray(VAO2);


		//glDrawArrays(GL_TRIANGLES, 0, 3); // 정점 3개로 삼각형 1개
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // 인덱스 6개로 사각형 2개
		// 3) 프레임 마무리
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}