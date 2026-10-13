/**
 * @file server.c
 * @brief Cross-platform Embedded HTTP REST Server in C
 *
 * Implements an HTTP server supporting Winsock2 (Windows) and POSIX Sockets (Linux/macOS).
 * Endpoints:
 *   GET  /                       -> frontend/index.html
 *   GET  /css/..., /js/...       -> Static frontend assets
 *   GET  /api/health             -> Health check and backend engine info
 *   GET  /api/data               -> Graph topology, locations, and roads
 *   GET  /api/shortest-path      -> Execute Dijkstra's algorithm and return path
 *   POST /api/locations          -> Add new vertex/location
 *   POST /api/roads              -> Add new edge/road
 *   POST /api/roads/delete       -> Remove road
 *   POST /api/locations/delete   -> Remove location
 *   POST /api/reset              -> Restore default sample network
 */

#include "../include/server.h"
#include "../include/dijkstra.h"
#include "../include/storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

static void send_response(SOCKET client, int status_code, const char* status_text,
                          const char* content_type, const char* body, int body_len) {
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Server: Pathfinder-C-Core/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS, DELETE, PUT\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n\r\n",
        status_code, status_text, content_type, body_len);

    send(client, header, header_len, 0);
    if (body && body_len > 0) {
        send(client, body, body_len, 0);
    }
}

static void send_json_error(SOCKET client, int status_code, const char* msg) {
    char body[512];
    int len = snprintf(body, sizeof(body),
        "{\"status\":\"error\",\"message\":\"%s\"}\n", msg);
    send_response(client, status_code, "Error", "application/json; charset=utf-8", body, len);
}

static const char* get_mime_type(const char* path) {
    const char* dot = strrchr(path, '.');
    if (!dot) return "text/plain";
    if (strcmp(dot, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(dot, ".css") == 0) return "text/css; charset=utf-8";
    if (strcmp(dot, ".js") == 0) return "application/javascript; charset=utf-8";
    if (strcmp(dot, ".json") == 0) return "application/json; charset=utf-8";
    if (strcmp(dot, ".png") == 0) return "image/png";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
    return "text/plain";
}

static int serve_file(SOCKET client, const char* filepath) {
    FILE* fp = fopen(filepath, "rb");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)malloc(size);
    if (!buffer) {
        fclose(fp);
        return 0;
    }

    size_t read_bytes = fread(buffer, 1, size, fp);
    fclose(fp);

    const char* mime = get_mime_type(filepath);
    send_response(client, 200, "OK", mime, buffer, (int)read_bytes);
    free(buffer);
    return 1;
}

static char* url_decode(const char* src) {
    size_t len = strlen(src);
    char* dst = (char*)malloc(len + 1);
    if (!dst) return NULL;
    char* d = dst;

    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)*(src + 1)) && isxdigit((unsigned char)*(src + 2))) {
            char hex[3] = { src[1], src[2], 0 };
            *d++ = (char)strtol(hex, NULL, 16);
            src += 3;
        } else if (*src == '+') {
            *d++ = ' ';
            src++;
        } else {
            *d++ = *src++;
        }
    }
    *d = '\0';
    return dst;
}

static int get_query_param(const char* url, const char* param, char* out_val, size_t out_max) {
    const char* q = strchr(url, '?');
    if (!q) return 0;
    q++;

    size_t plen = strlen(param);
    const char* curr = q;
    while (curr && *curr) {
        if (strncmp(curr, param, plen) == 0 && curr[plen] == '=') {
            const char* val_start = curr + plen + 1;
            const char* val_end = strchr(val_start, '&');
            size_t vlen = val_end ? (size_t)(val_end - val_start) : strlen(val_start);
            if (vlen >= out_max) vlen = out_max - 1;
            char temp[256];
            strncpy(temp, val_start, vlen);
            temp[vlen] = '\0';
            char* decoded = url_decode(temp);
            if (decoded) {
                strncpy(out_val, decoded, out_max - 1);
                out_val[out_max - 1] = '\0';
                free(decoded);
                return 1;
            }
        }
        curr = strchr(curr, '&');
        if (curr) curr++;
    }
    return 0;
}

