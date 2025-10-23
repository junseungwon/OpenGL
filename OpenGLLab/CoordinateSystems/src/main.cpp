#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

static bool gUsePerspective = true;
static bool gPrevP = false;



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
// 키 처리 함수 분리
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// P 키 토글(1회성)
	bool pNow = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
	if (pNow && !gPrevP) gUsePerspective = !gUsePerspective;
	gPrevP = pNow;
}

////////////////////////////////////////////////////////////////////////



void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}int main()
{
    // 1) 창/컨텍스트
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(960, 600, "Coordinate Systems - MVP", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n"; return -1;
    }




    glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화 (중요)

    // 2) 셰이더
    GLuint prog = CreateShaderProgramFromFiles("shaders/basic.vert", "shaders/basic.frag");

    // 3) 큐브 정점(위치 xyz + 색 rgb) 36개 (각 면 2삼각형*6면)
    float vertices[] = {
        // 뒤(-Z)면
        -0.5f,-0.5f,-0.5f, 1,0,0,   0.5f,-0.5f,-0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,0,1,
         0.5f, 0.5f,-0.5f, 0,0,1,  -0.5f, 0.5f,-0.5f, 1,1,0,  -0.5f,-0.5f,-0.5f, 1,0,0,
         // 앞(+Z)면
         -0.5f,-0.5f, 0.5f, 1,0,1,   0.5f,-0.5f, 0.5f, 0,1,1,   0.5f, 0.5f, 0.5f, 1,1,1,
          0.5f, 0.5f, 0.5f, 1,1,1,  -0.5f, 0.5f, 0.5f, 0.5,0.5,0.5,  -0.5f,-0.5f, 0.5f, 1,0,1,
          // 좌(-X)면
          -0.5f, 0.5f, 0.5f, 1,0.5,0, -0.5f, 0.5f,-0.5f, 0.6,0.2,0.2, -0.5f,-0.5f,-0.5f, 0.2,0.6,0.2,
          -0.5f,-0.5f,-0.5f, 0.2,0.6,0.2, -0.5f,-0.5f, 0.5f, 0.2,0.2,0.6, -0.5f, 0.5f, 0.5f, 1,0.5,0,
          // 우(+X)면
           0.5f, 0.5f, 0.5f, 0.9,0.4,0.1,  0.5f, 0.5f,-0.5f, 0.1,0.9,0.4,  0.5f,-0.5f,-0.5f, 0.4,0.1,0.9,
           0.5f,-0.5f,-0.5f, 0.4,0.1,0.9,  0.5f,-0.5f, 0.5f, 0.7,0.3,0.2,  0.5f, 0.5f, 0.5f, 0.9,0.4,0.1,
           // 하(-Y)면
           -0.5f,-0.5f,-0.5f, 0.2,0.2,0.2,  0.5f,-0.5f,-0.5f, 0.2,0.2,0.2,  0.5f,-0.5f, 0.5f, 0.2,0.2,0.2,
            0.5f,-0.5f, 0.5f, 0.4,0.4,0.4, -0.5f,-0.5f, 0.5f, 0.4,0.4,0.4, -0.5f,-0.5f,-0.5f, 0.4,0.4,0.4,
            // 상(+Y)면
            -0.5f, 0.5f,-0.5f, 0.8,0.8,0.2,  0.5f, 0.5f,-0.5f, 0.8,0.8,0.2,  0.5f, 0.5f, 0.5f, 0.8,0.8,0.2,
             0.5f, 0.5f, 0.5f, 0.2,0.8,0.8, -0.5f, 0.5f, 0.5f, 0.2,0.8,0.8, -0.5f, 0.5f,-0.5f, 0.2,0.8,0.8,
    };

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 위치(0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    // 색(1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Uniform 위치
    glUseProgram(prog);
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
        model = glm::rotate(model, t * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Y축 회전
        model = glm::rotate(model, t * glm::radians(17.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // X축 회전

        // --- View: 카메라를 뒤로 빼서 바라보기 ---
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

        // --- Projection: P키로 원근/직교 전환 ---
        float aspect = (height == 0) ? 1.0f : (float)width / (float)height;
        glm::mat4 proj;
        if (gUsePerspective) {
            std::cout << "원근" << std::endl;
            proj = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);
        }
        else {
            std::cout << "직근" << std::endl;
            float s = 1.5f;
            proj = glm::ortho(-s * aspect, s * aspect, -s, s, 0.1f, 100.0f);
        }

        // 업로드(열우선: 전치 필요 없음 -> GL_FALSE)
        glUseProgram(prog);
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