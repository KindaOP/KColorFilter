#include "renderer/opengl.h"
#include <backends/imgui_impl_opengl3.h>
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
	size_t maxElements,
	int textureWidth,
	int textureHeight
) 
	: Renderer(
		vertexShaderPath, fragmentShaderPath, maxVertices, maxElements,
		textureWidth, textureHeight
	  )
{
	this->createWindow();
	if (OpenGL::numInstances == 0) {
		ImGui_ImplGlfw_InitForOpenGL(this->window, true);
		ImGui_ImplOpenGL3_Init("#version 460");
	}
	this->createGraphicsPipeline();
	this->createVertexArray();
	this->createVertexBuffers();
	this->createTextures();
	OpenGL::numInstances += 1;
}


OpenGL::~OpenGL() {
	glDeleteTextures(1, &this->tex);
	glDeleteBuffers(1, &this->vbo);
	glDeleteBuffers(1, &this->ebo);
	glDeleteVertexArrays(1, &this->vao);
	glDeleteProgram(this->shader);
	OpenGL::numInstances -= 1;
	if (OpenGL::numInstances == 0) {
		ImGui_ImplOpenGL3_Shutdown();
	}
}


const char* OpenGL::getWindowName() const {
	return "KCF-OpenGL";
}


void OpenGL::setViewport(int width, int height) {
	glViewport(0, 0, width, height);
	this->windowWidth = width;
	this->windowHeight = height;
}


void OpenGL::clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	this->vertexOffset = 0;
	this->elementOffset = 0;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
}


bool OpenGL::add(const Object& obj) {
	const size_t numVertices = obj.getTransformedData().size();
	const size_t numElements = obj.eboData.size();
	if (
		this->vertexOffset + numVertices > this->maxVertices ||
		this->elementOffset + numElements > this->maxElements
	) {
		return false;
	}
	std::vector<unsigned int> shiftedEboData(obj.eboData);
	for (unsigned int& element : shiftedEboData) {
		element += this->vertexOffset;
	}
	glNamedBufferSubData(
		this->vbo, this->vertexOffset * sizeof(Vertex),
		numVertices * sizeof(Vertex), obj.getTransformedData().data()
	);
	glNamedBufferSubData(
		this->ebo, this->elementOffset * sizeof(unsigned int),
		numElements * sizeof(unsigned int), shiftedEboData.data()
	);
	this->vertexOffset += numVertices;
	this->elementOffset += numElements;
	return true;
}


bool OpenGL::updateTexture(const void* data, size_t index) {
	if (!data || index < 0 || index >= Renderer::maxTextures) {
		return false;
	}
	glTextureSubImage3D(
		this->tex, 0, 0, 0, index, this->textureWidth, this->textureHeight,
		1, GL_RGBA, GL_UNSIGNED_BYTE, data
	);
	glGenerateTextureMipmap(this->tex);
	return true;
}


void OpenGL::render() {
	glDrawElements(
		GL_TRIANGLES, this->elementOffset, GL_UNSIGNED_INT, nullptr
	);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void OpenGL::present() {
	glfwSwapBuffers(this->window);
	glfwPollEvents();
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
		this->getWindowName(), nullptr, nullptr
	);
	if (this->window == nullptr) {
		throw std::runtime_error("GLFW: Cannot create window.");
	}
	glfwMakeContextCurrent(this->window);
	if (glewInit() != GLEW_OK) {
		throw std::runtime_error("GLEW: Cannot initialize GLEW.");
	}
	glfwSetWindowUserPointer(this->window, this);
	glfwSetFramebufferSizeCallback(
		this->window, OpenGL::windowFrameBufferSizeCallback
	);
	glfwMaximizeWindow(this->window);
	glfwGetWindowSize(
		this->window, &this->windowWidth, &this->windowHeight
	);
	glViewport(0, 0, this->windowWidth, this->windowHeight);
	glfwSwapInterval(1);
	glDebugMessageCallback(OpenGL::glErrorCallback, nullptr);
}


void OpenGL::createGraphicsPipeline() {
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


void OpenGL::createVertexArray() {
	glCreateVertexArrays(1, &this->vao);
	size_t offset = 0;
	for (size_t iAttrib = 0; iAttrib < Vertex::layout.size(); iAttrib++) {
		glVertexArrayAttribBinding(this->vao, iAttrib, 0);
		glVertexArrayAttribFormat(
			this->vao, iAttrib, Vertex::layout[iAttrib],
			GL_FLOAT, GL_FALSE, offset
		);
		glEnableVertexArrayAttrib(this->vao, iAttrib);
		offset += Vertex::layout[iAttrib] * sizeof(float);
	}
	glBindVertexArray(this->vao);
}


void OpenGL::createVertexBuffers() {
	glCreateBuffers(1, &this->vbo);
	glCreateBuffers(1, &this->ebo);
	glNamedBufferStorage(
		this->vbo, this->maxVertices * sizeof(Vertex),
		nullptr, GL_DYNAMIC_STORAGE_BIT
	);
	glNamedBufferStorage(
		this->ebo, this->maxElements * sizeof(unsigned int),
		nullptr, GL_DYNAMIC_STORAGE_BIT
	);
	glVertexArrayVertexBuffer(
		this->vao, 0, this->vbo, 0, sizeof(Vertex)
	);
	glVertexArrayElementBuffer(this->vao, this->ebo);
}


void OpenGL::createTextures() {
	glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &this->tex);
	glTextureStorage3D(
		this->tex, 1, GL_RGBA8, this->textureWidth, 
		this->textureHeight, Renderer::maxTextures
	);
	glTextureParameteri(this->tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(this->tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTextureUnit(0, this->tex);
}


size_t OpenGL::numInstances = 0;


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
	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);
	return id;
}


void OpenGL::windowFrameBufferSizeCallback(
	GLFWwindow* window, int width, int height
) {
	auto openglRenderer = static_cast<OpenGL* const>(
		glfwGetWindowUserPointer(window)
	);
	openglRenderer->setViewport(width, height);
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
