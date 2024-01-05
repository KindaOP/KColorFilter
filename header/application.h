#pragma once
#include "renderer.h"
#include <imgui.h>
#include <array>


namespace kop {

	class Application {
	public:
		Application(Renderer& renderer);
		~Application() = default;
		void run();
	private:
		void createRectangle();
		void initGUIFrame() const;
		void addGUIColorPickers();
		void renderGUIFrame() const;
	private:
		Renderer* renderer = nullptr;
		ImGuiWindowFlags imguiWindowFlags = 0;
		ImGuiColorEditFlags imguiColorEditFlags = 0;
		std::array<float, 3> initHSV = { 0.6f, 1.0f, 1.0f };
		std::array<float, 3> finalHSV = { 0.7f, 1.0f, 1.0f };
		Object rect;
	};

}