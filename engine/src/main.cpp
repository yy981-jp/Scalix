#define SDL_MAIN_HANDLED

#include <core/engine.h>


int main() {
	{
		Engine engine;

		while (engine.isRunning()) {
			engine.tick();
		}
	}
	bgfx::shutdown();
}
