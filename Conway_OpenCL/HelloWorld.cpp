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
	int WORKER_SIZE = 1000;

	// ============================== OpenCL Setup ==================================================================

	/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms;	//the NO. of platforms
	cl_platform_id platform = NULL;	//the chosen platform
	cl_int	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS) {
		std::cout << "Error: Getting platforms!" << std::endl;
		return FAILURE;
	}

	 // Choose the first available platform
	if(numPlatforms > 0)
	{
		cl_platform_id* platforms = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
		platform = platforms[0];
		free(platforms);
	}

	/*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
	cl_uint				numDevices = 0;
	cl_device_id        *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);	
	if (numDevices == 0) { //no GPU available.
		std::cout << "No GPU device available." << std::endl;
		std::cout << "Choose CPU as default device." << std::endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);	
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
	}
	else {
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}
	

	/*Step 3: Create context.*/
	cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);
	
	/*Step 4: Creating command queue associate with the context.*/
	cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);


	// ============================== Kernel Compilation, Setup ====================================================

	/*Step 5: Create program object */
	const char *filename = "HelloWorld_Kernel.cl";
	std::string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);
	
	// Build program and set kernel
	status=clBuildProgram(program, 1,devices,NULL,NULL,NULL);

	if (status == CL_BUILD_PROGRAM_FAILURE) {
		// Determine the size of the log
		size_t log_size;
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = (char *)malloc(log_size);

		// Get the log
		clGetProgramBuildInfo(program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		// Print the log
		printf("%s\n", log);
	}

	cl_kernel kernel = clCreateKernel(program, "helloworld", NULL);

	// ======================================= Setup grid =========================================================

	// Setup the rng
	std::mt19937 rng(time(NULL));
	std::uniform_int_distribution<int> rgen(0, 4); // 25% chance

	// Init the grid 
	char* grid = new char[GRID_WIDTH * GRID_HEIGHT* 2];

	for (int i = 0; i < 1000 * 1000 * 2; i += 2) {
		if (rgen(rng) == 1) {
			grid[i] = 1;
			grid[i + 1] = 1;
		}
		else {
			grid[i] = 0;
			grid[i + 1] = 0;
		}
	}

	// ====================================== Setup SFML ==========================================================

	// Spites for drawing, probably where the biggest slowdown is
	sf::RectangleShape live_node;
	live_node.setFillColor(sf::Color(145, 181, 207));
	live_node.setSize(sf::Vector2f(1, 1));

	// Init window, and loop data
	sf::RenderWindow window(sf::VideoMode(GRID_WIDTH, GRID_HEIGHT), "Classic Games");

	float step_size = 0.0005f;
	double frame_time = 0.0, elapsed_time = 0.0, delta_time = 0.0, accumulator_time = 0.0, current_time = 0.0;
	int frame_count = 0;


	int err = 0;



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

		// Implicit dead node color
		window.clear(sf::Color(49, 68, 72));

		cl_mem inputBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, GRID_WIDTH * GRID_HEIGHT * 2 * sizeof(char), (void*)grid, &err);
		cl_mem workerCountBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &WORKER_SIZE, &err);
		cl_mem gridWidthBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &GRID_WIDTH, &err);
		cl_mem gridHeightBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &GRID_HEIGHT, &err);


		status = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&inputBuffer);
		status = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&workerCountBuffer);
		status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&gridWidthBuffer);
		status = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&gridHeightBuffer);
		
		//status = clEnqueueWriteBuffer(commandQueue, inputBuffer, CL_TRUE, 0, GRID_WIDTH * GRID_HEIGHT * 2 * sizeof(char), (void*)grid, NULL, 0, NULL);

		// One work item per group, don't really know if this impacts performance
		size_t global_work_size[1] = { 100 };

		// Run the kernel
		status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);

		// Get output, put back into grid
		//cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, GRID_WIDTH * GRID_HEIGHT * 2 * sizeof(char), grid, NULL);
		status = clEnqueueReadBuffer(commandQueue, inputBuffer, CL_TRUE, 0, GRID_WIDTH * GRID_HEIGHT * 2 * sizeof(char), (void*)grid, 0, NULL, NULL);

		// Temporary
		status = clReleaseMemObject(inputBuffer);
		status = clReleaseMemObject(workerCountBuffer);
		status = clReleaseMemObject(gridWidthBuffer);
		status = clReleaseMemObject(gridHeightBuffer);

		// Swap status's
		for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT * 2; i += 2) {
			grid[i] = grid[i + 1];
		}

		for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT * 2; i += 2) {
			if (grid[i] == 1) {
				live_node.setPosition(sf::Vector2f(((i / 2) % GRID_WIDTH), (i / 2) / GRID_WIDTH));
				window.draw(live_node);
			}
		}

		frame_count++;
		window.display();

	}




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