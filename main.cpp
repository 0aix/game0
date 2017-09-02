#include "Draw.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <chrono>
#include <iostream>
#include <vector>

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game0: Stacking - Level 1";
		glm::uvec2 size = glm::uvec2(640, 480);
	} config;

	//------------  initialization ------------

	//Initialize SDL library:
	SDL_Init(SDL_INIT_VIDEO);

	//Ask for an OpenGL context version 3.3, core profile, enable debug:
	SDL_GL_ResetAttributes();
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	//create window:
	SDL_Window *window = SDL_CreateWindow(
		config.title.c_str(),
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		config.size.x, config.size.y,
		SDL_WINDOW_OPENGL/*| SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI*/
	);

	if (!window) {
		std::cerr << "Error creating SDL window: " << SDL_GetError() << std::endl;
		return 1;
	}

	//Create OpenGL context:
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!context) {
		SDL_DestroyWindow(window);
		std::cerr << "Error creating OpenGL context: " << SDL_GetError() << std::endl;
		return 1;
	}

	#ifdef _WIN32
	//On windows, load OpenGL extensions:
	if (!init_gl_shims()) {
		std::cerr << "ERROR: failed to initialize shims." << std::endl;
		return 1;
	}
	#endif

	//Set VSYNC + Late Swap (prevents crazy FPS):
	if (SDL_GL_SetSwapInterval(-1) != 0) {
		std::cerr << "NOTE: couldn't set vsync + late swap tearing (" << SDL_GetError() << ")." << std::endl;
		if (SDL_GL_SetSwapInterval(1) != 0) {
			std::cerr << "NOTE: couldn't set vsync (" << SDL_GetError() << ")." << std::endl;
		}
	}

	//Hide mouse cursor (note: showing can be useful for debugging):
	SDL_ShowCursor(SDL_DISABLE);

	//------------  game state ------------
	const float length = 40.0f;
	const float min_width = 1.0f / config.size.x * 2.0f;
	const float width = 4.0f * length / config.size.x * 2.0f;
	const float height = length / config.size.y * 2.0f;
	const float level_speed_inc = 0.1f;
	const float starting_speed_inc = 0.2f;
	const int max_level = (int)(config.size.y / length);
	float starting_speed = 1.0f;
	float level_speed = starting_speed;
	float level_width = width;
	glm::vec2 level(0.0f, -1.0f + 0.5f * height);
	bool died = false;
	int levelnum = 1;

	std::vector<glm::vec4> tower;
	
	//------------  game loop ------------

	auto previous_time = std::chrono::high_resolution_clock::now();
	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEBUTTONDOWN && evt.button.button == SDL_BUTTON_LEFT) {
				if (died)
				{
					if (level_speed > 0.0f)
						level_speed = starting_speed;
					else
						level_speed = -starting_speed;
					level_width = width;
					level.y = -1.0f + 0.5f * height;
					died = false;
					tower.clear();
				}
				else if (tower.size() == max_level)
				{
					starting_speed += starting_speed_inc;
					if (level_speed > 0.0f)
						level_speed = starting_speed;
					else
						level_speed = -starting_speed;
					level_width = width;
					level.y = -1.0f + 0.5f * height;
					tower.clear();

					char buffer[256];
					sprintf_s(buffer, "Game0: Stacking - Level %d", ++levelnum);
					SDL_SetWindowTitle(window, buffer);
				}
				else
				{
					float left = level.x - 0.5f * level_width;
					float right = level.x + 0.5f * level_width;
					if (!tower.empty())
					{
						glm::vec4 prev = tower[tower.size() - 1];
						if (left < prev.x && prev.x < right)
						{
							level_width = right - prev.x;
							if (level_width < min_width)
							{
								level_width = min_width;
								right = prev.x + min_width;
							}
							left = prev.x;
						}
						else if (left < prev.z && prev.z < right)
						{
							level_width = prev.z - left;
							if (level_width < min_width)
							{
								level_width = min_width;
								left = prev.z - min_width;
							}
							right = prev.z;
						}
						else
						{
							std::cout << "You lose!" << std::endl;
							died = true;
							continue;
						}
					}
					tower.push_back(glm::vec4(left, level.y - 0.475f * height, right, level.y + 0.475f * height));
					if (tower.size() == max_level)
						std::cout << "You win!" << std::endl;
					if (level_speed > 0.0f)
						level_speed += level_speed_inc;
					else
						level_speed -= level_speed_inc;
					level.y += height;
				}
			} else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} else if (evt.type == SDL_QUIT) {
				should_quit = true;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration< float >(current_time - previous_time).count();
		previous_time = current_time;

		{ //update game state:
			if (!died && tower.size() != max_level)
			{
				level.x += elapsed * level_speed;
				while (true)
				{
					if (level.x > 1.0f - 0.5f * level_width)
					{
						level.x = 2.0f * (1.0f - 0.5f * level_width) - level.x;
						level_speed = -level_speed;
					}
					else if (level.x < -1.0f + 0.5f * level_width)
					{
						level.x = 2.0f * (-1.0f + 0.5f * level_width) - level.x;
						level_speed = -level_speed;
					}
					else
						break;
				}
			}
		}

		//draw output:
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		{ //draw game state:
			Draw draw;
			if (tower.size() == max_level)
				for (auto level : tower)
					draw.add_rectangle(glm::vec2(level.x, level.y), glm::vec2(level.z, level.w), glm::vec4(0x00, 0xFF, 0x00, 0xFF));
			else
			{
				if (!died)
					draw.add_rectangle(level - glm::vec2(0.5f * level_width, 0.475f * height), level + glm::vec2(0.5f * level_width, 0.475f * height), glm::u8vec4(0xFF, 0xFF, 0xFF, 0xFF));
				else
					draw.add_rectangle(level - glm::vec2(0.5f * level_width, 0.475f * height), level + glm::vec2(0.5f * level_width, 0.475f * height), glm::u8vec4(0xFF, 0x00, 0x00, 0xFF));

				for (auto level : tower)
					draw.add_rectangle(glm::vec2(level.x, level.y), glm::vec2(level.z, level.w), glm::vec4(0xFF, 0xFF, 0xFF, 0xFF));
			}
			
			draw.draw();
		}


		SDL_GL_SwapWindow(window);
	}


	//------------  teardown ------------

	SDL_GL_DeleteContext(context);
	context = 0;

	SDL_DestroyWindow(window);
	window = NULL;

	return 0;
}
