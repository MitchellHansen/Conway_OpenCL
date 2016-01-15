__kernel void conway_align(__read_only image2d_t front_image, __global unsigned char* back_image, __global int* num_workers, __global int* grid_width, __global int *grid_height)
{
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE  | CLK_FILTER_NEAREST;

	// Caclulate the start and end range that this worker will be calculating
	int data_length = *grid_width * *grid_height;

	int start_range = (data_length / *num_workers) * get_global_id(0);
	int end_range = (data_length / *num_workers) * (get_global_id(0) + 1);


	for (int i = start_range; i < end_range; i++){
		
			int2 pixelcoord = (int2) (i % *grid_width, i / *grid_height);
			int4 pixel;
			pixel = read_imagei(front_image, sampler, pixelcoord);

			if (pixel.w > 200){
				back_image[i] = 0;
			}
			else
				back_image[i] = 0;

			
	}

}