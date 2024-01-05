#include "application.h"
#include <opencv2/imgproc.hpp>

#ifdef NDEBUG
const bool IS_DEBUG = false;
#else
const bool IS_DEBUG = true;
#endif

using namespace kop;


Webcam::Webcam(
	unsigned int cameraId,
	int width,
	int height
)
	: cameraId(cameraId)
{
	this->camera.open(cameraId);
	if (!this->camera.isOpened()) {
		this->camera.release();
		return;
	}
	this->camera.set(cv::CAP_PROP_FRAME_WIDTH, width);
	this->camera.set(cv::CAP_PROP_FRAME_HEIGHT, height);
	this->width = static_cast<int>(
		this->camera.get(cv::CAP_PROP_FRAME_WIDTH)
		);
	this->height = static_cast<int>(
		this->camera.get(cv::CAP_PROP_FRAME_HEIGHT)
		);
	this->camera.release();
}


Webcam::~Webcam() {
	this->setActive(false);
}


bool Webcam::isActive() const {
	std::lock_guard<std::mutex> lock(this->activeLocker);
	return this->stateActive;
}


void Webcam::setActive(bool newState) {
	std::lock_guard<std::mutex> lock(this->activeLocker);
	if (newState && !this->stateActive) {
		std::thread th(&Webcam::streamingThreadLoop, this);
		th.detach();
	}
	else if (!newState && this->stateActive) {
		this->stateActive = false;
	}
}


int Webcam::getWidth() const {
	return this->width;
}


int Webcam::getHeight() const {
	return this->height;
}


void Webcam::getFrame(cv::Mat& image) {
	std::lock_guard<std::mutex> lock(this->frameLocker);
	cv::cvtColor(this->rgbFrame, image, cv::COLOR_BGR2RGBA);
}


void Webcam::streamingThreadLoop() {
	this->camera.open(this->cameraId);
	if (!this->camera.isOpened()) {
		this->camera.release();
		return;
	}
	this->camera.set(cv::CAP_PROP_FRAME_WIDTH, this->width);
	this->camera.set(cv::CAP_PROP_FRAME_HEIGHT, this->height);
	cv::Mat rgbFrame;
	{
		std::lock_guard<std::mutex> lock(this->activeLocker);
		this->stateActive = true;
	}
	while (this->camera.isOpened() && this->isActive()) {
		this->camera >> rgbFrame;
		{
			std::lock_guard<std::mutex> lock(this->frameLocker);
			cv::cvtColor(rgbFrame, this->rgbFrame, cv::COLOR_BGR2RGB);
		}
	}
	this->camera.release();
}


Application::Application(Renderer& renderer) 
	: renderer(&renderer)
{
	this->imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	this->imguiWindowFlags |= ImGuiWindowFlags_NoNavInputs;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_NoSidePreview;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_PickerHueWheel;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_Float;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_InputHSV;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_DisplayHSV;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_NoLabel;
}


void Application::run() {
	this->webcam.setActive(true);
	this->createRectangles();
	GLFWwindow* window = this->renderer->getWindow();
	glfwShowWindow(window);
	while (!glfwWindowShouldClose(window)) {
		this->renderer->clear();
		this->initGUIFrame();

		this->renderer->add(this->originalFrameRect);
		this->renderer->add(this->filteredFrameRect);
		this->addGUIColorPickers();

		this->renderGUIFrame();
		this->renderer->render();
		this->renderer->present();
	}
	
	// End
	glfwHideWindow(window);
	this->webcam.setActive(false);
}


void Application::createRectangles() {
	this->originalFrameRect.vboData = {
		{{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
	};
	this->originalFrameRect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	this->originalFrameRect.move({ -0.5f, 0.0f, 0.0f });
	this->originalFrameRect.scale({ 1.0f, 2.0f, 1.0f });
	this->originalFrameRect.applyTransform();

	this->filteredFrameRect.vboData = {
		{{0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	};
	this->filteredFrameRect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	this->filteredFrameRect.move({ 0.5f, 0.0f, 0.0f });
	this->filteredFrameRect.scale({ 1.0f, 2.0f, 1.0f });
	this->filteredFrameRect.applyTransform();
}


void Application::initGUIFrame() const {
	ImGui::NewFrame();
	if (IS_DEBUG) {
		ImGui::ShowDemoWindow();
	}
	ImGui::SetNextWindowPos(
		{ ImGui::GetIO().DisplaySize.x, 0 },
		ImGuiCond_Always, { 1, 0 }
	);
	ImGui::Begin("Settings", nullptr, this->imguiWindowFlags);
}


void Application::addGUIColorPickers() {
	ImGui::SeparatorText("Colors");
	ImGui::BeginGroup();
	ImGui::Text("Initial");
	ImGui::ColorPicker3(
		"InitialColor",
		this->initHSV.data(),
		this->imguiColorEditFlags
	);
	ImGui::EndGroup();
	ImGui::Spacing();
	ImGui::BeginGroup();
	ImGui::Text("Final");
	ImGui::ColorPicker3(
		"FinalColor",
		this->finalHSV.data(),
		this->imguiColorEditFlags
	);
	ImGui::EndGroup();
}


void Application::renderGUIFrame() const {
	ImGui::End();
	ImGui::Render();
}