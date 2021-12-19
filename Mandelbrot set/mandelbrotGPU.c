// Prevajanje programa: 
//              module load CUDA
//				gcc mandelbrotGPU.c -O2 -lm -fopenmp -lOpenCL -Wl,-rpath,./ -L./ -l:"libfreeimage.so.3" -o mandelbrotGPU
// Zagon programa: 
//				srun -n1 -G1 --reservation=fri mandelbrotGPU

#include <stdio.h>
#include <stdlib.h>
#include "FreeImage.h"
#include <math.h>
#include <omp.h>
#include <string.h>
#include <CL/cl.h>

#define WORKGROUP_SIZE (16)
#define MAX_SOURCE_SIZE (16384)

char    ch;
int     i, j;
cl_int  ret;
FILE    *fp;
char    *source_str;
size_t  source_size;

double start; 
double end;

int mandelbrotGPU(unsigned char *image, int height, int width) {
    
    // branje datoteke
    fp = fopen("mandelbrot.cl", "r");
    if(!fp) {
        fprintf(stderr, "Napaka pri branju datoteke!\n");
        return 1;
    }

    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    source_str[source_size] = '\0';
    fclose(fp);

    // podatki o platformi
    cl_platform_id  platform_id[10];
    cl_uint         ret_num_platforms;
    char            *buf;
    size_t          buf_len;
    ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);

    // podatki o napravi
    cl_device_id    device_id[10];
    cl_uint         ret_num_devices;
    ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10, device_id, &ret_num_devices);

    // kontekst
    cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);

    // ukazna vrsta
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);

    // delitev dela
    size_t local_item_size[2] = {WORKGROUP_SIZE, WORKGROUP_SIZE};
    size_t num_groups[2] = {((height - 1) / local_item_size[0] + 1), ((width - 1) / local_item_size[1] + 1)};
	size_t global_item_size[2] = {num_groups[0] * local_item_size[0], num_groups[1] * local_item_size[1]};

    // alokacija pomnilnika na napravi
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, height * width * sizeof(unsigned char) * 4, NULL, &ret);

    // priprava programa
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, NULL, &ret);

    // prevajanje
    ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);

    // scepec: priprava objekta
    cl_kernel kernel = clCreateKernel(program, "mandelbrotGPU", &ret);

    // scepec: argumenti
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&height);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&width);

    // start merjenja casa
    start = omp_get_wtime();

    // scepec: zagon
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_item_size, local_item_size, 0, NULL, NULL);
    
    // kopiranje rezultatov
    ret = clEnqueueReadBuffer(command_queue, a_mem_obj, CL_TRUE, 0, height * width * sizeof(float), image, 0, NULL, NULL);

    // stop merjenja casa
    end = omp_get_wtime();

    // ciscenje
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(a_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
}

int main(void) {

    int height = 2160;
    int width = 3840;
    int pitch = ((32 * width + 31) / 32) * 4;

    // prostor za sliko (RGBA)
    unsigned char *image = (unsigned char *)malloc(height * width * sizeof(unsigned char) * 4);

    mandelbrotGPU(image, height, width);

    printf("GPU (%d x %d):      %f s\n", width, height, end - start);

    // shranimo sliko
	FIBITMAP *dst = FreeImage_ConvertFromRawBits(image, width, height, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Save(FIF_PNG, dst, "mandelbrotGPU.png", 0);

	return 0;
}

/*

Dimenzija           CPU             GPU       Pohitritev

640 x 480       0.376525 s      0.001653 s      227 x

800 x 600       0.589776 s      0.002344 s      251 x

1600 x 900      1.760928 s      0.004783 s      368 x

1920 x 1080     2.537601 s      0.007171 s      353 x

3840 x 2160     10.125387 s     0.022218 s      455 x

*/