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
	public:
		mutable std::mutex frameLocker;
		cv::Mat rgbFrame = cv::Mat();
	private:
		void streamingThreadLoop();
	private:
		unsigned int cameraId = NULL;
		int width = NULL;
		int height = NULL;
		cv::VideoCapture camera;
		mutable std::mutex activeLocker;
		bool stateActive = false;
	};


	class Application {
	public:
		Application(Webcam& webcam, Renderer& renderer);
		~Application() = default;
		void run();
	private:
		void createRectangles();
		bool acquireImages();
		void initGUIFrame() const;
		void addGUIColorPickers();
		void renderGUIFrame() const;
	private:
		Webcam* webcam = nullptr;
		Renderer* renderer = nullptr;
		ImGuiWindowFlags imguiWindowFlags = NULL;
		ImGuiColorEditFlags imguiColorEditFlags = NULL;
		std::array<float, 3> inLowerHSV = { 0.6f, 1.0f, 1.0f };
		std::array<float, 3> inUpperHSV = { 0.7f, 1.0f, 1.0f };
		std::array<float, 3> outLowerHSV = { NULL, NULL, NULL };
		std::array<float, 3> outUpperHSV = { NULL, NULL, NULL };
		cv::Mat hsvImage = cv::Mat();
		cv::Mat hsvMask = cv::Mat();
		Object originalFrameRect;
		Object filteredFrameRect;
	};

}