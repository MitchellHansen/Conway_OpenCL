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

std::unique_ptr<sf::Texture> generate_pattern_texture(pattern_info pattern) {
	
	sf::Vector2i dimensions = pattern.dimensions;

	std::vector<sf::Uint8> pixels(dimensions.x * dimensions.y * 4, 0);

	for (int i = 0; i < dimensions.x * dimensions.y; i += 4) {
		
		if (pattern.nodes[i] != 0) {
			pixels.at(i + 0) = 125;
			pixels.at(i + 1) = 125;
			pixels.at(i + 2) = 125;
			pixels.at(i + 3) = 125;
		}
	}


	std::unique_ptr<sf::Texture> texture(new sf::Texture);
	texture->create(dimensions.x, dimensions.y);
	texture->update(pixels.data());

	return std::move(texture);

}

int main() {

	srand(time(NULL));

	// Load and setup the windows state
	sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "conways-game-of-life-opencl");

	int simulation_speed = 100;
	window.setFramerateLimit(simulation_speed);

	ImGui::SFML::Init(window);
	window.resetGLStates();
	

	// start up OpenCL
	OpenCL cl;

	if (!cl.init())
		return -1;
	
	while (!cl.compile_kernel("../kernels/conways.cl", "conways")) 
		std::cin.get();

	sf::Vector2i image_resolution(WINDOW_X, WINDOW_Y);
	cl.create_buffer("image_res", sizeof(sf::Vector2i), &image_resolution);
	cl.create_image_buffer("viewport_image", image_resolution, sf::Vector2f(0, 0), CL_MEM_READ_WRITE);
	
	sf::Uint8* nodes = new sf::Uint8[WINDOW_X * WINDOW_Y];
	cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
	cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);

	char buffer_flip = 0;
	cl.create_buffer("buffer_flip", sizeof(char), (void*)&buffer_flip, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR);

	cl.set_kernel_arg("conways", 0, "image_res");
	cl.set_kernel_arg("conways", 1, "viewport_image");
	cl.set_kernel_arg("conways", 2, "first_node_buffer");
	cl.set_kernel_arg("conways", 3, "second_node_buffer");
	cl.set_kernel_arg("conways", 4, "buffer_flip");

	Decoder d;
	std::vector<const char*> pattern_list = d.getPatternList();

	pattern_info pattern_info;
	sf::Sprite pattern_sprite;
	sf::Texture pattern_texture;

	sf::Clock sf_delta_clock;
	fps_counter render_fps;

	int pattern_number = 0;

	enum MouseState { PRESSED , DEPRESSED };
	MouseState mouse_state = DEPRESSED;

	sf::Vector2i mouse_position;
	float zoom_level = 1.0f;

	float physic_step = 0.0166f;
	float physic_time = 0.0f;
	double frame_time = 0.0, elapsed_time = 0.0, delta_time = 0.0, accumulator_time = 0.0, current_time = 0.0;

	while (window.isOpen())
	{
		sf::Event event; // Handle input
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			else if (event.type == sf::Event::MouseButtonPressed) {
				if (event.mouseButton.button == sf::Mouse::Right) {
					mouse_state = PRESSED;
					mouse_position = sf::Mouse::getPosition();
				}
				else if (event.mouseButton.button == sf::Mouse::Middle) {
					mouse_position = sf::Mouse::getPosition();
					copy_pattern(nodes, sf::Vector2i(WINDOW_X, WINDOW_Y), static_cast<sf::Vector2u>(mouse_position), pattern_info);
					cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
					cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
					cl.set_kernel_arg("conways", 2, "first_node_buffer");
					cl.set_kernel_arg("conways", 3, "second_node_buffer");

				}
			}
			else if (event.type == sf::Event::MouseButtonReleased) {
				if (event.mouseButton.button == sf::Mouse::Right) {
					mouse_state = DEPRESSED;
				}
			}
			else if (event.type == sf::Event::MouseWheelScrolled) {
				zoom_level -= event.mouseWheelScroll.delta / 10;
				sf::View v = window.getView();
				v.setSize(static_cast<sf::Vector2f>(window.getSize()) * zoom_level);
				window.setView(v);
			}
			else if (event.type == sf::Event::MouseMoved) {
				if (mouse_state == PRESSED) {

					sf::Vector2i mouse_delta = mouse_position - sf::Mouse::getPosition();
					mouse_position -= mouse_delta;

					sf::View v = window.getView();
					auto center = v.getCenter() + (static_cast<sf::Vector2f>(mouse_delta) * zoom_level);
					v.setCenter(center);
					window.setView(v);

				}


				pattern_sprite.setPosition(mouse_position.x, mouse_position.y);
				

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


		window.clear(sf::Color(0x696969FF));

		ImGui::Begin("Sim");

		if (ImGui::SliderInt("Simulation Speed", &simulation_speed, 30, 500)) {
			window.setFramerateLimit(simulation_speed);
		}

		cl.run_kernel("conways", image_resolution);
		cl.draw(&window);
		if (buffer_flip == 1)
			buffer_flip = 0;
		else
			buffer_flip = 1;


		if (ImGui::Button("Clear Grid")) {
			clear_nodes(nodes, sf::Vector2i(WINDOW_X, WINDOW_Y));
			cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.set_kernel_arg("conways", 2, "first_node_buffer");
			cl.set_kernel_arg("conways", 3, "second_node_buffer");
		}

		if (ImGui::Button("Load Pattern")) {
			pattern_info = d.decodePattern(pattern_list.at(pattern_number));

			sf::Vector2i dimensions = pattern_info.dimensions;

			sf::Uint8* pixels = new sf::Uint8[dimensions.x * dimensions.y * 4]{ 0 };

			for (int i = 0; i < dimensions.x * dimensions.y; i += 4) {

				if (pattern_info.nodes[i] != 0) {
					pixels[i + 0] = 0;
					pixels[i + 1] = 0;
					pixels[i + 2] = 255;
					pixels[i + 3] = 255;
				}
			}

			if (pattern_texture.create(dimensions.x, dimensions.y)) {
				pattern_texture.update(pixels);
				pattern_sprite.setTexture(pattern_texture, true);
			}

		}

		if (ImGui::Button("Rerun")) {
			generate_nodes(nodes);
			cl.create_buffer("first_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.create_buffer("second_node_buffer", WINDOW_X * WINDOW_Y, (void*)nodes, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR);
			cl.set_kernel_arg("conways", 2, "first_node_buffer");
			cl.set_kernel_arg("conways", 3, "second_node_buffer");
		}

		if (ImGui::ListBox("", &pattern_number, pattern_list.data(), pattern_list.size(), 30)) {

		}



		ImGui::End();

		render_fps.draw();

		ImGui::Render();

		// ImGui screws stuff up after the render, rendering a drawable resets it
		window.draw(sf::CircleShape(100));

		window.draw(pattern_sprite);
		window.display();

	}
	return 0;

}
