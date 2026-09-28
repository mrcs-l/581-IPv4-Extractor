#include <iostream>
#include <string>
#include "ipv4.hpp"

int main() {
    std::string line;
    while (true) {
        std::cout << "Enter a string (or 'END' to quit): ";
        if (!std::getline(std::cin, line)) {  // EOF (Ctrl-D / end of piped input) acts like END
            std::cout << '\n';
            break;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();  // tolerate Windows line endings
        if (line == "END") break;

        unsigned long address;
        int port;
        // Branch on the return value, not the address: 0.0.0.0 is valid and also 0.
        if (extractIPv4(line, address, port)) {
            std::cout << "Extracted IPv4 address: "
                      << ((address >> 24) & 255) << '.' << ((address >> 16) & 255) << '.'
                      << ((address >> 8) & 255) << '.' << (address & 255)
                      << " (decimal value: " << address << ", port: ";
            if (port == -1) {
                std::cout << "none";
            } else {
                std::cout << port;
            }
            std::cout << ")\n";
        } else {
            std::cout << "Invalid input: no valid IPv4 address found\n";
        }
    }
    std::cout << "Program terminated." << std::endl;
    return 0;
}
