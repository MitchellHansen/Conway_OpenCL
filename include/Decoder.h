#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <experimental/filesystem>
#include <vector>
#include <SFML/Graphics.hpp>

struct pattern_info {
	
	std::string title;
	std::string author;
	std::string comments;
	sf::Vector2i dimensions;

	char *nodes;

};

class Decoder {
	
public:

	Decoder();
	~Decoder();

	pattern_info decodePattern(std::string pattern);
	std::vector<const char*> getPatternList();

private:

	std::vector<const char*> pattern_list;

	
};