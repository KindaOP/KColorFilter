#pragma once


namespace kop {

	class Renderer {
	public:
		Renderer();
		virtual ~Renderer();
		virtual int getNumber() const = 0;
	};

}