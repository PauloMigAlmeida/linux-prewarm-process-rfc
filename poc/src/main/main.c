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
#define	ONDEMAND_MODE	1

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
static bool loading_prog_status[MAX_PROG_IN_MEM];

void do_read_program(char* dest, size_t len, char *path){
        FILE *fp;
        printf("loading program %s\n", path);

        fp = fopen(path, "r");
        fread(dest, len, 1, fp);
        fclose(fp);
}

void read_program(void* data) {
        char path[512];
	int prog_id = ((uintptr_t)data - (uintptr_t)mem) / PROG_SIZE;

        memset(path, 0, sizeof(path));
        snprintf(path, sizeof(path), "%s/program%d.o", progs_path, 1);

        do_read_program(data, PROG_SIZE, path);
	loading_prog_status[prog_id] = true;
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
	int prog_id = prog_num - 1;
	char* dst = mem + (prog_id * PROG_SIZE);

	/* reads program in ondemand_mode only if needed 
	 * or always if in normal_mode */
        if (run_mode == NORMAL_MODE || !loading_prog_status[prog_id])	
		read_program(dst);

	do_run(mem);
}

/**
 * simulates bash execution environment with both
 * sync mode (current behaviour) and on-demand mode
 * (the one we are trying to implement)
 * @param count - Number of programs to be executed
 */
void run_mocked_script(size_t count) {
        /*
         * in theory, we should run the equivalent
         * of the following bash script:
         *
	 * for i in ${1..$count}
	 * do
         *	./program1
         * done 
         * ...
         *  >
         */

        for (size_t i = 1; i <= count; i++){
                run_program(1);
        }
}

int main(int argc, char **argv) {
        if (argc != 4)
                goto arg_err;

        run_mode = atoi(argv[1]);
        if (run_mode != NORMAL_MODE && run_mode != ONDEMAND_MODE)
                goto arg_err;

        desired_prog_count = atoi(argv[2]);
        if (desired_prog_count * PROG_SIZE > sizeof(mem))
                goto alloc_err;

        memcpy(progs_path, argv[3], strlen(argv[3]));

        run_mocked_script(desired_prog_count);
        return 0;

arg_err:
        printf("error while parsing arguments ... exiting\n"
               "usage: ./main <run_mode> <desired_num_progs>"
               " <progs_path>\n");
        return -1;
alloc_err:
        printf("there is no space in the XBSS ELF section"
               "to accomodate that many executable. Since this is"
               "allocated a compilation phase, tweak the constant"
               "MAX_PROG_IN_MEM and recompile it. exiting\n");
        return -1;
}
