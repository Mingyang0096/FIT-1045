#include "splashkit.h"
#include <cstdio>   // For printf and scanf

// Define the dimensions of the grid
const int MAX_GRID = 2;   // Number of grids (like layers)
const int MAX_COL  = 3;   // Number of columns in each grid
const int MAX_ROW  = 4;   // Number of rows in each column

// ------------------------------------------------------------
// Converts a 3D index (grid, column, row) into a 1D array index.
// Returns -1 if the indices are out of valid range.
// ------------------------------------------------------------
int grid_data_index(int grid_index, int column_index, int row_index)
{
    // Check if any index is out of bounds
    if ( grid_index < 0 || grid_index >= MAX_GRID ||
         column_index < 0 || column_index >= MAX_COL ||
         row_index < 0 || row_index >= MAX_ROW )
    {
        return -1; // Invalid index
    }

    // Calculate the 1D index from the 3D coordinates
    return grid_index * (MAX_COL * MAX_ROW) +
           column_index * MAX_ROW +
           row_index;
}

// ------------------------------------------------------------
// Reads a value from the grid at the given 3D position.
// Returns 0 if the position is invalid.
// ------------------------------------------------------------
int read_grid_data(int *grid, int grid_index, int column_index, int row_index)
{
    int idx = grid_data_index(grid_index, column_index, row_index);

    if ( idx != -1 ) // Valid index
    {
        return grid[idx];
    }
    return 0; // Invalid index, return default value
}

// ------------------------------------------------------------
// Sets a value in the grid at the given 3D position.
// Does nothing if the position is invalid.
// Returns the calculated index (or -1 if invalid).
// ------------------------------------------------------------
int set_grid_data(int *grid, int grid_index, int column_index, int row_index, int value)
{
    int idx = grid_data_index(grid_index, column_index, row_index);

    if ( idx != -1 ) // Valid index
    {
        grid[idx] = value;
    }
    return idx;
}

// ------------------------------------------------------------
// Main program demonstrating how to use the above functions.
// ------------------------------------------------------------
int main()
{
    // Open a SplashKit window (not strictly needed for console output,
    // but included to show integration with SplashKit)
    open_window("Grid Data Demo", 800, 600);

    // Create a 1D array to store all grid data
    // Total size = MAX_GRID * MAX_COL * MAX_ROW
    int grid[MAX_GRID * MAX_COL * MAX_ROW] = {0}; // Initialize all to 0

    // Set some example values in the grid
    set_grid_data(grid, 0, 0, 0, 10); // grid 0, col 0, row 0 = 10
    set_grid_data(grid, 0, 1, 2, 25); // grid 0, col 1, row 2 = 25
    set_grid_data(grid, 1, 2, 3, 99); // grid 1, col 2, row 3 = 99

    // Read and print specific values
    printf("grid[0][0][0] = %d\n", read_grid_data(grid, 0, 0, 0));
    printf("grid[0][1][2] = %d\n", read_grid_data(grid, 0, 1, 2));
    printf("grid[1][2][3] = %d\n", read_grid_data(grid, 1, 2, 3));

    // Loop through all positions and print their values
    printf("\n--- Full Grid Data ---\n");
    for (int g = 0; g < MAX_GRID; g++)
    {
        for (int c = 0; c < MAX_COL; c++)
        {
            for (int r = 0; r < MAX_ROW; r++)
            {
                printf("grid[%d][%d][%d] = %d\n",
                       g, c, r, read_grid_data(grid, g, c, r));
            }
        }
    }

    // Keep the window open for 5 seconds before closing
    delay(5000);

    return 0;
}
