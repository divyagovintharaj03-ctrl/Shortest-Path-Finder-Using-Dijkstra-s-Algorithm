# Shortest Path Finder 🗺️
### Data Structures Mini Project | C Core Engine & Modern Interactive Web UI

A complete, fully functional Route-Finding Application implementing **Dijkstra's Shortest Path Algorithm** in the **C programming language** using an **Adjacency List** and an **Indexed Binary Min-Heap Priority Queue** ($O((V + E) \log V)$), paired with a responsive, modern **Light Theme** web interface.

---

## 🌟 Key Features

1. **Pure C Core Backend**:
   - Shortest path calculations execute strictly within compiled C code.
   - Built-in embedded HTTP REST server (Winsock2 on Windows, POSIX sockets on Linux/macOS).
   - Dynamic memory management with defensive bounds and overflow checks.
   - Rejection of negative weights ($w \le 0$) and graceful handling of disconnected components or identical source/destination.
2. **Optimal Data Structures**:
   - **Weighted Graph**: Adjacency list representation supporting dynamic vertex and edge management.
   - **Binary Min-Heap**: Priority queue maintaining an inverted position index `pos[v]` for $O(\log V)$ decrease-key operations.
   - **Backtracking**: Reconstruction of exact vertex sequences and edge segment costs.
3. **Interactive Web Frontend**:
   - **Clean Light Theme**: White and subtle slate backgrounds (`#f8fafc`, `#ffffff`) with vibrant purple/violet accents (`#7c3aed`).
   - **Interactive Canvas Map**: High-DPI canvas featuring draggable vertices, edge weight pills, zoom, pan, and real-time path glow highlights.
   - **Step-by-Step Algorithm Walkthrough**: Play, pause, or step through actual C engine relaxation traces.
   - **Complete CRUD Controls**: Add, edit, and delete locations and roads with immediate CSV persistence.
4. **Persistence & Verification**:
   - Automatic real-time persistence to `data/locations.csv` and `data/roads.csv`.
   - Realistic 10-location, 17-road sample fictional road network.
   - 27-check automated C unit test suite verifying all 10 academic edge cases.

---

## 📁 Project Directory Structure

```
divi mini/
├── backend/
│   ├── include/
│   │   ├── graph.h              # Graph & Adjacency List data structures
│   │   ├── minheap.h            # Binary Min-Heap Priority Queue with indexed decrease-key
│   │   ├── dijkstra.h           # Dijkstra algorithm declarations & step trace structures
│   │   ├── storage.h            # CSV file persistence headers
│   │   └── server.h             # Embedded HTTP REST server header
│   ├── src/
│   │   ├── graph.c              # Graph operations, edge management & vertex removal
│   │   ├── minheap.c            # Min-heapify, insert, extract-min, decrease-key
│   │   ├── dijkstra.c           # Dijkstra implementation, edge relaxation & path reconstruction
│   │   ├── storage.c            # CSV loading, saving, and default data restore
│   │   ├── server.c             # Cross-platform HTTP socket daemon (Winsock & POSIX)
│   │   └── main.c               # Unified CLI, server, and test runner
│   └── tests/
│       └── test_pathfinder.c    # 27 automated unit tests covering all 10 required test cases
├── data/
│   ├── locations.csv            # Persistent vertex storage
│   ├── roads.csv                # Persistent edge storage
│   └── sample_network.json      # Structured sample dataset with manual verification proof
├── frontend/
│   ├── index.html               # Semantic light-theme single page web application
│   ├── css/
│   │   └── styles.css           # Modern Light Theme CSS (violet palette, soft shadows)
│   └── js/
│       ├── api.js               # REST communication client connecting to C engine
│       ├── graph_vis.js         # Interactive Canvas map with node dragging & zoom/pan
│       ├── dijkstra_demo.js     # Step-by-step algorithmic trace player
│       └── app.js               # Main application controller, tables & state
├── server.py                    # Alternative Python bridge server (invokes compiled C binary)
├── Makefile                     # Standard GCC Makefile for Windows, Linux, and macOS
├── build.bat                    # One-click Windows build script
├── run.bat                      # One-click Windows launch script
├── run.sh                       # One-click Linux/macOS launch script
├── README.md                    # Project overview & quickstart guide
└── PROJECT_REPORT.md            # Comprehensive academic report, viva Q&A, and lab record
```

