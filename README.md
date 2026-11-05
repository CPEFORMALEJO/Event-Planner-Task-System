# Event Planner Task Dependency System

CPE 010 Data Structures and Algorithms, Final Project (CPE21S4, Group 4).
A C++17 console program that tracks event tasks, their dependencies, assignments,
due dates and statuses, and produces a valid task order.

## Features
- Add, edit, view and remove tasks (ID, name, assigned person, due date, status)
- Dependency links with cycle and redundant-link detection
- Automatic task order (Kahn's topological sort); reports a cycle if one exists
- Downstream impact of a delayed task (breadth-first search)
- Search by task ID or name (hash table)
- Sorted reports by name, due date or status (quick sort)
- Undo / redo (stacks), dashboard, save and load to a text file

## Data structures and algorithms
Dynamic array, hash table (separate chaining), directed graph (adjacency list),
queue, stack, quick sort, Kahn's algorithm, breadth-first search.

## How to compile and run
    g++ -std=c++17 -O2 main.cpp -o planner
    ./planner            (Windows: planner.exe)

Option 16 in the menu loads a sample event so every feature can be tried at once.

## Benchmark (Section 5.2)
    g++ -std=c++17 -O2 benchmark.cpp -o benchmark
    ./benchmark          (writes benchmark_results.csv)

## Files
- structures.h : Stack, Queue, HashTable, DiGraph, topological sort, quick sort
- planner.h    : Task record, validation, Planner class (undo/redo, save/load)
- main.cpp     : console menu
- benchmark.cpp: timing code
- cyclic_sample.txt : sample file containing a circular dependency (load with option 15)

## Sample input
Option 16, then 9 (task order), then 7 with prerequisite 1 and dependent 3
(rejected as redundant), then 7 with prerequisite 8 and dependent 1 (rejected as a cycle).

## Team members
(List the names of all members.)
