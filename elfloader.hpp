#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <elf.h>
#include <vector>
#include <sys/mman.h>

// Symbol Type 
#define TEXT 0
#define BSS 1
#define COM 2
#define LIB 3

using namespace std;

// Represents one object file
struct Object {
	// *********Kept in memory until the program is removed*********
	// Code section of the object file
	void *code;
	// BSS section of the object file
	void *bss;
	
	// *********No longer needed after the program is loaded********
	// String signs for symbol table
	char *symstrs;
	// String signs for section header 
	char *namestrs;
	// Symbol table
	Elf64_Sym *symtb;
	// Number of relocation entries in relocation table
	int reloctb_size;
	// Relocation table
	Elf64_Rela *reloctb;
	// Section header
	Elf64_Shdr *sheader;

	void finish_loading_cleanup() {
		free(this->symstrs);
		free(this->namestrs);
		free(this->symtb);
		free(this->sheader);
	}

	void finish_program_cleanup() {
		free(this->code);
		free(this->bss);
	}	
};

// A program
struct Program {
	// List of object fiels in the program
	vector<Object *> objects;
	// Address of the entry point of the program
	void *entry_point;

	Program() {}
	Program (vector<Object *> objects, void *entry_point) : objects(objects),
								entry_point(entry_point) {}
	void finish_loading_cleanup() {
		for (Object *obj : objects) {
			obj->finish_loading_cleanup();
		}
	}

	void finish_program_cleanup() {
		for (Object *obj : objects) {
			obj->finish_program_cleanup();
		}
	}
};

// A function in a program
struct func {
	// The object file the function belongs to
	Object *obj;
	// Idx of the function in the program's symbol table
	uint64_t idx;
	// Type of the function
        int type;
	// Address of the function if it's a library function
	uint64_t lib_addr;

	func() {}
	func (uint64_t lib_addr) : lib_addr(lib_addr),
				   type(LIB) {}
};

Program load_programs(int argc, char *argv[]);


