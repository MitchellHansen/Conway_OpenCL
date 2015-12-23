__kernel void helloworld(__global char* in, __global int num_workers, __global int grid_width, __global int grid_height)
{
	// Caclulate the start and end range that this worker will be calculating

	int data_length = grid_width * grid_height;

	int start_range = (data_length / num_workers) * get_global_id(0) * 2; // * 2 = padding
	int end_range = (data_length / num_workers) * (get_global_id(0) + 1) * 2;
	
	// x, y + 1

	int neighbors = 0;

	for (int i = start_range; i < end_range; i += 2){
		
		// add all 8 blocks to neghbors

		// Top
		neighbors += in[i - grid_width * 2];

		// Top right
		neightbors += in[i - grid_width * 2 + 2];

		// Right
		neighbors += in[i + 2];

		// Bottom Right
		neighbors += in[i + grid_width * 2 + 2];

		// Bottom
		neighbors += in[i + grid_width * 2];

		// Bottom Left
		neighbors += in[i + grid_width * 2 - 2];

		// Left
		neighbors += in[i - 2];

		// Top left
		neighbors += in[i - grid_width * 2 - 2];

		// push living status to the padded second char

		if (neighbors == 3 || (neighbors == 2 && in[i])){
			in[i + 1] = 1;
		}
	
		else
			in[i + 1] = 0;
	}
}