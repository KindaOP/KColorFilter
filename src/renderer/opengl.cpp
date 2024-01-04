#include "renderer/opengl.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

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
	this->window = glfwCreateWindow(
		this->windowWidth, this->windowHeight,
		this->getRendererName(), nullptr, nullptr
	);
	glfwMakeContextCurrent(this->window);
	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("GLEW: Cannot initialize GLEW.");
	}
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


void OpenGL::createVertexArray() {
	glCreateVertexArrays(1, &this->vao);
	this->numVertexElements = 0;
	for (size_t iAttrib = 0; iAttrib < Object::vertexLayout.size(); iAttrib++) {
		glVertexArrayAttribBinding(this->vao, iAttrib, 0);
		glVertexArrayAttribFormat(
			this->vao, iAttrib, Object::vertexLayout[iAttrib],
			GL_FLOAT, GL_FALSE, this->numVertexElements
		);
		glEnableVertexArrayAttrib(this->vao, iAttrib);
		this->numVertexElements += Object::vertexLayout[iAttrib] * sizeof(float);
	}
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
}
