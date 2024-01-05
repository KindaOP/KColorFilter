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


void Webcam::streamingThreadLoop() {
	this->camera.open(this->cameraId);
	if (!this->camera.isOpened()) {
		this->camera.release();
		return;
	}
	this->camera.set(cv::CAP_PROP_FRAME_WIDTH, this->width);
	this->camera.set(cv::CAP_PROP_FRAME_HEIGHT, this->height);
	cv::Mat bgrFrame;
	{
		std::lock_guard<std::mutex> lock(this->activeLocker);
		this->stateActive = true;
	}
	while (this->camera.isOpened() && this->isActive()) {
		this->camera >> bgrFrame;
		{
			std::lock_guard<std::mutex> lock(this->frameLocker);
			cv::cvtColor(bgrFrame, this->rgbFrame, cv::COLOR_BGR2RGB);
		}
	}
	this->camera.release();
}


Application::Application(Webcam& webcam, Renderer& renderer) 
	: webcam(&webcam),
	  renderer(&renderer)
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
	this->webcam->setActive(true);
	this->createOriginalRect();
	this->createFilteredRect();
	GLFWwindow* window = this->renderer->getWindow();
	bool imagesAreAcquired = false;
	while (!imagesAreAcquired) {
		imagesAreAcquired = this->acquireImages();
	}
	glfwShowWindow(window);
	while (!glfwWindowShouldClose(window)) {
		imagesAreAcquired = this->acquireImages();
		this->renderer->clear();
		this->initGUIFrame();

		if (imagesAreAcquired) {
			this->renderer->add(this->originalFrameRect);
			this->renderer->add(this->filteredFrameRect);
		}
		this->addGUIColorPickers();

		this->renderGUIFrame();
		this->renderer->render();
		this->renderer->present();
	}
	
	// End
	glfwHideWindow(window);
	this->webcam->setActive(false);
}


void Application::createOriginalRect() {
	this->originalFrameRect.vboData = {
		{
			{0.5f, 0.5f, 0.0f, 1.0f},
			{1.0f, 1.0f, 0.0f},
			{1.0f, 0.0f, 0.0f, 1.0f},
		},
		{
			{-0.5f, 0.5f, 0.0f, 1.0f},
			{0.0f, 1.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 1.0f},
		},
		{
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{0.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f, 1.0f},
		},
		{
			{0.5f, -0.5f, 0.0f, 1.0f},
			{1.0f, 0.0f, 0.0f},
			{1.0f, 1.0f, 0.0f, 1.0f},
		},
	};
	this->originalFrameRect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	this->originalFrameRect.move({ -0.5f, 0.0f, 0.0f });
	this->originalFrameRect.scale({ 1.0f, 2.0f, 1.0f });
	this->originalFrameRect.applyTransform();
}


void Application::createFilteredRect() {
	this->filteredFrameRect.vboData = {
		{
			{0.5f, 0.5f, 0.0f, 1.0f},
			{1.0f, 1.0f, 1.0f},
			{0.0f, 1.0f, 1.0f, 1.0f},
		},
		{
			{-0.5f, 0.5f, 0.0f, 1.0f},
			{0.0f, 1.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f},
		},
		{
			{-0.5f, -0.5f, 0.0f, 1.0f},
			{0.0f, 0.0f, 1.0f},
			{1.0f, 0.0f, 1.0f, 1.0f},
		},
		{
			{0.5f, -0.5f, 0.0f, 1.0f},
			{1.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 1.0f, 1.0f},
		},
	};
	this->filteredFrameRect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	this->filteredFrameRect.move({ 0.5f, 0.0f, 0.0f });
	this->filteredFrameRect.scale({ 1.0f, 2.0f, 1.0f });
	this->filteredFrameRect.applyTransform();
}


bool Application::acquireImages() {
	this->outLowerHSV[0] = 180.0f * this->inLowerHSV[0];
	this->outLowerHSV[1] = 255.0f * this->inLowerHSV[1];
	this->outLowerHSV[2] = 255.0f * this->inLowerHSV[2];
	this->outUpperHSV[0] = 180.0f * this->inUpperHSV[0];
	this->outUpperHSV[1] = 255.0f * this->inUpperHSV[1];
	this->outUpperHSV[2] = 255.0f * this->inUpperHSV[2];
	if (this->webcam->rgbFrame.empty()) {
		return false;
	}
	else {
		std::lock_guard<std::mutex> lock(this->webcam->frameLocker);
		cv::cvtColor(
			this->webcam->rgbFrame,
			this->originalFrameRect.textureImage, cv::COLOR_RGB2RGBA
		);
		cv::cvtColor(
			this->webcam->rgbFrame, 
			this->hsvImage, cv::COLOR_RGB2HSV
		);
	}
	cv::inRange(
		this->hsvImage, this->outLowerHSV,
		this->outUpperHSV, this->hsvMask
	);
	cv::copyTo(
		this->originalFrameRect.textureImage,
		this->filteredFrameRect.textureImage,
		this->hsvMask
	);
	return true;
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
	ImGui::SeparatorText("HSV Mask");
	ImGui::BeginGroup();
	ImGui::Text("Lower");
	ImGui::ColorPicker3(
		"LowerHSV",
		this->inLowerHSV.data(),
		this->imguiColorEditFlags
	);
	ImGui::EndGroup();
	ImGui::Spacing();
	ImGui::BeginGroup();
	ImGui::Text("Upper");
	ImGui::ColorPicker3(
		"UpperHSV",
		this->inUpperHSV.data(),
		this->imguiColorEditFlags
	);
	ImGui::EndGroup();
}


void Application::renderGUIFrame() const {
	ImGui::End();
	ImGui::Render();
}