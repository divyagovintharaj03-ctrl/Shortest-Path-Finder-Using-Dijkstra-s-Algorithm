# ACADEMIC PROJECT REPORT
## Title: Shortest Path Finder using Dijkstra's Algorithm
### Course: Data Structures and Algorithms Mini Project
### Backend: C Programming Language | Frontend: Modern Light Theme Web Interface

---

## 1. AIM & ALGORITHM (FOR PRACTICAL RECORD)

### AIM
To design, implement, and evaluate a route-finding application in the **C programming language** that models locations as vertices and roads as weighted edges in a graph, computing the minimum distance path between any chosen source and destination using **Dijkstra's Algorithm** with an **Adjacency List** and an **Indexed Binary Min-Heap Priority Queue**, accompanied by persistent file storage and a modern interactive web frontend.

---

### ALGORITHM

```text
Step 1: Start the program.
Step 2: Read the number of vertices V and edges E from persistent file storage (locations.csv, roads.csv).
Step 3: Construct the Graph using an Adjacency List where each vertex u maintains a linked list of edges (u, v, weight, direction).
Step 4: Prompt the user for Source vertex (src) and Destination vertex (dest).
Step 5: Validate the inputs:
        a. If src or dest is invalid or out of range, display an error message and exit.
        b. If any edge weight in the graph is <= 0, report invalid weight and exit.
        c. If src == dest, set distance = 0.0, path = [src], and proceed to Step 10.
Step 6: Initialize:
        a. For all vertices v in 0 to V-1:
           dist[v] = INFINITY (1e15)
           prev[v] = -1
        b. dist[src] = 0.0
        c. Insert all vertices into a Binary Min-Heap priority queue keyed by dist[v].
Step 7: While the Min-Heap is not empty:
        a. Extract the vertex u with minimum dist[u] from the Min-Heap (Extract-Min in O(log V)).
        b. If dist[u] == INFINITY, remaining vertices are unreachable; break the loop.
        c. If u == dest, destination has been settled with the optimal shortest distance; break the loop.
        d. For each adjacent edge (u, v) with weight w from Adj[u]:
           i. If v is not yet finalized and (dist[u] + w < dist[v]):
              - dist[v] = dist[u] + w
              - prev[v] = u
              - Update position of v in Min-Heap and bubble up (Decrease-Key in O(log V)).
Step 8: If dist[dest] == INFINITY:
        Print "No path exists between source and destination."
        Stop.
Step 9: Path Reconstruction:
        a. Start from curr = dest.
        b. While curr != -1:
           Append curr to route sequence.
           curr = prev[curr]
        c. Reverse the route sequence to obtain the ordered path from src to dest.
Step 10: Output the optimal total distance, number of segments, and sequence of locations.
Step 11: Stop the program.
```

---

## 2. PROJECT ABSTRACT
Route planning and shortest-path computation are foundational problems in computer science and real-world network routing systems (such as GPS navigation, telecommunication networks, and logistics). This project presents a high-performance **Shortest Path Finder** built with **C** as the core algorithmic computation engine and a modern **Light Theme Web Interface** for visual interaction. 

The application represents locations as vertices and roads as weighted edges in a dynamically allocated **Adjacency List**. Route queries are calculated using **Dijkstra's Algorithm** optimized via a custom **Binary Min-Heap Priority Queue**, achieving a time complexity of $O((V + E) \log V)$ instead of the traditional $O(V^2)$ adjacency matrix approach. The system incorporates real-time file persistence (CSV), comprehensive validation (rejection of non-positive edge weights, loop prevention, and disconnected graph detection), step-by-step algorithmic state tracing, and an interactive draggable canvas map.

---

## 3. PROBLEM STATEMENT
Given a network of $V$ physical locations connected by $E$ interconnecting roadways with positive distance costs ($w > 0$), calculate the most economical or shortest path from an arbitrary origin location $S$ to a destination location $D$. The system must:
1. Prevent algorithmic errors resulting from invalid memory access, integer overflow, or negative cycles.
2. Provide real-world scalability by avoiding quadratic time complexity ($O(V^2)$).
3. Ensure that calculations execute directly in compiled native code (C) rather than browser scripting.
4. Provide user-friendly visualization, location/road management, and transparent step-by-step execution traces for educational and engineering evaluation.

---

## 4. OBJECTIVES
1. **Core Data Structure**: Implement a memory-efficient weighted graph using an adjacency list in C.
2. **Optimal Priority Queue**: Develop a binary min-heap supporting $O(\log V)$ `extract-min` and $O(\log V)$ `decrease-key` using an inverted position array `pos[v]`.
3. **Robust Dijkstra Engine**: Calculate shortest paths with defensive guards against zero/negative weights, unreachable targets, and memory leaks.
4. **Interactive Light UI**: Design an accessible light-theme frontend adhering to modern UI standards (clean white/slate cards, violet primary accents, responsive layout).
5. **Interactive Graph Visualization**: Render vertices and edges on an HTML5 canvas with zoom, pan, and real-time path glow highlighting.
6. **Persistence**: Store and load vertices and edges persistently using CSV files.

