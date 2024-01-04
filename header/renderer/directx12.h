#pragma once
#include "renderer.h"

#define __KOP_RENDERER_TYPE__ DirectX12


namespace kop {

	class DirectX12 : public Renderer {
	public:
		DirectX12();
		~DirectX12() override;
		int getNumber() const override;
	};

}