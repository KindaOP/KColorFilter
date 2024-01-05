#pragma once
#include "renderer.h"


namespace kop {

	class DirectX12 : public Renderer {
	public:
		DirectX12(
			const char* vertexShaderPath,
			const char* fragmentShaderPath,
			size_t maxVertices,
			size_t maxElements
		);
		~DirectX12() override;
		const char* getWindowName() const override;
	private:
		void createWindow() override;
		void createShaderProgram() override;
	};

}