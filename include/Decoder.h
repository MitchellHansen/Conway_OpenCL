#pragma once
#include <iostream>

#include <experimental/filesystem>
#include <vector>
#include <SFML/Graphics.hpp>

struct pattern_info {
	
	std::string title;
	std::string author;
	std::string comments;
	sf::Vector2i dimensions;

	char *pattern;

};

class Decoder {
	
public:

	Decoder();
	~Decoder();

	pattern_info decodePattern(std::string pattern);
	std::vector<std::string> getPatternList();

private:

	std::vector<std::string> pattern_list;

	
};