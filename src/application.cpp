#include "application.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

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
		std::thread th(&Webcam::streamingThread, this);
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


void Webcam::getFrame(cv::Mat& image) const {
	std::lock_guard<std::mutex> lock(this->frameLocker);
	if (this->rgbFrame.empty()) {
		return;
	}
	cv::flip(this->rgbFrame, image, -1);
}


void Webcam::openSettings() {
	this->camera.set(cv::CAP_PROP_SETTINGS, 1);
}


void Webcam::setMafOrder(size_t order) {
	std::lock_guard<std::mutex> lock(this->orderLocker);
	if (order < 0 || order >= Webcam::maxMafOrder) {
		return;
	}
	this->mafOrder = order;
}


const cv::Scalar Webcam::nullColor = { 0.0f, 0.0f, 0.0f, 0.0f };


void Webcam::streamingThread() {
	this->camera.open(this->cameraId);
	if (!this->camera.isOpened()) {
		this->camera.release();
		return;
	}
	this->camera.set(cv::CAP_PROP_FRAME_WIDTH, this->width);
	this->camera.set(cv::CAP_PROP_FRAME_HEIGHT, this->height);
	{
		std::lock_guard<std::mutex> lock(this->activeLocker);
		this->stateActive = true;
	}
	this->threadLoop();
	{
		std::lock_guard<std::mutex> lock(this->activeLocker);
		this->stateActive = false;
	}
	this->camera.release();
}


void Webcam::threadLoop() {
	cv::Mat mafFrame = cv::Mat(this->height, this->width, CV_8UC3);
	std::array<cv::Mat, Webcam::maxMafOrder> mafBuffer = {};
	bool mafIsComplete = false;
	size_t mafCurrentOrder = NULL;
	size_t mafIter = 0;
	while (this->camera.isOpened() && this->isActive()) {
		{
			std::lock_guard<std::mutex> lock(this->orderLocker);
			mafCurrentOrder = this->mafOrder;
		}
		this->camera >> mafBuffer[mafIter];
		mafIter += 1;
		if (mafIter >= mafCurrentOrder) {
			mafIter = 0;
		}
		mafIsComplete = this->movingAverageFilter(
			mafCurrentOrder, mafFrame, mafBuffer
		);
		if (mafIsComplete) {
			std::lock_guard<std::mutex> lock(this->frameLocker);
			cv::cvtColor(mafFrame, this->rgbFrame, cv::COLOR_BGR2RGB);
		}
	}
}


bool Webcam::movingAverageFilter(
	size_t order, cv::Mat& image, 
	std::array<cv::Mat, maxMafOrder>& buffer
) {
	const float weight = 1.0f / order;
	image.setTo(Webcam::nullColor);
	for (size_t i = 0; i < order; i++) {
		const cv::Mat& bufferImage = buffer[i];
		if (bufferImage.empty()) {
			return false;
		}
		image += weight * bufferImage;
	}
	return true;
}


Application::Application(Webcam& webcam, Renderer& renderer) 
	: webcam(&webcam),
	  renderer(&renderer)
{
	this->imguiWindowFlags |= ImGuiWindowFlags_AlwaysAutoResize;
	this->imguiWindowFlags |= ImGuiWindowFlags_NoNavInputs;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_NoSidePreview;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_PickerHueBar;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_Float;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_InputHSV;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_DisplayHSV;
	this->imguiColorEditFlags |= ImGuiColorEditFlags_NoLabel;
	this->imguiSliderFlags |= ImGuiSliderFlags_AlwaysClamp;
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
		this->webcam->setMafOrder(this->mafOrder);
		this->renderer->clear();
		this->initGUIFrame();

		if (imagesAreAcquired) {
			this->renderer->add(this->originalRect);
			this->renderer->add(this->filteredRect);
			this->renderer->updateTexture(this->originalFrame.data, 0);
			this->renderer->updateTexture(this->filteredFrame.data, 1);
		}
		this->addGUIColorPickers();
		this->addGUIWebcamSettings();

		this->renderGUIFrame();
		this->renderer->render();
		this->renderer->present();
	}
	
	// End
	glfwHideWindow(window);
	this->webcam->setActive(false);
}


void Application::createOriginalRect() {
	this->originalRect.vboData = {
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
	this->originalRect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	this->originalRect.move({ -0.5f, 0.0f, 0.0f });
	this->originalRect.scale({ 1.0f, 2.0f, 1.0f });
	this->originalRect.applyTransform();
}


void Application::createFilteredRect() {
	this->filteredRect.vboData = {
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
	this->filteredRect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	this->filteredRect.move({ 0.5f, 0.0f, 0.0f });
	this->filteredRect.scale({ 1.0f, 2.0f, 1.0f });
	this->filteredRect.applyTransform();
}


bool Application::acquireImages() {
	this->outLowerHSV[0] = 180.0f * this->inLowerHSV[0];
	this->outLowerHSV[1] = 255.0f * this->inLowerHSV[1];
	this->outLowerHSV[2] = 255.0f * this->inLowerHSV[2];
	this->outUpperHSV[0] = 180.0f * this->inUpperHSV[0];
	this->outUpperHSV[1] = 255.0f * this->inUpperHSV[1];
	this->outUpperHSV[2] = 255.0f * this->inUpperHSV[2];
	this->webcam->getFrame(this->rgbFrame);
	if (this->rgbFrame.empty()) {
		return false;
	}
	this->filteredFrame.setTo(Webcam::nullColor);
	cv::cvtColor(
		this->rgbFrame, this->originalFrame, cv::COLOR_RGB2RGBA
	);
	cv::cvtColor(
		this->rgbFrame, this->hsvImage, cv::COLOR_RGB2HSV
	);
	cv::inRange(
		this->hsvImage, this->outLowerHSV, this->outUpperHSV, this->hsvMask
	);
	cv::copyTo(
		this->originalFrame, this->filteredFrame, this->hsvMask
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
	ImGui::SameLine();
	ImGui::BeginGroup();
	ImGui::Text("Upper");
	ImGui::ColorPicker3(
		"UpperHSV",
		this->inUpperHSV.data(),
		this->imguiColorEditFlags
	);
	ImGui::EndGroup();
}


void Application::addGUIWebcamSettings() {
	ImGui::SeparatorText("Webcam");
	ImGui::PushItemWidth(0.4f * ImGui::GetWindowWidth());
	if (ImGui::Button("Settings ...")) {
		this->webcam->openSettings();
	}
	ImGui::SameLine();
	ImGui::PushItemWidth(0.6f * ImGui::GetWindowWidth());
	ImGui::SliderInt(
		"MAF Order", &this->mafOrder, 1, 
		Webcam::maxMafOrder, "%d", this->imguiSliderFlags
	);
}


void Application::renderGUIFrame() const {
	ImGui::End();
	ImGui::Render();
}