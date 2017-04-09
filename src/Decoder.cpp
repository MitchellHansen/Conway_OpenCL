#include "Decoder.h"


Decoder::Decoder() {

	std::cout << "Loading patterns...";
	
	for (std::experimental::filesystem::directory_entry p : std::experimental::filesystem::directory_iterator("../assets/patterns/")) {
		pattern_list.push_back(p.path().generic_string());
	}

	std::cout << "Done" << std::endl;

}
Decoder::~Decoder() {

}

pattern_info Decoder::decodePattern(std::string pattern) {

	pattern_info info;

	//for (int i = 0; i < dimensions.x * dimensions.y; i++) {
	//	nodes[i] = 0;
	//}


	//std::ifstream file(filename);

	//if (!file.is_open()) {
	//	std::cout << "unable to open file" << std::endl;
	//	return info;
	//}

	//for (std::string line; std::getline(file, line); )
	//{
	//	// Grab the header comments
	//	if (line[0] == '#') {

	//		std::string data = line.substr(3, line.size() - 3);

	//		switch (line[1]) {

	//		case 'C': {
	//			info.comments += data;
	//			break;
	//		}
	//		case 'O': {
	//			info.author += data;
	//			break;
	//		}
	//		case 'N': {
	//			info.title += data;
	//			break;
	//		}
	//		default: {
	//			std::cout << "Case : " << line[1] << " not supported" << std::endl;
	//		}
	//		}
	//	}

	//	// grab the dimensions
	//	else if (line[0] == 'x') {

	//		// Naively assume that it is in the exact form "x = <num>, y = <num>,"
	//		std::stringstream ss(line);

	//		std::string temp;
	//		ss >> temp >> temp >> temp;
	//		info.dimensions.x = std::stoi(temp.substr(0, temp.size() - 1));

	//		ss >> temp >> temp >> temp;
	//		info.dimensions.y = std::stoi(temp.substr(0, temp.size() - 1));
	//	}

	//	// Decode the RLE
	//	else {

	//		std::vector<std::vector<char>> grid;

	//		std::stringstream ss(line);
	//		std::string token;

	//		while (std::getline(ss, token, '$')) {

	//			std::vector<char> char_line;
	//			unsigned int token_pos = 0;
	//			std::string tmp;

	//			while (token_pos < token.size()) {

	//				char status = -1;
	//				if (token[token_pos] == 'b')
	//					status = 0;
	//				else if (token[token_pos] == 'o')
	//					status = 1;
	//				else if (token[token_pos] == '!')
	//					break;
	//				else
	//					tmp += token[token_pos];

	//				token_pos++;

	//				if (status >= 0) {
	//					if (tmp.empty()) {
	//						char_line.push_back(status);
	//					}
	//					else {
	//						int count = std::stoi(tmp);
	//						for (int i = 0; i < count; i++) {
	//							char_line.push_back(status);
	//						}
	//						tmp.clear();
	//					}
	//				}

	//			}

	//			grid.push_back(char_line);
	//		}

	//		int y_mod = 200;
	//		int x_mod = 200;
	//		for (int y = 0; y < grid.size(); y++) {
	//			for (int x = 0; x < grid.at(y).size(); x++) {
	//				nodes[(y + y_mod) * dimensions.x + (x + x_mod)] = grid.at(y).at(x);
	//			}
	//		}

	//	}
	//}

	//std::cout << info.author << std::endl;
	//std::cout << info.title << std::endl;
	//std::cout << info.comments << std::endl;

	return info;
}

std::vector<std::string> Decoder::getPatternList() {

	return pattern_list;

}
