#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "read_elf.hpp"
using namespace std;

void *read_object_file(char *filename, size_t code_size) {
	void *buffer;
	int rc = posix_memalign(&buffer, getpagesize(), code_size);
	if (rc != 0) {
		fprintf(stderr, "%s", "Failed to allocate page-aligned buffer.\n");
	}

	FILE *ptr;
	ptr = fopen (filename, "rb");

	fread(buffer, code_size, 1, ptr);

	fclose(ptr);
	return buffer;
}

void update_relocation_record(char *symbol_filename, char *relocation_filename, void *code_start) {
	map<string, int> sym;
	
	FILE *symbol_table;
	symbol_table = fopen (symbol_filename, "rb");
	char sline[256];
	while (fgets(sline, sizeof(sline), symbol_table)){
		if(strstr(sline, "F .text")) {
			char *name = strrchr(sline, ' ');
			name = strtok(name,"\n");
			int loc = strtol(strtok(sline, " "), NULL, 16);
			sym.insert(std::make_pair(string(name), loc));
		}	
	}	
	fclose(symbol_table);

	FILE *reloc_records;
	reloc_records = fopen(relocation_filename, "rb");
	char line[256];
	while (fgets(line, sizeof(line), reloc_records)){
		if(strstr(line, "X86")) {
			char *name = strrchr(line,' ');
			name = strtok(name, "-");
			int loc = strtol(strtok(line, " "), NULL, 16);
			cout << name << " " << sym[name] << endl;
			int32_t rel = sym[name] - 4 - loc;
			cout << rel << " " << hex << rel << endl;
		
		        *((int32_t*)(code_start + loc)) = rel;	
		}
	}
	fclose(reloc_records);
	return;
}
