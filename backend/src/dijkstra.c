/**
 * @file dijkstra.c
 * @brief Complete implementation of Dijkstra's algorithm using Adjacency List and Min-Heap
 *
 * Detailed Algorithm Walkthrough:
 * 1. Initial State:
 *    - dist[src] = 0, dist[v] = INF for all v != src
 *    - prev[v] = -1 for all v
 *    - All vertices inserted into priority queue (binary min-heap)
 * 2. Processing (Greedy Choice):
 *    - Extract vertex u with minimum dist[u] from min-heap (O(log V))
 *    - Settle/mark u as finalized
 * 3. Relaxation:
 *    - For each directed edge (u, v) with weight w:
 *      if dist[u] + w < dist[v]:
 *         dist[v] = dist[u] + w
 *         prev[v] = u
 *         decrease_key in min-heap (O(log V))
 * 4. Path Reconstruction:
 *    - Trace backward from dest using prev[] to src, then reverse.
 */

#include "../include/dijkstra.h"
#include "../include/minheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void add_trace_entry(DijkstraResult* res, int* trace_cap,
                            int cur_idx, int cur_id,
                            int nbr_idx, int nbr_id,
                            double weight, double old_d, double new_d,
                            int relaxed, const char* desc) {
    if (!res->traces) return;
    if (res->trace_count >= *trace_cap) {
        *trace_cap *= 2;
        DijkstraStepTrace* new_traces = (DijkstraStepTrace*)realloc(res->traces, sizeof(DijkstraStepTrace) * (*trace_cap));
        if (!new_traces) return;
        res->traces = new_traces;
    }

    DijkstraStepTrace* t = &res->traces[res->trace_count++];
    t->step_number = res->trace_count;
    t->current_vertex_index = cur_idx;
    t->current_vertex_id = cur_id;
    t->neighbor_vertex_index = nbr_idx;
    t->neighbor_vertex_id = nbr_id;
    t->edge_weight = weight;
    t->old_dist = old_d;
    t->new_dist = new_d;
    t->relaxed = relaxed;
    strncpy(t->description, desc, sizeof(t->description) - 1);
    t->description[sizeof(t->description) - 1] = '\0';
}