---

## 5. EXISTING SYSTEM VS. PROPOSED SYSTEM

| Parameter | Existing Academic Systems | Proposed Shortest Path Finder |
| :--- | :--- | :--- |
| **Backend Language** | Basic CLI terminal programs in C/C++ or slow Python scripts | Highly optimized, modular C backend with embedded HTTP socket server |
| **Graph Storage** | Static 2D Adjacency Matrix ($O(V^2)$ space) | Dynamically allocated Adjacency List ($O(V + E)$ space) |
| **Priority Queue** | Linear search through distance array ($O(V)$ per extraction) | Indexed Binary Min-Heap ($O(\log V)$ extract & decrease-key) |
| **Time Complexity** | $O(V^2)$ — inefficient for sparse graphs | $O((V + E) \log V)$ — optimal for road networks |
| **User Interface** | Text-only console inputs (`printf`/`scanf`) | Modern Light Theme Web Application with SVG/Canvas visual map |
| **Path Visualization** | Text printout of IDs only | Draggable visual nodes, weight pills, glowing path highlights |
| **Persistence** | Hardcoded arrays in source code | Real-time CSV file storage (`locations.csv`, `roads.csv`) |
| **Step Walkthrough** | None | Real-time C execution trace step player (Play/Pause/Step) |

---

## 6. SYSTEM REQUIREMENTS

### Hardware Requirements
- **Processor**: Intel Core i3 / AMD Ryzen 3 or higher (x86_64 or ARM64).
- **RAM**: Minimum 512 MB (Project memory footprint < 10 MB).
- **Storage**: Minimum 20 MB free disk space.
- **Display**: 1024x768 minimum resolution (responsive for mobile and desktop).

### Software Requirements
- **Operating System**: Windows 10/11, Linux (Ubuntu, Debian, Fedora), or macOS.
- **C Compiler**: GCC 9.0+, Clang 10.0+, or LLVM-MinGW (with C99/C11 support).
- **Socket Library**: Winsock2 (`ws2_32`) on Windows; POSIX sockets on Linux/macOS.
- **Web Browser**: Any modern browser (Google Chrome, Mozilla Firefox, Microsoft Edge, Safari).

---

## 7. SYSTEM ARCHITECTURE

```
+---------------------------------------------------------------------------------+
|                                USER INTERFACE LAYER                             |
|  - Dashboard View   - Find Route View   - Canvas Visual Map   - Algorithm Demo  |
|  - Location CRUD    - Road CRUD         - Academic Record     - Toast System    |
+---------------------------------------------------------------------------------+
                                       ▲ │
                    REST API (HTTP/JSON) │ │ GET / POST
                                       │ ▼
+---------------------------------------------------------------------------------+
|                         C COMMUNICATION & SERVER LAYER                          |
|  - server.c : Embedded Socket Server (Winsock / POSIX) listening on :8080       |
|  - main.c   : Unified CLI subcommands (--query, --list, --add-loc, --reset)     |
+---------------------------------------------------------------------------------+
                                       ▲ │
                                       │ ▼
+---------------------------------------------------------------------------------+
|                            CORE C ALGORITHMIC ENGINE                            |
|  - dijkstra.c : Dijkstra Shortest Path Solver with Trace Recorder               |
|  - minheap.c  : Binary Min-Heap Priority Queue with pos[v] lookup               |
|  - graph.c    : Adjacency List Graph representation & edge management           |
|  - storage.c  : CSV File Persistence Engine (locations.csv, roads.csv)          |
+---------------------------------------------------------------------------------+
                                       ▲ │
                                       │ ▼
+---------------------------------------------------------------------------------+
|                                PERSISTENCE LAYER                                |
|  - data/locations.csv : Stores id, code, name, x, y coordinates                 |
|  - data/roads.csv     : Stores u_id, v_id, weight, bidirectional flag           |
+---------------------------------------------------------------------------------+
```

---

## 8. DETAILED DATA STRUCTURE EXPLANATIONS

### 1. Adjacency List Graph
An adjacency list represents a graph as an array of linked lists. Each index `i` in the array corresponds to vertex `i`. The linked list at `adj[i]` contains all outgoing edges originating from vertex `i`.

