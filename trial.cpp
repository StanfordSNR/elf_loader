#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "elfloader.hpp"
#include "wasm-rt/wasm-rt.h"

/** Takes filename, offset of .text segment and size of .text segment*/
int main(int argc, char* argv[]) {
	Program prog = load_programs(argc, argv); 
	prog.finish_loading_cleanup();	
        printf("Completed loading programs\n");	

	int register rax asm("rax") = (u_int64_t)prog.entry_point;
	int register esi asm("esi") = 13;
	int register edi asm("edi") = 20;
	asm("call *%rax");
	int i;
	asm("movl %%eax,%0":"=r"(i));
	printf("The result is %d\n", i);
	
        prog.finish_program_cleanup();	
	return 0;
}
