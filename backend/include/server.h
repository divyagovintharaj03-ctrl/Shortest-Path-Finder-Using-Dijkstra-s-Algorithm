/**
 * @file server.h
 * @brief Lightweight Embedded HTTP Server in C for Shortest Path Finder
 *
 * Supports both Windows (Winsock2) and Linux/POSIX socket APIs.
 * Serves frontend static files and RESTful API endpoints.
 */

#ifndef SERVER_H
#define SERVER_H

#include "graph.h"

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 65536

/**
 * @brief Starts the HTTP server loop
 * @param g Pointer to Graph instance
 * @param port Port number to listen on (e.g., 8080)
 * @param loc_file Path to locations.csv
 * @param road_file Path to roads.csv
 * @param frontend_dir Path to frontend directory
 * @return 0 on exit, non-zero on error
 */
int server_start(Graph* g, int port, const char* loc_file, const char* road_file, const char* frontend_dir);

#endif /* SERVER_H */