```c
typedef struct AdjListNode {
    int target_index;            /* Index of adjacent vertex */
    double weight;               /* Positive distance/cost */
    int is_bidirectional;        /* 1 if two-way, 0 if directed */
    struct AdjListNode* next;    /* Next adjacent edge */
} AdjListNode;

typedef struct {
    Location* locations;         /* Dynamic array of vertices */
    AdjListNode** adj;           /* Array of adjacency list heads */
    int num_vertices;            /* Vertex count */
    int capacity;                /* Allocated capacity */
} Graph;
```

**Why Adjacency List over Adjacency Matrix?**
- Road networks are **sparse graphs** where $E \ll V^2$.
- Space required: $O(V + E)$ vs. $O(V^2)$ for a matrix. For 1,000 locations, a matrix requires $1,000,000$ cells, whereas an adjacency list requires only the actual roads ($~3,000$ nodes).
- Iterating through neighbors of a vertex takes $O(\text{degree}(v))$ instead of $O(V)$.

---

### 2. Indexed Binary Min-Heap Priority Queue
In standard binary heaps, finding where an arbitrary vertex `v` is located in the heap takes $O(V)$ linear scan, which ruins the time complexity of `decrease_key`.

Our implementation solves this with an **inverted position array**:
```c
typedef struct {
    int vertex_index;
    double dist;
} MinHeapNode;

typedef struct {
    int size;
    int capacity;
    int* pos;            /* pos[v] = index in array[] where vertex v is stored */
    MinHeapNode** array; /* Binary heap tree stored in array */
} MinHeap;
```

When two nodes at heap indices `i` and `j` are swapped during `min_heapify` or `decrease_key`:
```c
void swap_heap_nodes(MinHeap* heap, int i, int j) {
    MinHeapNode* temp = heap->array[i];
    heap->array[i] = heap->array[j];
    heap->array[j] = temp;

    /* Update position array in O(1) */
    heap->pos[heap->array[i]->vertex_index] = i;
    heap->pos[heap->array[j]->vertex_index] = j;
}
```
This enables $O(1)$ lookup and $O(\log V)$ Decrease-Key!

---

## 9. TIME AND SPACE COMPLEXITY ANALYSIS

### Time Complexity
- **Initialization**: Initializing distance array and inserting $V$ vertices into min-heap: $O(V)$.
- **Extract-Min**: Executed at most $V$ times. Each extraction takes $O(\log V)$. Total: $O(V \log V)$.
- **Edge Relaxation & Decrease-Key**: Each edge $(u, v)$ is examined at most once (or twice for bidirectional roads). Each relaxation triggers `decrease_key` taking $O(\log V)$. Total: $O(E \log V)$.
- **Overall Worst-Case Time Complexity**:
  $$\mathbf{T(V, E) = O((V + E) \log V)}$$

### Space Complexity
- Adjacency List: $O(V + E)$
- Binary Min-Heap array and position tracker: $O(V)$
- Distance, Predecessor, and Settled arrays: $O(V)$
- **Overall Space Complexity**:
  $$\mathbf{S(V, E) = O(V + E)}$$

---

## 10. TEST CASES & EXPERIMENTAL VERIFICATION

All 10 required test cases were implemented in `backend/tests/test_pathfinder.c` and verified using GCC:

| # | Test Scenario | Input Data | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | Direct shortest route | $1 \to 2$ with weight $5.5$ | Distance = $5.50$, segments = 1 | Distance = $5.50$, segments = 1 | **PASS** |
| **2** | Multiple intermediate hops | $1 \to 2 (2), 2 \to 3 (3), 3 \to 4 (4)$ | Sequence: $1 \to 2 \to 3 \to 4$, dist = $9.0$ | Sequence: $1 \to 2 \to 3 \to 4$, dist = $9.0$ | **PASS** |
| **3** | Multiple competing routes | Route A: cost 25.0, Route B: cost 14.0 | Chooses Route B (14.0) | Distance = $14.00$, passes node 3 | **PASS** |
| **4** | Unreachable destination | Disconnected islands $1$ and $2$ | Returns success = 0, error code -3 | "No reachable path found" | **PASS** |
| **5** | Source == Destination | Query from $1$ to $1$ | Distance = $0.00$, path length = 1 | Distance = $0.00$, length = 1 | **PASS** |
| **6** | Single location, 0 roads | Graph with 1 vertex | Correct initialization, 0 roads | Vertex count = 1, roads = 0 | **PASS** |
| **7** | Invalid location IDs | Target ID = 999 or -5 | Rejection without segmentation fault | Error: Location ID not found | **PASS** |
| **8** | Zero & negative weights | Road with weight $0.0$ or $-4.5$ | Rejection at input validation | Rejected (code < 0) | **PASS** |
| **9** | Road update & deletion | Update cost from 10 to 3, then delete | Path cost updates to 3, then fails | Cost = 3.0, then unreachable | **PASS** |
| **10**| Persistent graph reloading | Write to CSV, reload into fresh graph | Reloaded graph matches original | Distance = 12.80 on reloaded graph | **PASS** |