static void handle_api_data(SOCKET client, const Graph* g) {
    /* Allocate response buffer */
    size_t cap = 131072;
    char* buf = (char*)malloc(cap);
    if (!buf) {
        send_json_error(client, 500, "Out of memory");
        return;
    }

    int offset = snprintf(buf, cap, "{\"status\":\"success\",\"backend\":\"C Adjacency-List & Min-Heap\",\"num_locations\":%d,\"locations\":[", g->num_vertices);

    for (int i = 0; i < g->num_vertices; i++) {
        offset += snprintf(buf + offset, cap - offset,
            "{\"id\":%d,\"code\":\"%s\",\"name\":\"%s\",\"x\":%.1f,\"y\":%.1f}%s",
            g->locations[i].id,
            g->locations[i].code,
            g->locations[i].name,
            g->locations[i].x,
            g->locations[i].y,
            (i < g->num_vertices - 1) ? "," : "");
    }

    offset += snprintf(buf + offset, cap - offset, "],\"roads\":[");

    int first_road = 1;
    for (int i = 0; i < g->num_vertices; i++) {
        int u_id = g->locations[i].id;
        AdjListNode* edge = g->adj[i];
        while (edge) {
            int v_idx = edge->target_index;
            int v_id = g->locations[v_idx].id;
            /* In JSON output, list each undirected road once (when i < v_idx), and one-way roads always */
            if (!edge->is_bidirectional || i < v_idx) {
                if (!first_road) {
                    offset += snprintf(buf + offset, cap - offset, ",");
                }
                first_road = 0;
                offset += snprintf(buf + offset, cap - offset,
                    "{\"u_id\":%d,\"v_id\":%d,\"u_name\":\"%s\",\"v_name\":\"%s\",\"weight\":%.2f,\"bidirectional\":%d}",
                    u_id, v_id, g->locations[i].name, g->locations[v_idx].name, edge->weight, edge->is_bidirectional);
            }
            edge = edge->next;
        }
    }

    int bidir = 0, oneway = 0;
    int total_roads = graph_count_roads(g, &bidir, &oneway);
    offset += snprintf(buf + offset, cap - offset,
        "],\"stats\":{\"total_locations\":%d,\"total_roads\":%d,\"bidirectional\":%d,\"oneway\":%d}}\n",
        g->num_vertices, total_roads, bidir, oneway);

    send_response(client, 200, "OK", "application/json; charset=utf-8", buf, offset);
    free(buf);
}

