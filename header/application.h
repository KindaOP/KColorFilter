#pragma once
#include "renderer.h"
#include <opencv2/videoio.hpp>
#include <imgui.h>
#include <array>
#include <mutex>


namespace kop {

	class Webcam {
	public:
		Webcam(
			unsigned int cameraId,
			int width,
			int height
		);
		~Webcam();
		bool isActive() const;
		void setActive(bool newState);
		int getWidth() const;
		int getHeight() const;
		void getFrame(cv::Mat& image) const;
		void openSettings();
		void setMafOrder(size_t order);
	public:
		static constexpr size_t maxMafOrder = 5;
		static const cv::Scalar nullColor;
	private:
		void streamingThread();
		void threadLoop();
	private:
		unsigned int cameraId = NULL;
		int width = NULL;
		int height = NULL;
		cv::VideoCapture camera;
		mutable std::mutex activeLocker;
		bool stateActive = false;
		mutable std::mutex frameLocker;
		cv::Mat rgbFrame = cv::Mat();
		mutable std::mutex orderLocker;
		size_t mafOrder = 1;
	private:
		static bool movingAverageFilter(
			size_t order, cv::Mat& image,
			const std::array<cv::Mat, maxMafOrder>& buffer
		);
	};


	class Application {
	public:
		Application(Webcam& webcam, Renderer& renderer);
		~Application() = default;
		void run();
	public:
		static constexpr size_t fpsUpdateFrameCount = 60;
	private:
		void createOriginalRect();
		void createFilteredRect();
		bool acquireImages();
		void initGUIFrame() const;
		void addGUIColorPickers();
		void addGUIWebcamSettings();
		void renderGUIFrame() const;
	private:
		Webcam* webcam = nullptr;
		Renderer* renderer = nullptr;
		ImGuiWindowFlags imguiWindowFlags = NULL;
		ImGuiColorEditFlags imguiColorEditFlags = NULL;
		ImGuiSliderFlags imguiSliderFlags = NULL;
		std::array<float, 3> inLowerHSV = { 0.00f, 0.20f, 0.20f };
		std::array<float, 3> inUpperHSV = { 0.15f, 1.00f, 1.00f };
		std::array<float, 3> outLowerHSV = { NULL, NULL, NULL };
		std::array<float, 3> outUpperHSV = { NULL, NULL, NULL };
		cv::Mat rgbFrame;
		cv::Mat blurredFrame;
		cv::Mat hsvImage;
		cv::Mat hsvMask;
		cv::Mat originalFrame;
		cv::Mat filteredFrame;
		Object originalRect;
		Object filteredRect;
		int mafOrder = 1;
	};

}