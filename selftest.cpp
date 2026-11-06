#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "planner.h"

using namespace std;

static int passed = 0, failed = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (cond) ++passed;                                                  \
        else {                                                               \
            ++failed;                                                        \
            printf("FAILED line %d: %s\n", __LINE__, #cond);                 \
        }                                                                    \
    } while (0)

static void testDynamicArray() {
    mt19937 rng(7);
    DynamicArray<int> a;
    vector<int> ref;
    for (int step = 0; step < 20000; ++step) {
        int op = static_cast<int>(rng() % 7);
        int v = static_cast<int>(rng() % 50);
        if (op <= 2) { a.push_back(v); ref.push_back(v); }
        else if (op == 3 && !ref.empty()) { a.pop_back(); ref.pop_back(); }
        else if (op == 4 && !ref.empty()) {
            size_t i = rng() % ref.size();
            a.erase(i);
            ref.erase(ref.begin() + static_cast<long>(i));
        } else if (op == 5) {
            size_t i = rng() % (ref.size() + 1);
            a.insert(i, v);
            ref.insert(ref.begin() + static_cast<long>(i), v);
        } else if (op == 6 && step % 50 == 0) {
            a.removeAll(v);
            ref.erase(remove(ref.begin(), ref.end(), v), ref.end());
        }
        if (a.size() != ref.size()) { CHECK(false); return; }
    }
    bool same = true;
    for (size_t i = 0; i < ref.size(); ++i) same = same && a[i] == ref[i];
    CHECK(same);

    DynamicArray<int> b = a;
    DynamicArray<int> c;
    c = a;
    a.push_back(-1);
    CHECK(b.size() + 1 == a.size());
    CHECK(c.size() == b.size());
    bool deep = true;
    for (size_t i = 0; i < b.size(); ++i) deep = deep && b[i] == c[i];
    CHECK(deep);

    DynamicArray<int> moved = std::move(c);
    CHECK(c.size() == 0 && moved.size() == b.size());

    DynamicArray<string> s;
    s.push_back("alpha");
    for (int i = 0; i < 100; ++i) s.push_back(s[0]);
    CHECK(s.size() == 101 && s[100] == "alpha");

    DynamicArray<int> r(5, 9);
    r.resize(12, 3);
    CHECK(r.size() == 12 && r[4] == 9 && r[5] == 3 && r[11] == 3);
    CHECK(r.indexOf(3) == 5 && r.contains(9) && !r.contains(77));

    DynamicArray<DynamicArray<int>> nested;
    nested.resize(4);
    nested[2].push_back(5);
    DynamicArray<DynamicArray<int>> nestedCopy = nested;
    nested[2].push_back(6);
    CHECK(nestedCopy[2].size() == 1 && nested[2].size() == 2);
    a = a;
    CHECK(a.size() == ref.size() + 1);
}

static void testStackQueue() {
    Stack<int> st;
    for (int i = 0; i < 100; ++i) st.push(i);
    bool lifo = true;
    for (int i = 99; i >= 0; --i) { lifo = lifo && st.top() == i; st.pop(); }
    CHECK(lifo && st.empty());
    Queue<int> q;
    for (int i = 0; i < 100; ++i) q.enqueue(i);
    bool fifo = true;
    for (int i = 0; i < 100; ++i) fifo = fifo && q.dequeue() == i;
    CHECK(fifo && q.empty());
}

static void testHashTable() {
    mt19937 rng(11);
    HashTable h;
    map<string, int> ref;
    for (int step = 0; step < 30000; ++step) {
        string key = "k" + to_string(rng() % 3000);
        int op = static_cast<int>(rng() % 4);
        if (op <= 1) { int v = static_cast<int>(rng() % 1000); h.put(key, v); ref[key] = v; }
        else if (op == 2) { CHECK(h.erase(key) == (ref.erase(key) == 1)); }
        else {
            int out = -1;
            bool found = h.get(key, out);
            auto it = ref.find(key);
            if (found != (it != ref.end()) || (found && out != it->second)) { CHECK(false); return; }
        }
    }
    CHECK(h.size() == ref.size());
    h.clear();
    int out;
    CHECK(h.size() == 0 && !h.get("k1", out));
}

