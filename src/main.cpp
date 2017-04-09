#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <chrono>
#include "util.hpp"
#include <thread>
#include "OpenCL.h"
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"
#include <experimental/filesystem>
#include "Decoder.h"

float elap_time() {
	static std::chrono::time_point<std::chrono::system_clock> start;
	static bool started = false;

	if (!started) {
		start = std::chrono::system_clock::now();
		started = true;
	}

	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_time = now - start;
	return static_cast<float>(elapsed_time.count());
}

const int WINDOW_X = 1920;
const int WINDOW_Y = 1080;

void generate_nodes(sf::Uint8* nodes) {

	int x_divisor = rand() % 49 + 1;
	int y_divisor = rand() % 10 + 1;

	for (int i = 0; i < WINDOW_X * WINDOW_Y; i += 1) {

		sf::Vector2i pos(i % WINDOW_X, i / WINDOW_X);

		if ((pos.x % x_divisor == 0) || (pos.y % y_divisor == 0))
			nodes[i] = 1;
		else
			nodes[i] = 0;
	}

	for (int i = 0; i < WINDOW_X * WINDOW_Y; i += 1) {

		sf::Vector2i pos(i % WINDOW_X, i / WINDOW_X);

		if (rand() % 200 == 0)
			nodes[i] = 1;

	}

}

void copy_pattern (sf::Uint8* nodes, sf::Vector2i dimensions, sf::Vector2u position, pattern_info pattern) {
	
	for (int x = 0; x < pattern.dimensions.x; x++) {
		for (int y = 0; y < pattern.dimensions.y; y++) {

			if (position.x + pattern.dimensions.x < dimensions.x ||
				position.y + pattern.dimensions.y < dimensions.y  ) {
				
				nodes[(y+position.y) * dimensions.x + (x+position.x)] = pattern.nodes[y * pattern.dimensions.x + x];
			}
		}
	}
}

void clear_nodes(sf::Uint8 *nodes, sf::Vector2i dimensions) {
	for (int i = 0; i < dimensions.x * dimensions.y; i++) {
		nodes[i] = 0;
	}
}

int main() {

	srand(time(NULL));

	Decoder d;

	std::vector<const char*> pattern_list = d.getPatternList();

	sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "conways-game-of-life-opencl");

	int simulation_speed = 100;
	window.setFramerateLimit(simulation_speed);

	ImGui::SFML::Init(window);
	window.resetGLStates();

	float physic_step = 0.0166f;
	float physic_time = 0.0f;
	double frame_time = 0.0, elapsed_time = 0.0, delta_time = 0.0, accumulator_time = 0.0, current_time = 0.0;
	
	OpenCL cl;

	if (!cl.init())
		return -1;
	
	while (!cl.compile_kernel("../kernels/conways.cl", "conways")) {
		std::cin.get();
	}

	sf::Vector2i image_resolution(WINDOW_X, WINDOW_Y);
	cl.create_image_buffer("viewport_image", image_resolution, sf::Vector2f(0, 0), CL_MEM_READ_WRITE);
	cl.create_buffer("image_res", sizeof(sf::Vector2i), &image_resolution);

	sf::Uint8* nodes = new sf::Uint8[WINDOW_X * WINDOW_Y];
	generate_nodes(nodes);

	//load_rle(nodes, sf::Vector2i(WINDOW_X, WINDOW_Y), "../assets/snark.rle");
	cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
	cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);

	char buffer_flip = 0;
	cl.create_buffer("buffer_flip", sizeof(char), (void*)&buffer_flip, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR);

	cl.set_kernel_arg("conways", 0, "image_res");
	cl.set_kernel_arg("conways", 1, "viewport_image");
	cl.set_kernel_arg("conways", 2, "first_node_buffer");
	cl.set_kernel_arg("conways", 3, "second_node_buffer");
	cl.set_kernel_arg("conways", 4, "buffer_flip");

	sf::Clock sf_delta_clock;
	fps_counter render_fps;

	int c = 0;

	while (window.isOpen())
	{
		sf::Event event; // Handle input
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::Down) {

				}
				if (event.key.code == sf::Keyboard::Up) {
			
				}
				if (event.key.code == sf::Keyboard::Right) {
					
				}
				if (event.key.code == sf::Keyboard::Left) {
				
				}
				if (event.key.code == sf::Keyboard::Equal) {
				
				}
				if (event.key.code == sf::Keyboard::Dash) {
			
				}
			}
		}

		elapsed_time = elap_time(); // Handle time
		delta_time = elapsed_time - current_time;
		current_time = elapsed_time;
		if (delta_time > 0.05f)
			delta_time = 0.05f;
		accumulator_time += delta_time;

		while (accumulator_time >= physic_step) { // While the frame has sim time, update 
			accumulator_time -= physic_step;
			physic_time += physic_step;			
		}

		ImGui::SFML::Update(window, sf_delta_clock.restart());
		render_fps.frame(delta_time);


		window.clear(sf::Color::White);


		ImGui::Begin("Sim");

		if (ImGui::SliderInt("Simulation Speed", &simulation_speed, 30, 500)) {
			window.setFramerateLimit(simulation_speed);
		}
		if (ImGui::Button("One shot")) {
			
			std::cout << "sim" << std::endl;

			/*std::cout << buffer_flip << std::endl;
			cl.map_buffer("buffer_flip", sizeof(char), &buffer_flip);*/
		}
		cl.run_kernel("conways", image_resolution);
		cl.draw(&window);
		if (buffer_flip == 1)
			buffer_flip = 0;
		else
			buffer_flip = 1;

		ImGui::Columns(2);

		if (ImGui::Button("Load Pattern")) {
			clear_nodes(nodes, sf::Vector2i(WINDOW_X, WINDOW_Y));
			pattern_info p = d.decodePattern(pattern_list.at(c));
			copy_pattern(nodes, sf::Vector2i(WINDOW_X, WINDOW_Y), sf::Vector2u(100, 300), p);
			cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.set_kernel_arg("conways", 2, "first_node_buffer");
			cl.set_kernel_arg("conways", 3, "second_node_buffer");
		}

		if (ImGui::ListBox("", &c, pattern_list.data(), pattern_list.size(), 30)) {

		}

		ImGui::NextColumn();

		if (ImGui::Button("Rerun")) {
			generate_nodes(nodes);
			cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.set_kernel_arg("conways", 2, "first_node_buffer");
			cl.set_kernel_arg("conways", 3, "second_node_buffer");
		}

		ImGui::End();

		render_fps.draw();

		ImGui::Render();


		

		


		// ImGui screws stuff up after the render, rendering a drawable resets it
		window.draw(sf::CircleShape(0));
		window.display();

	}
	return 0;

}
