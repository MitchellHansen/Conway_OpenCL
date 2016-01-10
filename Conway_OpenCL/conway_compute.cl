__kernel void conway_compute(__write_only image2d_t front_grid, __global int* num_workers, __global int* grid_width, __global int *grid_height)
{

//int width = *grid_width;
//int height = grid_height;

int2 pixelcoord = (int2) (get_global_id(0), get_global_id(1));
//if (pixelcoord.x < width && pixelcoord.y < height)
//{   
    //float4 pixel = read_imagef(image1, sampler, (int2)(pixelcoord.x, pixelcoord.y));
    float4 black = (float4)(0,0,0,0);


  //  write_imagef(front_grid, pixelcoord, black);


	int num = *grid_width * *grid_height * 4;

	for (int i = 0; i < num ; i += 4){
	
		write_imagef(front_grid, pixelcoord, black);
//	
	}
//}
}