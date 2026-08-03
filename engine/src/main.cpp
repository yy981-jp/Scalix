#define SDL_MAIN_HANDLED

#include <core/engine.h>


int main() {
	{
		Engine game;

		while (game.isRunning()) {
			game.tick();
		}
	}
	bgfx::shutdown();
}
