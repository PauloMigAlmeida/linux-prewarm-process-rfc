#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define MAX_PROG_SIZE	1024
#define	NORMAL_MODE	0
#define	PREWARM_MODE	1

static int run_mode;
static char progs_path[255];

/* 
 * this is where purposely-crafted programs will be loaded to.
 * the idea is to avoid malloc'ing that area as brk syscall's
 * latency can vary depeding on system's memory fragmentation
 * state.
 */
static char mem[4096];
static int prog_count;

void read_program(char* dest, size_t len, char *path){
	FILE *fp;
	printf("loading program %s\n", path);
	
	fp = fopen(path, "r");
	fread(dest, len, 1, fp);
        fclose(fp);
}

void prewarm_programs(void){
	DIR *d;
	struct dirent *dir;
	char path[512];

	if ((d = opendir(progs_path))) {
		while ((dir = readdir(d)) != NULL) {
			if(strcmp(dir->d_name, ".") && strcmp(dir->d_name, "..")){
				printf("found %s/%s\n", progs_path, dir->d_name);
	
				memset(path, 0, sizeof(path));
				snprintf(path, sizeof(path), "%s/%s", progs_path, dir->d_name);

				read_program(mem + (prog_count * MAX_PROG_SIZE),
					     MAX_PROG_SIZE, path);
				prog_count++;
			}
		}
		closedir(d);
	}	
}


void do_run(char* src) {
	printf("do_run was called\n");
}

void run_program(int prog_num) {
	if (run_mode == PREWARM_MODE) {
		do_run(mem + ((prog_num - 1) * MAX_PROG_SIZE));
	} else {
		char path[512];
		memset(path, 0, sizeof(path));
		snprintf(path, sizeof(path), "%s/program%d.o", progs_path, prog_num);
		read_program(mem, MAX_PROG_SIZE, path);
		do_run(mem);
	}
}

void run_mocked_script(void) {
	/* 
	 * in theory we should run the equivalent 
	 * of the following bash script:
	 *
	 * ./program1
	 * ...
	 *
	 * the only difference will be whether we will
	 * load the executables when we need them (which 
	 * is how bash currently does it which is undoubtedly
	 * more memory efficient) or load them at the beginning
	 * and then just execute it from memory (which will
	 * require more memory available to accomodate all
	 * instances of the executables but it should be faster
	 * to execute the whole script as we don't waste CPU cycles 
	 * waiting for the program to be loaded into memory)
	 */

	if (run_mode == PREWARM_MODE)
		prewarm_programs();

	run_program(1);

}

int main(int argc, char **argv) {
	if (argc != 3)
		goto err;
	
	run_mode = atoi(argv[1]);
	if (run_mode != NORMAL_MODE && run_mode != PREWARM_MODE)
			goto err;

	memcpy(progs_path, argv[2], strlen(argv[2]));

	run_mocked_script();		
	return 0;

err:
	printf("error while parsing arguments ... exiting\n"
	       "usage: ./main <run_mode> <progs_path>\n");
}

