#include "application.h"

#ifdef NDEBUG
const bool IS_DEBUG = false;
#else
const bool IS_DEBUG = true;
#endif

using namespace kop;


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
	this->createRectangle();
	GLFWwindow* window = this->renderer->getWindow();
	glfwShowWindow(window);
	while (!glfwWindowShouldClose(window)) {
		this->renderer->clear();
		this->initGUIFrame();

		this->renderer->add(rect);
		this->addGUIColorPickers();

		this->renderGUIFrame();
		this->renderer->render();
		this->renderer->present();
	}
	
	// End
	glfwHideWindow(window);
}


void Application::createRectangle() {
	this->rect.vboData = {
		{{0.5f, 0.5f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}},
	};
	this->rect.eboData = {
		0, 1, 2,
		0, 3, 2,
	};
	rect.move({ -0.5f, 0.0f, 0.0f });
	rect.scale({ 1.0f, 2.0f, 1.0f });
	rect.applyTransform();
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