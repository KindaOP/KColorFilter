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
			size_t maxElements
		);
		~OpenGL() override;
		const char* getRendererName() const override;
		void setViewport(int width, int height) override;
		void clear() override;
		bool add(const Object& obj) override;
		void render() override;
		void present() override;
	private:
		void createWindow() override;
		void createShaderProgram() override;
		void createVertexArray();
		void createVertexBuffers();
	private:
		unsigned int shader = 0;
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;
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