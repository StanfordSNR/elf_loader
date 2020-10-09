#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <elf.h>
#include <vector>
#include <sys/mman.h>

using namespace std;

struct Program {
	void *code;
	char *symstrs;
	Elf64_Sym *symtb;
	int reloctb_size;
	Elf64_Rela *reloctb;
	Elf64_Shdr *sheader;
};

struct func {
	Program prog;
	uint64_t idx;
};

pair<vector<Program>, void*> load_programs(int argc, char *argv[]);


