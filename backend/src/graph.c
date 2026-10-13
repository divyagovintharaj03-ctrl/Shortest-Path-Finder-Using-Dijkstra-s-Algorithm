/**
 * @file graph.c
 * @brief Implementation of Adjacency List Graph for Shortest Path Finder
 */

#include "../include/graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static AdjListNode* create_adj_node(int target_index, double weight, int is_bidirectional) {
    AdjListNode* node = (AdjListNode*)malloc(sizeof(AdjListNode));
    if (!node) return NULL;
    node->target_index = target_index;
    node->weight = weight;
    node->is_bidirectional = is_bidirectional;
    node->next = NULL;
    return node;
}

static void free_adj_list(AdjListNode* head) {
    AdjListNode* curr = head;
    while (curr) {
        AdjListNode* temp = curr;
        curr = curr->next;
        free(temp);
    }
}

Graph* graph_create(int initial_capacity) {
    if (initial_capacity <= 0) initial_capacity = DEFAULT_GRAPH_CAPACITY;
    Graph* g = (Graph*)malloc(sizeof(Graph));
    if (!g) return NULL;

    g->capacity = initial_capacity;
    g->num_vertices = 0;
    g->locations = (Location*)malloc(sizeof(Location) * g->capacity);
    g->adj = (AdjListNode**)calloc(g->capacity, sizeof(AdjListNode*));

    if (!g->locations || !g->adj) {
        if (g->locations) free(g->locations);
        if (g->adj) free(g->adj);
        free(g);
        return NULL;
    }
    return g;
}

void graph_clear(Graph* g) {
    if (!g) return;
    for (int i = 0; i < g->num_vertices; i++) {
        free_adj_list(g->adj[i]);
        g->adj[i] = NULL;
    }
    g->num_vertices = 0;
}

void graph_free(Graph* g) {
    if (!g) return;
    graph_clear(g);
    if (g->locations) free(g->locations);
    if (g->adj) free(g->adj);
    free(g);
}

static int graph_ensure_capacity(Graph* g) {
    if (g->num_vertices < g->capacity) return 1;

    int new_cap = g->capacity * 2;
    Location* new_locs = (Location*)realloc(g->locations, sizeof(Location) * new_cap);
    if (!new_locs) return 0;
    g->locations = new_locs;

    AdjListNode** new_adj = (AdjListNode**)realloc(g->adj, sizeof(AdjListNode*) * new_cap);
    if (!new_adj) return 0;
    g->adj = new_adj;

    for (int i = g->capacity; i < new_cap; i++) {
        g->adj[i] = NULL;
    }
    g->capacity = new_cap;
    return 1;
}

int graph_find_index_by_id(const Graph* g, int id) {
    if (!g) return -1;
    for (int i = 0; i < g->num_vertices; i++) {
        if (g->locations[i].id == id) {
            return i;
        }
    }
    return -1;
}

int graph_find_index_by_code(const Graph* g, const char* code) {
    if (!g || !code) return -1;
    for (int i = 0; i < g->num_vertices; i++) {
        if (strcmp(g->locations[i].code, code) == 0) {
            return i;
        }
    }
    return -1;
}

const Location* graph_get_location_by_id(const Graph* g, int id) {
    int idx = graph_find_index_by_id(g, id);
    if (idx < 0) return NULL;
    return &g->locations[idx];
}

int graph_add_location(Graph* g, int id, const char* code, const char* name, double x, double y) {
    if (!g || id <= 0 || !code || !name) return -1; /* invalid arguments */
    if (strlen(code) == 0 || strlen(name) == 0) return -1;

    /* Check duplicates */
    if (graph_find_index_by_id(g, id) != -1) return -2; /* Duplicate ID */
    if (graph_find_index_by_code(g, code) != -1) return -3; /* Duplicate Code */

    if (!graph_ensure_capacity(g)) return -4; /* Memory allocation failure */

    int idx = g->num_vertices;
    g->locations[idx].id = id;
    strncpy(g->locations[idx].code, code, MAX_CODE_LEN - 1);
    g->locations[idx].code[MAX_CODE_LEN - 1] = '\0';
    strncpy(g->locations[idx].name, name, MAX_NAME_LEN - 1);
    g->locations[idx].name[MAX_NAME_LEN - 1] = '\0';
    g->locations[idx].x = x;
    g->locations[idx].y = y;

    g->adj[idx] = NULL;
    g->num_vertices++;
    return 0; /* Success */
}

int graph_update_location(Graph* g, int id, const char* name, double x, double y) {
    if (!g || !name || strlen(name) == 0) return -1;
    int idx = graph_find_index_by_id(g, id);
    if (idx < 0) return -2; /* Not found */

    strncpy(g->locations[idx].name, name, MAX_NAME_LEN - 1);
    g->locations[idx].name[MAX_NAME_LEN - 1] = '\0';
    g->locations[idx].x = x;
    g->locations[idx].y = y;
    return 0;
}

static void remove_edges_to_vertex(AdjListNode** head_ref, int target_idx) {
    AdjListNode* curr = *head_ref;
    AdjListNode* prev = NULL;

    while (curr) {
        if (curr->target_index == target_idx) {
            AdjListNode* to_free = curr;
            if (prev) {
                prev->next = curr->next;
            } else {
                *head_ref = curr->next;
            }
            curr = curr->next;
            free(to_free);
        } else {
            /* If this edge pointed to an index greater than deleted index, decrement it */
            if (curr->target_index > target_idx) {
                curr->target_index--;
            }
            prev = curr;
            curr = curr->next;
        }
    }
}

