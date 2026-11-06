#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

template <typename T>
class DynamicArray {
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;

    void reallocate(std::size_t newCapacity) {
        T* fresh = new T[newCapacity];
        for (std::size_t i = 0; i < size_; ++i) fresh[i] = std::move(data_[i]);
        delete[] data_;
        data_ = fresh;
        capacity_ = newCapacity;
    }

    void grow() { reallocate(capacity_ == 0 ? 4 : capacity_ * 2); }

public:
    DynamicArray() = default;

    explicit DynamicArray(std::size_t count)
        : data_(count ? new T[count]() : nullptr), size_(count), capacity_(count) {}

    DynamicArray(std::size_t count, const T& value)
        : data_(count ? new T[count] : nullptr), size_(count), capacity_(count) {
        for (std::size_t i = 0; i < size_; ++i) data_[i] = value;
    }

    DynamicArray(const DynamicArray& other)
        : data_(other.size_ ? new T[other.size_] : nullptr), size_(other.size_), capacity_(other.size_) {
        for (std::size_t i = 0; i < size_; ++i) data_[i] = other.data_[i];
    }

    DynamicArray(DynamicArray&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    DynamicArray& operator=(DynamicArray other) noexcept {
        swap(other);
        return *this;
    }

    ~DynamicArray() { delete[] data_; }

    void swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    void push_back(T value) {
        if (size_ == capacity_) grow();
        data_[size_++] = std::move(value);
    }

    void pop_back() {
        if (size_ > 0) --size_;
    }

    void insert(std::size_t index, T value) {
        if (index > size_) return;
        if (size_ == capacity_) grow();
        for (std::size_t i = size_; i > index; --i) data_[i] = std::move(data_[i - 1]);
        data_[index] = std::move(value);
        ++size_;
    }

    void erase(std::size_t index) {
        if (index >= size_) return;
        for (std::size_t i = index; i + 1 < size_; ++i) data_[i] = std::move(data_[i + 1]);
        --size_;
    }

    std::size_t indexOf(const T& value) const {
        for (std::size_t i = 0; i < size_; ++i)
            if (data_[i] == value) return i;
        return size_;
    }

    bool contains(const T& value) const { return indexOf(value) != size_; }

    void removeAll(const T& value) {
        std::size_t kept = 0;
        for (std::size_t i = 0; i < size_; ++i)
            if (!(data_[i] == value)) {
                if (kept != i) data_[kept] = std::move(data_[i]);
                ++kept;
            }
        size_ = kept;
    }

    void resize(std::size_t newSize, const T& fill = T()) {
        if (newSize > capacity_) reallocate(std::max(newSize, capacity_ * 2));
        for (std::size_t i = size_; i < newSize; ++i) data_[i] = fill;
        size_ = newSize;
    }

    void clear() { size_ = 0; }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }
    T& front() { return data_[0]; }
    const T& front() const { return data_[0]; }
    T& back() { return data_[size_ - 1]; }
    const T& back() const { return data_[size_ - 1]; }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
};

template <typename T>
class Stack {
    DynamicArray<T> data_;

public:
    void push(const T& value) { data_.push_back(value); }
    void pop() { data_.pop_back(); }
    T& top() { return data_.back(); }
    bool empty() const { return data_.empty(); }
    std::size_t size() const { return data_.size(); }
    void clear() { data_.clear(); }
};

template <typename T>
class Queue {
    DynamicArray<T> data_;
    std::size_t head_ = 0;

public:
    void enqueue(const T& value) { data_.push_back(value); }
    T dequeue() { return data_[head_++]; }
    bool empty() const { return head_ >= data_.size(); }
    std::size_t size() const { return data_.size() - head_; }
};

class HashTable {
    struct Entry {
        std::string key;
        int value = 0;
    };

    using Bucket = DynamicArray<Entry>;

    DynamicArray<Bucket> buckets_;
    std::size_t count_ = 0;

    static std::size_t hashString(const std::string& s) {
        std::size_t h = 5381;
        for (unsigned char c : s) h = h * 33 + c;
        return h;
    }

    void rehash(std::size_t newSize) {
        DynamicArray<Bucket> fresh(newSize);
        for (const Bucket& bucket : buckets_)
            for (const Entry& e : bucket) fresh[hashString(e.key) % newSize].push_back(e);
        buckets_.swap(fresh);
    }

public:
    explicit HashTable(std::size_t initialBuckets = 16) : buckets_(initialBuckets) {}

    void put(const std::string& key, int value) {
        if ((count_ + 1) * 4 > buckets_.size() * 3) rehash(buckets_.size() * 2);
        Bucket& bucket = buckets_[hashString(key) % buckets_.size()];
        for (Entry& e : bucket) {
            if (e.key == key) {
                e.value = value;
                return;
            }
        }
        bucket.push_back(Entry{key, value});
        ++count_;
    }

