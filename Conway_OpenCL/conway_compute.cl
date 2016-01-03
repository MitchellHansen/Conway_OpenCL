__kernel void conway(__global unsigned char* front_grid, __global unsigned char* rear_grid, __global unsigned char* pixel_out, __global int* num_workers, __global int* grid_width, __global int* grid_height)
{
	// Caclulate the start and end range that this worker will be calculating

	int data_length = *grid_width * *grid_height;

	int start_range = (data_length / *num_workers) * get_global_id(0);
	int end_range = (data_length / *num_workers) * (get_global_id(0) + 1);
	
	// x, y + 1

	int neighbors = 0;

	for (int i = start_range; i < end_range; i++){
		
		// add all 8 blocks to neighbors
		neighbors = 0;

		// Top
		neighbors += front_grid[i - *grid_width];

		// Top right
		neighbors += front_grid[i - *grid_width + 1];

		// Right
		neighbors += front_grid[i + 1];

		// Bottom Right
		neighbors += front_grid[i + *grid_width + 1];

		// Bottom
		neighbors += front_grid[i + *grid_width];

		// Bottom Left
		neighbors += front_grid[i + *grid_width - 1];

		// Left
		neighbors += front_grid[i - 1];

		// Top left
		neighbors += front_grid[i - *grid_width - 1];


		if (neighbors == 3 || (neighbors == 2 && front_grid[i])) {
			rear_grid[i] = 1;
			pixel_out[i * 4] = 255; // R
			pixel_out[i * 4 + 1] = 255; // G
			pixel_out[i * 4 + 2] = 255; // B
			pixel_out[i * 4 + 3] = 255; // A
		}
	
		else {
			rear_grid[i] = 0;
			pixel_out[i * 4] = 49; // R
			pixel_out[i * 4 + 1] = 68; // G
			pixel_out[i * 4 + 2] = 72; // B
			pixel_out[i * 4 + 3] = 255; // A
		}

	}
}