int graph_delete_location(Graph* g, int id) {
    if (!g) return -1;
    int idx = graph_find_index_by_id(g, id);
    if (idx < 0) return -2; /* Not found */

    /* 1. Free outgoing edges from vertex `idx` */
    free_adj_list(g->adj[idx]);

    /* 2. For all other vertices, remove edges pointing to `idx` and shift indices > idx */
    for (int i = 0; i < g->num_vertices; i++) {
        if (i == idx) continue;
        remove_edges_to_vertex(&g->adj[i], idx);
    }

    /* 3. Shift locations and adj list pointers */
    for (int i = idx; i < g->num_vertices - 1; i++) {
        g->locations[i] = g->locations[i + 1];
        g->adj[i] = g->adj[i + 1];
    }
    g->adj[g->num_vertices - 1] = NULL;
    g->num_vertices--;

    return 0; /* Success */
}

static int add_directed_edge(Graph* g, int u_idx, int v_idx, double weight, int is_bidir) {
    /* Check if edge already exists, if so update it */
    AdjListNode* curr = g->adj[u_idx];
    while (curr) {
        if (curr->target_index == v_idx) {
            curr->weight = weight;
            curr->is_bidirectional = is_bidir;
            return 0;
        }
        curr = curr->next;
    }

    AdjListNode* node = create_adj_node(v_idx, weight, is_bidir);
    if (!node) return -1;
    node->next = g->adj[u_idx];
    g->adj[u_idx] = node;
    return 0;
}

int graph_add_road(Graph* g, int u_id, int v_id, double weight, int is_bidirectional) {
    if (!g) return -1;
    if (u_id == v_id) return -2; /* Self loops not allowed */
    if (weight <= 0.0) return -3; /* Negative/zero weight rejected */

    int u_idx = graph_find_index_by_id(g, u_id);
    int v_idx = graph_find_index_by_id(g, v_id);
    if (u_idx < 0 || v_idx < 0) return -4; /* Location does not exist */

    if (add_directed_edge(g, u_idx, v_idx, weight, is_bidirectional) != 0) return -5;

    if (is_bidirectional) {
        if (add_directed_edge(g, v_idx, u_idx, weight, is_bidirectional) != 0) return -5;
    }

    return 0; /* Success */
}

static int remove_directed_edge(Graph* g, int u_idx, int v_idx) {
    AdjListNode* curr = g->adj[u_idx];
    AdjListNode* prev = NULL;

    while (curr) {
        if (curr->target_index == v_idx) {
            if (prev) {
                prev->next = curr->next;
            } else {
                g->adj[u_idx] = curr->next;
            }
            free(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1; /* Not found */
}

int graph_delete_road(Graph* g, int u_id, int v_id) {
    if (!g) return -1;
    int u_idx = graph_find_index_by_id(g, u_id);
    int v_idx = graph_find_index_by_id(g, v_id);
    if (u_idx < 0 || v_idx < 0) return -2;

    int res1 = remove_directed_edge(g, u_idx, v_idx);
    int res2 = remove_directed_edge(g, v_idx, u_idx);

    if (res1 == 0 || res2 == 0) return 0;
    return -3; /* Road was not found */
}

int graph_update_road(Graph* g, int u_id, int v_id, double new_weight, int is_bidirectional) {
    if (!g) return -1;
    if (new_weight <= 0.0) return -2;

    int u_idx = graph_find_index_by_id(g, u_id);
    int v_idx = graph_find_index_by_id(g, v_id);
    if (u_idx < 0 || v_idx < 0) return -3;

    /* Delete old edges first to cleanly support bidirectional <-> one-way conversions */
    remove_directed_edge(g, u_idx, v_idx);
    remove_directed_edge(g, v_idx, u_idx);

    return graph_add_road(g, u_id, v_id, new_weight, is_bidirectional);
}

int graph_road_exists(const Graph* g, int u_id, int v_id, double* out_weight, int* out_bidir) {
    if (!g) return 0;
    int u_idx = graph_find_index_by_id(g, u_id);
    int v_idx = graph_find_index_by_id(g, v_id);
    if (u_idx < 0 || v_idx < 0) return 0;

    AdjListNode* curr = g->adj[u_idx];
    while (curr) {
        if (curr->target_index == v_idx) {
            if (out_weight) *out_weight = curr->weight;
            if (out_bidir) *out_bidir = curr->is_bidirectional;
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

int graph_count_roads(const Graph* g, int* out_bidirectional, int* out_oneway) {
    if (!g) return 0;
    int bidir_count = 0;
    int oneway_count = 0;

    for (int i = 0; i < g->num_vertices; i++) {
        AdjListNode* curr = g->adj[i];
        while (curr) {
            if (curr->is_bidirectional) {
                if (i < curr->target_index) {
                    bidir_count++;
                }
            } else {
                oneway_count++;
            }
            curr = curr->next;
        }
    }

    if (out_bidirectional) *out_bidirectional = bidir_count;
    if (out_oneway) *out_oneway = oneway_count;
    return bidir_count + oneway_count;
}

void graph_print_summary(const Graph* g, FILE* stream) {
    if (!g || !stream) return;
    int bidir = 0, oneway = 0;
    int total_roads = graph_count_roads(g, &bidir, &oneway);
    fprintf(stream, "Graph Summary:\n");
    fprintf(stream, "  Total Locations: %d\n", g->num_vertices);
    fprintf(stream, "  Total Distinct Roads: %d (Bidirectional: %d, One-way: %d)\n",
            total_roads, bidir, oneway);
}
