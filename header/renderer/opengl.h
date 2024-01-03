#pragma once
#include "renderer.h"


namespace kop {

	class OpenGL : public Renderer {
	public:
		OpenGL();
		~OpenGL() override;
		int getNumber() const override;
	};

}