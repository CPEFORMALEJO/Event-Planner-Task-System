// planner.h
// Task record, validation helpers, and the Planner class that ties the data
// structures together.  The Planner does no console I/O; main.cpp handles that.

#ifndef PLANNER_H
#define PLANNER_H

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "structures.h"

// ---------------------------------------------------------------------------
// Task record and helpers
// ---------------------------------------------------------------------------
struct Task {
    int id = 0;
    std::string name;
    std::string assignee;
    std::string due;     // YYYY-MM-DD, so plain string comparison orders dates correctly
    int status = 0;      // 0 = Not started, 1 = Ongoing, 2 = Done
};

inline std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

inline std::string statusText(int s) {
    static const char* names[] = {"Not started", "Ongoing", "Done"};
    return (s >= 0 && s < 3) ? names[s] : "Unknown";
}

inline bool isValidDate(const std::string& d) {
    if (d.size() != 10 || d[4] != '-' || d[7] != '-') return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9})
        if (!std::isdigit(static_cast<unsigned char>(d[static_cast<std::size_t>(i)]))) return false;
    const int y = std::stoi(d.substr(0, 4));
    const int m = std::stoi(d.substr(5, 2));
    const int day = std::stoi(d.substr(8, 2));
    if (y < 2000 || y > 2100 || m < 1 || m > 12 || day < 1) return false;
    static const int daysIn[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysIn[m - 1];
    const bool leap = (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    if (m == 2 && leap) maxDay = 29;
    return day <= maxDay;
}

// Strict orderings (ties broken by id) for the sorted reports.
inline bool lessByName(const Task& a, const Task& b) {
    const std::string x = toLower(a.name), y = toLower(b.name);
    if (x != y) return x < y;
    return a.id < b.id;
}
inline bool lessByDue(const Task& a, const Task& b) {
    if (a.due != b.due) return a.due < b.due;
    return a.id < b.id;
}
inline bool lessByStatus(const Task& a, const Task& b) {
    if (a.status != b.status) return a.status < b.status;
    if (a.due != b.due) return a.due < b.due;
    return a.id < b.id;
}

// ---------------------------------------------------------------------------
// Planner
// ---------------------------------------------------------------------------
class Planner {
public:
    struct Result {
        bool ok;
        std::string message;
    };

private:
    struct State {                    // one undo / redo snapshot
        std::vector<Task> tasks;
        DiGraph graph;
        int nextId;
    };

    std::vector<Task> tasks_;         // dynamic array of task records
    DiGraph graph_;                   // dependency graph (adjacency list)
    int nextId_ = 1;
    std::string eventName_ = "Untitled Event";
    HashTable idIndex_;               // "id"        -> position in tasks_
    HashTable nameIndex_;             // lower(name) -> position in tasks_
    Stack<State> undo_, redo_;

    State capture() const { return State{tasks_, graph_, nextId_}; }

    void restore(const State& s) {
        tasks_ = s.tasks;
        graph_ = s.graph;
        nextId_ = s.nextId;
        rebuildIndexes();
    }

    void snapshot() {
        undo_.push(capture());
        redo_.clear();
    }

    void rebuildIndexes() {
        idIndex_.clear();
        nameIndex_.clear();
        for (std::size_t i = 0; i < tasks_.size(); ++i) {
            idIndex_.put(std::to_string(tasks_[i].id), static_cast<int>(i));
            nameIndex_.put(toLower(tasks_[i].name), static_cast<int>(i));
        }
    }

    Result validate(const std::string& name, const std::string& assignee, const std::string& due,
                    int status, int ignoreId) const {
        if (name.empty()) return {false, "Task name cannot be empty."};
        if (name.size() > 40) return {false, "Task name is too long (maximum 40 characters)."};
        if (assignee.empty()) return {false, "Assigned person cannot be empty."};
        if (assignee.size() > 30) return {false, "Assigned person is too long (maximum 30 characters)."};
        if (name.find('|') != std::string::npos || assignee.find('|') != std::string::npos)
            return {false, "The '|' character is not allowed."};
        if (!isValidDate(due)) return {false, "Invalid due date. Use YYYY-MM-DD, for example 2026-12-05."};
        if (status < 0 || status > 2) return {false, "Status must be Not started, Ongoing, or Done."};
        int pos;
        if (nameIndex_.get(toLower(name), pos) && tasks_[static_cast<std::size_t>(pos)].id != ignoreId)
            return {false, "Another task already uses that name."};
        return {true, ""};
    }

    int insertTask(const std::string& name, const std::string& assignee, const std::string& due,
                   int status) {
        Task t;
        t.id = nextId_++;
        t.name = name;
        t.assignee = assignee;
        t.due = due;
        t.status = status;
        tasks_.push_back(t);
        graph_.addNode(t.id);
        idIndex_.put(std::to_string(t.id), static_cast<int>(tasks_.size()) - 1);
        nameIndex_.put(toLower(t.name), static_cast<int>(tasks_.size()) - 1);
        return t.id;
    }

public:
    const std::vector<Task>& tasks() const { return tasks_; }
    const DiGraph& graph() const { return graph_; }
    const std::string& eventName() const { return eventName_; }

    Result setEventName(const std::string& name) {
        if (name.empty()) return {false, "Event name cannot be empty."};
        if (name.size() > 50) return {false, "Event name is too long (maximum 50 characters)."};
        if (name.find('|') != std::string::npos) return {false, "The '|' character is not allowed."};
        eventName_ = name;
        return {true, "Event name set to \"" + name + "\"."};
    }

    // ---- lookup (hash table) -------------------------------------------
    const Task* findById(int id) const {
        int pos;
        if (!idIndex_.get(std::to_string(id), pos)) return nullptr;
        return &tasks_[static_cast<std::size_t>(pos)];
    }

    const Task* findByName(const std::string& name) const {
        int pos;
        if (!nameIndex_.get(toLower(trim(name)), pos)) return nullptr;
        return &tasks_[static_cast<std::size_t>(pos)];
    }

    // ---- task records ----------------------------------------------------
    Result addTask(const std::string& name, const std::string& assignee, const std::string& due,
                   int status) {
        Result v = validate(name, assignee, due, status, -1);
        if (!v.ok) return v;
        snapshot();
        int id = insertTask(name, assignee, due, status);
        return {true, "Task added with ID " + std::to_string(id) + "."};
    }

    // Empty strings (and status -1) mean "keep the current value".
    Result editTask(int id, const std::string& name, const std::string& assignee,
                    const std::string& due, int status) {
        const Task* old = findById(id);
        if (!old) return {false, "No task with ID " + std::to_string(id) + "."};
        std::string n = name.empty() ? old->name : name;
        std::string a = assignee.empty() ? old->assignee : assignee;
        std::string d = due.empty() ? old->due : due;
        int s = (status < 0) ? old->status : status;
        Result v = validate(n, a, d, s, id);
        if (!v.ok) return v;
        snapshot();
        int pos;
        idIndex_.get(std::to_string(id), pos);
        Task& t = tasks_[static_cast<std::size_t>(pos)];
        t.name = n;
        t.assignee = a;
        t.due = d;
        t.status = s;
        rebuildIndexes();
        return {true, "Task " + std::to_string(id) + " updated."};
    }

    Result removeTask(int id) {
        int pos;
        if (!idIndex_.get(std::to_string(id), pos))
            return {false, "No task with ID " + std::to_string(id) + "."};
        snapshot();
        tasks_.erase(tasks_.begin() + pos);
        graph_.removeNode(id);
        rebuildIndexes();
        return {true, "Task " + std::to_string(id) + " and its dependency links were removed."};
    }

    // ---- dependencies (directed graph) ----------------------------------
    // prerequisite -> dependent : the prerequisite must be finished first.
    Result addDependency(int prerequisite, int dependent) {
        if (!findById(prerequisite) || !findById(dependent))
            return {false, "Both task IDs must exist."};
        if (prerequisite == dependent) return {false, "A task cannot depend on itself."};
        if (graph_.hasEdge(prerequisite, dependent))
            return {false, "Redundant link: that dependency already exists."};
        if (graph_.hasPath(dependent, prerequisite))
            return {false, "Rejected: this link would create a cycle (the prerequisite already "
                           "depends on the dependent task, directly or indirectly)."};
        if (graph_.hasPath(prerequisite, dependent))
            return {false, "Redundant link: the dependent task is already reachable through "
                           "other dependencies."};
        snapshot();
        graph_.addEdge(prerequisite, dependent);
        return {true, "Dependency added."};
    }

    Result removeDependency(int prerequisite, int dependent) {
        if (!findById(prerequisite) || !findById(dependent))
            return {false, "Both task IDs must exist."};
        if (!graph_.hasEdge(prerequisite, dependent)) return {false, "That dependency does not exist."};
        snapshot();
        graph_.removeEdge(prerequisite, dependent);
        return {true, "Dependency removed."};
    }

    std::vector<int> prerequisitesOf(int id) const {
        std::vector<int> result;
        for (const Task& t : tasks_)
            if (graph_.hasEdge(t.id, id)) result.push_back(t.id);
        return result;
    }

    // Kahn's algorithm.  Returns true with `order` filled, or false with one
    // cycle (by task id) in `cycle`.
    bool taskOrder(std::vector<int>& order, std::vector<int>& cycle) const {
        std::vector<int> stuck;
        if (topologicalOrder(graph_, order, &stuck)) return true;
        cycle = extractCycle(graph_, stuck);
        return false;
    }

    // Breadth-first search from a delayed task: every task that directly or
    // indirectly depends on it, paired with its distance (1 = direct).
    std::vector<std::pair<int, int>> downstream(int id) const {
        std::vector<std::pair<int, int>> affected;
        if (!graph_.hasNode(id)) return affected;
        std::vector<int> dist(static_cast<std::size_t>(graph_.capacity()), 0);
        std::vector<char> seen(static_cast<std::size_t>(graph_.capacity()), 0);
        Queue<int> q;
        q.enqueue(id);
        seen[static_cast<std::size_t>(id)] = 1;
        while (!q.empty()) {
            int u = q.dequeue();
            for (int v : graph_.neighbors(u)) {
                if (seen[static_cast<std::size_t>(v)]) continue;
                seen[static_cast<std::size_t>(v)] = 1;
                dist[static_cast<std::size_t>(v)] = dist[static_cast<std::size_t>(u)] + 1;
                affected.push_back({v, dist[static_cast<std::size_t>(v)]});
                q.enqueue(v);
            }
        }
        return affected;
    }

    // ---- sorted reports (quick sort) -------------------------------------
    // key: 1 = name, 2 = due date, 3 = status
    std::vector<Task> sortedBy(int key) const {
        std::vector<Task> copy = tasks_;
        if (key == 1) quickSort(copy, lessByName);
        else if (key == 2) quickSort(copy, lessByDue);
        else quickSort(copy, lessByStatus);
        return copy;
    }

    // ---- undo / redo (stacks) ---------------------------------------------
    Result undo() {
        if (undo_.empty()) return {false, "Nothing to undo."};
        redo_.push(capture());
        restore(undo_.top());
        undo_.pop();
        return {true, "Last action undone."};
    }

    Result redo() {
        if (redo_.empty()) return {false, "Nothing to redo."};
        undo_.push(capture());
        restore(redo_.top());
        redo_.pop();
        return {true, "Action redone."};
    }

    // ---- save / load -------------------------------------------------------
    Result save(const std::string& path) const {
        std::ofstream out(path);
        if (!out) return {false, "Cannot write to '" + path + "'."};
        out << "N|" << eventName_ << '\n';
        for (const Task& t : tasks_)
            out << "T|" << t.id << '|' << t.name << '|' << t.assignee << '|' << t.due << '|'
                << t.status << '\n';
        for (const Task& t : tasks_)
            for (int v : graph_.neighbors(t.id)) out << "E|" << t.id << '|' << v << '\n';
        return {true, "Saved " + std::to_string(tasks_.size()) + " tasks to '" + path + "'."};
    }

    Result load(const std::string& path) {
        std::ifstream in(path);
        if (!in) return {false, "Cannot open '" + path + "'."};

        std::vector<Task> loaded;
        std::vector<std::pair<int, int>> edges;
        std::string loadedName = "Untitled Event";
        std::string line;
        int lineNo = 0;
        while (std::getline(in, line)) {
            ++lineNo;
            if (trim(line).empty()) continue;
            std::vector<std::string> p;
            std::stringstream ss(line);
            std::string part;
            while (std::getline(ss, part, '|')) p.push_back(part);
            const std::string where = " (line " + std::to_string(lineNo) + ")";
            try {
                if (p[0] == "T" && p.size() == 6) {
                    Task t;
                    t.id = std::stoi(p[1]);
                    t.name = p[2];
                    t.assignee = p[3];
                    t.due = p[4];
                    t.status = std::stoi(p[5]);
                    if (t.id < 1 || t.name.empty() || t.assignee.empty() || !isValidDate(t.due) ||
                        t.status < 0 || t.status > 2)
                        return {false, "Invalid task record" + where + ". Nothing was loaded."};
                    loaded.push_back(t);
                } else if (p[0] == "N" && p.size() == 2 && !trim(p[1]).empty()) {
                    loadedName = trim(p[1]);
                } else if (p[0] == "E" && p.size() == 3) {
                    edges.push_back({std::stoi(p[1]), std::stoi(p[2])});
                } else {
                    return {false, "Unrecognized line" + where + ". Nothing was loaded."};
                }
            } catch (...) {
                return {false, "Malformed number" + where + ". Nothing was loaded."};
            }
        }

        HashTable seenIds, seenNames;
        int maxId = 0, dummy;
        for (const Task& t : loaded) {
            if (seenIds.get(std::to_string(t.id), dummy) || seenNames.get(toLower(t.name), dummy))
                return {false, "Duplicate task ID or name in file. Nothing was loaded."};
            seenIds.put(std::to_string(t.id), 1);
            seenNames.put(toLower(t.name), 1);
            maxId = std::max(maxId, t.id);
        }
        for (const auto& e : edges)
            if (!seenIds.get(std::to_string(e.first), dummy) ||
                !seenIds.get(std::to_string(e.second), dummy))
                return {false, "A dependency refers to a missing task. Nothing was loaded."};

        snapshot();
        tasks_ = loaded;
        graph_.clear();
        for (const Task& t : tasks_) graph_.addNode(t.id);
        for (const auto& e : edges) graph_.addEdge(e.first, e.second);
        nextId_ = maxId + 1;
        eventName_ = loadedName;
        rebuildIndexes();
        return {true, "Loaded " + std::to_string(tasks_.size()) + " tasks and " +
                          std::to_string(edges.size()) + " dependencies from '" + path + "'."};
    }

    // Replaces the current data with a small sample event (for demos and screenshots).
    Result loadSample() {
        snapshot();
        tasks_.clear();
        graph_.clear();
        nextId_ = 1;
        eventName_ = "Sample Wedding Reception";
        insertTask("Finalize guest list", "Maria", "2026-11-10", 2);
        insertTask("Book venue hall", "Jose", "2026-11-12", 1);
        insertTask("Hire caterer", "Ana", "2026-11-20", 0);
        insertTask("Send invitations", "Maria", "2026-11-15", 0);
        insertTask("Arrange sound system", "Carlo", "2026-12-01", 0);
        insertTask("Hall styling setup", "Ana", "2026-12-05", 0);
        insertTask("Final walkthrough", "Jose", "2026-12-07", 0);
        insertTask("Event day check-in", "Carlo", "2026-12-10", 0);
        const int links[][2] = {{1, 2}, {2, 3}, {2, 4}, {2, 5}, {2, 6},
                                {3, 7}, {5, 7}, {6, 7}, {4, 8}, {7, 8}};
        for (const auto& l : links) graph_.addEdge(l[0], l[1]);
        return {true, "Sample event loaded (8 tasks, 10 dependencies)."};
    }
};

#endif  // PLANNER_H