static void testGraph() {
    mt19937 rng(5);
    for (int trial = 0; trial < 50; ++trial) {
        int n = 2 + static_cast<int>(rng() % 60);
        DiGraph g;
        for (int i = 1; i <= n; ++i) g.addNode(i);
        vector<pair<int, int>> edges;
        for (int e = 0; e < 3 * n; ++e) {
            int u = 1 + static_cast<int>(rng() % static_cast<unsigned>(n));
            int v = 1 + static_cast<int>(rng() % static_cast<unsigned>(n));
            if (u == v || g.hasEdge(min(u, v), max(u, v))) continue;
            g.addEdge(min(u, v), max(u, v));
            edges.push_back({min(u, v), max(u, v)});
        }
        DynamicArray<int> order;
        CHECK(topologicalOrder(g, order));
        vector<int> pos(static_cast<size_t>(n) + 1, -1);
        for (size_t i = 0; i < order.size(); ++i) pos[static_cast<size_t>(order[i])] = static_cast<int>(i);
        bool valid = order.size() == static_cast<size_t>(n);
        for (auto& e : edges) valid = valid && pos[static_cast<size_t>(e.first)] < pos[static_cast<size_t>(e.second)];
        CHECK(valid);
    }
    DiGraph g;
    for (int i = 1; i <= 5; ++i) g.addNode(i);
    g.addEdge(1, 2); g.addEdge(2, 3); g.addEdge(3, 1); g.addEdge(1, 4); g.addEdge(4, 5);
    DynamicArray<int> order, stuck;
    CHECK(!topologicalOrder(g, order, &stuck));
    DynamicArray<int> cycle = extractCycle(g, stuck);
    bool real = cycle.size() >= 2;
    for (size_t i = 0; i < cycle.size(); ++i)
        real = real && g.hasEdge(cycle[i], cycle[(i + 1) % cycle.size()]);
    CHECK(real);
    CHECK(g.hasPath(1, 5) && !g.hasPath(5, 1) && !g.hasPath(1, 99));
    g.removeNode(4);
    CHECK(!g.hasNode(4) && !g.hasPath(1, 5) && g.edgeCount() == 3);
}

static void testQuickSort() {
    mt19937 rng(3);
    for (int trial = 0; trial < 200; ++trial) {
        size_t n = rng() % 300;
        int mode = trial % 4;
        DynamicArray<Task> a;
        vector<Task> ref;
        for (size_t i = 0; i < n; ++i) {
            Task t;
            t.id = static_cast<int>(i) + 1;
            int day = mode == 0 ? static_cast<int>(rng() % 28) + 1 : mode == 1 ? static_cast<int>(i % 28) + 1
                    : mode == 2 ? 28 - static_cast<int>(i % 28) : 5;
            char buf[16];
            snprintf(buf, sizeof(buf), "2026-12-%02d", day);
            t.due = buf;
            a.push_back(t);
            ref.push_back(t);
        }
        quickSort(a, lessByDue);
        sort(ref.begin(), ref.end(), lessByDue);
        bool same = true;
        for (size_t i = 0; i < n; ++i) same = same && a[i].id == ref[i].id;
        CHECK(same);
    }
    DynamicArray<Task> big;
    for (int i = 0; i < 200000; ++i) {
        Task t;
        t.id = i + 1;
        t.due = "2026-12-01";
        big.push_back(t);
    }
    quickSort(big, lessByDue);
    CHECK(big[0].id == 1 && big[199999].id == 200000);
}

