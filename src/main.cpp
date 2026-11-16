#include "Benchmark.h"

#include "GraphInput.h"

int main(int argc, char* argv[]) {
    try {
        const std::string mode = argc > 1 ? argv[1] : "--benchmark";
        if (mode == "--random") {
            runRandomGraph(argc, argv);
        } else if (argc > 2) {
            throw std::runtime_error("Unexpected arguments. Use --help for usage");
        } else if (mode == "--help" || mode == "-h") {
            printHelp();
        } else if (mode == "--benchmark") {
            runBenchmark("benchmark.html");
        } else if (mode == "-") {
            std::cout << "Enter n and source, then n*n weights (-1 = no edge):\n";
            runGraph(std::cin);
        } else if (!mode.empty() && mode.front() == '-') {
            throw std::runtime_error("Unknown option: " + mode + ". Use --help for usage");
        } else {
            std::ifstream input(mode);
            if (!input) throw std::runtime_error("Cannot open matrix file: " + mode);
            runGraph(input);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
