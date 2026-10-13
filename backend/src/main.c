/**
 * @file main.c
 * @brief Main Entry Point for Shortest Path Finder
 *
 * Provides:
 * 1. CLI Shortest Path queries with JSON output
 * 2. Graph inspection & manipulation (locations, roads)
 * 3. Sample data persistence
 * 4. Embedded HTTP REST Server mode
 */

#include "../include/graph.h"
#include "../include/minheap.h"
#include "../include/dijkstra.h"
#include "../include/storage.h"
#include "../include/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOC_FILE "data/locations.csv"
#define ROAD_FILE "data/roads.csv"
#define FRONTEND_DIR "frontend"

static void print_usage(const char* prog) {
    printf("=================================================================\n");
    printf("   SHORTEST PATH FINDER - C BACKEND ENGINE\n");
    printf("   Data Structures Project: Dijkstra's Algorithm + Binary Min-Heap\n");
    printf("=================================================================\n");
    printf("Usage:\n");
    printf("  %s --serve [port]                     Run local embedded HTTP REST server (default: 8080)\n", prog);
    printf("  %s --query <src_id> <dest_id> [--trace] Calculate shortest path and output JSON\n", prog);
    printf("  %s --list                             List all locations and roads as JSON\n", prog);
    printf("  %s --add-loc <id> <code> <name> [x] [y] Add new location\n", prog);
    printf("  %s --del-loc <id>                     Delete a location\n", prog);
    printf("  %s --add-road <u> <v> <weight> [bidir] Add new road\n", prog);
    printf("  %s --del-road <u> <v>                 Delete a road\n", prog);
    printf("  %s --reset                            Reset data to default 10 locations & 17 roads\n", prog);
    printf("  %s --help                             Show this help message\n", prog);
    printf("=================================================================\n");
}

