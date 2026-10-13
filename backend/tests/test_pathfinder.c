/**
 * @file test_pathfinder.c
 * @brief Automated Unit and Integration Test Suite for Shortest Path Finder
 *
 * Verifies all 10 required test cases:
 *  1. Direct shortest route
 *  2. Route requiring multiple intermediate locations
 *  3. Multiple possible routes (verifies optimal path chosen)
 *  4. Unreachable destination in disconnected graph
 *  5. Source equals destination
 *  6. Graph with one location and no roads
 *  7. Invalid location IDs handling
 *  8. Zero and negative weight validation & rejection
 *  9. Updating and deleting road with recalculation
 * 10. Persistence after graph reload
 */

#include "../include/graph.h"
#include "../include/minheap.h"
#include "../include/dijkstra.h"
#include "../include/storage.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static int g_tests_run = 0;
static int g_tests_passed = 0;

#define TEST_ASSERT(cond, name) do { \
    g_tests_run++; \
    if (cond) { \
        g_tests_passed++; \
        printf("  [PASS] Test %d: %s\n", g_tests_run, name); \
    } else { \
        printf("  [FAIL] Test %d: %s (Line %d)\n", g_tests_run, name, __LINE__); \
    } \
} while(0)

static int approx_eq(double a, double b) {
    return fabs(a - b) < 0.001;
}

void test_direct_route(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "A", "Point A", 0, 0);
    graph_add_location(g, 2, "B", "Point B", 10, 10);
    graph_add_road(g, 1, 2, 5.5, 1);

    DijkstraResult* res = dijkstra_find_shortest_path(g, 1, 2, 0);
    TEST_ASSERT(res && res->success == 1, "Direct route successfully found");
    TEST_ASSERT(res && approx_eq(res->total_distance, 5.5), "Direct route distance equals 5.5");
    TEST_ASSERT(res && res->path_length == 2, "Direct route has 2 vertices");

    dijkstra_free_result(res);
    graph_free(g);
}

void test_multiple_intermediates(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "A", "Node A", 0, 0);
    graph_add_location(g, 2, "B", "Node B", 1, 1);
    graph_add_location(g, 3, "C", "Node C", 2, 2);
    graph_add_location(g, 4, "D", "Node D", 3, 3);

    graph_add_road(g, 1, 2, 2.0, 1);
    graph_add_road(g, 2, 3, 3.0, 1);
    graph_add_road(g, 3, 4, 4.0, 1);

    DijkstraResult* res = dijkstra_find_shortest_path(g, 1, 4, 0);
    TEST_ASSERT(res && res->success == 1, "Multiple intermediate route found");
    TEST_ASSERT(res && approx_eq(res->total_distance, 9.0), "Total distance equals 2.0+3.0+4.0 = 9.0");
    TEST_ASSERT(res && res->path_length == 4, "Path contains 4 nodes in sequence (A->B->C->D)");

    dijkstra_free_result(res);
    graph_free(g);
}

void test_multiple_possible_routes(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "SRC", "Source", 0, 0);
    graph_add_location(g, 2, "UP", "Upper Node", 1, 2);
    graph_add_location(g, 3, "LOW", "Lower Node", 1, -2);
    graph_add_location(g, 4, "DST", "Destination", 3, 0);

    /* Upper route: 1 -> 2 (10) -> 4 (15) = 25 */
    graph_add_road(g, 1, 2, 10.0, 1);
    graph_add_road(g, 2, 4, 15.0, 1);

    /* Lower route: 1 -> 3 (6) -> 4 (8) = 14 (Shorter!) */
    graph_add_road(g, 1, 3, 6.0, 1);
    graph_add_road(g, 3, 4, 8.0, 1);

    DijkstraResult* res = dijkstra_find_shortest_path(g, 1, 4, 0);
    TEST_ASSERT(res && res->success == 1, "Multiple routes comparison found route");
    TEST_ASSERT(res && approx_eq(res->total_distance, 14.0), "Chooses optimal lower path with cost 14.0 over 25.0");
    TEST_ASSERT(res && res->path_ids[1] == 3, "Path passes through lower node (ID 3)");

    dijkstra_free_result(res);
    graph_free(g);
}

void test_unreachable_destination(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "ISLAND1", "Island A", 0, 0);
    graph_add_location(g, 2, "ISLAND2", "Island B", 100, 100);

    DijkstraResult* res = dijkstra_find_shortest_path(g, 1, 2, 0);
    TEST_ASSERT(res && res->success == 0, "Unreachable destination correctly detected");
    TEST_ASSERT(res && res->error_code == -3, "Returns unreachable error code (-3)");

    dijkstra_free_result(res);
    graph_free(g);
}

void test_source_equals_destination(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "SAME", "Single Point", 0, 0);

    DijkstraResult* res = dijkstra_find_shortest_path(g, 1, 1, 0);
    TEST_ASSERT(res && res->success == 1, "Source == Destination handled gracefully");
    TEST_ASSERT(res && approx_eq(res->total_distance, 0.0), "Distance from node to itself is 0.0");
    TEST_ASSERT(res && res->path_length == 1, "Path contains exactly 1 node");

    dijkstra_free_result(res);
    graph_free(g);
}

