#pragma once
#include "renderer.h"


namespace kop {

	class Vulkan : public Renderer {
	public:
		Vulkan();
		~Vulkan() override;
		int getNumber() const override;
	};

}