/**
 * @file storage.h
 * @brief File Persistence for Graph Locations and Roads
 *
 * Implements persistent loading and saving using CSV format.
 * Includes fallback logic to initialize default sample networks if files are missing.
 */

#ifndef STORAGE_H
#define STORAGE_H

#include "graph.h"

int storage_load_locations(Graph* g, const char* filepath);
int storage_save_locations(const Graph* g, const char* filepath);

int storage_load_roads(Graph* g, const char* filepath);
int storage_save_roads(const Graph* g, const char* filepath);

int storage_load_all(Graph* g, const char* loc_file, const char* road_file);
int storage_save_all(const Graph* g, const char* loc_file, const char* road_file);

int storage_reset_defaults(Graph* g, const char* loc_file, const char* road_file);

#endif /* STORAGE_H */
