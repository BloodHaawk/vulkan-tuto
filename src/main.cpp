#include <iostream>

#include "application.hpp"

int main()
{
    auto app = Application();

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
