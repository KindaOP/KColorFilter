#pragma once
#include "renderer.h"


namespace kop {

	class Application {
	public:
		Application(Renderer& renderer);
		~Application();
		void run() const;
	private:
		Renderer* renderer = nullptr;
	};

}