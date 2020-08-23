#include "Decoder.h"


Decoder::Decoder() {

	std::cout << "Loading patterns...";
	
	for (std::experimental::filesystem::directory_entry p : std::experimental::filesystem::directory_iterator("../assets/patterns/")) {
		// good lord c++
		std::string s = p.path().generic_string();
		const char* char_shit = new char[s.size() + 1];
		memcpy((void*)char_shit, s.c_str(), s.size()+1);

        delete char_shit;

		pattern_list.push_back(char_shit);
	}

	std::cout << "Done" << std::endl;

}
Decoder::~Decoder() {

}

pattern_info Decoder::decodePattern(std::string pattern) {

	pattern_info info;

	std::ifstream file(pattern);

	if (!file.is_open()) {
		std::cout << "unable to open file" << std::endl;
		return info;
	}

	std::vector<std::vector<char>> grid;
	std::stringstream rle_stream;

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
			default: {
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

			rle_stream << line;
			std::string token;

			while (std::getline(rle_stream, token, '$')) {

				if (rle_stream.eof()) {

					if (token.back() != '!') {
						rle_stream.seekg(-token.size(), std::ios::end);
						break;
					}
				}

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

				char_line.resize(info.dimensions.x);
				grid.push_back(char_line);
			}

			rle_stream.clear();

		}
	}

	info.dimensions = sf::Vector2i(grid.at(0).size(), grid.size());
	info.nodes = new char[info.dimensions.x * info.dimensions.y];
	
	for (int y = 0; y < grid.size(); y++) {
		for (int x = 0; x < grid.at(y).size(); x++) {
			info.nodes[y * info.dimensions.x + x] = grid.at(y).at(x);
		}
	}

	file.close();
	return info;
}

std::vector<const char*> Decoder::getPatternList() {

	return pattern_list;

}