DijkstraResult* dijkstra_find_shortest_path(const Graph* g, int src_id, int dest_id, int capture_trace) {
    DijkstraResult* res = (DijkstraResult*)calloc(1, sizeof(DijkstraResult));
    if (!res) return NULL;

    res->source_id = src_id;
    res->destination_id = dest_id;

    if (!g || g->num_vertices == 0) {
        res->success = 0;
        res->error_code = -1;
        snprintf(res->message, sizeof(res->message), "Error: Graph is empty or uninitialized.");
        return res;
    }

    int src_idx = graph_find_index_by_id(g, src_id);
    int dest_idx = graph_find_index_by_id(g, dest_id);

    if (src_idx < 0) {
        res->success = 0;
        res->error_code = -1;
        snprintf(res->message, sizeof(res->message), "Error: Source location ID %d not found.", src_id);
        return res;
    }

    if (dest_idx < 0) {
        res->success = 0;
        res->error_code = -1;
        snprintf(res->message, sizeof(res->message), "Error: Destination location ID %d not found.", dest_id);
        return res;
    }

    /* Verify non-negative weights throughout graph */
    for (int i = 0; i < g->num_vertices; i++) {
        AdjListNode* edge = g->adj[i];
        while (edge) {
            if (edge->weight < 0.0) {
                res->success = 0;
                res->error_code = -2;
                snprintf(res->message, sizeof(res->message),
                         "Error: Negative edge weight (%.2f) detected. Dijkstra's algorithm requires non-negative weights.",
                         edge->weight);
                return res;
            }
            edge = edge->next;
        }
    }

    int V = g->num_vertices;
    res->distances = (double*)malloc(sizeof(double) * V);
    res->predecessors = (int*)malloc(sizeof(int) * V);
    res->visited_order = (int*)malloc(sizeof(int) * V);
    int* settled = (int*)calloc(V, sizeof(int));

    int trace_cap = 64;
    if (capture_trace) {
        res->traces = (DijkstraStepTrace*)malloc(sizeof(DijkstraStepTrace) * trace_cap);
    }

    if (!res->distances || !res->predecessors || !res->visited_order || !settled) {
        if (settled) free(settled);
        dijkstra_free_result(res);
        return NULL;
    }

    /* Handle identical source and destination */
    if (src_id == dest_id) {
        res->success = 1;
        res->error_code = 0;
        res->total_distance = 0.0;
        res->path_length = 1;
        res->path_indices = (int*)malloc(sizeof(int));
        res->path_ids = (int*)malloc(sizeof(int));
        res->path_indices[0] = src_idx;
        res->path_ids[0] = src_id;
        for (int i = 0; i < V; i++) {
            res->distances[i] = (i == src_idx) ? 0.0 : DIJKSTRA_INF;
            res->predecessors[i] = -1;
        }
        res->visited_count = 1;
        res->visited_order[0] = src_idx;
        snprintf(res->message, sizeof(res->message), "Source and destination are the same location.");
        free(settled);
        return res;
    }

    /* Initialize distances and min-heap */
    MinHeap* heap = min_heap_create(V);
    for (int v = 0; v < V; v++) {
        res->distances[v] = DIJKSTRA_INF;
        res->predecessors[v] = -1;
        min_heap_insert(heap, v, DIJKSTRA_INF);
    }

    /* Set source distance to 0 and update heap */
    res->distances[src_idx] = 0.0;
    min_heap_decrease_key(heap, src_idx, 0.0);

    if (capture_trace) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Initialized distances: dist[%s] = 0.0, all other nodes = INF",
                 g->locations[src_idx].name);
        add_trace_entry(res, &trace_cap, src_idx, src_id, -1, -1, 0.0, 0.0, 0.0, 1, msg);
    }

    /* Dijkstra main loop */
    while (!min_heap_is_empty(heap)) {
        MinHeapNode* min_node = min_heap_extract_min(heap);
        if (!min_node) break;

        int u = min_node->vertex_index;
        double d_u = min_node->dist;
        free(min_node);

        /* If extracted distance is infinity, remaining nodes are disconnected */
        if (d_u >= DIJKSTRA_INF / 2.0) {
            break;
        }

        settled[u] = 1;
        res->visited_order[res->visited_count++] = u;

        if (capture_trace) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Settled node '%s' (ID %d) with optimal distance %.2f",
                     g->locations[u].name, g->locations[u].id, d_u);
            add_trace_entry(res, &trace_cap, u, g->locations[u].id, -1, -1, 0.0, d_u, d_u, 0, msg);
        }

        /* If we reached the destination, we can finish */
        if (u == dest_idx) {
            break;
        }

        /* Examine all outgoing edges from vertex u */
        AdjListNode* edge = g->adj[u];
        while (edge) {
            int v = edge->target_index;
            double w = edge->weight;

            /* Only relax edges to unsettled vertices */
            if (!settled[v]) {
                double alt = d_u + w;
                double old_d = res->distances[v];

                if (alt < old_d) {
                    res->distances[v] = alt;
                    res->predecessors[v] = u;
                    min_heap_decrease_key(heap, v, alt);

                    if (capture_trace) {
                        char msg[256];
                        snprintf(msg, sizeof(msg),
                                 "Relaxed edge (%s -> %s): updated distance from %s to %.2f via %s",
                                 g->locations[u].name, g->locations[v].name,
                                 (old_d >= DIJKSTRA_INF / 2.0) ? "INF" : "",
                                 alt, g->locations[u].name);
                        add_trace_entry(res, &trace_cap, u, g->locations[u].id,
                                        v, g->locations[v].id, w, old_d, alt, 1, msg);
                    }
                } else if (capture_trace) {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Checked edge (%s -> %s): candidate %.2f is not shorter than current %.2f (no relaxation)",
                             g->locations[u].name, g->locations[v].name, alt, old_d);
                    add_trace_entry(res, &trace_cap, u, g->locations[u].id,
                                    v, g->locations[v].id, w, old_d, old_d, 0, msg);
                }
            }
            edge = edge->next;
        }
    }

    min_heap_free(heap);
    free(settled);

    /* Check if target is reachable */
    if (res->distances[dest_idx] >= DIJKSTRA_INF / 2.0) {
        res->success = 0;
        res->error_code = -3;
        res->total_distance = -1.0;
        res->path_length = 0;
        snprintf(res->message, sizeof(res->message),
                 "No reachable path found between '%s' (ID %d) and '%s' (ID %d).",
                 g->locations[src_idx].name, src_id,
                 g->locations[dest_idx].name, dest_id);
        return res;
    }

    /* Reconstruct shortest path by following predecessor pointers */
    int temp_path[1024];
    int count = 0;
    int curr = dest_idx;
    while (curr != -1 && count < 1024) {
        temp_path[count++] = curr;
        if (curr == src_idx) break;
        curr = res->predecessors[curr];
    }

    if (count == 0 || temp_path[count - 1] != src_idx) {
        res->success = 0;
        res->error_code = -3;
        snprintf(res->message, sizeof(res->message), "Failed to reconstruct path backwards to source.");
        return res;
    }

    res->path_length = count;
    res->path_indices = (int*)malloc(sizeof(int) * count);
    res->path_ids = (int*)malloc(sizeof(int) * count);

    /* Reverse path so it reads source -> destination */
    for (int i = 0; i < count; i++) {
        int v_idx = temp_path[count - 1 - i];
        res->path_indices[i] = v_idx;
        res->path_ids[i] = g->locations[v_idx].id;
    }

    res->total_distance = res->distances[dest_idx];
    res->success = 1;
    res->error_code = 0;
    snprintf(res->message, sizeof(res->message),
             "Shortest path found successfully: %d segments, total distance: %.2f km",
             count - 1, res->total_distance);

    return res;
}

void dijkstra_free_result(DijkstraResult* res) {
    if (!res) return;
    if (res->path_indices) free(res->path_indices);
    if (res->path_ids) free(res->path_ids);
    if (res->distances) free(res->distances);
    if (res->predecessors) free(res->predecessors);
    if (res->visited_order) free(res->visited_order);
    if (res->traces) free(res->traces);
    free(res);
}

void dijkstra_print_result(const DijkstraResult* res, const Graph* g, FILE* stream) {
    if (!res || !stream) return;
    fprintf(stream, "===============================================\n");
    fprintf(stream, "DIJKSTRA'S SHORTEST PATH CALCULATION RESULT\n");
    fprintf(stream, "===============================================\n");
    fprintf(stream, "Status: %s\n", res->success ? "SUCCESS" : "FAILED");
    fprintf(stream, "Message: %s\n", res->message);

    if (res->success && g) {
        fprintf(stream, "Total Distance / Cost: %.2f\n", res->total_distance);
        fprintf(stream, "Number of Vertices in Path: %d\n", res->path_length);
        fprintf(stream, "Route Sequence: ");
        for (int i = 0; i < res->path_length; i++) {
            int v_idx = res->path_indices[i];
            fprintf(stream, "%s (ID: %d)", g->locations[v_idx].name, res->path_ids[i]);
            if (i < res->path_length - 1) {
                fprintf(stream, " -> ");
            }
        }
        fprintf(stream, "\n");
    }
    fprintf(stream, "===============================================\n");
}
