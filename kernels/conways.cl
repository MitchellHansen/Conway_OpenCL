int pixel_to_index(int2 dimensions, int2 pixel) {

	if (pixel.x >= 0 && pixel.x <= dimensions.x &&
		pixel.y >= 0 && pixel.y <= dimensions.y ){

		return pixel.y * dimensions.x + pixel.x;
	}

	return -1;
}


__kernel void conways (
		global int2* image_res,
		__read_write image2d_t image,
		global char* first_node_buffer,
        global char* second_node_buffer,
        global char* buffer_flip
	){

	size_t x_pixel = get_global_id(0);
	size_t y_pixel = get_global_id(1);


	//printf(": %i", y_pixel);

	int2 pixel = (int2)(x_pixel, y_pixel);

	//if (pixel.x > 1800)
	//	printf("%i, %i", pixel.x, pixel.y);

	float4 dead = (float4)(.51, .49, .39, .9);
	float4 alive = (float4)(.9, .9, .9, .9);
	float4 flavor = (float4)(.21, .76, .83, .8);

	// add all 8 blocks to neghbors
	int neighbors = 0;
	int val = 0;

	if (*buffer_flip == 0){

		// Top
		val = pixel_to_index(*image_res, (int2)(pixel.x, pixel.y+1));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		// Top Right
		val = pixel_to_index(*image_res, (int2)(pixel.x+1, pixel.y+1));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		// Right
		val = pixel_to_index(*image_res, (int2)(pixel.x+1, pixel.y));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		// Bottom Right
		val = pixel_to_index(*image_res, (int2)(pixel.x+1, pixel.y-1));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		// Bottom
		val = pixel_to_index(*image_res, (int2)(pixel.x, pixel.y-1));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		// Bottom Left
		val = pixel_to_index(*image_res, (int2)(pixel.x-1, pixel.y-1));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		// Left
		val = pixel_to_index(*image_res, (int2)(pixel.x-1, pixel.y));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		// Top Left
		val = pixel_to_index(*image_res, (int2)(pixel.x-1, pixel.y+1));
		if (val >= 0)
			neighbors += first_node_buffer[val];

		int base = pixel_to_index(*image_res, pixel);
		if (neighbors == 3 || (neighbors == 2 && first_node_buffer[base])){
			write_imagef(image, pixel, alive);
			second_node_buffer[base] = 1;
		} else {
			write_imagef(image, pixel, dead);
			//write_imagef(image, pixel, mix(read_imagef(image, pixel), dead, 0.01f));
			second_node_buffer[base] = 0;
		}

	} else {

		// Top
		val = pixel_to_index(*image_res, (int2)(pixel.x, pixel.y+1));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		// Top Right
		val = pixel_to_index(*image_res, (int2)(pixel.x+1, pixel.y+1));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		// Right
		val = pixel_to_index(*image_res, (int2)(pixel.x+1, pixel.y));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		// Bottom Right
		val = pixel_to_index(*image_res, (int2)(pixel.x+1, pixel.y-1));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		// Bottom
		val = pixel_to_index(*image_res, (int2)(pixel.x, pixel.y-1));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		// Bottom Left
		val = pixel_to_index(*image_res, (int2)(pixel.x-1, pixel.y-1));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		// Left
		val = pixel_to_index(*image_res, (int2)(pixel.x-1, pixel.y));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		// Top Left
		val = pixel_to_index(*image_res, (int2)(pixel.x-1, pixel.y+1));
		if (val >= 0)
			neighbors += second_node_buffer[val];

		int base = pixel_to_index(*image_res, pixel);
		if (neighbors == 3 || (neighbors == 2 && second_node_buffer[base])){
		   	write_imagef(image, pixel, alive);
		   	first_node_buffer[base] = 1;
		} else {
			write_imagef(image, pixel, dead);
		  	//write_imagef(image, pixel, mix(read_imagef(image, pixel), flavor, 0.01f));
		   	first_node_buffer[base] = 0;
		}
	}
}