**Summary**: 27 / 27 unit test assertions PASSED (100.0%).

---

## 11. SAMPLE VERIFIED ROUTE QUERY

- **Query**: Origin `College` (ID 1) $\to$ Destination `Airport` (ID 10)
- **Computed Shortest Route**:
  1. `College` (ID 1)
  2. `Hospital` (ID 4) [Road cost: 6.50 km]
  3. `Park` (ID 7) [Road cost: 3.00 km]
  4. `Airport` (ID 10) [Road cost: 8.50 km]
- **Total Distance**: **18.00 km**
- **Manual Verification Proof**:
  - Alternate Path 1 (via Bus Stand & Library): $1 \to 2 (4.0) \to 5 (3.5) \to 8 (5.2) \to 10 (6.0) = \mathbf{18.70\text{ km}}$
  - Alternate Path 2 (via Hospital, Park, City Centre): $1 \to 4 (6.5) \to 7 (3.0) \to 8 (5.0) \to 10 (6.0) = \mathbf{20.50\text{ km}}$
  - Alternate Path 3 (via Railway Station & Mall): $1 \to 2 (4.0) \to 3 (5.0) \to 6 (4.2) \to 9 (4.0) \to 10 (5.5) = \mathbf{22.70\text{ km}}$
  - $\mathbf{18.00\text{ km} < 18.70\text{ km} < 20.50\text{ km} < 22.70\text{ km}}$. The C engine output is mathematically optimal.

---

## 12. VIVA QUESTIONS AND MODEL ANSWERS

**Q1: What is the main difference between Dijkstra's algorithm and Breadth-First Search (BFS)?**
> *Answer:* BFS calculates shortest paths in an **unweighted graph** (where every edge has uniform cost 1), using a FIFO queue in $O(V + E)$ time. Dijkstra's algorithm computes shortest paths in a **weighted graph** with positive weights, using a priority queue (min-heap) to greedily settle the smallest tentative distance node in $O((V + E) \log V)$ time.

**Q2: What is the greedy choice property in Dijkstra's algorithm?**
> *Answer:* At every iteration, Dijkstra's algorithm greedily chooses the unsettled vertex $u$ with the minimum tentative distance $dist[u]$. Because all edge weights are non-negative, no subsequent path through other vertices could possibly reach $u$ with a smaller cost. Thus, $dist[u]$ is guaranteed to be optimal upon extraction.

**Q3: Why can't Dijkstra handle negative edge weights?**
> *Answer:* If an edge weight is negative, a future path could reduce the total distance to an already settled vertex. Since Dijkstra marks extracted nodes as finalized and never re-inserts them into the priority queue, it fails to find the true shortest path in the presence of negative weights. The **Bellman-Ford algorithm** ($O(V \cdot E)$) must be used instead.

**Q4: How do you prevent integer or floating-point overflow when using infinity?**
> *Answer:* Instead of using `DBL_MAX` (where adding any edge weight causes overflow or `+INF`), we define infinity as a sufficiently large safe constant (e.g., `1e15` or `1,000,000,000.0`). When relaxing edges, we also check `if (d_u < DIJKSTRA_INF / 2.0)` before computing `d_u + weight`.

**Q5: What is the purpose of the predecessor array (`prev[]`)?**
> *Answer:* Dijkstra's algorithm computes optimal scalar distances. To recover the actual sequence of vertices forming the route, whenever an edge $(u, v)$ is relaxed, we store `prev[v] = u`. Once the destination is settled, we trace backwards from destination to source: $dest \to prev[dest] \to prev[prev[dest]] \dots \to src$, and reverse the array.

---

## 13. CONCLUSION & FUTURE ENHANCEMENTS

### Conclusion
The Shortest Path Finder mini project successfully demonstrates how classic Data Structures (Graphs, Adjacency Lists, Linked Lists, and Binary Min-Heaps) combine with efficient algorithmic paradigms (Greedy Method, Edge Relaxation, and Backtracking) to solve real-world routing problems. By executing all calculations in a compiled C backend and connecting it via HTTP REST sockets to a modern Light Theme web frontend, the project bridges low-level systems programming with high-level user interface design.

### Future Enhancements
1. **A* Search Algorithm**: Incorporate Euclidean or Manhattan distance heuristics to accelerate point-to-point queries.
2. **Multi-Modal Route Optimization**: Add support for varying road types (highways, pedestrian walkways, tolls).
3. **Turn Restrictions & Time-Dependent Traffic**: Model real-time congestion penalties and one-way time windows.
4. **GeoJSON & OpenStreetMap Integration**: Import real city spatial datasets via latitude and longitude coordinates.
