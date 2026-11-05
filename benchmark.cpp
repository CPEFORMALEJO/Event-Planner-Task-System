// benchmark.cpp
// Timing code for Section 5.2 (Experimental Analysis).
// Measures four operations at n = 100, 1,000, 10,000 and 100,000.
// Each measurement is repeated 5 times and the average is reported.
// Only the operation itself is timed (data generation is outside the timer).
//
// Build: g++ -std=c++17 -O2 benchmark.cpp -o benchmark
// Output: a table on screen and benchmark_results.csv (for the line graph).

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "planner.h"

using namespace std;
using Clock = chrono::steady_clock;

static const int REPS = 5;
static const int LOOKUPS = 1000;          // number of searches per measurement
static volatile long long g_sink = 0;     // stops the compiler from removing the work

template <typename F>
static double timeMs(F&& work) {
    auto start = Clock::now();
    work();
    auto stop = Clock::now();
    return chrono::duration<double, milli>(stop - start).count();
}

static string randomDate(mt19937& rng) {
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
    vector<vector<double>> results;   // results[size index][operation]

    for (int n : sizes) {
        mt19937 rng(12345);           // same seed: same data on every run
        vector<Task> tasks(static_cast<size_t>(n));
        vector<string> names(static_cast<size_t>(n));
        HashTable table;
        for (int i = 0; i < n; ++i) {
            Task& t = tasks[static_cast<size_t>(i)];
            t.id = i + 1;
            t.name = "task_" + to_string(i);
            t.assignee = "person";
            t.due = randomDate(rng);
            t.status = static_cast<int>(rng() % 3);
            names[static_cast<size_t>(i)] = t.name;
            table.put(t.name, i);
        }

        vector<string> queries(LOOKUPS);
        for (auto& q : queries) q = names[rng() % static_cast<unsigned>(n)];

        // Random DAG: n nodes, 2n edges, always from a lower to a higher id.
        DiGraph graph;
        for (int i = 0; i < n; ++i) graph.addNode(i);
        for (int e = 0; e < 2 * n; ++e) {
            int u = static_cast<int>(rng() % static_cast<unsigned>(n));
            int v = static_cast<int>(rng() % static_cast<unsigned>(n));
            if (u == v) continue;
            graph.addEdge(min(u, v), max(u, v));
        }

        double total[4] = {0, 0, 0, 0};
        for (int r = 0; r < REPS; ++r) {
            // 1. Quick sort (fresh unsorted copy each run, copy not timed)
            vector<Task> copy = tasks;
            total[0] += timeMs([&] { quickSort(copy, lessByDue); });
            for (size_t i = 1; i < copy.size(); ++i)
                if (lessByDue(copy[i], copy[i - 1])) { cerr << "SORT ERROR\n"; return 1; }

            // 2. Hash table search
            total[1] += timeMs([&] {
                long long acc = 0;
                int pos;
                for (const string& q : queries) if (table.get(q, pos)) acc += pos;
                g_sink = acc;
            });

            // 3. Linear search over the same names
            total[2] += timeMs([&] {
                long long acc = 0;
                for (const string& q : queries)
                    for (size_t i = 0; i < names.size(); ++i)
                        if (names[i] == q) { acc += static_cast<long long>(i); break; }
                g_sink = acc;
            });

            // 4. Kahn's topological sort
            vector<int> order;
            total[3] += timeMs([&] { topologicalOrder(graph, order); });
            if (order.size() != static_cast<size_t>(n)) { cerr << "TOPO ERROR\n"; return 1; }
        }

        vector<double> avg;
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

    ofstream csv("benchmark_results.csv");
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