    bool get(const std::string& key, int& value) const {
        const Bucket& bucket = buckets_[hashString(key) % buckets_.size()];
        for (const Entry& e : bucket) {
            if (e.key == key) {
                value = e.value;
                return true;
            }
        }
        return false;
    }

    bool erase(const std::string& key) {
        Bucket& bucket = buckets_[hashString(key) % buckets_.size()];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].key == key) {
                bucket.erase(i);
                --count_;
                return true;
            }
        }
        return false;
    }

    void clear() {
        buckets_ = DynamicArray<Bucket>(16);
        count_ = 0;
    }

    std::size_t size() const { return count_; }
};

class DiGraph {
    DynamicArray<DynamicArray<int>> adj_;
    DynamicArray<char> alive_;

public:
    void addNode(int id) {
        if (id >= static_cast<int>(adj_.size())) {
            adj_.resize(static_cast<std::size_t>(id) + 1);
            alive_.resize(static_cast<std::size_t>(id) + 1, 0);
        }
        alive_[static_cast<std::size_t>(id)] = 1;
    }

    void removeNode(int id) {
        if (!hasNode(id)) return;
        adj_[static_cast<std::size_t>(id)].clear();
        alive_[static_cast<std::size_t>(id)] = 0;
        for (DynamicArray<int>& list : adj_) list.removeAll(id);
    }

    bool hasNode(int id) const {
        return id >= 0 && id < static_cast<int>(alive_.size()) && alive_[static_cast<std::size_t>(id)];
    }

    int capacity() const { return static_cast<int>(adj_.size()); }

    void addEdge(int from, int to) { adj_[static_cast<std::size_t>(from)].push_back(to); }

    bool hasEdge(int from, int to) const { return adj_[static_cast<std::size_t>(from)].contains(to); }

    void removeEdge(int from, int to) { adj_[static_cast<std::size_t>(from)].removeAll(to); }

    const DynamicArray<int>& neighbors(int id) const { return adj_[static_cast<std::size_t>(id)]; }

    std::size_t edgeCount() const {
        std::size_t total = 0;
        for (const DynamicArray<int>& list : adj_) total += list.size();
        return total;
    }

    void clear() {
        adj_ = DynamicArray<DynamicArray<int>>();
        alive_ = DynamicArray<char>();
    }

    bool hasPath(int from, int to) const {
        if (!hasNode(from) || !hasNode(to)) return false;
        DynamicArray<char> seen(adj_.size(), 0);
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

inline bool topologicalOrder(const DiGraph& g, DynamicArray<int>& order,
                             DynamicArray<int>* stuck = nullptr) {
    order.clear();
    const int cap = g.capacity();
    DynamicArray<int> indegree(static_cast<std::size_t>(cap), 0);
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

inline DynamicArray<int> extractCycle(const DiGraph& g, const DynamicArray<int>& stuck) {
    const std::size_t cap = static_cast<std::size_t>(g.capacity());
    DynamicArray<char> inStuck(cap, 0);
    for (int s : stuck) inStuck[static_cast<std::size_t>(s)] = 1;

    DynamicArray<int> pred(cap, -1);
    for (int u : stuck)
        for (int v : g.neighbors(u))
            if (inStuck[static_cast<std::size_t>(v)]) pred[static_cast<std::size_t>(v)] = u;

    DynamicArray<int> position(cap, -1), path;
    int current = stuck.front();
    while (position[static_cast<std::size_t>(current)] == -1) {
        position[static_cast<std::size_t>(current)] = static_cast<int>(path.size());
        path.push_back(current);
        current = pred[static_cast<std::size_t>(current)];
    }
    DynamicArray<int> cycle;
    for (std::size_t i = path.size(); i-- > static_cast<std::size_t>(position[static_cast<std::size_t>(current)]);)
        cycle.push_back(path[i]);
    return cycle;
}

template <typename T, typename Less>
void quickSort(DynamicArray<T>& a, long lo, long hi, Less less) {
    while (lo < hi) {
        const long mid = lo + (hi - lo) / 2;
        if (less(a[mid], a[lo])) std::swap(a[mid], a[lo]);
        if (less(a[hi], a[lo])) std::swap(a[hi], a[lo]);
        if (less(a[mid], a[hi])) std::swap(a[mid], a[hi]);

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
void quickSort(DynamicArray<T>& a, Less less) {
    if (a.size() > 1) quickSort(a, 0, static_cast<long>(a.size()) - 1, less);
}

#endif