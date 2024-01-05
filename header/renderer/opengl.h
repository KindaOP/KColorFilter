#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include "renderer.h"


namespace kop {

	class OpenGL : public Renderer {
	public:
		OpenGL(
			const char* vertexShaderPath,
			const char* fragmentShaderPath,
			size_t maxVertices,
			size_t maxElements,
			int textureWidth,
			int textureHeight
		);
		~OpenGL() override;
		const char* getWindowName() const override;
		void setViewport(int width, int height) override;
		void clear() override;
		bool add(const Object& obj) override;
		void render() override;
		void present() override;
	private:
		void createWindow() override;
		void createShaderProgram() override;
		void createVertexArray();
		void createVertexBuffers() override;
		void createTextures() override;
	private:
		unsigned int shader = NULL;
		unsigned int vao = NULL;
		unsigned int vbo = NULL;
		unsigned int ebo = NULL;
		unsigned int tex = NULL;
	private:
		static size_t numInstance;
	private:
		static unsigned int createShaderModule(
			GLenum shaderType, 
			const char* shaderPath
		);
		static void windowFrameBufferSizeCallback(
			GLFWwindow* window, int width, int height
		);
		static void APIENTRY glErrorCallback(
			GLenum source, GLenum type, GLuint idx, GLenum severity,
			GLsizei length, const char* message, const void* userParams
		);
	};

}