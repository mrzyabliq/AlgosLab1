#pragma once

#include "TestGenerator.h"
#include "Visualizer.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

inline void runBenchmark(const std::string& reportPath) {
    constexpr int firstN = 100;
    constexpr int lastN = 5000;
    constexpr int step = 100;
    constexpr int repeats = 10;
    constexpr long long minWeight = 1;
    constexpr long long maxWeight = 10000;

    TestGenerator generator(42);
    std::mt19937_64 edgeRandom(42);
    std::vector<BenchmarkPoint> results[4];
    const char* names[] = {"Sparse", "Medium", "Dense", "ManyDecreases"};
    std::ofstream samples("benchmark_samples.csv");
    if (!samples) throw std::runtime_error("Cannot open benchmark_samples.csv");
    samples.imbue(std::locale::classic());
    samples << std::setprecision(17)
            << "type,n,m,repeat,first,two_seconds,fibonacci_seconds\n";
    auto median = [](std::vector<double>& times) {
        std::sort(times.begin(), times.end());
        const std::size_t middle = times.size() / 2;
        return times.size() % 2 ? times[middle] : (times[middle - 1] + times[middle]) / 2;
    };
    for (int type = 0; type < 4; ++type) {
        auto& points = results[type];
        for (int n = firstN; n <= lastN; n += step) {
            const auto count = static_cast<std::size_t>(n);
            const std::size_t sparseEdges = 4 * count;
            const std::size_t denseEdges = count * (count - 1) / 4;
            std::size_t m = type == 2 ? denseEdges : sparseEdges;
            if (type == 1) {
                std::uniform_int_distribution<std::size_t> edges(sparseEdges + 1, denseEdges - 1);
                m = edges(edgeRandom);
            } else if (type == 3) {
                const std::size_t chain = count / 2;
                m = chain * (count - chain) + chain - 1;
            }
            std::cout << names[type] << ": n=" << n << ", m=" << m
                      << " - generating..." << std::endl;
            const auto adjacency = type == 3 ? generator.generateManyDecreases(n)
                                            : generator.generate(n, m, minWeight, maxWeight);
            std::cout << "Measuring..." << std::endl;
            const auto expected = dijkstraTwoHeap(adjacency, 1).dist;
            if (dijkstraFibonacciHeap(adjacency, 1).dist != expected) {
                throw std::runtime_error("Benchmark algorithms disagree during warm-up");
            }
            std::vector<double> twoTimes, fibonacciTimes;
            twoTimes.reserve(repeats);
            fibonacciTimes.reserve(repeats);
            for (int repeat = 0; repeat < repeats; ++repeat) {
                DijkstraResult two, fibonacci;
                double timeTwo = 0, timeFibonacci = 0;
                auto measureTwo = [&] {
                    const auto start = std::chrono::steady_clock::now();
                    two = dijkstraTwoHeap(adjacency, 1);
                    timeTwo = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - start).count();
                };
                auto measureFibonacci = [&] {
                    const auto start = std::chrono::steady_clock::now();
                    fibonacci = dijkstraFibonacciHeap(adjacency, 1);
                    timeFibonacci = std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - start).count();
                };
                if (repeat % 2 == 0) {
                    measureTwo();
                    measureFibonacci();
                } else {
                    measureFibonacci();
                    measureTwo();
                }
                if (two.dist != expected || fibonacci.dist != expected) {
                    throw std::runtime_error("Benchmark algorithms produced different distances");
                }
                twoTimes.push_back(timeTwo);
                fibonacciTimes.push_back(timeFibonacci);
                samples << names[type] << ',' << n << ',' << m << ',' << repeat + 1 << ','
                        << (repeat % 2 == 0 ? "two" : "fibonacci") << ','
                        << timeTwo << ',' << timeFibonacci << '\n';
            }
            points.push_back({n, m, median(twoTimes), median(fibonacciTimes)});
        }
    }
    samples.flush();
    if (!samples) throw std::runtime_error("Cannot write benchmark_samples.csv");
    Visualizer visualizer;
    visualizer.saveBenchmarkHtml(reportPath, results[0], results[1], results[2], results[3],
                                 repeats, minWeight, maxWeight);
    std::cout << "Benchmark: 4 graph types, " << results[0].size() << " sizes each, "
              << repeats << " runs each -> " << reportPath << '\n';
}
