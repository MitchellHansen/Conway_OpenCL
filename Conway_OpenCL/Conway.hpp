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
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"

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

// convert the kernel file into a string
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

int main(int argc, char* argv[])
{
	int WINDOW_X = 1000;
	int WINDOW_Y = 1000;
	int GRID_WIDTH = WINDOW_X;
	int GRID_HEIGHT = WINDOW_Y;
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
	const char *compute_kernel_filename = "conway_compute.cl";
	const char *align_kernel_filename = "conway_align.cl";

	std::string compute_kernel_string;
	std::string align_kernel_string;

	convertToString(compute_kernel_filename, compute_kernel_string);
	convertToString(compute_kernel_filename, align_kernel_string);

	// Create a program with the source
	const char *compute_source = compute_kernel_string.c_str();
	const char *align_source = align_kernel_string.c_str();

	size_t compute_source_size[] = {strlen(compute_source)};
	size_t align_source_size[] = { strlen(align_source) };

	cl_program compute_program = clCreateProgramWithSource(context, 1, &compute_source, compute_source_size, NULL);
	cl_program align_program = clCreateProgramWithSource(context, 1, &align_source, align_source_size, NULL);

	// Build the compute program
	status = clBuildProgram(compute_program, 1, devices, NULL, NULL, NULL);

	if (status == CL_BUILD_PROGRAM_FAILURE) {

		size_t log_size;
		clGetProgramBuildInfo(compute_program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char *log = new char[log_size];

		clGetProgramBuildInfo(compute_program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		std::cout << log << std::endl;
	}

	// Build the align program
	status = clBuildProgram(align_program, 1, devices, NULL, NULL, NULL);
	
	if (status == CL_BUILD_PROGRAM_FAILURE) {

		size_t log_size;
		clGetProgramBuildInfo(align_program, devices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		char *log = new char[log_size];

		clGetProgramBuildInfo(align_program, devices[0], CL_PROGRAM_BUILD_LOG, log_size, log, NULL);

		std::cout << log << std::endl;
	}

	// Now create the kernels
	cl_kernel front_kernel = clCreateKernel(compute_program, "conway_compute", NULL);
	cl_kernel back_kernel = clCreateKernel(align_program, "conway_align", NULL);

	// ======================================= Setup OpenGL =======================================================
	
	glfwInit();
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* gl_window = glfwCreateWindow(GRID_WIDTH, GRID_HEIGHT, "GPU accelerated life", nullptr, nullptr);
	glfwMakeContextCurrent(gl_window);

	glfwSetKeyCallback(gl_window, key_callback);
	
	glewExperimental = GL_TRUE;
	glewInit();

	glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	Shader ourShader("Z:\\VS_Projects\\Conway_OpenCL\\Conway_OpenCL\\vertex_shader.sh", "Z:\\VS_Projects\\Conway_OpenCL\\Conway_OpenCL\\fragment_shader.sh");

	GLfloat vertices[] = {
		// Positions          // Colors           // Texture Coords
		1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // Top Right
		1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // Bottom Right
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // Bottom Left
		-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // Top Left 
	};
	GLuint indices[] = {
		0, 1, 3, // First Triangle
		1, 2, 3  // Second Triangle
	};

	GLuint VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// TexCoord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); // Unbind VAO


	// ======================================= Setup grid =========================================================

	// Setup the rng
	std::mt19937 rng(time(NULL));
	std::uniform_int_distribution<int> rgen(0, 4); // 25% chance

	// Init the grids
	unsigned char* front_grid = new unsigned char[GRID_WIDTH * GRID_HEIGHT];

	for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
		if (rgen(rng) == 1) {
			front_grid[i] = 1;
		}
		else {
			front_grid[i] = 0;
		}
	}

	unsigned char* back_grid = new unsigned char[GRID_WIDTH * GRID_HEIGHT];

	for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++) {
		back_grid[i] = front_grid[i];
	}

	// ====================================== Setup Rendering ==========================================================

	unsigned char* pixel_array = new sf::Uint8[WINDOW_X * WINDOW_Y * 4];

	for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT * 4; i += 4) {

		pixel_array[i] = 29; // R?
		pixel_array[i + 1] = 70; // G?
		pixel_array[i + 2] = 100; // B?
		pixel_array[i + 3] = 200; // A?
	}

	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	//////////////////////////

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_DECAL);

	// when texture area is small, bilinear filter the closest mipmap
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	// when texture area is large, bilinear filter the first mipmap
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// texture should tile
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GRID_WIDTH, GRID_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel_array);

	glGenerateMipmap(GL_TEXTURE_2D);


	delete pixel_array;



	// ========================================= Setup the buffers ==================================================

	int err = 0;

	cl_mem frontBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, GRID_WIDTH * GRID_HEIGHT * sizeof(char), (void*)front_grid, &err);
	cl_mem backBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, GRID_WIDTH * GRID_HEIGHT * sizeof(char), (void*)back_grid, &err);
	//cl_mem pixelBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, GRID_WIDTH * GRID_HEIGHT * sizeof(char), (void*)pixel_array, &err);

	cl_mem workerCountBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &WORKER_SIZE, &err);
	cl_mem gridWidthBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &GRID_WIDTH, &err);
	cl_mem gridHeightBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &GRID_HEIGHT, &err);

	// Kernel args for front kernel
	status = clSetKernelArg(front_kernel, 0, sizeof(cl_mem), (void *)&frontBuffer);
	status = clSetKernelArg(front_kernel, 1, sizeof(cl_mem), (void *)&backBuffer);
	//status = clSetKernelArg(front_kernel, 2, sizeof(cl_mem), (void *)&pixelBuffer);

	status = clSetKernelArg(front_kernel, 3, sizeof(cl_mem), (void *)&workerCountBuffer);
	status = clSetKernelArg(front_kernel, 4, sizeof(cl_mem), (void *)&gridWidthBuffer);
	status = clSetKernelArg(front_kernel, 5, sizeof(cl_mem), (void *)&gridHeightBuffer);

	// Flipped kernel args for the back kernel
	status = clSetKernelArg(back_kernel, 0, sizeof(cl_mem), (void *)&backBuffer); // Flipped
	status = clSetKernelArg(back_kernel, 1, sizeof(cl_mem), (void *)&frontBuffer); // Flipped
	//status = clSetKernelArg(back_kernel, 2, sizeof(cl_mem), (void *)&pixelBuffer);

	status = clSetKernelArg(back_kernel, 3, sizeof(cl_mem), (void *)&workerCountBuffer);
	status = clSetKernelArg(back_kernel, 4, sizeof(cl_mem), (void *)&gridWidthBuffer);
	status = clSetKernelArg(back_kernel, 5, sizeof(cl_mem), (void *)&gridHeightBuffer);


	// ===================================== Loop ==================================================================

	while (!glfwWindowShouldClose(gl_window)) {

		//glfwPollEvents();
		//glClear(GL_COLOR_BUFFER_BIT);

		// ======================================= OpenCL Shtuff ===================================================

		// Update the data in GPU memory
		//status = clEnqueueWriteBuffer(commandQueue, frontBuffer, CL_TRUE, 0, GRID_WIDTH * GRID_HEIGHT * 2 * sizeof(char), (void*)grid, NULL, 0, NULL);

		// Work size, for each y line
		size_t global_work_size[1] = { WORKER_SIZE };


		status = clEnqueueNDRangeKernel(commandQueue, back_kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);
		//status = clEnqueueReadBuffer(commandQueue, pixelBuffer, CL_TRUE, 0, GRID_WIDTH * GRID_HEIGHT * 4 * sizeof(unsigned char), (void*)pixel_array, 0, NULL, NULL);
		

		// ======================================= Rendering Shtuff =================================================

		glfwPollEvents();

		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(ourShader.Program, "ourTexture1"), 0);

		// Draw the triangle
		ourShader.Use();
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// Swap the screen buffers
		glfwSwapBuffers(gl_window);



	}

	glfwTerminate();
	
	// Release the buffers
	status = clReleaseMemObject(frontBuffer);
	status = clReleaseMemObject(backBuffer);
	//status = clReleaseMemObject(pixelBuffer);
	status = clReleaseMemObject(workerCountBuffer);
	status = clReleaseMemObject(gridWidthBuffer);
	status = clReleaseMemObject(gridHeightBuffer);

	// And the program stuff
	status = clReleaseKernel(front_kernel);
	status = clReleaseProgram(compute_program);
	status = clReleaseProgram(align_program);
	status = clReleaseCommandQueue(commandQueue);
	status = clReleaseContext(context);

	if (devices != NULL)
	{
		delete devices;
		devices = NULL;
	}

	return SUCCESS;
}