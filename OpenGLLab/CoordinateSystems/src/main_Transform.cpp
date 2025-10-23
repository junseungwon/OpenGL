#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>




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
    // 1) GLFW
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(960, 600, "Transformations", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 2) GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n"; return -1;
    }

    // 3) 셰이더
    std::string vsSrc = ReadFile("shaders/transform.vert");
    std::string fsSrc = ReadFile("shaders/transform.frag");
    GLuint program = CreateShaderProgram(vsSrc.c_str(), fsSrc.c_str());
    glUseProgram(program);
    GLint locTransform = glGetUniformLocation(program, "uTransform");

    // 4) 정점(사각형) + 색
    float vertices[] = {
        // pos                // color
         0.5f,  0.5f, 0.0f,   1.f, 0.f, 0.f,
         0.5f, -0.5f, 0.0f,   0.f, 1.f, 0.f,
        -0.5f, -0.5f, 0.0f,   0.f, 0.f, 1.f,
        -0.5f,  0.5f, 0.0f,   1.f, 1.f, 0.f
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glm::mat4 mm(1);
    glm::vec3 v3(0);
    // 5) 루프
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        float t = (float)glfwGetTime();
        glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);

        // (A) 왼쪽: 이동 → 회전 → 축소
        float speed = 0.005f;
        glm::mat4 M1(1);
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            v3 += glm::vec3(0, -1*speed, 0);
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            v3 += glm::vec3(0, speed, 0);
        }
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            v3 += glm::vec3(-1*speed,0, 0);
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            v3 += glm::vec3(speed,0, 0);
        }
        M1 = glm::translate(M1, v3);
          
        // --- 여기부터 피벗 회전 ---
       // glm::vec2 pivot(0.2f, 0.2f);                           // 회전 중심점 정의
       // M1 = glm::translate(M1, glm::vec3(pivot, 0.0f));       // T(p)
       // M1 = glm::rotate(M1, t, glm::vec3(0, 0, 1));           // R(θ)
       // M1 = glm::translate(M1, glm::vec3(-pivot, 0.0f));      // T(-p)
        // --- 여기까지 피벗 회전 ---

        //M1 = glm::scale(M1, glm::vec3(2.0f,0.5f, 1));
        //mm = M1;
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(M1));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // (B) 오른쪽: 이동 → 시간 스케일
        glm::mat4 M2(1.0f);
        M2 = glm::translate(M2, glm::vec3(0.5f, 0.0f, 0.0f));
        float s = 0.5f + 0.25f * (std::sin(t) + 1.0f); // 0.5 ~ 1.0
       // M2 = glm::scale(M2, glm::vec3(s));
        glUniformMatrix4fv(locTransform, 1, GL_FALSE, glm::value_ptr(M2));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

     glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteBuffers(1,&EBO);
    glDeleteProgram(program);
    glfwTerminate();
    return 0;
}