static void handle_api_shortest_path(SOCKET client, const Graph* g, const char* url) {
    char s_val[32] = {0}, d_val[32] = {0}, t_val[32] = {0};
    if (!get_query_param(url, "src", s_val, sizeof(s_val)) ||
        !get_query_param(url, "dest", d_val, sizeof(d_val))) {
        send_json_error(client, 400, "Missing required 'src' or 'dest' query parameters.");
        return;
    }

    int src_id = atoi(s_val);
    int dest_id = atoi(d_val);
    int trace = 0;
    if (get_query_param(url, "trace", t_val, sizeof(t_val))) {
        trace = atoi(t_val);
    }

    DijkstraResult* res = dijkstra_find_shortest_path(g, src_id, dest_id, trace);
    if (!res) {
        send_json_error(client, 500, "Failed to run Dijkstra algorithm.");
        return;
    }

    size_t cap = 262144;
    char* buf = (char*)malloc(cap);
    if (!buf) {
        dijkstra_free_result(res);
        send_json_error(client, 500, "Memory allocation error.");
        return;
    }

    int offset = 0;
    if (!res->success) {
        offset = snprintf(buf, cap,
            "{\"status\":\"error\",\"error_code\":%d,\"message\":\"%s\",\"source_id\":%d,\"destination_id\":%d}\n",
            res->error_code, res->message, src_id, dest_id);
        send_response(client, 200, "OK", "application/json; charset=utf-8", buf, offset);
        free(buf);
        dijkstra_free_result(res);
        return;
    }

    offset = snprintf(buf, cap,
        "{\"status\":\"success\",\"message\":\"%s\",\"source_id\":%d,\"destination_id\":%d,\"total_distance\":%.2f,\"num_segments\":%d,\"path\":[",
        res->message, src_id, dest_id, res->total_distance, (res->path_length > 0) ? res->path_length - 1 : 0);

    for (int i = 0; i < res->path_length; i++) {
        int v_idx = res->path_indices[i];
        offset += snprintf(buf + offset, cap - offset,
            "{\"id\":%d,\"name\":\"%s\",\"code\":\"%s\",\"x\":%.1f,\"y\":%.1f}%s",
            g->locations[v_idx].id,
            g->locations[v_idx].name,
            g->locations[v_idx].code,
            g->locations[v_idx].x,
            g->locations[v_idx].y,
            (i < res->path_length - 1) ? "," : "");
    }

    offset += snprintf(buf + offset, cap - offset, "],\"roads_used\":[");
    for (int i = 0; i < res->path_length - 1; i++) {
        int u_idx = res->path_indices[i];
        int v_idx = res->path_indices[i + 1];
        double w = 0.0;
        int b = 1;
        graph_road_exists(g, g->locations[u_idx].id, g->locations[v_idx].id, &w, &b);
        offset += snprintf(buf + offset, cap - offset,
            "{\"from_id\":%d,\"to_id\":%d,\"weight\":%.2f}%s",
            g->locations[u_idx].id, g->locations[v_idx].id, w,
            (i < res->path_length - 2) ? "," : "");
    }

    offset += snprintf(buf + offset, cap - offset, "],\"traces\":[");
    for (int i = 0; i < res->trace_count; i++) {
        DijkstraStepTrace* t = &res->traces[i];
        offset += snprintf(buf + offset, cap - offset,
            "{\"step\":%d,\"cur_id\":%d,\"nbr_id\":%d,\"weight\":%.2f,\"old_dist\":%.2f,\"new_dist\":%.2f,\"relaxed\":%d,\"desc\":\"%s\"}%s",
            t->step_number, t->current_vertex_id, t->neighbor_vertex_id,
            t->edge_weight, t->old_dist, t->new_dist, t->relaxed, t->description,
            (i < res->trace_count - 1) ? "," : "");
    }

    offset += snprintf(buf + offset, cap - offset, "]}\n");

    send_response(client, 200, "OK", "application/json; charset=utf-8", buf, offset);
    free(buf);
    dijkstra_free_result(res);
}

