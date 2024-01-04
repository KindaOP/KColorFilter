#pragma once
#include "renderer.h"

#define __KOP_RENDERER_TYPE__ Vulkan


namespace kop {

	class Vulkan : public Renderer {
	public:
		Vulkan();
		~Vulkan() override;
		int getNumber() const override;
	};

}