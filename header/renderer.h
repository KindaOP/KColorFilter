#pragma once
#include <GLFW/glfw3.h>
#include <array>
#include <vector>


namespace kop {

	class Entity {

	};


	struct Vertex {
	public:
		float positions[4] = { 0.0f };
		float color[4] = { 0.0f };
	public:
		// methods
	public:
		static constexpr const std::array<size_t, 2> layout = {
			4, // position
			4, // color
		};
	};


	struct Object : public Entity {
	public:
		std::vector<Vertex> vboData = {};
		std::vector<unsigned int> eboData = {};
	public:
		// methods
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
		virtual void clear() = 0;
		virtual bool add(const Object& obj) = 0;
		virtual void render() = 0;
		virtual void present() = 0;
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
		size_t vertexOffset = 0;
		size_t elementOffset = 0;
	protected:
		static size_t numInstances;
	};

}