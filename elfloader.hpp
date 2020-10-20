#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <elf.h>
#include <vector>
#include <sys/mman.h>

#define TEXT 0
#define BSS 1
#define COM 2
#define LIB 3

using namespace std;

struct Program {
	void *code;
	void *bss;
	char *symstrs;
	Elf64_Sym *symtb;
	int reloctb_size;
	Elf64_Rela *reloctb;
	Elf64_Shdr *sheader;
};

struct func {
	Program *prog;
	uint64_t idx;
        int type;
	uint64_t lib_addr;

	func() {}
	func (uint64_t lib_addr) : lib_addr(lib_addr),
				   type(LIB) {}
};

pair<vector<Program *>, void*> load_programs(int argc, char *argv[]);


