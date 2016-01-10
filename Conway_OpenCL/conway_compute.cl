__kernel void conway_compute(__global unsigned char* front_grid, __global int* num_workers, __global int* grid_width, __global int* grid_height)
{
	int num = *grid_width * *grid_height * 4;

	for (int i = 0; i < num ; i += 4){
	
		front_grid[i] = 0;
		front_grid[i + 1] = 0;
		front_grid[i + 2] = 0;
		front_grid[i + 3] = 0;
	
	}

	front_grid[90000] = 0;
	front_grid[90001] = 0;
	front_grid[90002] = 0;
	front_grid[90003] = 0;
	front_grid[90004] = 0;
	front_grid[90005] = 0;
	front_grid[90006] = 0;
}