int main(int argc, char* argv[]) {
    Graph* g = graph_create(64);
    if (!g) {
        fprintf(stderr, "Error: Failed to allocate graph memory.\n");
        return 1;
    }

    /* Load persistent data from CSV; if missing, initialize defaults */
    if (storage_load_all(g, LOC_FILE, ROAD_FILE) != 0 || g->num_vertices == 0) {
        storage_reset_defaults(g, LOC_FILE, ROAD_FILE);
    }

    if (argc < 2) {
        print_usage(argv[0]);
        graph_free(g);
        return 0;
    }

    if (strcmp(argv[1], "--serve") == 0) {
        int port = DEFAULT_PORT;
        if (argc >= 3) {
            port = atoi(argv[2]);
            if (port <= 0 || port > 65535) port = DEFAULT_PORT;
        }
        int rc = server_start(g, port, LOC_FILE, ROAD_FILE, FRONTEND_DIR);
        graph_free(g);
        return rc;
    }

    if (strcmp(argv[1], "--query") == 0) {
        if (argc < 4) {
            fprintf(stderr, "{\"status\":\"error\",\"message\":\"Missing source or destination ID.\"}\n");
            graph_free(g);
            return 1;
        }

        int src_id = atoi(argv[2]);
        int dest_id = atoi(argv[3]);
        int trace = (argc >= 5 && strcmp(argv[4], "--trace") == 0) ? 1 : 0;

        DijkstraResult* res = dijkstra_find_shortest_path(g, src_id, dest_id, trace);
        if (!res) {
            fprintf(stderr, "{\"status\":\"error\",\"message\":\"Failed to compute path.\"}\n");
            graph_free(g);
            return 1;
        }

        if (!res->success) {
            printf("{\"status\":\"error\",\"error_code\":%d,\"message\":\"%s\",\"source_id\":%d,\"destination_id\":%d}\n",
                   res->error_code, res->message, src_id, dest_id);
        } else {
            printf("{\"status\":\"success\",\"source_id\":%d,\"destination_id\":%d,\"total_distance\":%.2f,\"num_segments\":%d,\"path_length\":%d,\"path\":[",
                   src_id, dest_id, res->total_distance, (res->path_length > 0) ? res->path_length - 1 : 0, res->path_length);
            for (int i = 0; i < res->path_length; i++) {
                int v_idx = res->path_indices[i];
                printf("{\"id\":%d,\"name\":\"%s\",\"code\":\"%s\",\"x\":%.1f,\"y\":%.1f}%s",
                       g->locations[v_idx].id,
                       g->locations[v_idx].name,
                       g->locations[v_idx].code,
                       g->locations[v_idx].x,
                       g->locations[v_idx].y,
                       (i < res->path_length - 1) ? "," : "");
            }
            printf("],\"roads_used\":[");
            for (int i = 0; i < res->path_length - 1; i++) {
                int u_idx = res->path_indices[i];
                int v_idx = res->path_indices[i + 1];
                double w = 0.0;
                int b = 1;
                graph_road_exists(g, g->locations[u_idx].id, g->locations[v_idx].id, &w, &b);
                printf("{\"from_id\":%d,\"to_id\":%d,\"weight\":%.2f}%s",
                       g->locations[u_idx].id, g->locations[v_idx].id, w,
                       (i < res->path_length - 2) ? "," : "");
            }
            printf("]}\n");
        }

        dijkstra_free_result(res);
        graph_free(g);
        return 0;
    }

    if (strcmp(argv[1], "--list") == 0) {
        printf("{\"status\":\"success\",\"num_locations\":%d,\"locations\":[", g->num_vertices);
        for (int i = 0; i < g->num_vertices; i++) {
            printf("{\"id\":%d,\"code\":\"%s\",\"name\":\"%s\",\"x\":%.1f,\"y\":%.1f}%s",
                   g->locations[i].id,
                   g->locations[i].code,
                   g->locations[i].name,
                   g->locations[i].x,
                   g->locations[i].y,
                   (i < g->num_vertices - 1) ? "," : "");
        }
        printf("],\"roads\":[");
        int first_road = 1;
        for (int i = 0; i < g->num_vertices; i++) {
            int u_id = g->locations[i].id;
            AdjListNode* edge = g->adj[i];
            while (edge) {
                int v_idx = edge->target_index;
                int v_id = g->locations[v_idx].id;
                if (!edge->is_bidirectional || i < v_idx) {
                    if (!first_road) printf(",");
                    first_road = 0;
                    printf("{\"u_id\":%d,\"v_id\":%d,\"u_name\":\"%s\",\"v_name\":\"%s\",\"weight\":%.2f,\"bidirectional\":%d}",
                           u_id, v_id, g->locations[i].name, g->locations[v_idx].name, edge->weight, edge->is_bidirectional);
                }
                edge = edge->next;
            }
        }
        int b = 0, o = 0;
        int t = graph_count_roads(g, &b, &o);
        printf("],\"stats\":{\"total_locations\":%d,\"total_roads\":%d,\"bidirectional\":%d,\"oneway\":%d}}\n",
               g->num_vertices, t, b, o);
        graph_free(g);
        return 0;
    }

    if (strcmp(argv[1], "--add-loc") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: %s --add-loc <id> <code> <name> [x] [y]\n", argv[0]);
            graph_free(g);
            return 1;
        }
        int id = atoi(argv[2]);
        const char* code = argv[3];
        const char* name = argv[4];
        double x = (argc >= 6) ? atof(argv[5]) : 400.0;
        double y = (argc >= 7) ? atof(argv[6]) : 300.0;

        int rc = graph_add_location(g, id, code, name, x, y);
        if (rc == 0) {
            storage_save_all(g, LOC_FILE, ROAD_FILE);
            printf("{\"status\":\"success\",\"message\":\"Location added.\",\"id\":%d}\n", id);
        } else {
            printf("{\"status\":\"error\",\"message\":\"Failed to add location (code %d).\"}\n", rc);
        }
        graph_free(g);
        return rc == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "--del-loc") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s --del-loc <id>\n", argv[0]);
            graph_free(g);
            return 1;
        }
        int id = atoi(argv[2]);
        int rc = graph_delete_location(g, id);
        if (rc == 0) {
            storage_save_all(g, LOC_FILE, ROAD_FILE);
            printf("{\"status\":\"success\",\"message\":\"Location deleted.\",\"id\":%d}\n", id);
        } else {
            printf("{\"status\":\"error\",\"message\":\"Location not found.\",\"id\":%d}\n", id);
        }
        graph_free(g);
        return rc == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "--add-road") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: %s --add-road <u> <v> <weight> [bidirectional]\n", argv[0]);
            graph_free(g);
            return 1;
        }
        int u = atoi(argv[2]);
        int v = atoi(argv[3]);
        double w = atof(argv[4]);
        int bidir = (argc >= 6) ? atoi(argv[5]) : 1;

        int rc = graph_add_road(g, u, v, w, bidir);
        if (rc == 0) {
            storage_save_all(g, LOC_FILE, ROAD_FILE);
            printf("{\"status\":\"success\",\"message\":\"Road added.\",\"u\":%d,\"v\":%d,\"weight\":%.2f}\n", u, v, w);
        } else {
            printf("{\"status\":\"error\",\"message\":\"Failed to add road (code %d).\"}\n", rc);
        }
        graph_free(g);
        return rc == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "--del-road") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s --del-road <u> <v>\n", argv[0]);
            graph_free(g);
            return 1;
        }
        int u = atoi(argv[2]);
        int v = atoi(argv[3]);
        int rc = graph_delete_road(g, u, v);
        if (rc == 0) {
            storage_save_all(g, LOC_FILE, ROAD_FILE);
            printf("{\"status\":\"success\",\"message\":\"Road deleted.\",\"u\":%d,\"v\":%d}\n", u, v);
        } else {
            printf("{\"status\":\"error\",\"message\":\"Road not found.\",\"u\":%d,\"v\":%d}\n", u, v);
        }
        graph_free(g);
        return rc == 0 ? 0 : 1;
    }

    if (strcmp(argv[1], "--reset") == 0) {
        storage_reset_defaults(g, LOC_FILE, ROAD_FILE);
        printf("{\"status\":\"success\",\"message\":\"Reset to default network complete.\"}\n");
        graph_free(g);
        return 0;
    }

    print_usage(argv[0]);
    graph_free(g);
    return 0;
}
