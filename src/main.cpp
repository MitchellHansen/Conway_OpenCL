
#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <chrono>
#include "util.hpp"
#include <thread>
#include "OpenCL.h"

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

	for (int i = 0; i < WINDOW_X * WINDOW_Y; i += 1) {
		if (rand() % 10 > 8)
			nodes[i] = 1;
		else
			nodes[i] = 0;
	}

}

int main() {

	sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "conways-game-of-life-opencl");
	window.setFramerateLimit(60);

	float physic_step = 0.166f;
	float physic_time = 0.0f;
	double frame_time = 0.0, elapsed_time = 0.0, delta_time = 0.0, accumulator_time = 0.0, current_time = 0.0;
	
	OpenCL cl;

	if (!cl.init())
		return -1;
	
	while (!cl.compile_kernel("../kernels/conways.cl", "conways")) {
		std::cin.get();
	}

	sf::Vector2i image_resolution(WINDOW_X, WINDOW_Y);
	cl.create_image_buffer("viewport_image", image_resolution, sf::Vector2f(0, 0), CL_MEM_WRITE_ONLY);
	cl.create_buffer("image_res", sizeof(sf::Vector2i), &image_resolution);

	sf::Uint8* nodes = new sf::Uint8[WINDOW_X * WINDOW_Y];
	generate_nodes(nodes);
	nodes[0] = 1;
	cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
	cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);

	char buffer_flip = 0;
	cl.create_buffer("buffer_flip", sizeof(char), (void*)&buffer_flip, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR);

	cl.set_kernel_arg("conways", 0, "image_res");
	cl.set_kernel_arg("conways", 1, "viewport_image");
	cl.set_kernel_arg("conways", 2, "first_node_buffer");
	cl.set_kernel_arg("conways", 3, "second_node_buffer");
	cl.set_kernel_arg("conways", 4, "buffer_flip");

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
		if (delta_time > 0.02f)
			delta_time = 0.02f;
		accumulator_time += delta_time;

		while (accumulator_time >= physic_step) { // While the frame has sim time, update 
			accumulator_time -= physic_step;
			physic_time += physic_step;
		}

		window.clear(sf::Color::White);
	
		cl.run_kernel("conways", image_resolution);
		//cl.run_kernel("conways", sf::Vector2i(5, 5));
		cl.draw(&window);
		
		if (buffer_flip == 1)
			buffer_flip = 0;
		else
			buffer_flip = 1;

		window.display();

	}
	return 0;

}
