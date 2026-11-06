#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#include "planner.h"

static std::string ask(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s)) {
        std::cout << "\nInput closed. Exiting.\n";
        exit(0);
    }
    return trim(s);
}

static int askInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::string s = ask(prompt);
        try {
            std::size_t used = 0;
            long v = std::stol(s, &used);
            if (used == s.size() && v >= lo && v <= hi) return static_cast<int>(v);
        } catch (...) {
        }
        std::cout << "Invalid input. Enter a whole number from " << lo << " to " << hi << ".\n";
    }
}

static std::string askDate(const std::string& prompt, bool allowBlank) {
    while (true) {
        std::string s = ask(prompt);
        if (s.empty() && allowBlank) return s;
        if (isValidDate(s)) return s;
        std::cout << "Invalid date. Use YYYY-MM-DD, for example 2026-12-05.\n";
    }
}

static int askStatus(bool allowBlank) {
    while (true) {
        std::string s = ask("Status (1 = Not started, 2 = Ongoing, 3 = Done)" +
                       std::string(allowBlank ? ", Enter to keep: " : ": "));
        if (s.empty() && allowBlank) return -1;
        if (s == "1" || s == "2" || s == "3") return std::stoi(s) - 1;
        std::cout << "Invalid status. Enter 1, 2, or 3.\n";
    }
}

static void report(const Planner::Result& r) { std::cout << (r.ok ? "[OK] " : "[Error] ") << r.message << "\n"; }

template <typename Array>
static std::string idList(const Array& ids) {
    if (ids.empty()) return "-";
    std::string out;
    for (std::size_t i = 0; i < ids.size(); ++i) out += (i ? "," : "") + std::to_string(ids[i]);
    return out;
}

template <typename Array>
static void printTasks(const Planner& p, const Array& list) {
    std::cout << "Event: " << p.eventName() << "\n";
    if (list.empty()) {
        std::cout << "No tasks to show.\n";
        return;
    }
    std::cout << std::left << std::setw(5) << "ID" << std::setw(28) << "Task" << std::setw(14) << "Assigned To" << std::setw(12)
         << "Due Date" << std::setw(13) << "Status" << "Needs (IDs)\n";
    std::cout << std::string(80, '-') << "\n";
    for (const Task& t : list)
        std::cout << std::left << std::setw(5) << t.id << std::setw(28) << t.name << std::setw(14) << t.assignee << std::setw(12)
             << t.due << std::setw(13) << statusText(t.status) << idList(p.prerequisitesOf(t.id)) << "\n";
}

static void printTaskLine(const Task& t) {
    std::cout << "  [" << t.id << "] " << t.name << " | " << t.assignee << " | due " << t.due << " | "
         << statusText(t.status) << "\n";
}

static void showMenu(const Planner& p) {
    std::cout << "\n==== EVENT PLANNER TASK DEPENDENCY SYSTEM ====\n"
         << "Event: " << p.eventName() << "\n"
         << " 1  Add task              9  Generate task order\n"
         << " 2  Edit task            10  Show downstream impact\n"
         << " 3  Remove task          11  Dashboard\n"
         << " 4  View all tasks       12  Undo\n"
         << " 5  Search task          13  Redo\n"
         << " 6  Sorted report        14  Save to file\n"
         << " 7  Add dependency       15  Load from file\n"
         << " 8  Remove dependency    16  Load sample event\n"
         << "17  Set event name\n"
         << " 0  Exit\n";
}

static void addTask(Planner& p) {
    std::string name = ask("Task name: ");
    std::string who = ask("Assigned person: ");
    std::string due = askDate("Due date (YYYY-MM-DD): ", false);
    int status = askStatus(false);
    report(p.addTask(name, who, due, status));
}

static void editTask(Planner& p) {
    int id = askInt("Task ID to edit: ", 1, 999999);
    if (!p.findById(id)) {
        std::cout << "[Error] No task with ID " << id << ".\n";
        return;
    }
    std::cout << "Press Enter on any field to keep its current value.\n";
    std::string name = ask("New name: ");
    std::string who = ask("New assigned person: ");
    std::string due = askDate("New due date (YYYY-MM-DD): ", true);
    int status = askStatus(true);
    report(p.editTask(id, name, who, due, status));
}

static void removeTask(Planner& p) {
    int id = askInt("Task ID to remove: ", 1, 999999);
    const Task* t = p.findById(id);
    if (!t) {
        std::cout << "[Error] No task with ID " << id << ".\n";
        return;
    }
    std::string confirm = ask("Remove \"" + t->name + "\" and its links? (y/n): ");
    if (confirm == "y" || confirm == "Y") report(p.removeTask(id));
    else std::cout << "Cancelled.\n";
}

static void searchTask(const Planner& p) {
    int mode = askInt("Search by (1 = ID, 2 = name): ", 1, 2);
    const Task* t = nullptr;
    if (mode == 1) t = p.findById(askInt("Task ID: ", 1, 999999));
    else t = p.findByName(ask("Task name (exact, not case sensitive): "));
    if (!t) {
        std::cout << "[Not found] No matching task.\n";
        return;
    }
    std::cout << "Found:\n";
    printTaskLine(*t);
    std::cout << "  Needs (prerequisites): " << idList(p.prerequisitesOf(t->id)) << "\n";
}