---

## 🚀 Setup & Execution Guide

### Prerequisites
- **C Compiler**: GCC (MinGW / LLVM-MinGW on Windows, GCC/Clang on Linux/macOS).
- *(Optional)* **Python 3.8+**: If using the optional Python bridge server.

---

### Option A: Windows (One-Click)

1. **Build the C Executable and Run Unit Tests**:
   Double click `build.bat` or run in terminal:
   ```cmd
   build.bat
   ```
2. **Start the Application**:
   Double click `run.bat` or run:
   ```cmd
   run.bat
   ```
3. Open your browser to: **`http://localhost:8080`**

---

### Option B: Linux / macOS (Makefile)

1. **Compile**:
   ```bash
   make all
   ```
2. **Run Automated Test Suite**:
   ```bash
   make test
   ```
3. **Launch Server**:
   ```bash
   make serve
   ```
4. Open your browser to: **`http://localhost:8080`**

---

### Option C: Python Bridge Mode (Alternative)

If you prefer running via a Python bridge that invokes the compiled C binary:
```bash
python server.py
```
Then visit: **`http://localhost:8080`**

---

## 💻 CLI Commands (C Engine Directly)

You can invoke the compiled C backend binary directly from your terminal:

```bash
# Query shortest path between College (1) and Airport (10)
./pathfinder --query 1 10

# Query with step-by-step trace capture
./pathfinder --query 1 10 --trace

# Dump current locations, roads, and graph stats as JSON
./pathfinder --list

# Add a new location
./pathfinder --add-loc 11 STADIUM "Sports Stadium" 500 350

# Add a road
./pathfinder --add-road 1 11 3.2 1

# Reset persistent network to standard 10 locations & 17 roads
./pathfinder --reset

# Run embedded HTTP server on custom port
./pathfinder --serve 8080
```

---

## 🧪 Automated Unit Test Suite

The test suite in `backend/tests/test_pathfinder.c` validates all 10 project test requirements:
1. Direct shortest route.
2. Route requiring multiple intermediate hops ($A \to B \to C \to D$).
3. Multiple possible competing paths (picks 14.0 over 25.0).
4. Unreachable destination in disconnected graph components.
5. Source equals destination ($cost = 0.0$, path length = 1).
6. Single location with no roads.
7. Invalid and non-existent location IDs.
8. Zero and negative weight validation & rejection ($w \le 0$).
9. Updating and deleting roads followed by dynamic route recalculation.
10. File persistence across graph reloads.

**Run Tests**:
```bash
gcc -Wall -Wextra -O2 -Ibackend/include backend/src/graph.c backend/src/minheap.c backend/src/dijkstra.c backend/src/storage.c backend/tests/test_pathfinder.c -o test_pathfinder.exe
./test_pathfinder.exe
```
**Result**: 27 / 27 assertions passed (100.0%).

---

## 📊 Sample Verified Query

- **Source**: `College` (ID: 1)
- **Destination**: `Airport` (ID: 10)
- **Shortest Route**: `College (1)` $\to$ `Hospital (4)` $\to$ `Park (7)` $\to$ `Airport (10)`
- **Total Distance**: **18.00 km**
  - Segment 1: College $\to$ Hospital = $6.50\text{ km}$
  - Segment 2: Hospital $\to$ Park = $3.00\text{ km}$
  - Segment 3: Park $\to$ Airport = $8.50\text{ km}$
- **Alternate Suboptimal Routes Compared**:
  - College $\to$ Bus Stand $\to$ Library $\to$ City Centre $\to$ Airport = $18.70\text{ km}$
  - College $\to$ Hospital $\to$ Park $\to$ City Centre $\to$ Airport = $20.50\text{ km}$
  - College $\to$ Bus Stand $\to$ Railway Station $\to$ Market $\to$ Shopping Mall $\to$ Airport = $22.70\text{ km}$
