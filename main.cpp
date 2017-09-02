#include "Draw.hpp"
#include "GL.hpp"

#include <SDL.h>
#include <glm/glm.hpp>

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

int main(int argc, char **argv) {
	//Configuration:
	struct {
		std::string title = "Game0: Hammer";
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
	//SDL_ShowCursor(SDL_DISABLE);
	//SDL_SetRelativeMouseMode(SDL_TRUE);

	//------------  game state ------------
	const float length = 25.0f;
	const float width = length / config.size.x * 2.0f;
	const float height = length / config.size.y * 2.0f;
	const float spawn_delay_decrease = 0.01f;
	const float drain_increase = 0.001f;
	const float target_speed_increase = 0.01f;
	const float min_spawn_delay = 0.25f;
	const float max_drain = 0.3f;
	const float max_target_speed = 2.5f;
	const float max_hammer_length = 4.0f;
	const float gain = 0.5f;
	float hammer_length = max_hammer_length;
	float spawn_timer = 0.0f;
	float spawn_delay = 0.5f;
	float target_speed = 1.0f;
	float drain = 0.1f;
	unsigned int points = 0;
	int state = 0;

	glm::vec2 hammer(0.0, 0.0f);
	std::vector<glm::vec4> targets;

	// http://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> loc_dist(-1.0f + 0.5f * width, std::nextafterf(1.0f - 0.5f * width, FLT_MAX));
	std::uniform_real_distribution<float> angle_dist((float)M_PI / 3.0f, std::nextafterf(2.0f * (float)M_PI / 3.0f, FLT_MAX)); // 45 deg to 135 deg
	std::uniform_real_distribution<float> speed_dist(-0.1f, std::nextafterf(0.0f, FLT_MAX)); // variance in speed
	std::uniform_int_distribution<int> dist(0, 1);
	
	//------------  game loop ------------

	auto previous_time = std::chrono::high_resolution_clock::now();
	bool should_quit = false;
	while (true) {
		static SDL_Event evt;
		while (SDL_PollEvent(&evt) == 1) {
			//handle input:
			if (evt.type == SDL_MOUSEMOTION) {
				if (state == 1)
					hammer.x = (evt.motion.x + 0.5f) / config.size.x * 2.0f - 1.0f;
			} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
				if (state == 0)
					state = 1;
			} else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE) {
				should_quit = true;
			} else if (evt.type == SDL_QUIT) {
				should_quit = true;
			}
		}
		if (should_quit) break;

		auto current_time = std::chrono::high_resolution_clock::now();
		float elapsed = min(1.0f / 60.0f, std::chrono::duration< float >(current_time - previous_time).count());
		previous_time = current_time;

		{ //update game state:
			// drain
			if (state > 0)
			{
				float health = hammer_length;
				if (state == 1)
				{
					if (hammer.x > 1.0f - 0.5f * hammer_length * width)
						hammer.x = 1.0f - 0.5f * hammer_length * width;
					else if (hammer.x < -1.0f + 0.5f * hammer_length * width)
						hammer.x = -1.0f + 0.5f * hammer_length * width;

					health -= elapsed * drain;
					// hulk smash
					for (auto target = targets.begin(); target != targets.end(); )
					{
						if (target->y < height && target->y > -height && target->x - 0.5f * width < hammer.x + 0.5f * hammer_length * width && target->x + 0.5f * width > hammer.x - 0.5f * hammer_length * width)
						{
							target = targets.erase(target);
							health += gain;
							if (spawn_delay > min_spawn_delay)
								spawn_delay -= spawn_delay_decrease;
							if (drain < max_drain)
								drain += drain_increase;
							if (target_speed < max_target_speed)
								target_speed += target_speed_increase;
							points++;
							std::cout << points << std::endl;
						}
						else
							target++;
					}

					// spawn targets
					spawn_timer -= elapsed;
					while (spawn_timer <= 0.0f)
					{
						float angle = angle_dist(gen);
						float x = loc_dist(gen);
						float vx = cosf(angle) * target_speed;
						float vy = sinf(angle) * target_speed;
						if (targets.size() < 25)
						{
							if (dist(gen))
								targets.push_back(glm::vec4(x, 1.0f - 0.5f * height, vx, -vy));
							else
								targets.push_back(glm::vec4(x, -1.0f + 0.5f * height, vx, vy));
						}

						spawn_timer += spawn_delay;
					}
				}

				for (auto target = targets.begin(); target != targets.end(); )
				{
					float dx = elapsed * target->z;
					float dy = elapsed * target->w;
					target->x += dx;
					target->y += dy;
					// maybe reverse off wall
					if (target->x <= -1.0f + 0.5f * width)
						target->z = std::abs(target->z);
					else if (target->x >= 1.0f - 0.5f * width)
						target->z = -std::abs(target->z);
					if (target->y <= -1.0f + 0.5f * height) // MAYBE PASS THROUGH INSTEAD.
						target->w = std::abs(target->w);
					else if (target->y >= 1.0f - 0.5f * height)
						target->w = -std::abs(target->w);
					if (state == 1 && target->y < height && target->y > -height && target->x - 0.5f * width < hammer.x + 0.5f * hammer_length * width && target->x + 0.5f * width > hammer.x - 0.5f * hammer_length * width)
					{
						target = targets.erase(target);
						health -= drain;
					}
					else
						target++;
				}

				hammer_length = min(health, max_hammer_length);
				if (hammer_length <= 0.0f)
				{
					hammer_length = 0.0f;
					state = 2;
				}
			}
		}

		//draw output:
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		{ //draw game state:
			Draw draw;
			if (hammer_length > 1.0f)
				draw.add_rectangle(hammer - glm::vec2(0.5f * hammer_length * width, 0.5f * height), hammer + glm::vec2(0.5f * hammer_length * width, 0.5f * height), glm::u8vec4(0xFF, 0xFF, 0xFF, 0xFF));
			else
				draw.add_rectangle(hammer - glm::vec2(0.5f * width, 0.5f * height), hammer + glm::vec2(0.5f * width, 0.5f * height), glm::u8vec4(0xFF, (int)(0xFF * hammer_length), (int)(0xFF * hammer_length), 0xFF));
			draw.add_rectangle(glm::vec2(-1.0f, 0.6f * height), glm::vec2(1.0f, 0.5f * height), glm::u8vec4(0x00, 0x00, 0xFF, 0xFF));
			draw.add_rectangle(glm::vec2(-1.0f, -0.6f * height), glm::vec2(1.0f, -0.5f * height), glm::u8vec4(0x00, 0x00, 0xFF, 0xFF));
			for (glm::vec4 target : targets)
				draw.add_rectangle(glm::vec2(target.x - 0.5f * width, target.y - 0.5f * height), glm::vec2(target.x + 0.5f * width, target.y + 0.5f * width), glm::u8vec4(0xFF, 0xFF, 0xFF, 0xFF));
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
