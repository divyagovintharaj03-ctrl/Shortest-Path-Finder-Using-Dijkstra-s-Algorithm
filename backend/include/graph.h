/**
 * @file graph.h
 * @brief Weighted Graph Representation using Adjacency List for Shortest Path Finder
 *
 * Core Data Structures:
 * - Location (Vertex): ID, code, name, and 2D canvas coordinates (x, y)
 * - AdjListNode (Edge): target vertex index, positive weight, bidirectional flag, next pointer
 * - Graph: dynamic array of locations and adjacency lists
 */

#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_CODE_LEN 32
#define MAX_NAME_LEN 64
#define DEFAULT_GRAPH_CAPACITY 64

/**
 * @struct Location
 * @brief Represents a vertex (node) in the route network
 */
typedef struct {
    int id;                      /**< Unique positive integer ID */
    char code[MAX_CODE_LEN];     /**< Short unique code (e.g., "COLLEGE") */
    char name[MAX_NAME_LEN];     /**< Descriptive location name */
    double x;                    /**< 2D layout X coordinate for graph visualization */
    double y;                    /**< 2D layout Y coordinate for graph visualization */
} Location;

/**
 * @struct AdjListNode
 * @brief Represents a directed or bidirectional edge in the adjacency list
 */
typedef struct AdjListNode {
    int target_index;            /**< Index in Graph->locations array */
    double weight;               /**< Positive distance/travel time/cost */
    int is_bidirectional;        /**< 1 if bidirectional, 0 if directed/one-way */
    struct AdjListNode* next;    /**< Pointer to next adjacent edge */
} AdjListNode;

/**
 * @struct Graph
 * @brief Graph data structure maintaining vertices and adjacency lists
 */
typedef struct {
    Location* locations;         /**< Array of locations (vertices) */
    AdjListNode** adj;           /**< Array of pointers to adjacency lists */
    int num_vertices;            /**< Current count of active vertices */
    int capacity;                /**< Allocated memory capacity for vertices */
} Graph;

/* Lifecycle functions */
Graph* graph_create(int initial_capacity);
void graph_free(Graph* g);
void graph_clear(Graph* g);

/* Vertex (Location) operations */
int graph_add_location(Graph* g, int id, const char* code, const char* name, double x, double y);
int graph_update_location(Graph* g, int id, const char* name, double x, double y);
int graph_delete_location(Graph* g, int id);
int graph_find_index_by_id(const Graph* g, int id);
int graph_find_index_by_code(const Graph* g, const char* code);
const Location* graph_get_location_by_id(const Graph* g, int id);

/* Edge (Road) operations */
int graph_add_road(Graph* g, int u_id, int v_id, double weight, int is_bidirectional);
int graph_update_road(Graph* g, int u_id, int v_id, double new_weight, int is_bidirectional);
int graph_delete_road(Graph* g, int u_id, int v_id);
int graph_road_exists(const Graph* g, int u_id, int v_id, double* out_weight, int* out_bidir);

/* Utility & diagnostic functions */
int graph_count_roads(const Graph* g, int* out_bidirectional, int* out_oneway);
void graph_print_summary(const Graph* g, FILE* stream);

#endif /* GRAPH_H */
