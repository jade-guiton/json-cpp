#include <iostream>
#include <fstream>

#include "json.hpp"
#include "unicode.hpp"

int main(int argc, char const *argv[]) {
	if(argc != 2) {
		std::cerr << "Expected 1 command line argument" << std::endl;
		return 1;
	}
	std::ifstream is { argv[1], std::ios::in | std::ios::binary };
	std::istreambuf_iterator<char> cit { is.rdbuf() }, cend;
	Utf8Validator<std::istreambuf_iterator<char>> uit { cit }, uend { cend };
	
	try {
		auto json = Json::parse(uit, uend);
	} catch(JsonError err) {
		std::cerr << "Parse error: " << err.what() << std::endl;
		return 2;
	} catch(UnicodeError err) {
		std::cerr << "Unicode error: " << err.what() << std::endl;
		return 2;
	}
	
	return 0;
}