static void testPlanner() {
    Planner p;
    CHECK(p.addTask("Book venue", "Jose", "2026-11-12", 0).ok);
    CHECK(!p.addTask("book VENUE", "Ana", "2026-11-13", 0).ok);
    CHECK(!p.addTask("", "Ana", "2026-11-13", 0).ok);
    CHECK(!p.addTask("X", "Ana", "2026-02-30", 0).ok);
    CHECK(!p.addTask("X|Y", "Ana", "2026-11-13", 0).ok);
    CHECK(p.addTask("Hire caterer", "Ana", "2026-11-20", 1).ok);
    CHECK(p.addTask("Send invitations", "Maria", "2026-11-15", 0).ok);
    CHECK(p.findByName("  HIRE CATERER ") && p.findByName("hire caterer")->id == 2);
    CHECK(p.addDependency(1, 2).ok && p.addDependency(2, 3).ok);
    CHECK(!p.addDependency(1, 3).ok && !p.addDependency(3, 1).ok && !p.addDependency(1, 1).ok);
    CHECK(!p.addDependency(1, 2).ok && !p.addDependency(1, 99).ok);
    DynamicArray<int> order, cycle;
    CHECK(p.taskOrder(order, cycle) && order.size() == 3 && order[0] == 1 && order[2] == 3);
    CHECK(p.downstream(1).size() == 2 && p.downstream(3).size() == 0);

    CHECK(p.removeTask(2).ok && !p.findById(2) && !p.findByName("hire caterer"));
    CHECK(p.graph().edgeCount() == 0 && p.findById(3)->name == "Send invitations");
    CHECK(p.undo().ok && p.findById(2) && p.graph().edgeCount() == 2);
    CHECK(p.redo().ok && !p.findById(2));
    CHECK(p.undo().ok);

    CHECK(p.setEventName("Spring Gala").ok && p.eventName() == "Spring Gala");
    CHECK(p.undo().ok && p.eventName() == "Untitled Event");
    CHECK(p.redo().ok && p.eventName() == "Spring Gala");
    CHECK(!p.setEventName("a|b").ok && !p.setEventName("").ok);

    Planner q;
    CHECK(q.addTask("Zzz old task", "Ana", "2026-11-12", 0).ok);
    CHECK(q.loadSample().ok);
    CHECK(q.findByName("zzz old task") == nullptr);
    CHECK(q.addTask("Zzz old task", "Ana", "2026-11-12", 0).ok);
    CHECK(q.findByName("zzz old task")->id == 9);
    CHECK(q.undo().ok && q.undo().ok && q.findByName("zzz old task") != nullptr && q.tasks().size() == 1);

    Planner s;
    s.loadSample();
    s.setEventName("Birthday Party");
    CHECK(s.save("selftest_tmp.txt").ok);
    Planner t;
    CHECK(t.load("selftest_tmp.txt").ok);
    CHECK(t.eventName() == "Birthday Party" && t.tasks().size() == 8 && t.graph().edgeCount() == 10);
    CHECK(t.findById(8)->name == "Event day check-in");
    remove("selftest_tmp.txt");

    auto writeFile = [](const char* text) {
        FILE* f = fopen("selftest_tmp.txt", "w");
        fputs(text, f);
        fclose(f);
    };
    Planner u;
    writeFile("T|1|A|x|2026-01-01|0\nT|2|B|x|2026-01-01|0\nE|1|2\nE|2|1\n");
    CHECK(u.load("selftest_tmp.txt").ok);
    CHECK(!u.taskOrder(order, cycle) && cycle.size() == 2);
    writeFile("T|1|A|x|2026-01-01|0\nE|1|1\n");
    CHECK(!u.load("selftest_tmp.txt").ok);
    writeFile("T|1|A|x|2026-01-01|0\nT|2|B|x|2026-01-01|0\nE|1|2\nE|1|2\n");
    CHECK(!u.load("selftest_tmp.txt").ok);
    writeFile("T|2000000000|A|x|2026-01-01|0\n");
    CHECK(!u.load("selftest_tmp.txt").ok);
    writeFile("T|1|A|x|2026-01-01|0\nT|1|B|x|2026-01-01|0\n");
    CHECK(!u.load("selftest_tmp.txt").ok);
    writeFile("garbage\n");
    CHECK(!u.load("selftest_tmp.txt").ok);
    writeFile("T|1|A|x|2026-13-01|0\n");
    CHECK(!u.load("selftest_tmp.txt").ok);
    writeFile("T|1|A|x|2026-01-01|0\nE|1|7\n");
    CHECK(!u.load("selftest_tmp.txt").ok);
    remove("selftest_tmp.txt");
    CHECK(!u.load("does_not_exist.txt").ok);
}

int main() {
    testDynamicArray();
    testStackQueue();
    testHashTable();
    testGraph();
    testQuickSort();
    testPlanner();
    printf("%d checks passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
