#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "planner.h"

using Clock = std::chrono::steady_clock;

static const int REPS = 5;
static const int LOOKUPS = 1000;
static volatile long long g_sink = 0;

template <typename F>
static double timeMs(F&& work) {
    auto start = Clock::now();
    work();
    auto stop = Clock::now();
    return std::chrono::duration<double, std::milli>(stop - start).count();
}

static std::string randomDate(std::mt19937& rng) {
    int y = 2026 + static_cast<int>(rng() % 3);
    int m = 1 + static_cast<int>(rng() % 12);
    int d = 1 + static_cast<int>(rng() % 28);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    return buf;
}

int main() {
    const int sizes[] = {100, 1000, 10000, 100000};
    const char* opNames[] = {"Quick sort by due date", "Hash search (1000 lookups)",
                             "Linear search (1000 lookups)", "Topological sort (Kahn)"};
    std::vector<std::vector<double>> results;

    for (int n : sizes) {
        std::mt19937 rng(12345);
        DynamicArray<Task> tasks(static_cast<size_t>(n));
        DynamicArray<std::string> names(static_cast<size_t>(n));
        HashTable table;
        for (int i = 0; i < n; ++i) {
            Task& t = tasks[static_cast<size_t>(i)];
            t.id = i + 1;
            t.name = "task_" + std::to_string(i);
            t.assignee = "person";
            t.due = randomDate(rng);
            t.status = static_cast<int>(rng() % 3);
            names[static_cast<size_t>(i)] = t.name;
            table.put(t.name, i);
        }

        DynamicArray<std::string> queries(LOOKUPS);
        for (std::string& q : queries) q = names[rng() % static_cast<unsigned>(n)];

        DiGraph graph;
        for (int i = 0; i < n; ++i) graph.addNode(i);
        for (int e = 0; e < 2 * n; ++e) {
            int u = static_cast<int>(rng() % static_cast<unsigned>(n));
            int v = static_cast<int>(rng() % static_cast<unsigned>(n));
            if (u == v) continue;
            graph.addEdge(std::min(u, v), std::max(u, v));
        }

        double total[4] = {0, 0, 0, 0};
        for (int r = 0; r < REPS; ++r) {
            DynamicArray<Task> copy = tasks;
            total[0] += timeMs([&] { quickSort(copy, lessByDue); });
            for (size_t i = 1; i < copy.size(); ++i)
                if (lessByDue(copy[i], copy[i - 1])) { std::cerr << "SORT ERROR\n"; return 1; }

            total[1] += timeMs([&] {
                long long acc = 0;
                int pos;
                for (const std::string& q : queries) if (table.get(q, pos)) acc += pos;
                g_sink = acc;
            });

            total[2] += timeMs([&] {
                long long acc = 0;
                for (const std::string& q : queries)
                    for (size_t i = 0; i < names.size(); ++i)
                        if (names[i] == q) { acc += static_cast<long long>(i); break; }
                g_sink = acc;
            });

            DynamicArray<int> order;
            total[3] += timeMs([&] { topologicalOrder(graph, order); });
            if (order.size() != static_cast<size_t>(n)) { std::cerr << "TOPO ERROR\n"; return 1; }
        }

        std::vector<double> avg;
        for (double t : total) avg.push_back(t / REPS);
        results.push_back(avg);
    }

    printf("\nAverage of %d runs, time in milliseconds\n\n", REPS);
    printf("%-10s", "n");
    for (const char* name : opNames) printf("%-32s", name);
    printf("\n");
    for (size_t s = 0; s < results.size(); ++s) {
        printf("%-10d", sizes[s]);
        for (double v : results[s]) printf("%-32.4f", v);
        printf("\n");
    }

    std::ofstream csv("benchmark_results.csv");
    csv << "n";
    for (const char* name : opNames) csv << ",\"" << name << " (ms)\"";
    csv << "\n";
    for (size_t s = 0; s < results.size(); ++s) {
        csv << sizes[s];
        for (double v : results[s]) csv << ',' << v;
        csv << "\n";
    }
    printf("\nSaved benchmark_results.csv\n");
    return 0;
}