int server_start(Graph* g, int port, const char* loc_file, const char* road_file, const char* frontend_dir) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }
#endif

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed.\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        fprintf(stderr, "Socket bind failed on port %d.\n", port);
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(server_fd, 10) == SOCKET_ERROR) {
        fprintf(stderr, "Socket listen failed.\n");
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("\n======================================================\n");
    printf("  SHORTEST PATH FINDER - C HTTP ENGINE RUNNING\n");
    printf("  Port: %d\n", port);
    printf("  URL:  http://localhost:%d\n", port);
    printf("======================================================\n\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client == INVALID_SOCKET) continue;

        char req_buf[BUFFER_SIZE];
        int received = recv(client, req_buf, sizeof(req_buf) - 1, 0);
        if (received <= 0) {
            closesocket(client);
            continue;
        }
        req_buf[received] = '\0';

        char method[16] = {0};
        char url[1024] = {0};
        sscanf(req_buf, "%15s %1023s", method, url);

        /* Handle OPTIONS pre-flight */
        if (strcmp(method, "OPTIONS") == 0) {
            send_response(client, 204, "No Content", "text/plain", "", 0);
            closesocket(client);
            continue;
        }

        /* Check for API routes */
        if (strncmp(url, "/api/health", 11) == 0) {
            const char* msg = "{\"status\":\"ok\",\"engine\":\"C (LLVM-MinGW/Clang)\",\"algorithm\":\"Dijkstra Min-Heap O((V+E)logV)\"}";
            send_response(client, 200, "OK", "application/json", msg, (int)strlen(msg));
        } else if (strncmp(url, "/api/data", 9) == 0) {
            handle_api_data(client, g);
        } else if (strncmp(url, "/api/shortest-path", 18) == 0) {
            handle_api_shortest_path(client, g, url);
        } else if (strncmp(url, "/api/reset", 10) == 0) {
            storage_reset_defaults(g, loc_file, road_file);
            const char* msg = "{\"status\":\"success\",\"message\":\"Sample network restored successfully.\"}";
            send_response(client, 200, "OK", "application/json", msg, (int)strlen(msg));
        } else if (strncmp(url, "/api/locations/delete", 21) == 0) {
            char id_str[32] = {0};
            if (get_query_param(url, "id", id_str, sizeof(id_str))) {
                int id = atoi(id_str);
                int rc = graph_delete_location(g, id);
                if (rc == 0) {
                    storage_save_all(g, loc_file, road_file);
                    const char* msg = "{\"status\":\"success\",\"message\":\"Location deleted.\"}";
                    send_response(client, 200, "OK", "application/json", msg, (int)strlen(msg));
                } else {
                    send_json_error(client, 400, "Location not found or deletion failed.");
                }
            } else {
                send_json_error(client, 400, "Missing location ID.");
            }
        } else if (strncmp(url, "/api/roads/delete", 17) == 0) {
            char u_str[32] = {0}, v_str[32] = {0};
            if (get_query_param(url, "u_id", u_str, sizeof(u_str)) &&
                get_query_param(url, "v_id", v_str, sizeof(v_str))) {
                int u = atoi(u_str);
                int v = atoi(v_str);
                int rc = graph_delete_road(g, u, v);
                if (rc == 0) {
                    storage_save_all(g, loc_file, road_file);
                    const char* msg = "{\"status\":\"success\",\"message\":\"Road deleted.\"}";
                    send_response(client, 200, "OK", "application/json", msg, (int)strlen(msg));
                } else {
                    send_json_error(client, 400, "Road not found or deletion failed.");
                }
            } else {
                send_json_error(client, 400, "Missing road endpoints.");
            }
        } else if (strncmp(url, "/api/locations", 14) == 0 && strcmp(method, "POST") == 0) {
            /* Support both URL query parameters and form encoded data */
            char id_str[32] = {0}, code[MAX_CODE_LEN] = {0}, name[MAX_NAME_LEN] = {0}, x_str[32] = {0}, y_str[32] = {0};
            const char* body = strstr(req_buf, "\r\n\r\n");
            if (body) body += 4;

            int got_params = get_query_param(url, "id", id_str, sizeof(id_str)) &&
                             get_query_param(url, "code", code, sizeof(code)) &&
                             get_query_param(url, "name", name, sizeof(name));

            if (!got_params && body) {
                char dummy_url[1024];
                snprintf(dummy_url, sizeof(dummy_url), "?%s", body);
                got_params = get_query_param(dummy_url, "id", id_str, sizeof(id_str)) &&
                             get_query_param(dummy_url, "code", code, sizeof(code)) &&
                             get_query_param(dummy_url, "name", name, sizeof(name));
                get_query_param(dummy_url, "x", x_str, sizeof(x_str));
                get_query_param(dummy_url, "y", y_str, sizeof(y_str));
            } else {
                get_query_param(url, "x", x_str, sizeof(x_str));
                get_query_param(url, "y", y_str, sizeof(y_str));
            }

            if (got_params) {
                int id = atoi(id_str);
                double x = (strlen(x_str) > 0) ? atof(x_str) : 400.0;
                double y = (strlen(y_str) > 0) ? atof(y_str) : 300.0;
                int rc = graph_add_location(g, id, code, name, x, y);
                if (rc == 0) {
                    storage_save_all(g, loc_file, road_file);
                    const char* msg = "{\"status\":\"success\",\"message\":\"Location added successfully.\"}";
                    send_response(client, 200, "OK", "application/json", msg, (int)strlen(msg));
                } else if (rc == -2) {
                    send_json_error(client, 400, "Duplicate location ID already exists.");
                } else if (rc == -3) {
                    send_json_error(client, 400, "Duplicate location Code already exists.");
                } else {
                    send_json_error(client, 400, "Invalid location parameters.");
                }
            } else {
                send_json_error(client, 400, "Missing required location fields (id, code, name).");
            }
        } else if (strncmp(url, "/api/roads", 10) == 0 && strcmp(method, "POST") == 0) {
            char u_str[32] = {0}, v_str[32] = {0}, w_str[32] = {0}, b_str[32] = {0};
            const char* body = strstr(req_buf, "\r\n\r\n");
            if (body) body += 4;

            int got_params = get_query_param(url, "u_id", u_str, sizeof(u_str)) &&
                             get_query_param(url, "v_id", v_str, sizeof(v_str)) &&
                             get_query_param(url, "weight", w_str, sizeof(w_str));

            if (!got_params && body) {
                char dummy_url[1024];
                snprintf(dummy_url, sizeof(dummy_url), "?%s", body);
                got_params = get_query_param(dummy_url, "u_id", u_str, sizeof(u_str)) &&
                             get_query_param(dummy_url, "v_id", v_str, sizeof(v_str)) &&
                             get_query_param(dummy_url, "weight", w_str, sizeof(w_str));
                get_query_param(dummy_url, "bidirectional", b_str, sizeof(b_str));
            } else {
                get_query_param(url, "bidirectional", b_str, sizeof(b_str));
            }

            if (got_params) {
                int u = atoi(u_str);
                int v = atoi(v_str);
                double w = atof(w_str);
                int bidir = (strlen(b_str) > 0) ? atoi(b_str) : 1;

                if (w <= 0.0) {
                    send_json_error(client, 400, "Road weight must be strictly positive (> 0).");
                } else {
                    int rc = graph_add_road(g, u, v, w, bidir);
                    if (rc == 0) {
                        storage_save_all(g, loc_file, road_file);
                        const char* msg = "{\"status\":\"success\",\"message\":\"Road added or updated successfully.\"}";
                        send_response(client, 200, "OK", "application/json", msg, (int)strlen(msg));
                    } else if (rc == -2) {
                        send_json_error(client, 400, "Self loops are not allowed.");
                    } else if (rc == -4) {
                        send_json_error(client, 400, "One or both endpoint locations do not exist.");
                    } else {
                        send_json_error(client, 400, "Failed to create road.");
                    }
                }
            } else {
                send_json_error(client, 400, "Missing required road fields (u_id, v_id, weight).");
            }
        } else {
            /* Serve static files */
            char filepath[1024];
            char clean_url[512];
            const char* q = strchr(url, '?');
            size_t ulen = q ? (size_t)(q - url) : strlen(url);
            if (ulen >= sizeof(clean_url)) ulen = sizeof(clean_url) - 1;
            strncpy(clean_url, url, ulen);
            clean_url[ulen] = '\0';

            if (strcmp(clean_url, "/") == 0 || strcmp(clean_url, "/index.html") == 0) {
                snprintf(filepath, sizeof(filepath), "%s/index.html", frontend_dir);
            } else {
                /* Strip leading slash */
                const char* rel = clean_url;
                if (*rel == '/') rel++;
                snprintf(filepath, sizeof(filepath), "%s/%s", frontend_dir, rel);
            }

            if (!serve_file(client, filepath)) {
                send_json_error(client, 404, "404 Not Found");
            }
        }

        closesocket(client);
    }

    closesocket(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
