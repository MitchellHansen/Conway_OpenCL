__kernel void conway_compute(__write_only image2d_t front_grid, __global int* num_workers, __global int* grid_width, __global int *grid_height)
{

//int width = *grid_width;
//int height = grid_height;

	for (int i = 0; i < 90000; i ++){
	int2 pixelcoord = (int2) (i % *grid_width, i / *grid_height);
	//if (pixelcoord.x < width && pixelcoord.y < height)
	//{   
		//float4 pixel = read_imagef(image1, sampler, (int2)(pixelcoord.x, pixelcoord.y));
    int4 black = (int4)(0,0,0,0);


    //write_imagef(front_grid, pixelcoord, black);

	write_imagei(front_grid, pixelcoord, black);

}
//}
}