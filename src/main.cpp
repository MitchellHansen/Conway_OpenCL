
#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <chrono>
#include "util.hpp"
#include <thread>
#include "OpenCL.h"
#include "imgui/imgui-SFML.h"
#include "imgui/imgui.h"

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

		sf::Vector2i pos(i % WINDOW_X, i / WINDOW_X);

		if ((pos.x % 5 == 0) || (pos.y % 8 == 0))
			nodes[i] = 1;
		else
			nodes[i] = 0;
	}

}

struct rle_info {
	
	std::string title;
	std::string author;
	std::string comments;
	sf::Vector2i dimensions;

};

rle_info load_rle(sf::Uint8* nodes, sf::Vector2i dimensions, std::string filename) {

	rle_info info;

	for (int i = 0; i < dimensions.x * dimensions.y; i++) {
		nodes[i] = 0;
	}

	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cout << "unable to open file" << std::endl;
		return info;
	}
	
	for (std::string line; std::getline(file, line); )
	{
		// Grab the header comments
		if (line[0] == '#') {
			
			std::string data = line.substr(3, line.size() - 3);
			
			switch (line[1]) {
				
				case 'C': {
					info.comments += data;
					break;
				}
				case 'O': {
					info.author += data;
					break;
				}
				case 'N': {
					info.title += data;
					break;
				}
				default : {
					std::cout << "Case : " << line[1] << " not supported" << std::endl;
				}
			}
		}

		// grab the dimensions
		else if (line[0] == 'x') {

			// Naively assume that it is in the exact form "x = <num>, y = <num>,"
			std::stringstream ss(line);

			std::string temp;
			ss >> temp >> temp >> temp;
			info.dimensions.x = std::stoi(temp.substr(0, temp.size() - 1));
			
			ss >> temp >> temp >> temp;
			info.dimensions.y = std::stoi(temp.substr(0, temp.size() - 1));
		}

		// Decode the RLE
		else {
			
			std::vector<std::vector<char>> grid;

			std::stringstream ss(line);
			std::string token;
			
			while (std::getline(ss, token, '$')) {
				
				std::vector<char> char_line;
				unsigned int token_pos = 0;
				std::string tmp;

				while (token_pos < token.size()) {
					
					char status = -1;
					if (token[token_pos] == 'b')
						status = 0;
					else if (token[token_pos] == 'o')
						status = 1;
					else if (token[token_pos] == '!')
						break;
					else
						tmp += token[token_pos];
					
					token_pos++;

					if (status >= 0) {
						if (tmp.empty()) {
							char_line.push_back(status);
						}
						else {
							int count = std::stoi(tmp);
							for (int i = 0; i < count; i++) {
								char_line.push_back(status);
							}
							tmp.clear();
						}
					}
					
				}

				grid.push_back(char_line);
			}

			int y_mod = 200;
			int x_mod = 200;
			for (int y = 0; y < grid.size(); y++) {
				for (int x = 0; x < grid.at(y).size(); x++) {
					nodes[(y+y_mod) * dimensions.x + (x+x_mod)] = grid.at(y).at(x);
				}
			}

		}
	}

	std::cout << info.author << std::endl;
	std::cout << info.title << std::endl;
	std::cout << info.comments << std::endl;

	return info;
}

int main() {

	sf::RenderWindow window(sf::VideoMode(WINDOW_X, WINDOW_Y), "conways-game-of-life-opencl");

	int simulation_speed = 100;
	window.setFramerateLimit(simulation_speed);

	ImGui::SFML::Init(window);
	window.resetGLStates();

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
	cl.create_image_buffer("viewport_image", image_resolution, sf::Vector2f(0, 0), CL_MEM_READ_WRITE);
	cl.create_buffer("image_res", sizeof(sf::Vector2i), &image_resolution);

	sf::Uint8* nodes = new sf::Uint8[WINDOW_X * WINDOW_Y];
	//generate_nodes(nodes);

	load_rle(nodes, sf::Vector2i(WINDOW_X, WINDOW_Y), "../assets/puffer_breeder_1.rle");
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

		ImGui::SFML::Update(window, sf_delta_clock.restart());

		window.clear(sf::Color::White);

		cl.run_kernel("conways", image_resolution);
		cl.draw(&window);


		ImGui::Begin("Sim");

		if (ImGui::SliderInt("Simulation Speed", &simulation_speed, 30, 500)) {
			window.setFramerateLimit(simulation_speed);
		}

		ImGui::End();
		ImGui::Render();


		
		
		if (buffer_flip == 1)
			buffer_flip = 0;
		else
			buffer_flip = 1;
		
		// ImGui screws stuff up after the render, rendering a drawable resets it
		window.draw(sf::CircleShape(0));
		window.display();

	}
	return 0;

}
