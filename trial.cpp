#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "read_elf.hpp"

/** Takes filename, offset of .text segment and size of .text segment*/
int main(int argc, char* argv[]) {
	if (argc != 7) {
		printf("Wrong number of arguments.\n");
		return 0;
	}
	
        size_t code_size = atoi(argv[2]) + atoi(argv[3]);
	void *buf;
	buf = read_object_file(argv[1], code_size);
	printf("Buffer at %p\n", buf);
	for (int i = atoi(argv[2]); i < code_size; i++) {
		printf("%x ", ((unsigned char*)buf)[i]);
	}
        int rc = mprotect(buf, code_size, PROT_EXEC|PROT_READ|PROT_WRITE);
	if (rc != 0) {
		printf("Failed to set code buffer executable.\n");
		return 0;
	}
	
	update_relocation_record(argv[5], argv[6], buf + atoi(argv[2]));
	for (int i = atoi(argv[2]); i < code_size; i++) {
		printf("%x ", ((unsigned char*)buf)[i]);
	}

	int register rax asm("rax") = (u_int64_t)(buf + atoi(argv[2]) + atoi(argv[4]));
	int register esi asm("esi") = 3;
	int register edi asm("edi") = 20;
	asm("call *%rax");
	int i;
	asm("movl %%eax,%0":"=r"(i));
	printf("The result is %d\n", i);
	free(buf);
	return 0;
}
