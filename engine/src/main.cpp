#define SDL_MAIN_HANDLED

#include <core/engine.h>
#include <CLI/CLI.hpp>


int main(int argc, char* argv[]) {
	// TODO: CLI11ベースで作る
	CLI::App app{"Scalix : Copyright (c) 2026 yy981"};
	auto sub_ui = app.add_subcommand("ui", "rmlui Test mode");
	auto sub_smokeTest = app.add_subcommand("smokeTest", "smoke test mode");

	CLI11_PARSE(app, argc, argv);



	if (*sub_ui) {

	} else if (*sub_smokeTest) {

	} else {
		Engine engine;
		while (engine.isRunning()) {
			engine.tick();
		}
	}
}
