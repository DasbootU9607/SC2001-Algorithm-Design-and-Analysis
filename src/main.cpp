// Command-line entry point. Experiment settings and measurements live in experiments.cpp.
#include "experiments.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char* argv[]) {
    try {
        bool quick = false;
        bool output_given = false;
        std::filesystem::path output_directory = "results";

        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--quick") {
                quick = true;
            } else if (argument == "--output" && i + 1 < argc) {
                output_directory = argv[++i];
                output_given = true;
                if (output_directory.empty()
                    || output_directory.string().rfind("--", 0) == 0) {
                    throw std::invalid_argument("--output needs a directory path");
                }
            } else if (argument == "--help") {
                std::cout << "Usage: project1 [--quick] [--output DIRECTORY]\n"
                             "Full mode: five trials, up to 10 million integers.\n"
                             "Quick mode: three trials, up to 10,000 integers.\n"
                             "Existing benchmark files in the output directory are replaced.\n";
                return 0;
            } else {
                throw std::invalid_argument("Unknown or incomplete argument: " + argument);
            }
        }

        if (quick && !output_given) {
            output_directory = "results/quick";
        }
        experiments::run(quick, output_directory);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
