//  main.cpp

#include "application.hpp"
#include <iostream>

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: ./tgBot <config_json>\n";
        return 1;
    }

    try {
        Application app(argv[1]);
        
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "[Critical Error] " << e.what() << '\n';
    }

    std::cout << "Bye!\n";
    return 0;

}
