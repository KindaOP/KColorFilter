#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <opencv2/core.hpp>
#include <backends/imgui_impl_glfw.h>
#include <array>
#include <vector>


namespace kop {

	struct Vertex {
	public:
		float position[4] = { 0.0f };
		float texCoord[3] = { 0.0f };
		float color[4] = { 0.0f };
	public:
		// methods
	public:
		static constexpr const std::array<size_t, 3> layout = {
			4, // position
			3, // texture coordinate (2-D + layer)
			4, // color
		};
	};


	class Entity {
	public:
		Entity() = default;
		virtual ~Entity() = default;
		glm::vec3 getPos() const;
		glm::mat3 getDir() const;
		const glm::mat4& readRMat() const;
		const glm::mat4& readSMat() const;
		const glm::mat4& readTMat() const;
		void setPos(const glm::vec3& xyz);
		void setDir(const glm::vec3& ypr);
		void setScale(const glm::vec3& xyz);
		void move(const glm::vec3& xyz);
		void rotate(const glm::vec3& ypr);
		void scale(const glm::vec3& xyz);
	public:
		constexpr static const glm::mat4 eye = glm::mat4(1);
	private:
		glm::mat4 mT = glm::mat4(1);
		glm::mat4 mR = glm::mat4(1);
		glm::mat4 mS = glm::mat4(1);
	};


	struct Object : public Entity {
	public:
		std::vector<Vertex> vboData = {};
		std::vector<unsigned int> eboData = {};
	public:
		void applyTransform();
		const std::vector<Vertex>& getTransformedData() const;
	private:
		std::vector<Vertex> vbodataTransformed = {};
	};


	class Renderer {
	public:
		Renderer(
			const char* vertexShaderPath,
			const char* fragmentShaderPath,
			size_t maxVertices,
			size_t maxElements,
			int textureWidth,
			int textureHeight
		);
		virtual ~Renderer();
		GLFWwindow* getWindow() const;
		virtual const char* getWindowName() const = 0;
		virtual void setViewport(int width, int height) = 0;
		virtual void clear() = 0;
		virtual bool add(const Object& obj) = 0;
		virtual bool updateTexture(const void* data, size_t index) = 0;
		virtual void render() = 0;
		virtual void present() = 0;
	public:
		static constexpr const size_t maxTextures = 2;
	public:
		const char* vertexShaderPath;
		const char* fragmentShaderPath;
		const size_t maxVertices;
		const size_t maxElements;
		const int textureWidth;
		const int textureHeight;
	protected:
		virtual void createWindow() = 0;
		virtual void createShaderProgram() = 0;
		virtual void createVertexBuffers() = 0;
		virtual void createTextures() = 0;
	protected:
		GLFWwindow* window = nullptr;
		int windowWidth = 800;
		int windowHeight = 600;
		size_t vertexOffset = 0;
		size_t elementOffset = 0;
		ImGuiContext* imgui = nullptr;
	private:
		static size_t numInstances;
	};

}