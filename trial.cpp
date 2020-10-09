#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "elfloader.hpp"

/** Takes filename, offset of .text segment and size of .text segment*/
int main(int argc, char* argv[]) {
	std::pair<vector<Program>, void*> res = load_programs(argc, argv); 	
        printf("Completed loading programs\n");	

	int register rax asm("rax") = (u_int64_t)res.second;
	int register esi asm("esi") = 3;
	int register edi asm("edi") = 20;
	asm("call *%rax");
	int i;
	asm("movl %%eax,%0":"=r"(i));
	printf("The result is %d\n", i);
	
	for (auto &prog : res.first) {
		free(prog.code);
		free(prog.symstrs);
		free(prog.symtb);
		if (prog.reloctb_size != 0) {
			free(prog.reloctb);
		}
		free(prog.sheader);
	}
	
	return 0;
}
