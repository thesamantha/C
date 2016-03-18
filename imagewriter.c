#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "bitmap.h"
#include "color.h"
#include "raytrace.h"
#include "util.h"

																			/** Raytracer: Image Writer **/
/** Given image data and a file destination, writes the image pixel-by-pixel as a .bmp to the file destination.  **/
/** Displays comparative speeds of writing through simple, loop, and parallel processes.  **/
/** Samantha Tite-Webber, 2015.  **/


void init_raytracer () {
	// init raytracer
	vec_t bounds[4];
	scene_t *scene = create_scene();
	calculate_casting_bounds(scene->cam, bounds);
}


/*******SIMPLE WRITE: write image from top to bottom**********/
int raytracer_simple(const char* filename){

	printf("%s      :  ", filename);
	unsigned long start, end;

	// init time measurement
	start = current_time_millis();

	// init raytracer
	init_raytracer();

	// Allocate buffer for picture data
	pix_t *img = (pix_t*) calloc(HEIGHT * WIDTH, sizeof(pix_t));
	if (img){

		// calculate the data for the image (do the actual raytrace)
		raytrace(img, bounds, scene, 0, 0, WIDTH, HEIGHT);

		delete_scene(scene);

		// open file
		FILE *file = fopen(filename, "wb");
		if (file) {
			// write the header
			write_bitmap_header(file, WIDTH, HEIGHT);

			// write image to file on disk
			fwrite(img, 3, WIDTH * HEIGHT, file);

			// free buffer
			free(img);
			
			// close file
			fclose(file);
			
			// print the measured time
			end = current_time_millis();
			printf("Render time: %.3fs\n", (double) (end - start) / 1000);
			
			return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}

/***********LOOP WRITE: Split image into segments and iterate through segments in a loop**********/
int raytracer_loop(const char* filename, int processcount){

	printf("%s (%i)    :  ", filename, processcount);
	unsigned long start, end;
	int success = EXIT_FAILURE;

	// init time measurement
	start = current_time_millis();

	// init raytracer
	init_raytracer();

	//split image calculation into segments - keep one set of offsets constant, as in, we always draw from top to bottom,
	//and the other set of offsets changes depending on the step in the for-loop. we can write to the image within this same loop,
	//keeping in mind that the header will only be written once.

	//allocate buffer for picture data. we want enough space for the entire picture, so we give the whole lengths of width and height
	pix_t *img = (pix_t*) calloc(HEIGHT * WIDTH, sizeof(pix_t));

	//open file for writing picture data
	FILE *file = fopen(filename, "wb");

	// write the header
	write_bitmap_header(file, WIDTH, HEIGHT);

	//we write in sections across the photo, always from left to right with height of HEIGHT/process count. Thus we pass a constant height,
	//while the y offset is always the endpoint of the last trace, starting with 0 during the first trace.
	for(int i = 0; i < processcount; i++) {

		int y = HEIGHT/processcount;

		if(i == processcount - 1) {
			
			y+= HEIGHT%processcount;
		}
	
		raytrace(img, bounds, scene, 0, i*y, WIDTH, (HEIGHT/processcount));

		if(img && file) {
		
			success = EXIT_SUCCESS;
			fwrite(img, 3, WIDTH * y, file);	//always write the same amount of space; "img" will determine
										//what section of the photo we are writing in that space
		}
	}

	delete_scene(scene);

	// free buffer
	free(img);
			
	// close file
	fclose(file);

	// print the measured time
	end = current_time_millis();
	printf("Render time: %.3fs\n", (double) (end - start) / 1000);
	
	return success;
}

/*******PARALLEL WRITE: Split image into segments, and spawn one process for the writing of each portion********/
int raytracer_parallel(const char* filename, int processcount) {

	printf("%s (%i):  ", filename, processcount);
	unsigned long start, end;

	int status = 0;

	// init time measurement
	start = current_time_millis();

	// init raytracer
	init_raytracer();

	FILE *file = fopen(filename, "wb");

	if(file) {
	
		// write the header
		write_bitmap_header(file, WIDTH, HEIGHT);

		unsigned char pixels [3] = {255, 255, 0};

		for(int i = 0; i < processcount; i++) {
			fwrite(pixels, sizeof(pix_t), WIDTH * HEIGHT, file);
		}
	}

	fclose(file);

	//spawn one process for each portion of the photo, and handle file management in this process
	for(int i = 0; i < processcount; i++) {

		pid_t childPID = fork();

		//check for error in spawning
		if(childPID < 0) {
			perror("Error spawning new process. Exiting.\n");
			exit(-1);
		}

		//when inside a child process
		if(childPID == 0) {

			//allocate space in memory for this process' portion of the photo
			pix_t *img = (pix_t*) calloc((HEIGHT/processcount) * WIDTH, sizeof(pix_t));

			//open file for writing picture data
			FILE *file = fopen(filename, "rb+");

			if(file && img) {

				//write image data to portion in memory
				raytrace(img, bounds, scene, 0, i*(HEIGHT/processcount), WIDTH, (HEIGHT/processcount));

				//write data in memory to file
		//		fseek(file, BITMAP_HEADER_SIZE+((HEIGHT/processcount)*i), SEEK_SET);	//first determine place to write in file
	//			fwrite(img, 3, WIDTH * (HEIGHT/processcount), file);	//then write from this point to the point reached by offset

				//clean up...
				free(img);	// free buffer
				fclose(file);
			}

			else {
				printf("Error opening file or allocating memory in portion %d.\n", i);
			}
			
			exit(0);	//finished with duties for this process; leave
		}
	}

	//wait for all children to finish, then continue with the parent process
	while(wait(&status) > 0);

	delete_scene(scene);
	end = current_time_millis();
	printf("Render time: %.3fs\n", (double) (end - start) / 1000);
	
	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Usage: raytracer PROCESSCOUNT\n");
		return EXIT_FAILURE;
	}
	
	unsigned int processcount = strtol(argv[1], NULL, 10);

	if (raytracer_simple("image-simple.bmp") != EXIT_SUCCESS){
		printf("Error or not implemented.\n\n");
	}
	
	if (raytracer_loop("image-loop.bmp", processcount) != EXIT_SUCCESS){
		printf("Error or not implemented.\n\n");
	}

	if (raytracer_parallel("image-parallel.bmp", processcount) != EXIT_SUCCESS){
		printf("Error or not implemented.\n\n");
	}


	return 0;
}
