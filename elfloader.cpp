#include <iostream>
#include <map>
#include "elfloader.hpp"

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


Program read_file(char *filename, map<string, func> *func_map){
	printf("Loading object file %s\n", filename);
	Program prog;
	prog.reloctb_size = 0;

	Elf64_Ehdr header;
	FILE *file;
	file = fopen(filename,"rb");
	fread(&header, sizeof(header), 1, file);

	// Read program headers (should be empty for .o files)
	fseek(file, header.e_phoff, SEEK_SET);
	int pheader_size = header.e_phnum * header.e_phentsize;
	Elf64_Phdr *pheader = (Elf64_Phdr *)malloc(pheader_size);
	fread(pheader, pheader_size, 1, file);

	for (int i = 0; i < header.e_phnum; i++) {
		printf("P[%d] Type:%d Starts:0x%lx Size:0x%lx", i, pheader[i].p_type, pheader[i].p_offset, pheader[i].p_filesz);
	}	

	// Read section headers
	fseek(file, header.e_shoff, SEEK_SET);
	int sheader_size = header.e_shnum * header.e_shentsize;
	prog.sheader = (Elf64_Shdr *)malloc(sheader_size);
	fread(prog.sheader, sheader_size, 1, file);
	
	// Read section header string table
	fseek(file, prog.sheader[header.e_shstrndx].sh_offset, SEEK_SET);
        char *namestrs = (char *)malloc(prog.sheader[header.e_shstrndx].sh_size);
	fread(namestrs, prog.sheader[header.e_shstrndx].sh_size, 1, file);
	
	for (int i = 0; i < header.e_shnum; i++) {
		printf("S[%d] sh_offset:%lx sh_name:%s\n", i, prog.sheader[i].sh_offset, namestrs+prog.sheader[i].sh_name);
		if(string(namestrs+prog.sheader[i].sh_name) == ".text") {
			fseek(file, prog.sheader[i].sh_offset, SEEK_SET);
			
			int rc = posix_memalign(&prog.code, getpagesize(), prog.sheader[i].sh_size);
			if (rc != 0) {
				fprintf(stderr, "%s", "Failed to allocate page-aligned buffer.\n");
			}
			printf("Assigned code block %p\n", prog.code);

			rc = mprotect(prog.code, prog.sheader[i].sh_size, PROT_EXEC|PROT_READ|PROT_WRITE);
			if (rc != 0) {
				fprintf(stderr, "Failed to set code buffer executable.\n");
			}
			
			fread(prog.code, prog.sheader[i].sh_size, 1, file);
		}

		if(prog.sheader[i].sh_type == SHT_SYMTAB) {
			fseek(file, prog.sheader[i].sh_offset, SEEK_SET);
			prog.symtb = (Elf64_Sym *)malloc(prog.sheader[i].sh_size);
			fread(prog.symtb, prog.sheader[i].sh_size, 1, file);
			
			int symbstrs_idx = prog.sheader[i].sh_link;
			prog.symstrs = (char *)malloc(prog.sheader[symbstrs_idx].sh_size);
			fseek(file, prog.sheader[symbstrs_idx].sh_offset, SEEK_SET);
			fread(prog.symstrs, prog.sheader[symbstrs_idx].sh_size, 1, file);
			
			for (int j = 0; j < prog.sheader[i].sh_size/sizeof(Elf64_Sym); j++) {
				printf("Name:%s value:%lx size:%lx\n", prog.symstrs+ prog.symtb[j].st_name, prog.symtb[j].st_value, prog.symtb[j].st_size);
				if (prog.symtb[j].st_name != 0 && prog.symtb[j].st_size != 0) {
					if (ELF64_ST_TYPE(prog.symtb[j].st_info) != STT_NOTYPE) {
						func function;
						string name = string(prog.symstrs + prog.symtb[j].st_name);
						function.prog = prog;
						function.idx = j;
						printf("Added function:%s at idx%ld\n", name.c_str(), function.idx);
						(*func_map)[name] = function;
					}
			
				}
			}
		}

		if(prog.sheader[i].sh_type == SHT_RELA || prog.sheader[i].sh_type == SHT_REL) {
			if(string(namestrs+prog.sheader[i].sh_name) == ".rela.text") {
				prog.reloctb = (Elf64_Rela *)malloc(prog.sheader[i].sh_size);
				fseek(file, prog.sheader[i].sh_offset, SEEK_SET);
				fread(prog.reloctb, prog.sheader[i].sh_size, 1, file);
				prog.reloctb_size = prog.sheader[i].sh_size/sizeof(Elf64_Rela);
				//for (int j = 0; j < sheader[i].sh_size/sizeof(Elf64_Rela); j++) {
				//	printf("offset:%lx info:%lx addend:%ld\n", relatab[j].r_offset, relatab[j].r_info, relatab[j].r_addend);
				//	printf("Type:%lx Index:%lx\n", ELF64_R_TYPE(relatab[j].r_info), ELF64_R_SYM(relatab[j].r_info));
				//}
			}
		}

		if(prog.sheader[i].sh_type == SHT_DYNAMIC) {
			printf("This is a dynamic linking table.\n");
		}
	}
	
	free(namestrs);	
	free(pheader);
	fclose(file);
	return prog;
}

pair<vector<Program>, void *> load_programs(int argc, char* argv[]){
	vector<Program> res;
	map<string, func> func_map;	

	for (int i = 1; i < argc; i++) {
		Program prog = read_file(argv[i], &func_map);
		res.push_back(prog);
	}
	
	for (auto &prog : res) {
		for (int i = 0; i < prog.reloctb_size; i++) {
			printf("offset:%lx index:%lx addend:%ld\n", prog.reloctb[i].r_offset, ELF64_R_SYM(prog.reloctb[i].r_info), prog.reloctb[i].r_addend);
			//First find local code block
			int idx = ELF64_R_SYM(prog.reloctb[i].r_info);
			if(ELF64_ST_TYPE(prog.symtb[idx].st_info) != STT_NOTYPE) {
				*((int32_t *)(prog.code + prog.reloctb[i].r_offset)) = prog.symtb[idx].st_value + prog.reloctb[i].r_addend - prog.reloctb[i].r_offset;
				printf("Value is %ld\n", prog.symtb[idx].st_value + prog.reloctb[i].r_addend - prog.reloctb[i].r_offset);
				for (int k =0 ; k < 76; k++) {
					printf("%x ", ((unsigned char*)prog.code)[k]);	
				}
			} else {
				string name = string(prog.symstrs + prog.symtb[idx].st_name);
				printf("Name is %s\n", name.c_str());
				func dest_func = func_map[name];
				printf("Offset is %lx size is %lx\n", dest_func.prog.symtb[dest_func.idx].st_value, dest_func.prog.symtb[dest_func.idx].st_size);
				*((int32_t *)(prog.code + prog.reloctb[i].r_offset)) = (int64_t)(dest_func.prog.code) - (int64_t)(prog.code) + dest_func.prog.symtb[dest_func.idx].st_value + prog.reloctb[i].r_addend - prog.reloctb[i].r_offset;
			}
		}
	}	
	
	string name = "main";
	func dest_func = func_map[name];
	printf("Offset is %lx size is %lx\n", dest_func.prog.symtb[dest_func.idx].st_value, dest_func.prog.symtb[dest_func.idx].st_size);
        printf("code block at %p\n", dest_func.prog.code);
	void * entry_point = dest_func.prog.code + dest_func.prog.symtb[dest_func.idx].st_value;	
	return std::make_pair(res, entry_point);	
}

