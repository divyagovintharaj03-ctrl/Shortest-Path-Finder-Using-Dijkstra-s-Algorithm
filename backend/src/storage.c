/**
 * @file storage.c
 * @brief Persistent CSV Storage Implementation for Graph Data
 */

#include "../include/storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int storage_load_locations(Graph* g, const char* filepath) {
    if (!g || !filepath) return -1;
    FILE* fp = fopen(filepath, "r");
    if (!fp) return -2;

    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line_num == 1 && (strstr(line, "id") || strstr(line, "code"))) {
            /* Skip header */
            continue;
        }

        /* Trim newline */
        char* nl = strchr(line, '\r');
        if (nl) *nl = '\0';
        nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        if (strlen(line) == 0) continue;

        int id;
        char code[MAX_CODE_LEN] = {0};
        char name[MAX_NAME_LEN] = {0};
        double x = 0.0, y = 0.0;

        /* Parse comma-separated fields: id,code,name,x,y */
        char* token = strtok(line, ",");
        if (!token) continue;
        id = atoi(token);

        token = strtok(NULL, ",");
        if (token) strncpy(code, token, MAX_CODE_LEN - 1);

        token = strtok(NULL, ",");
        if (token) strncpy(name, token, MAX_NAME_LEN - 1);

        token = strtok(NULL, ",");
        if (token) x = atof(token);

        token = strtok(NULL, ",");
        if (token) y = atof(token);

        if (id > 0 && strlen(code) > 0 && strlen(name) > 0) {
            graph_add_location(g, id, code, name, x, y);
        }
    }

    fclose(fp);
    return 0;
}

int storage_save_locations(const Graph* g, const char* filepath) {
    if (!g || !filepath) return -1;
    FILE* fp = fopen(filepath, "w");
    if (!fp) return -2;

    fprintf(fp, "id,code,name,x,y\n");
    for (int i = 0; i < g->num_vertices; i++) {
        fprintf(fp, "%d,%s,%s,%.1f,%.1f\n",
                g->locations[i].id,
                g->locations[i].code,
                g->locations[i].name,
                g->locations[i].x,
                g->locations[i].y);
    }

    fclose(fp);
    return 0;
}

int storage_load_roads(Graph* g, const char* filepath) {
    if (!g || !filepath) return -1;
    FILE* fp = fopen(filepath, "r");
    if (!fp) return -2;

    char line[256];
    int line_num = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (line_num == 1 && (strstr(line, "u_id") || strstr(line, "weight"))) {
            continue;
        }

        /* Trim newline */
        char* nl = strchr(line, '\r');
        if (nl) *nl = '\0';
        nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        if (strlen(line) == 0) continue;

        int u_id = 0, v_id = 0, bidir = 1;
        double weight = 0.0;

        char* token = strtok(line, ",");
        if (!token) continue;
        u_id = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        v_id = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        weight = atof(token);

        token = strtok(NULL, ",");
        if (token) bidir = atoi(token);

        if (u_id > 0 && v_id > 0 && weight > 0.0) {
            graph_add_road(g, u_id, v_id, weight, bidir);
        }
    }

    fclose(fp);
    return 0;
}

int storage_save_roads(const Graph* g, const char* filepath) {
    if (!g || !filepath) return -1;
    FILE* fp = fopen(filepath, "w");
    if (!fp) return -2;

    fprintf(fp, "u_id,v_id,weight,bidirectional\n");

    /* To avoid duplicating bidirectional roads in the CSV file,
       we only write (u < v) for bidirectional edges, and all one-way edges */
    for (int i = 0; i < g->num_vertices; i++) {
        int u_id = g->locations[i].id;
        AdjListNode* edge = g->adj[i];
        while (edge) {
            int v_idx = edge->target_index;
            int v_id = g->locations[v_idx].id;

            if (edge->is_bidirectional) {
                if (i < v_idx) {
                    fprintf(fp, "%d,%d,%.2f,1\n", u_id, v_id, edge->weight);
                }
            } else {
                fprintf(fp, "%d,%d,%.2f,0\n", u_id, v_id, edge->weight);
            }
            edge = edge->next;
        }
    }

    fclose(fp);
    return 0;
}

int storage_load_all(Graph* g, const char* loc_file, const char* road_file) {
    graph_clear(g);
    int r1 = storage_load_locations(g, loc_file);
    if (r1 != 0) return r1;
    int r2 = storage_load_roads(g, road_file);
    return r2;
}

int storage_save_all(const Graph* g, const char* loc_file, const char* road_file) {
    int r1 = storage_save_locations(g, loc_file);
    if (r1 != 0) return r1;
    int r2 = storage_save_roads(g, road_file);
    return r2;
}

int storage_reset_defaults(Graph* g, const char* loc_file, const char* road_file) {
    graph_clear(g);

    /* 10 Default Locations */
    graph_add_location(g, 1, "COLLEGE", "College", 130, 160);
    graph_add_location(g, 2, "BUS_STAND", "Bus Stand", 290, 250);
    graph_add_location(g, 3, "RAILWAY_STN", "Railway Station", 460, 130);
    graph_add_location(g, 4, "HOSPITAL", "Hospital", 220, 430);
    graph_add_location(g, 5, "LIBRARY", "Library", 460, 310);
    graph_add_location(g, 6, "MARKET", "Market", 640, 210);
    graph_add_location(g, 7, "PARK", "Park", 410, 500);
    graph_add_location(g, 8, "CITY_CENTRE", "City Centre", 670, 400);
    graph_add_location(g, 9, "SHOPPING_MALL", "Shopping Mall", 830, 260);
    graph_add_location(g, 10, "AIRPORT", "Airport", 870, 490);

    /* 17 Default Roads */
    graph_add_road(g, 1, 2, 4.0, 1);
    graph_add_road(g, 1, 4, 6.5, 1);
    graph_add_road(g, 2, 3, 5.0, 1);
    graph_add_road(g, 2, 5, 3.5, 1);
    graph_add_road(g, 3, 6, 4.2, 1);
    graph_add_road(g, 4, 5, 4.0, 1);
    graph_add_road(g, 4, 7, 3.0, 1);
    graph_add_road(g, 5, 6, 3.8, 1);
    graph_add_road(g, 5, 7, 4.5, 1);
    graph_add_road(g, 5, 8, 5.2, 1);
    graph_add_road(g, 6, 9, 4.0, 1);
    graph_add_road(g, 6, 8, 3.5, 1);
    graph_add_road(g, 7, 8, 5.0, 1);
    graph_add_road(g, 7, 10, 8.5, 1);
    graph_add_road(g, 8, 9, 3.0, 1);
    graph_add_road(g, 8, 10, 6.0, 1);
    graph_add_road(g, 9, 10, 5.5, 1);

    return storage_save_all(g, loc_file, road_file);
}
