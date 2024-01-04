#include "renderer/opengl.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef NDEBUG
const bool IS_DEBUG = false;
#else
const bool IS_DEBUG = true;
#endif

using namespace kop;


OpenGL::OpenGL(
	const char* vertexShaderPath,
	const char* fragmentShaderPath,
	size_t maxVertices,
	size_t maxElements
) 
	: Renderer(vertexShaderPath, fragmentShaderPath, maxVertices, maxElements)
{
	this->createWindow();
	this->createShaderProgram();
	this->createVertexArray();
	this->createVertexBuffers();
}


OpenGL::~OpenGL() {
	glDeleteBuffers(1, &this->vbo);
	glDeleteBuffers(1, &this->ebo);
	glDeleteVertexArrays(1, &this->vao);
	glDeleteProgram(this->shader);
}


const char* OpenGL::getRendererName() const {
	return "OpenGL";
}


void OpenGL::createWindow() {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	if (IS_DEBUG) {
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	}
	this->window = glfwCreateWindow(
		this->windowWidth, this->windowHeight,
		this->getRendererName(), nullptr, nullptr
	);
	glfwMakeContextCurrent(this->window);
	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("GLEW: Cannot initialize GLEW.");
	}
	glDebugMessageCallback(OpenGL::glErrorCallback, nullptr);
}


void OpenGL::createShaderProgram() {
	this->shader = glCreateProgram();
	const unsigned int vertexShader = OpenGL::createShaderModule(
		GL_VERTEX_SHADER, this->vertexShaderPath
	);
	const unsigned int fragmentShader = OpenGL::createShaderModule(
		GL_FRAGMENT_SHADER, this->fragmentShaderPath
	);
	glAttachShader(this->shader, vertexShader);
	glAttachShader(this->shader, fragmentShader);
	glLinkProgram(this->shader);
	glValidateProgram(this->shader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glUseProgram(this->shader);
}


void OpenGL::createVertexBuffers() {
	glCreateBuffers(1, &this->vbo);
	glCreateBuffers(1, &this->ebo);
	glNamedBufferStorage(
		this->vbo, this->maxVertices * sizeof(float),
		nullptr, GL_DYNAMIC_STORAGE_BIT
	);
	glNamedBufferStorage(
		this->ebo, this->maxElements * sizeof(unsigned int),
		nullptr, GL_DYNAMIC_STORAGE_BIT
	);
	glVertexArrayVertexBuffer(
		this->vao, 0, this->vbo, 0, this->numVertexElements * sizeof(float)
	);
	glVertexArrayElementBuffer(this->vao, this->ebo);
}


void OpenGL::createVertexArray() {
	glCreateVertexArrays(1, &this->vao);
	this->numVertexElements = 0;
	for (size_t iAttrib = 0; iAttrib < Object::vertexLayout.size(); iAttrib++) {
		glVertexArrayAttribBinding(this->vao, iAttrib, 0);
		glVertexArrayAttribFormat(
			this->vao, iAttrib, Object::vertexLayout[iAttrib],
			GL_FLOAT, GL_FALSE, this->numVertexElements * sizeof(float)
		);
		glEnableVertexArrayAttrib(this->vao, iAttrib);
		this->numVertexElements += Object::vertexLayout[iAttrib];
	}
	glBindVertexArray(this->vao);
}


void OpenGL::clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}


bool OpenGL::add(const Object& obj) {
	return true;
}


void OpenGL::render() {
	glfwSwapBuffers(this->window);
}


void OpenGL::present() {
	glfwPollEvents();
}


unsigned int OpenGL::createShaderModule(
	GLenum shaderType, const char* shaderPath
) {
	std::ifstream file(shaderPath);
	if (!file.is_open()) {
		const std::string errorMessage(
			"OpenGL: Cannot load shader source from "
		);
		throw std::runtime_error(
			errorMessage + shaderPath + '.'
		);
	}
	std::stringstream ss;
	std::string line;
	while (std::getline(file, line)) {
		ss << line << '\n';
	}
	file.close();

	const unsigned int id = glCreateShader(shaderType);
	std::string source = ss.str();
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, 0);
	glCompileShader(id);
	return id;
}


void APIENTRY OpenGL::glErrorCallback(
	GLenum source, GLenum type, GLuint idx, GLenum severity,
	GLsizei length, const char* message, const void* userParams
) {
	std::cout << source << '\n';
	std::cout << type << '\n';
	std::cout << idx << '\n';
	std::cout << severity << '\n';
	std::cout << message << std::endl;
}
