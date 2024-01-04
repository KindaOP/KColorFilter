#pragma once
#include <GLFW/glfw3.h>
#include <array>


namespace kop {

	class Entity {

	};

	class Object : public Entity {
	public:
		static constexpr const std::array<size_t, 2> vertexLayout = {
			4, // position
			4, // color
		};
	};

	class Renderer {
	public:
		Renderer(
			const char* vertexShaderPath,
			const char* fragmentShaderPath,
			size_t maxVertices,
			size_t maxElements
		);
		virtual ~Renderer();
		GLFWwindow* getWindow() const;
		virtual const char* getRendererName() const = 0;
	public:
		const char* vertexShaderPath;
		const char* fragmentShaderPath;
		const size_t maxVertices;
		const size_t maxElements;
	protected:
		virtual void createWindow() = 0;
		virtual void createShaderProgram() = 0;
	protected:
		GLFWwindow* window = nullptr;
		int windowWidth = 800;
		int windowHeight = 600;
		size_t numVertexElements = 0;
	protected:
		static size_t numInstances;
	};

}