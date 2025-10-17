#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "stb_image.h"

static float g_mix = 0.2f;
static GLint  g_wrapModes[3] = { GL_REPEAT,GL_MIRRORED_REPEAT,GL_CLAMP_TO_EDGE };
static int    g_wrapIdx = 0;
static bool   g_linearFilter = true;
static GLuint tex0 = 0, tex1 = 0;

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

static void applyTexParams(GLuint tex) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, g_wrapModes[g_wrapIdx]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, g_wrapModes[g_wrapIdx]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_linearFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_linearFilter ? GL_LINEAR : GL_NEAREST);
}
static GLuint makeTexture2D(const char* path) {
	int w, h, nc; stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path, &w, &h, &nc, 0);
	if (!data) { std::cerr << "Load fail: " << path << "\n"; return 0; }
	GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
	applyTexParams(t);
	GLenum fmt = (nc == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return t;
}
////////////////////////////////////////////////////////////////////////



void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* win = glfwCreateWindow(800, 600, "Two Textures (Z:Filter, X:Wrap, Up/Down:Mix)", nullptr, nullptr);
	glfwMakeContextCurrent(win);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

	float verts[] = {
		// pos            // color         // uv
		 0.5f,  0.5f,0,   1,0,0,           1,1,
		 0.5f, -0.5f,0,   0,1,0,           1,0,
		-0.5f, -0.5f,0,   0,0,1,           0,0,
		-0.5f,  0.5f,0,   1,1,0,           0,1
	};
	unsigned idx[] = { 0,1,3, 1,2,3 };

	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao); glBindVertexArray(vao);
	glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	glGenBuffers(1, &ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

	tex0 = makeTexture2D("assets/container.jpg");
	tex1 = makeTexture2D("assets/awesomeface.png");

	GLuint prog = CreateShaderProgramFromFiles("shaders/tex_mix.vert", "shaders/tex_mix.frag");
	glUseProgram(prog);
	glUniform1i(glGetUniformLocation(prog, "uTex0"), 0);

	glUniform1i(glGetUniformLocation(prog, "uTex1"), 1);

	// glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ÇÊ¿ä ½Ã

	double last = glfwGetTime();
	bool zPrev = false, xPrev = false;

	while (!glfwWindowShouldClose(win)) {
		double now = glfwGetTime(); float dt = float(now - last); last = now;
		if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win, true);
		if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)   g_mix = std::min(1.0f, g_mix + 0.7f * dt);
		if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) g_mix = std::max(0.0f, g_mix - 0.7f * dt);

		bool zNow = (glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
		bool xNow = (glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
		if (zNow && !zPrev) {
			g_linearFilter = !g_linearFilter; applyTexParams(tex0); applyTexParams(tex1);
			std::cout << "Filter: " << (g_linearFilter ? "LINEAR" : "NEAREST") << "\n";
		}
		if (xNow && !xPrev) {
			g_wrapIdx = (g_wrapIdx + 1) % 3; applyTexParams(tex0); applyTexParams(tex1);
			std::cout << "Wrap: " << (g_wrapIdx == 0 ? "REPEAT" : g_wrapIdx == 1 ? "MIRRORED_REPEAT" : "CLAMP_TO_EDGE") << "\n";
		}
		zPrev = zNow; xPrev = xNow;

		glClearColor(0.08f, 0.08f, 0.1f, 1); glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(prog);
		glUniform1f(glGetUniformLocation(prog, "uMix"), g_mix);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(win); glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}
