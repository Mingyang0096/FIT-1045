#ifndef DYNAMIC_ARRAY_HPP
#define DYNAMIC_ARRAY_HPP

#include "splashkit.h"
#include <cstdlib>      // for size_t
#include <iostream>     // indirectly includes <new> for std::nothrow

// Dynamic array structure (stores only int)
struct DynamicArray
{
    int *data;       // Pointer to array data
    size_t size;     // Current number of elements
    size_t capacity; // Allocated capacity
};

// Initialize the dynamic array
void init_array(DynamicArray &arr, size_t initial_capacity)
{
    arr.size = 0;
    arr.capacity = (initial_capacity > 0) ? initial_capacity : 1;
    arr.data = new (std::nothrow) int[arr.capacity];

    if (arr.data == NULL)
    {
        arr.capacity = 0;
        write_line("Memory allocation failed during init_array!");
    }
}

// Free the memory of the dynamic array
void free_array(DynamicArray &arr)
{
    delete[] arr.data;
    arr.data = NULL;
    arr.size = 0;
    arr.capacity = 0;
}

// Resize the array (called when capacity is insufficient)
bool resize_array(DynamicArray &arr, size_t new_capacity)
{
    if (new_capacity <= arr.capacity) return true;

    int *new_data = new (std::nothrow) int[new_capacity];
    if (new_data == NULL)
    {
        write_line("Memory allocation failed during resize_array!");
        return false;
    }

    // Copy old data
    for (size_t i = 0; i < arr.size; ++i)
    {
        new_data[i] = arr.data[i];
    }

    delete[] arr.data;
    arr.data = new_data;
    arr.capacity = new_capacity;
    return true;
}

// Add an element to the end of the array
bool push_back(DynamicArray &arr, int value)
{
    if (arr.size >= arr.capacity)
    {
        if (!resize_array(arr, arr.capacity * 2))
            return false;
    }
    arr.data[arr.size++] = value;
    return true;
}

// Get the element at the specified index (no bounds checking)
int get_at(const DynamicArray &arr, size_t index)
{
    return arr.data[index];
}

// Set the element at the specified index (no bounds checking)
void set_at(DynamicArray &arr, size_t index, int value)
{
    arr.data[index] = value;
}

#endif // DYNAMIC_ARRAY_HPP
