#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <random>
#include <ctime>
#include <SFML/Graphics.hpp>
#include <windows.h>

#define SUCCESS 0
#define FAILURE 1

float elap_time() {
	static __int64 start = 0;
	static __int64 frequency = 0;

	if (start == 0) {
		QueryPerformanceCounter((LARGE_INTEGER*)&start);
		QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
		return 0.0f;
	}

	__int64 counter = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&counter);
	return (float)((counter - start) / double(frequency));
}

/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if(f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size+1];
		if(!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	std::cout << "Error: failed to open file\n:" << filename << std::endl;
	return FAILURE;
}

int main(int argc, char* argv[])
{
	int WINDOW_X = 1000;
	int WINDOW_Y = 1000;
	int GRID_WIDTH = 1000;
	int GRID_HEIGHT = 1000;
	int WORKER_SIZE = 2000;

	// ============================== OpenCL Setup ==================================================================

	// Get the platforms
	cl_uint numPlatforms;
	cl_platform_id platform = NULL;
	cl_int	status = clGetPlatformIDs(0, NULL, &numPlatforms); // Retrieve the number of platforms
	if (status != CL_SUCCESS) {
		std::cout << "Error: Getting platforms!" << std::endl;
		return FAILURE;
	}

	 // Choose the first available platform
	if(numPlatforms > 0) {
		cl_platform_id* platforms = new cl_platform_id[numPlatforms]; 
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);	// Now populate the array with the platforms
		platform = platforms[0];
		delete platforms;
	}

	
	cl_uint	numDevices = 0;
	cl_device_id *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);	
	if (numDevices == 0) { //no GPU available.
		std::cout << "No GPU device available." << std::endl;
		std::cout << "Choose CPU as default device." << std::endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);	
		devices = new cl_device_id[numDevices];
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
	}
	else {
		devices = new cl_device_id[numDevices];
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}
	
	cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);
	cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);


	// ============================== Kernel Compilation, Setup ====================================================
	
	// Read the kernel from the file to a string
	const char *filename = "conway_kernel.cl";
	std::string sourceStr;
	status = convertToString(filename, sourceStr);

	// Create a program with the source
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);
	
	// Build the program
	status = clBuildProgram(program, 1,devices,NULL,NULL,NULL);

	// If the build failed
	if (status == CL_BUILD_PROGRAM_FAILURE) {

		// Determine the size of the log
		size_t log_size;
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = new char[log_size];

		// Get the log
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		// Print the log
		std::cout << log << std::endl;
	}

	// Now create the kernel
	cl_kernel kernel = clCreateKernel(program, "conway", NULL);

	// ======================================= Setup grid =========================================================

	// Setup the rng
	std::mt19937 rng(time(NULL));
	std::uniform_int_distribution<int> rgen(0, 12); // 25% chance

	// Init the grids
	unsigned char* front_grid = new unsigned char[GRID_WIDTH * GRID_HEIGHT* 2];

	for (int i = 0; i < 1000 * 1000; i += 2) {
		if (rgen(rng) == 1) {
			front_grid[i] = 1;
		}
		else {
			front_grid[i] = 0;
		}
	}

	unsigned char* rear_grid = new unsigned char[GRID_WIDTH * GRID_HEIGHT * 2];

	for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
		rear_grid[i] = front_grid[i];
	}

	// ====================================== Setup SFML ==========================================================

	sf::Uint8* asdf = rear_grid;

	sf::Uint8* pixel_array = new sf::Uint8[WINDOW_X * WINDOW_Y * 4];

	for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT * 2; i += 2) {

		int p = i / 2;

		pixel_array[p * 4] = 49; // R?
		pixel_array[p * 4 + 1] = 68; // G?
		pixel_array[p * 4 + 2] = 72; // B?
		pixel_array[p * 4 + 3] = 255; // A?
	}

	char* arr = new char[1000 * 1000];


	// Init window, and loop data
	sf::RenderWindow window(sf::VideoMode(GRID_WIDTH, GRID_HEIGHT), "Classic Games");

	float step_size = 0.0005f;
	double frame_time = 0.0, elapsed_time = 0.0, delta_time = 0.0, accumulator_time = 0.0, current_time = 0.0;
	int frame_count = 0;


	int err = 0;
	cl_mem frontBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, GRID_WIDTH * GRID_HEIGHT * sizeof(char), (void*)front_grid, &err);
	cl_mem rearBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, GRID_WIDTH * GRID_HEIGHT * sizeof(char), (void*)rear_grid, &err);

	cl_mem workerCountBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &WORKER_SIZE, &err);
	cl_mem gridWidthBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &GRID_WIDTH, &err);
	cl_mem gridHeightBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &GRID_HEIGHT, &err);

	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&frontBuffer);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&workerCountBuffer);
	status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&gridWidthBuffer);
	status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&gridHeightBuffer);


	sf::Texture texture;
	texture.create(WINDOW_X, WINDOW_Y);
	sf::Sprite sprite(texture);


	// ===================================== Loop ==================================================================
	while (window.isOpen()) {

		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
		}

		// Time keeping
		//elapsed_time = elap_time();
		delta_time = elapsed_time - current_time;
		current_time = elapsed_time;
		if (delta_time > 0.02f)
			delta_time = 0.02f;
		accumulator_time += delta_time;

		while ((accumulator_time - step_size) >= step_size) {
			accumulator_time -= step_size;
			// Do nothing, FPS tied update()
		}

		// ======================================= OpenCL Shtuff =============================================

		// Update the data in GPU memory
		//status = clEnqueueWriteBuffer(commandQueue, frontBuffer, CL_TRUE, 0, GRID_WIDTH * GRID_HEIGHT * 2 * sizeof(char), (void*)grid, NULL, 0, NULL);
		
		// Work size, for each y line
		size_t global_work_size[1] = { WORKER_SIZE };

		// Run the kernel
		status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

		// Get output, put back into grid
		status = clEnqueueReadBuffer(commandQueue, frontBuffer, CL_TRUE, 0, GRID_WIDTH * GRID_HEIGHT * sizeof(char), (void*)rear_grid, 0, NULL, NULL);


		texture.update(pixel_array);
		window.draw(sprite);

		frame_count++;
		window.display();

	}


	// Temporary
	status = clReleaseMemObject(frontBuffer);
	status = clReleaseMemObject(workerCountBuffer);
	status = clReleaseMemObject(gridWidthBuffer);
	status = clReleaseMemObject(gridHeightBuffer);

	/*Step 12: Clean the resources.*/
	status = clReleaseKernel(kernel);				//Release kernel.
	status = clReleaseProgram(program);				//Release the program object.
	status = clReleaseCommandQueue(commandQueue);	//Release  Command queue.
	status = clReleaseContext(context);				//Release context.

	if (devices != NULL)
	{
		free(devices);
		devices = NULL;
	}

	return SUCCESS;
}