__kernel void conway_compute(__write_only image2d_t front_image, __global unsigned char* back_image, __global int* num_workers, __global int* grid_width, __global int *grid_height)
{

	float4 black = (float4)(1.0, 0.0, 0.0, 1.0);
	float4 white = (float4)(0.0, 0.0, 1.0, 0.0);

	// Caclulate the start and end range that this worker will be calculating
	int data_length = *grid_width * *grid_height;

	int start_range = (data_length / *num_workers) * get_global_id(0);
	int end_range = (data_length / *num_workers) * (get_global_id(0) + 1);
	
	// x, y + 1

	int neighbors = 0;

	for (int i = start_range; i < end_range; i++){

		unsigned char im = back_image[i];

		int2 pixelcoord = (int2) (i % *grid_width, i / *grid_height);

		// add all 8 blocks to neghbors
		neighbors = 0;
		// Top
		neighbors += back_image[i - *grid_width];

		// Top right
		neighbors += back_image[i - *grid_width + 1];

		/// Right
		neighbors += back_image[i + 1];

		/// Bottom Right
		neighbors += back_image[i + *grid_width + 1];

		// Bottom
		neighbors += back_image[i + *grid_width];

		// Bottom Left
		neighbors += back_image[i + *grid_width - 1];

		// Left
		neighbors += back_image[i - 1];

		// Top left
		neighbors += back_image[i - *grid_width - 1];

		// push living status to the padded second char


		//write_imagef(front_image, pixelcoord, black);
		
		if (neighbors == 3 || (neighbors == 2 && back_image[i] == 1) || (i % 10) == 1){
			write_imagef(front_image, pixelcoord, black);
		}
	
		else{
			write_imagef(front_image, pixelcoord, white);
		}
	}
}