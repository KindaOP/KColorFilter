#pragma once
#include "renderer.h"


namespace kop {

	class DirectX12 : public Renderer {
	public:
		DirectX12();
		~DirectX12() override;
		int getNumber() const override;
	};

}