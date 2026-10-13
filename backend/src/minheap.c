/**
 * @file minheap.c
 * @brief Implementation of Binary Min-Heap Priority Queue
 */

#include "../include/minheap.h"
#include <stdio.h>
#include <stdlib.h>

static MinHeapNode* create_heap_node(int v, double dist) {
    MinHeapNode* node = (MinHeapNode*)malloc(sizeof(MinHeapNode));
    if (!node) return NULL;
    node->vertex_index = v;
    node->dist = dist;
    return node;
}

MinHeap* min_heap_create(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!heap) return NULL;

    heap->pos = (int*)malloc(sizeof(int) * capacity);
    heap->size = 0;
    heap->capacity = capacity;
    heap->array = (MinHeapNode**)malloc(sizeof(MinHeapNode*) * capacity);

    if (!heap->pos || !heap->array) {
        if (heap->pos) free(heap->pos);
        if (heap->array) free(heap->array);
        free(heap);
        return NULL;
    }

    for (int i = 0; i < capacity; i++) {
        heap->pos[i] = -1;
        heap->array[i] = NULL;
    }

    return heap;
}

void min_heap_free(MinHeap* heap) {
    if (!heap) return;
    for (int i = 0; i < heap->size; i++) {
        if (heap->array[i]) free(heap->array[i]);
    }
    if (heap->array) free(heap->array);
    if (heap->pos) free(heap->pos);
    free(heap);
}

static void swap_heap_nodes(MinHeap* heap, int i, int j) {
    MinHeapNode* temp = heap->array[i];
    heap->array[i] = heap->array[j];
    heap->array[j] = temp;

    /* Update pos array to keep track of new indices in heap */
    heap->pos[heap->array[i]->vertex_index] = i;
    heap->pos[heap->array[j]->vertex_index] = j;
}

static void min_heapify(MinHeap* heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->array[left]->dist < heap->array[smallest]->dist) {
        smallest = left;
    }

    if (right < heap->size && heap->array[right]->dist < heap->array[smallest]->dist) {
        smallest = right;
    }

    if (smallest != idx) {
        swap_heap_nodes(heap, idx, smallest);
        min_heapify(heap, smallest);
    }
}

int min_heap_is_empty(const MinHeap* heap) {
    return (!heap || heap->size == 0);
}

MinHeapNode* min_heap_extract_min(MinHeap* heap) {
    if (min_heap_is_empty(heap)) return NULL;

    /* Root node contains minimum distance */
    MinHeapNode* root = heap->array[0];

    /* Replace root with last node */
    MinHeapNode* last_node = heap->array[heap->size - 1];
    heap->array[0] = last_node;

    /* Update position of last node */
    heap->pos[root->vertex_index] = -1;
    heap->pos[last_node->vertex_index] = 0;

    heap->size--;
    min_heapify(heap, 0);

    return root;
}

void min_heap_decrease_key(MinHeap* heap, int vertex_index, double new_dist) {
    if (!heap || vertex_index < 0 || vertex_index >= heap->capacity) return;

    int i = heap->pos[vertex_index];
    if (i < 0 || i >= heap->size) return;

    heap->array[i]->dist = new_dist;

    /* Bubble up while current node is smaller than its parent */
    while (i > 0 && heap->array[i]->dist < heap->array[(i - 1) / 2]->dist) {
        int parent = (i - 1) / 2;
        swap_heap_nodes(heap, i, parent);
        i = parent;
    }
}

void min_heap_insert(MinHeap* heap, int vertex_index, double dist) {
    if (!heap || heap->size >= heap->capacity) return;

    MinHeapNode* node = create_heap_node(vertex_index, dist);
    if (!node) return;

    int i = heap->size;
    heap->array[i] = node;
    heap->pos[vertex_index] = i;
    heap->size++;

    /* Bubble up to restore heap invariant */
    while (i > 0 && heap->array[i]->dist < heap->array[(i - 1) / 2]->dist) {
        int parent = (i - 1) / 2;
        swap_heap_nodes(heap, i, parent);
        i = parent;
    }
}

int min_heap_is_in_heap(const MinHeap* heap, int vertex_index) {
    if (!heap || vertex_index < 0 || vertex_index >= heap->capacity) return 0;
    return (heap->pos[vertex_index] >= 0 && heap->pos[vertex_index] < heap->size);
}
