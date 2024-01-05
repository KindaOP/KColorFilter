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
		void getFrame(cv::Mat& image);
	private:
		void streamingThreadLoop();
	private:
		unsigned int cameraId = 0;
		int width = 0;
		int height = 0;
		cv::VideoCapture camera;
		mutable std::mutex frameLocker;
		cv::Mat rgbFrame = cv::Mat();
		mutable std::mutex activeLocker;
		bool stateActive = false;
	};


	class Application {
	public:
		Application(Renderer& renderer);
		~Application() = default;
		void run();
	private:
		void createRectangles();
		void initGUIFrame() const;
		void addGUIColorPickers();
		void renderGUIFrame() const;
	private:
		Renderer* renderer = nullptr;
		ImGuiWindowFlags imguiWindowFlags = 0;
		ImGuiColorEditFlags imguiColorEditFlags = 0;
		std::array<float, 3> initHSV = { 0.6f, 1.0f, 1.0f };
		std::array<float, 3> finalHSV = { 0.7f, 1.0f, 1.0f };
		Webcam webcam = Webcam(0, 10000, 10000);
		Object originalFrameRect;
		Object filteredFrameRect;
	};

}