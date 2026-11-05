// structures.h
// Core data structures and algorithms for the Event Planner Task Dependency System.
// Header-only so that main.cpp and benchmark.cpp can share the same code.

#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Stack (array based): used for undo / redo.  push, pop, top are O(1).
// ---------------------------------------------------------------------------
template <typename T>
class Stack {
    std::vector<T> data_;

public:
    void push(const T& value) { data_.push_back(value); }
    void pop() { if (!data_.empty()) data_.pop_back(); }
    T& top() { return data_.back(); }
    bool empty() const { return data_.empty(); }
    std::size_t size() const { return data_.size(); }
    void clear() { data_.clear(); }
};

// ---------------------------------------------------------------------------
// Queue (array based with a moving head index): used by Kahn's algorithm
// and by breadth-first search.  enqueue and dequeue are O(1).
// ---------------------------------------------------------------------------
template <typename T>
class Queue {
    std::vector<T> data_;
    std::size_t head_ = 0;

public:
    void enqueue(const T& value) { data_.push_back(value); }
    T dequeue() { return data_[head_++]; }
    bool empty() const { return head_ >= data_.size(); }
    std::size_t size() const { return data_.size() - head_; }
};

// ---------------------------------------------------------------------------
// Hash table (separate chaining): maps a string key to an integer value.
// Average O(1) put / get / erase; resizes when the load factor passes 0.75.
// ---------------------------------------------------------------------------
class HashTable {
    struct Entry {
        std::string key;
        int value;
    };

    std::vector<std::vector<Entry>> buckets_;
    std::size_t count_ = 0;

    static std::size_t hashString(const std::string& s) {
        std::size_t h = 5381;                       // djb2 hash
        for (unsigned char c : s) h = h * 33 + c;
        return h;
    }

    void rehash(std::size_t newSize) {
        std::vector<std::vector<Entry>> fresh(newSize);
        for (const auto& bucket : buckets_)
            for (const auto& e : bucket)
                fresh[hashString(e.key) % newSize].push_back(e);
        buckets_.swap(fresh);
    }

public:
    explicit HashTable(std::size_t initialBuckets = 16) : buckets_(initialBuckets) {}

    void put(const std::string& key, int value) {
        if ((count_ + 1) * 4 > buckets_.size() * 3) rehash(buckets_.size() * 2);
        auto& bucket = buckets_[hashString(key) % buckets_.size()];
        for (auto& e : bucket) {
            if (e.key == key) { e.value = value; return; }
        }
        bucket.push_back(Entry{key, value});
        ++count_;
    }

    bool get(const std::string& key, int& value) const {
        const auto& bucket = buckets_[hashString(key) % buckets_.size()];
        for (const auto& e : bucket) {
            if (e.key == key) { value = e.value; return true; }
        }
        return false;
    }

    bool erase(const std::string& key) {
        auto& bucket = buckets_[hashString(key) % buckets_.size()];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].key == key) {
                bucket.erase(bucket.begin() + static_cast<long>(i));
                --count_;
                return true;
            }
        }
        return false;
    }

    void clear() {
        buckets_.assign(16, std::vector<Entry>());
        count_ = 0;
    }

    std::size_t size() const { return count_; }
};

// ---------------------------------------------------------------------------
// Directed graph (adjacency list).  An edge u -> v means "u must be finished
// before v can start".  Node ids may be sparse (removed tasks leave gaps).
// ---------------------------------------------------------------------------
class DiGraph {
    std::vector<std::vector<int>> adj_;
    std::vector<char> alive_;

public:
    void addNode(int id) {
        if (id >= static_cast<int>(adj_.size())) {
            adj_.resize(static_cast<std::size_t>(id) + 1);
            alive_.resize(static_cast<std::size_t>(id) + 1, 0);
        }
        alive_[static_cast<std::size_t>(id)] = 1;
    }

    // Removes the node and every edge that touches it: O(V + E).
    void removeNode(int id) {
        if (!hasNode(id)) return;
        adj_[static_cast<std::size_t>(id)].clear();
        alive_[static_cast<std::size_t>(id)] = 0;
        for (auto& list : adj_)
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
    }

    bool hasNode(int id) const {
        return id >= 0 && id < static_cast<int>(alive_.size()) && alive_[static_cast<std::size_t>(id)];
    }

    int capacity() const { return static_cast<int>(adj_.size()); }

    // Unchecked insert: the caller is responsible for validating the edge.
    void addEdge(int from, int to) { adj_[static_cast<std::size_t>(from)].push_back(to); }

    bool hasEdge(int from, int to) const {
        const auto& list = adj_[static_cast<std::size_t>(from)];
        return std::find(list.begin(), list.end(), to) != list.end();
    }

    void removeEdge(int from, int to) {
        auto& list = adj_[static_cast<std::size_t>(from)];
        list.erase(std::remove(list.begin(), list.end(), to), list.end());
    }

    const std::vector<int>& neighbors(int id) const { return adj_[static_cast<std::size_t>(id)]; }

