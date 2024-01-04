#pragma once
#include "renderer.h"

#define __KOP_RENDERER_TYPE__ OpenGL


namespace kop {

	class OpenGL : public Renderer {
	public:
		OpenGL();
		~OpenGL() override;
		int getNumber() const override;
	};

}