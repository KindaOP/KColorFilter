#include "renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

using namespace kop;


glm::vec3 Entity::getPos() const {
	return glm::vec3(
		this->mT[3][0], this->mT[3][1], this->mT[3][2]
	);
}


glm::mat3 Entity::getDir() const {
	return glm::mat3(this->mR);
}


const glm::mat4& Entity::readTMat() const {
	return this->mT;
}


const glm::mat4& Entity::readRMat() const {
	return this->mR;
}


const glm::mat4& Entity::readSMat() const {
	return this->mS;
}


void Entity::setPos(const glm::vec3& xyz) {
	this->mT = glm::translate(Entity::eye, xyz);
}


void Entity::setDir(const glm::vec3& ypr) {
	const glm::mat3 cDir = glm::mat3(this->mR);
	const glm::mat4 mYaw = glm::rotate(
		Entity::eye, glm::radians(ypr[0]),
		cDir[1]
	);
	const glm::mat4 mPitch = glm::rotate(
		Entity::eye, glm::radians(ypr[1]),
		cDir[0]
	);
	const glm::mat4 mRoll = glm::rotate(
		Entity::eye, glm::radians(ypr[2]),
		cDir[2]
	);
	this->mR = mYaw * mPitch * mRoll;
}


void Entity::setScale(const glm::vec3& xyz) {
	this->mS = glm::scale(Entity::eye, xyz);
}


void Entity::move(const glm::vec3& xyz) {
	this->mT = glm::translate(this->mT, xyz);
}


void Entity::rotate(const glm::vec3& ypr) {
	const glm::mat3 cDir = glm::mat3(this->mR);
	const glm::mat4 mYaw = glm::rotate(
		Entity::eye, glm::radians(ypr[0]),
		cDir[1]
	);
	const glm::mat4 mPitch = glm::rotate(
		Entity::eye, glm::radians(ypr[1]),
		cDir[0]
	);
	const glm::mat4 mRoll = glm::rotate(
		Entity::eye, glm::radians(ypr[2]),
		cDir[2]
	);
	this->mR = mYaw * mPitch * mRoll * this->mR;
}


void Entity::scale(const glm::vec3& xyz) {
	this->mS = glm::scale(this->mS, xyz);
}


void Object::applyTransform() {
	const glm::mat4 trMat = (
		this->readTMat() * this->readRMat() * this->readSMat()
	);
	const size_t numVertices = this->vboData.size();
	if (this->vbodataTransformed.size() != numVertices) {
		this->vbodataTransformed = this->vboData;
	}
	for (size_t i = 0; i < numVertices; i++) {
		const glm::vec4 vec = trMat * glm::make_vec4(this->vboData[i].position);
		for (size_t j = 0; j < 4; j++) {
			this->vbodataTransformed[i].position[j] = vec[j];
		}
	}
}


const std::vector<Vertex>& Object::getTransformedData() const {
	return this->vbodataTransformed;
}


Renderer::Renderer(
	const char* vertexShaderPath,
	const char* fragmentShaderPath,
	size_t maxVertices,
	size_t maxElements,
	int textureWidth,
	int textureHeight
) 
	: vertexShaderPath(vertexShaderPath),
	  fragmentShaderPath(fragmentShaderPath),
	  maxVertices(maxVertices),
	  maxElements(maxElements),
	  textureWidth(textureWidth),
	  textureHeight(textureHeight)
{
	if (Renderer::numInstances == 0) {
		if (!glfwInit()) {
			throw std::runtime_error("GLFW: Cannot initialize GLFW.");
		}
		IMGUI_CHECKVERSION();
	}
	Renderer::numInstances += 1;
	this->imgui = ImGui::CreateContext();
	ImGuiIO& imguiIo = ImGui::GetIO();
	imguiIo.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}


Renderer::~Renderer() {
	glfwDestroyWindow(this->window);
	Renderer::numInstances -= 1;
	if (Renderer::numInstances == 0) {
		ImGui_ImplGlfw_Shutdown();
		glfwTerminate();
	}
	ImGui::DestroyContext(this->imgui);
}


GLFWwindow* Renderer::getWindow() const {
	return this->window;
}


void Renderer::endLoop() {

}


size_t Renderer::numInstances = 0;