void test_single_location_no_roads(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 10, "ALONE", "Isolated Node", 0, 0);

    TEST_ASSERT(g->num_vertices == 1, "Graph initialized with 1 location");
    int b = 0, o = 0;
    TEST_ASSERT(graph_count_roads(g, &b, &o) == 0, "Road count is 0");

    DijkstraResult* res = dijkstra_find_shortest_path(g, 10, 10, 0);
    TEST_ASSERT(res && res->success == 1 && approx_eq(res->total_distance, 0.0), "Self query on isolated node succeeds");

    dijkstra_free_result(res);
    graph_free(g);
}

void test_invalid_location_ids(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "A", "Node A", 0, 0);

    DijkstraResult* res = dijkstra_find_shortest_path(g, 1, 999, 0);
    TEST_ASSERT(res && res->success == 0, "Invalid destination ID rejected");
    dijkstra_free_result(res);

    res = dijkstra_find_shortest_path(g, -5, 1, 0);
    TEST_ASSERT(res && res->success == 0, "Negative source ID rejected");
    dijkstra_free_result(res);

    graph_free(g);
}

void test_zero_and_negative_weights(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "A", "Node A", 0, 0);
    graph_add_location(g, 2, "B", "Node B", 1, 1);

    int r_zero = graph_add_road(g, 1, 2, 0.0, 1);
    TEST_ASSERT(r_zero < 0, "Zero weight road rejected by validation");

    int r_neg = graph_add_road(g, 1, 2, -4.5, 1);
    TEST_ASSERT(r_neg < 0, "Negative weight road rejected by validation");

    graph_free(g);
}

void test_road_update_and_deletion(void) {
    Graph* g = graph_create(10);
    graph_add_location(g, 1, "A", "Node A", 0, 0);
    graph_add_location(g, 2, "B", "Node B", 1, 1);
    graph_add_road(g, 1, 2, 10.0, 1);

    DijkstraResult* res1 = dijkstra_find_shortest_path(g, 1, 2, 0);
    TEST_ASSERT(res1 && approx_eq(res1->total_distance, 10.0), "Initial road cost is 10.0");
    dijkstra_free_result(res1);

    /* Update road to 3.0 */
    graph_update_road(g, 1, 2, 3.0, 1);
    DijkstraResult* res2 = dijkstra_find_shortest_path(g, 1, 2, 0);
    TEST_ASSERT(res2 && approx_eq(res2->total_distance, 3.0), "Updated road cost is now 3.0");
    dijkstra_free_result(res2);

    /* Delete road */
    graph_delete_road(g, 1, 2);
    DijkstraResult* res3 = dijkstra_find_shortest_path(g, 1, 2, 0);
    TEST_ASSERT(res3 && res3->success == 0, "After deletion, path is unreachable");
    dijkstra_free_result(res3);

    graph_free(g);
}

void test_persistence_reloading(void) {
    const char* test_loc = "data/test_loc.csv";
    const char* test_road = "data/test_road.csv";

    Graph* g1 = graph_create(10);
    graph_add_location(g1, 101, "T1", "Persist One", 50, 60);
    graph_add_location(g1, 102, "T2", "Persist Two", 150, 160);
    graph_add_road(g1, 101, 102, 12.8, 1);

    storage_save_all(g1, test_loc, test_road);
    graph_free(g1);

    /* Create fresh graph and reload */
    Graph* g2 = graph_create(10);
    int rc = storage_load_all(g2, test_loc, test_road);
    TEST_ASSERT(rc == 0, "Saved graph files loaded successfully");
    TEST_ASSERT(g2->num_vertices == 2, "Reloaded graph has 2 locations");

    DijkstraResult* res = dijkstra_find_shortest_path(g2, 101, 102, 0);
    TEST_ASSERT(res && res->success == 1 && approx_eq(res->total_distance, 12.8),
                "Recalculation on reloaded persistent graph matches original cost (12.8)");

    dijkstra_free_result(res);
    graph_free(g2);

    /* Clean up temporary test files */
    remove(test_loc);
    remove(test_road);
}

int main(void) {
    printf("=================================================================\n");
    printf("  RUNNING SHORTEST PATH FINDER AUTOMATED C UNIT TEST SUITE\n");
    printf("=================================================================\n\n");

    test_direct_route();
    test_multiple_intermediates();
    test_multiple_possible_routes();
    test_unreachable_destination();
    test_source_equals_destination();
    test_single_location_no_roads();
    test_invalid_location_ids();
    test_zero_and_negative_weights();
    test_road_update_and_deletion();
    test_persistence_reloading();

    printf("\n=================================================================\n");
    printf("  TEST SUITE RESULTS: %d / %d PASSED (%.1f%%)\n",
           g_tests_passed, g_tests_run, (100.0 * g_tests_passed) / g_tests_run);
    printf("=================================================================\n");

    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
