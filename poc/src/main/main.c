#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define PROG_SIZE	1024
#define MAX_PROG_IN_MEM 5000
#define	NORMAL_MODE	0
#define	PREWARM_MODE	1

/* CLI arguments */
static int run_mode;
static char progs_path[255];
static unsigned int desired_prog_count;

/*
 * ELF section .bss is by default non-executable, so we have
 * to place executables in a bss-like section with X flag set.
 */
#define _xbss __attribute__((section(".xbss,\"awx\",@progbits#")))

/*
 * this is where purposely-crafted programs will be loaded to.
 * the idea is to avoid malloc'ing that area as brk syscall's
 * latency can vary depeding on system's memory fragmentation
 * state.
 */
static char _xbss mem[MAX_PROG_IN_MEM * PROG_SIZE];
static size_t loaded_prog_count;
static bool loading_prog_status[MAX_PROG_IN_MEM];
pthread_mutex_t lock;

void read_program(int thread_id, char* dest, size_t len, char *path){
        FILE *fp;
        printf("[thread id: %d] :: loading program %s\n", thread_id, path);

        fp = fopen(path, "r");
        fread(dest, len, 1, fp);
        fclose(fp);
}

void* thread_read_program(void* data) {
        char path[512];
	int thread_id = ((uintptr_t)data - (uintptr_t)mem) / PROG_SIZE;

        memset(path, 0, sizeof(path));
        snprintf(path, sizeof(path), "%s/program%d.o", progs_path, 1);
        read_program(thread_id, data, PROG_SIZE, path);

        pthread_mutex_lock(&lock);
        loaded_prog_count++;
	loading_prog_status[thread_id] = true;
        pthread_mutex_unlock(&lock);
        return NULL;
}

void* thread_prewarm_programs(void* data) {
	size_t count = *(size_t*)data;
        pthread_t thread;
        for (size_t i=0; i < count; i++) {
                pthread_create(&thread, 
				NULL, 
				&thread_read_program, 
				mem + (i * PROG_SIZE));
        }
	return NULL;
}

void prewarm_programs(size_t count){
	pthread_t thread;
	pthread_create(&thread,
			NULL,
			&thread_prewarm_programs,
			&count);
}


void do_run(char* src) {
        long rdi, rax;

        rdi = (uint64_t) src;
        asm volatile(
                "call %[fnc] \n\t"
                : "=&a" (rax)
                : [fnc] "D" (rdi)
                : "memory"
        );
}

void run_program(int prog_num) {
        if (run_mode == PREWARM_MODE) {
		int prog_id = prog_num - 1;
		while(!loading_prog_status[prog_id])
			/* we block it */;

                do_run(mem + (prog_id * PROG_SIZE));
        } else {
                char path[512];
                memset(path, 0, sizeof(path));
                snprintf(path, sizeof(path), "%s/program%d.o", progs_path, 1);
                read_program(0, mem, PROG_SIZE, path);
                do_run(mem);
        }
}

/**
 * simulates bash execution environment with both
 * sync mode (current behaviour) and prewarm mode
 * (the one we are trying to implement)
 * @param count - Number of programs to be executed
 */
void run_mocked_script(size_t count) {
        /*
         * in theory, we should run the equivalent
         * of the following bash script:
         *
         * ./program1
         * ...
         * ./program<count>
         */

        if (run_mode == PREWARM_MODE)
                prewarm_programs(count);

        for (size_t i = 1; i <= count; i++){
                run_program(i);
        }
}

int main(int argc, char **argv) {
        if (argc != 4)
                goto arg_err;

        run_mode = atoi(argv[1]);
        if (run_mode != NORMAL_MODE && run_mode != PREWARM_MODE)
                goto arg_err;

        desired_prog_count = atoi(argv[2]);
        if (desired_prog_count * PROG_SIZE > sizeof(mem))
                goto alloc_err;

        memcpy(progs_path, argv[3], strlen(argv[3]));

        if (pthread_mutex_init(&lock, NULL))
                goto mutex_err;

        run_mocked_script(desired_prog_count);
        return 0;

arg_err:
        printf("error while parsing arguments ... exiting\n"
               "usage: ./main <run_mode> <desired_num_progs>"
               " <progs_path>\n");
        return -1;
mutex_err:
        printf("couldn't initialise pthread "
               "mutex lock ... exiting\n");
        return -1;
alloc_err:
        printf("there is no space in the XBSS ELF section"
               "to accomodate that many executable. Since this is"
               "allocated a compilation phase, tweak the constant"
               "MAX_PROG_IN_MEM and recompile it. exiting\n");
        return -1;
}