static void sortedReport(const Planner& p) {
    int key = askInt("Sort by (1 = task name, 2 = due date, 3 = status): ", 1, 3);
    static const char* label[] = {"", "task name", "due date", "status"};
    std::cout << "Report sorted by " << label[key] << ":\n";
    printTasks(p, p.sortedBy(key));
}

static void addDependency(Planner& p) {
    std::cout << "Link: the prerequisite must be finished before the dependent task can start.\n";
    int a = askInt("Prerequisite task ID: ", 1, 999999);
    int b = askInt("Dependent task ID: ", 1, 999999);
    report(p.addDependency(a, b));
}

static void removeDependency(Planner& p) {
    int a = askInt("Prerequisite task ID: ", 1, 999999);
    int b = askInt("Dependent task ID: ", 1, 999999);
    report(p.removeDependency(a, b));
}

static void generateOrder(const Planner& p) {
    if (p.tasks().empty()) {
        std::cout << "No tasks to order.\n";
        return;
    }
    DynamicArray<int> order; // transition from std::vector to DynamicArray (structure.h)
    DynamicArray<int> cycle;
    if (p.taskOrder(order, cycle)) {
        std::cout << "Valid task order (every task comes after its prerequisites):\n";
        for (std::size_t i = 0; i < order.size(); ++i) {
            const Task* t = p.findById(order[i]);
            std::cout << "  " << (i + 1) << ". " << t->name << "  (ID " << t->id << ", due " << t->due << ")\n";
        }
    } else {
        std::cout << "[Cycle detected] No valid order exists. Circular dependency:\n  ";
        for (int id : cycle) std::cout << p.findById(id)->name << " -> ";
        std::cout << p.findById(cycle.front())->name << "\nRemove one of these links and try again.\n";
    }
}

static void showImpact(const Planner& p) {
    int id = askInt("ID of the delayed task: ", 1, 999999);
    const Task* t = p.findById(id);
    if (!t) {
        std::cout << "[Error] No task with ID " << id << ".\n";
        return;
    }
    auto affected = p.downstream(id);
    std::cout << "If \"" << t->name << "\" is delayed, ";
    if (affected.empty()) {
        std::cout << "no other task is affected.\n";
        return;
    }
    std::cout << affected.size() << " task(s) are affected:\n";
    for (const auto& a : affected) {
        std::cout << (a.second == 1 ? "  direct:   " : "  indirect: ");
        printTaskLine(*p.findById(a.first));
    }
}

static void dashboard(const Planner& p) {
    int counts[3] = {0, 0, 0};
    auto open = p.tasks();
    open.clear();
    for (const Task& t : p.tasks()) {
        ++counts[t.status];
        if (t.status != 2) open.push_back(t);
    }
    quickSort(open, lessByDue);
    std::cout << "---- DASHBOARD: " << p.eventName() << " ----\n"
         << "Total tasks: " << p.tasks().size() << "   Dependency links: " << p.graph().edgeCount() << "\n"
         << "Not started: " << counts[0] << "   Ongoing: " << counts[1] << "   Done: " << counts[2] << "\n";
    const std::size_t total = p.tasks().size();
    const int tenths = total ? static_cast<int>((counts[2] * 1000 + total / 2) / total) : 0;
    std::cout << "Completion rate: " << tenths / 10 << "." << tenths % 10 << "%\n";
    std::cout << "Next deadlines (not done):\n";
    if (open.empty()) std::cout << "  none\n";
    for (std::size_t i = 0; i < open.size() && i < 3; ++i) printTaskLine(open[i]);
}

static std::string askFile(const std::string& prompt) {
    std::string s = ask(prompt + " [default event_tasks.txt]: ");
    return s.empty() ? "event_tasks.txt" : s;
}

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    Planner planner;
    std::cout << "Welcome to the Event Planner Task Dependency System.\n";

    while (true) {
        showMenu(planner);
        int choice = askInt("Choose an option: ", 0, 17);
        std::cout << "\n";
        switch (choice) {
            case 0:
                std::cout << "Goodbye.\n";
                return 0;
            case 1: addTask(planner); break;
            case 2: editTask(planner); break;
            case 3: removeTask(planner); break;
            case 4: printTasks(planner, planner.tasks()); break;
            case 5: searchTask(planner); break;
            case 6: sortedReport(planner); break;
            case 7: addDependency(planner); break;
            case 8: removeDependency(planner); break;
            case 9: generateOrder(planner); break;
            case 10: showImpact(planner); break;
            case 11: dashboard(planner); break;
            case 12: report(planner.undo()); break;
            case 13: report(planner.redo()); break;
            case 14: report(planner.save(askFile("File to save to"))); break;
            case 15: report(planner.load(askFile("File to load"))); break;
            case 16: report(planner.loadSample()); break;
            case 17: report(planner.setEventName(ask("Event name: "))); break;
        }
    }
}