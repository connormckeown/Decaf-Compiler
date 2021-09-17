#include <iostream>
#include <string>
#include <cstdlib>

std::string trim(std::string& s, const char* c = " \t") {
    s.erase(0, s.find_first_not_of(c));
    return s;
}

int main() {
    for (std::string line; std::getline(std::cin, line);) {
	std::string trimmed_line = trim(line);
        std::cout << trimmed_line << std::endl;
    }
    exit(EXIT_SUCCESS);
}
