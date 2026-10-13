/**
 * @file minheap.h
 * @brief Binary Min-Heap Priority Queue for Dijkstra's Algorithm
 *
 * Implements an indexed min-priority queue supporting:
 * - Insert: O(log V)
 * - Extract-Min: O(log V)
 * - Decrease-Key: O(log V) using an inverted position lookup array
 * Overall Dijkstra Complexity achieved: O((V + E) log V)
 */

#ifndef MINHEAP_H
#define MINHEAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/**
 * @struct MinHeapNode
 * @brief Represents a vertex and its current shortest distance estimate
 */
typedef struct {
    int vertex_index;    /**< Index of vertex in Graph */
    double dist;         /**< Current tentative shortest distance from source */
} MinHeapNode;

/**
 * @struct MinHeap
 * @brief Binary min-heap data structure with O(1) index position lookup
 */
typedef struct {
    int size;            /**< Current count of elements in the heap */
    int capacity;        /**< Maximum capacity */
    int* pos;            /**< pos[v] = index in array[] where vertex v is stored */
    MinHeapNode** array; /**< Array of heap node pointers */
} MinHeap;

/* Lifecycle */
MinHeap* min_heap_create(int capacity);
void min_heap_free(MinHeap* heap);

/* Core Priority Queue Operations */
MinHeapNode* min_heap_extract_min(MinHeap* heap);
void min_heap_decrease_key(MinHeap* heap, int vertex_index, double new_dist);
void min_heap_insert(MinHeap* heap, int vertex_index, double dist);
int min_heap_is_in_heap(const MinHeap* heap, int vertex_index);
int min_heap_is_empty(const MinHeap* heap);

#endif /* MINHEAP_H */
