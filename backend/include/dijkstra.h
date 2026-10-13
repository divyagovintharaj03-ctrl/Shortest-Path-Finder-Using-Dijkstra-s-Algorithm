/**
 * @file dijkstra.h
 * @brief Dijkstra's Shortest Path Algorithm with Binary Min-Heap Priority Queue
 *
 * Algorithm Specifications:
 * - Time Complexity: O((V + E) log V) with adjacency list and indexed binary min-heap
 * - Space Complexity: O(V + E)
 * - Safe numeric handling: Uses 1e15 as infinity to prevent overflow
 * - Defensive verification: Checks for negative edge weights and loops
 * - Reconstructs shortest path from predecessor array
 * - Supports capturing step-by-step traces for visualization & demonstration
 */

#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include "graph.h"

#define DIJKSTRA_INF 1e15

/**
 * @struct DijkstraStepTrace
 * @brief Records an individual algorithmic event during execution
 */
typedef struct {
    int step_number;
    int current_vertex_index;
    int current_vertex_id;
    int neighbor_vertex_index;
    int neighbor_vertex_id;
    double edge_weight;
    double old_dist;
    double new_dist;
    int relaxed;                 /**< 1 if distance improved (relaxation), 0 otherwise */
    char description[256];       /**< Human-readable step log */
} DijkstraStepTrace;

/**
 * @struct DijkstraResult
 * @brief Encapsulates complete results of Dijkstra route computation
 */
typedef struct {
    int success;                 /**< 1 if path found, 0 if unreachable/error */
    int error_code;              /**< 0: success, -1: invalid args, -2: negative edge, -3: unreachable */
    char message[256];           /**< Detailed status message */
    int source_id;               /**< Requested source ID */
    int destination_id;          /**< Requested destination ID */
    double total_distance;       /**< Optimal total path cost/distance */
    int path_length;             /**< Number of vertices in shortest path */
    int* path_indices;           /**< Ordered graph indices from source to destination */
    int* path_ids;               /**< Ordered unique IDs from source to destination */
    double* distances;           /**< Final shortest distances from source to all V */
    int* predecessors;           /**< Predecessors for all V (-1 if none) */
    int visited_count;           /**< Number of vertices visited/extracted */
    int* visited_order;          /**< Ordered list of visited vertex indices */
    int trace_count;             /**< Number of step traces captured */
    DijkstraStepTrace* traces;   /**< Array of step traces */
} DijkstraResult;

/**
 * @brief Compute the shortest path from src_id to dest_id using Dijkstra's algorithm
 * @param g Pointer to Graph
 * @param src_id Unique ID of starting location
 * @param dest_id Unique ID of target destination
 * @param capture_trace 1 to capture step traces for demo/walkthrough, 0 for fast query
 * @return Allocated DijkstraResult (caller must free with dijkstra_free_result)
 */
DijkstraResult* dijkstra_find_shortest_path(const Graph* g, int src_id, int dest_id, int capture_trace);

/**
 * @brief Free memory allocated by DijkstraResult
 */
void dijkstra_free_result(DijkstraResult* res);

/**
 * @brief Print formatted shortest path calculation results to a stream
 */
void dijkstra_print_result(const DijkstraResult* res, const Graph* g, FILE* stream);

#endif /* DIJKSTRA_H */