    std::size_t edgeCount() const {
        std::size_t total = 0;
        for (const auto& list : adj_) total += list.size();
        return total;
    }

    void clear() {
        adj_.clear();
        alive_.clear();
    }

    // Breadth-first search: is there a directed path from -> to?  O(V + E).
    bool hasPath(int from, int to) const {
        if (!hasNode(from) || !hasNode(to)) return false;
        std::vector<char> seen(adj_.size(), 0);
        Queue<int> q;
        q.enqueue(from);
        seen[static_cast<std::size_t>(from)] = 1;
        while (!q.empty()) {
            int u = q.dequeue();
            if (u == to) return true;
            for (int v : adj_[static_cast<std::size_t>(u)]) {
                if (!seen[static_cast<std::size_t>(v)]) {
                    seen[static_cast<std::size_t>(v)] = 1;
                    q.enqueue(v);
                }
            }
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// Topological sort (Kahn's algorithm with a queue).  O(V + E).
// Returns true and fills `order` when the graph is acyclic.  Otherwise returns
// false and `stuck` holds the nodes that could not be scheduled.
// ---------------------------------------------------------------------------
inline bool topologicalOrder(const DiGraph& g, std::vector<int>& order,
                             std::vector<int>* stuck = nullptr) {
    order.clear();
    const int cap = g.capacity();
    std::vector<int> indegree(static_cast<std::size_t>(cap), 0);
    std::size_t aliveCount = 0;

    for (int u = 0; u < cap; ++u) {
        if (!g.hasNode(u)) continue;
        ++aliveCount;
        for (int v : g.neighbors(u)) ++indegree[static_cast<std::size_t>(v)];
    }

    Queue<int> q;
    for (int u = 0; u < cap; ++u)
        if (g.hasNode(u) && indegree[static_cast<std::size_t>(u)] == 0) q.enqueue(u);

    while (!q.empty()) {
        int u = q.dequeue();
        order.push_back(u);
        for (int v : g.neighbors(u))
            if (--indegree[static_cast<std::size_t>(v)] == 0) q.enqueue(v);
    }

    if (order.size() == aliveCount) return true;

    if (stuck) {
        stuck->clear();
        for (int u = 0; u < cap; ++u)
            if (g.hasNode(u) && indegree[static_cast<std::size_t>(u)] > 0) stuck->push_back(u);
    }
    return false;
}

// Given the `stuck` nodes from a failed topological sort, return one actual
// cycle in forward order (a -> b -> c, and c leads back to a).
inline std::vector<int> extractCycle(const DiGraph& g, const std::vector<int>& stuck) {
    const std::size_t cap = static_cast<std::size_t>(g.capacity());
    std::vector<char> inStuck(cap, 0);
    for (int s : stuck) inStuck[static_cast<std::size_t>(s)] = 1;

    std::vector<int> pred(cap, -1);
    for (int u : stuck)
        for (int v : g.neighbors(u))
            if (inStuck[static_cast<std::size_t>(v)]) pred[static_cast<std::size_t>(v)] = u;

    std::vector<int> position(cap, -1), path;
    int current = stuck.front();
    while (position[static_cast<std::size_t>(current)] == -1) {
        position[static_cast<std::size_t>(current)] = static_cast<int>(path.size());
        path.push_back(current);
        current = pred[static_cast<std::size_t>(current)];
    }
    std::vector<int> cycle(path.begin() + position[static_cast<std::size_t>(current)], path.end());
    std::reverse(cycle.begin(), cycle.end());
    return cycle;
}

// ---------------------------------------------------------------------------
// Quick sort (median-of-three pivot, Lomuto partition).
// Average O(n log n), worst O(n^2).  The smaller side is always handled by
// recursion and the larger side by the loop, so stack depth stays O(log n).
// The comparator must give a strict ordering (tie-break on id) so results
// are deterministic.
// ---------------------------------------------------------------------------
template <typename T, typename Less>
void quickSort(std::vector<T>& a, long lo, long hi, Less less) {
    while (lo < hi) {
        const long mid = lo + (hi - lo) / 2;
        if (less(a[mid], a[lo])) std::swap(a[mid], a[lo]);
        if (less(a[hi], a[lo])) std::swap(a[hi], a[lo]);
        if (less(a[mid], a[hi])) std::swap(a[mid], a[hi]);   // a[hi] now holds the median

        long i = lo - 1;
        for (long j = lo; j < hi; ++j) {
            if (less(a[j], a[hi])) {
                ++i;
                if (i != j) std::swap(a[i], a[j]);
            }
        }
        if (i + 1 != hi) std::swap(a[i + 1], a[hi]);
        const long p = i + 1;

        if (p - lo < hi - p) {
            quickSort(a, lo, p - 1, less);
            lo = p + 1;
        } else {
            quickSort(a, p + 1, hi, less);
            hi = p - 1;
        }
    }
}

template <typename T, typename Less>
void quickSort(std::vector<T>& a, Less less) {
    if (a.size() > 1) quickSort(a, 0, static_cast<long>(a.size()) - 1, less);
}

#endif  // STRUCTURES_H
