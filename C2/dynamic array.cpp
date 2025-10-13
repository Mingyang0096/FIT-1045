#include "splashkit.h"
#include "dynamic array.hpp"

int main()
{
    DynamicArray arr;

    // Initialize array with capacity 2
    init_array(arr, 2);

    // Add elements
    push_back(arr, 10);
    push_back(arr, 20);
    push_back(arr, 30); // This will trigger resize

    // Print elements
    write_line("Current array elements:");
    for (size_t i = 0; i < arr.size; ++i)
    {
        write_line("Index " + std::to_string(i) + ": " + std::to_string(get_at(arr, i)));
    }

    // Modify an element
    set_at(arr, 1, 99);
    write_line("After modification:");
    for (size_t i = 0; i < arr.size; ++i)
    {
        write_line("Index " + std::to_string(i) + ": " + std::to_string(get_at(arr, i)));
    }

    // Free memory
    free_array(arr);
    write_line("Array memory freed.");

    return 0;
}
