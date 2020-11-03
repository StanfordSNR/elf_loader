#include <iostream>
#include <map>
#include "elfloader.hpp"
#include "wasm-rt/wasm-rt.h"

#define DEBUG 0
#if DEBUG
#define DEBUGPRINT printf
#else
#define DEBUGPRINT(...) do {} while (0)
#endif

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


Object *read_file(char *filename, map<string, func> *func_map){
	DEBUGPRINT("Loading object file %s\n", filename);
	Object *obj = new Object();
	obj->reloctb_size = 0;

	//Initialize size recorder for .bss and *COM*
	uint64_t bss_size = 0;
	uint64_t com_size = 0;

	int text_idx;
	int bss_idx;

	//list of *COM* symbol waiting for an updated offset
	vector<Elf64_Sym *> com_symtb_entry;

	// Step 0: Read Elf header
	Elf64_Ehdr header;
	FILE *file;
	file = fopen(filename,"rb");
	fread(&header, sizeof(header), 1, file);

	// Step 1: Read program headers (should be empty for .o files)
	fseek(file, header.e_phoff, SEEK_SET);
	int pheader_size = header.e_phnum * header.e_phentsize;
	Elf64_Phdr *pheader = (Elf64_Phdr *)malloc(pheader_size);
	fread(pheader, pheader_size, 1, file);

	for (int i = 0; i < header.e_phnum; i++) {
		DEBUGPRINT("P[%d] Type:%d Starts:0x%lx Size:0x%lx", i, pheader[i].p_type, pheader[i].p_offset, pheader[i].p_filesz);
	}	

	// Step 2: Read section headers
	fseek(file, header.e_shoff, SEEK_SET);
	int sheader_size = header.e_shnum * header.e_shentsize;
	obj->sheader = (Elf64_Shdr *)malloc(sheader_size);
	fread(obj->sheader, sheader_size, 1, file);
	
	// Step 2.1: Read section header string table
	fseek(file, obj->sheader[header.e_shstrndx].sh_offset, SEEK_SET);
        obj->namestrs = (char *)malloc(obj->sheader[header.e_shstrndx].sh_size);
	fread(obj->namestrs, obj->sheader[header.e_shstrndx].sh_size, 1, file);
	
	for (int i = 0; i < header.e_shnum; i++) {
		DEBUGPRINT("S[%d] sh_offset:%lx sh_name:%s sh_addralign:%lx\n", i, obj->sheader[i].sh_offset, obj->namestrs+obj->sheader[i].sh_name, obj->sheader[i].sh_addralign);
		
		// Allocate code block for .text section
		if(string(obj->namestrs+obj->sheader[i].sh_name) == ".text") {
			text_idx = i;
			fseek(file, obj->sheader[i].sh_offset, SEEK_SET);
			
			int rc = posix_memalign(&(obj->code), getpagesize(), obj->sheader[i].sh_size);
			if (rc != 0) {
				fprintf(stderr, "%s", "Failed to allocate page-aligned buffer.\n");
			}
			DEBUGPRINT("Assigned code block %p\n", obj->code);

			rc = mprotect(obj->code, obj->sheader[i].sh_size, PROT_EXEC|PROT_READ|PROT_WRITE);
			if (rc != 0) {
				fprintf(stderr, "Failed to set code buffer executable.\n");
			}
			
			fread(obj->code, obj->sheader[i].sh_size, 1, file);
		}

		// Record size of .bss section
		if(string(obj->namestrs+obj->sheader[i].sh_name) == ".bss") {
			fseek(file, obj->sheader[i].sh_offset, SEEK_SET);
			bss_size = obj->sheader[i].sh_size;
			bss_idx = i;
		}

		// Process symbol table
		if(obj->sheader[i].sh_type == SHT_SYMTAB) {
			// Load symbol table
			fseek(file, obj->sheader[i].sh_offset, SEEK_SET);
			obj->symtb = (Elf64_Sym *)malloc(obj->sheader[i].sh_size);
			fread(obj->symtb, obj->sheader[i].sh_size, 1, file);
			
			// Load symbol table string
			int symbstrs_idx = obj->sheader[i].sh_link;
			obj->symstrs = (char *)malloc(obj->sheader[symbstrs_idx].sh_size);
			fseek(file, obj->sheader[symbstrs_idx].sh_offset, SEEK_SET);
			fread(obj->symstrs, obj->sheader[symbstrs_idx].sh_size, 1, file);
			
			
			for (int j = 0; j < obj->sheader[i].sh_size/sizeof(Elf64_Sym); j++) {
				if (ELF64_ST_TYPE(obj->symtb[j].st_info) == STT_SECTION) {
					DEBUGPRINT("Idx:%d value:%lx size:%lx st_shndx:%x\n", j, obj->symtb[j].st_value, obj->symtb[j].st_size, obj->symtb[j].st_shndx);
				}	
				if (obj->symtb[j].st_name != 0) {
					DEBUGPRINT("Idx:%d Name:%s value:%lx size:%lx st_shndx:%x\n", j, obj->symstrs+ obj->symtb[j].st_name, obj->symtb[j].st_value, obj->symtb[j].st_size, obj->symtb[j].st_shndx);
					// Funtion/Variable not defined
					if (obj->symtb[j].st_shndx == SHN_UNDEF) {
						DEBUGPRINT("Function not defined\n");
						continue;
					} 

					func entry;
				 	string name = string(obj->symstrs + obj->symtb[j].st_name);
					entry.obj = obj;
					entry.idx = j;

					// A function
					if (obj->symtb[j].st_shndx == text_idx) {
						entry.type = TEXT;
						DEBUGPRINT("Added function:%s at idx%ld\n", name.c_str(), entry.idx);
					}
					// An .bss variable
					else if (obj->symtb[j].st_shndx == bss_idx) {
						entry.type = BSS;
						DEBUGPRINT("Added bss variable:%s at idx%ld\n", name.c_str(), entry.idx);
					}
					// An COM variable
					else if (obj->symtb[j].st_shndx == SHN_COMMON) {
						entry.type = COM;
						DEBUGPRINT("Added *COM* variable:%s at idx%ld\n", name.c_str(), entry.idx);
						com_size += obj->symtb[j].st_size;
						com_symtb_entry.push_back(&(obj->symtb[j]));
					}
					(*func_map)[name] = entry;

				}
			}
		}

		// Load relocation table
		if(obj->sheader[i].sh_type == SHT_RELA || obj->sheader[i].sh_type == SHT_REL) {
			if(string(obj->namestrs+obj->sheader[i].sh_name) == ".rela.text") {
				obj->reloctb = (Elf64_Rela *)malloc(obj->sheader[i].sh_size);
				fseek(file, obj->sheader[i].sh_offset, SEEK_SET);
				fread(obj->reloctb, obj->sheader[i].sh_size, 1, file);
				obj->reloctb_size = obj->sheader[i].sh_size/sizeof(Elf64_Rela);
				//for (int j = 0; j < sheader[i].sh_size/sizeof(Elf64_Rela); j++) {
				//	printf("offset:%lx info:%lx addend:%ld\n", relatab[j].r_offset, relatab[j].r_info, relatab[j].r_addend);
				//	printf("Type:%lx Index:%lx\n", ELF64_R_TYPE(relatab[j].r_info), ELF64_R_SYM(relatab[j].r_info));
				//}
			}
		}

		if(obj->sheader[i].sh_type == SHT_DYNAMIC) {
			DEBUGPRINT("This is a dynamic linking table.\n");
		}
	}
	
	// Step 3.0: Assign .bss section
	obj->bss = calloc(bss_size + com_size, 1);
	
	// Step 3.1: Update symbol table entry for *COM*
	uint64_t com_base = bss_size;
	for (auto &com_sym : com_symtb_entry) {
		com_sym->st_value = com_base;
		com_base += com_sym->st_size;
	}		

	free(pheader);
	fclose(file);
	return obj;
}

Program load_programs(int argc, char* argv[]){
	vector<Object *> objects;
	map<string, func> func_map;	
	
	// Step 0: Read in object files
	for (int i = 1; i < argc; i++) {
		Object *obj = read_file(argv[i], &func_map);
		objects.push_back(obj);
	}
	
	// Step 1: Add wasm-rt functions to func_map	
	func_map["wasm_rt_trap"] = func((uint64_t)wasm_rt_trap);		
	func_map["wasm_rt_register_func_type"] = func((uint64_t)wasm_rt_register_func_type);
	func_map["wasm_rt_allocate_memory"] = func((uint64_t)wasm_rt_allocate_memory);
	func_map["wasm_rt_grow_memory"] = func((uint64_t)wasm_rt_grow_memory);
	func_map["wasm_rt_allocate_table"] = func((uint64_t)wasm_rt_allocate_table);
	func_map["wasm_rt_call_stack_depth"] = func((uint64_t)&wasm_rt_call_stack_depth);

	for (auto &obj : objects) {
		for (int i = 0; i < obj->reloctb_size; i++) {
			DEBUGPRINT("offset:%lx index:%lx type:%lx addend:%ld\n", obj->reloctb[i].r_offset, ELF64_R_SYM(obj->reloctb[i].r_info), ELF64_R_TYPE(obj->reloctb[i].r_info), obj->reloctb[i].r_addend);
			int idx = ELF64_R_SYM(obj->reloctb[i].r_info);
			int32_t rel_offset = obj->reloctb[i].r_addend - (int64_t)(obj->code) - obj->reloctb[i].r_offset;
			// Check whether is a section
			if (ELF64_ST_TYPE(obj->symtb[idx].st_info) == STT_SECTION) {
				string sec_name = string(obj->namestrs+obj->sheader[obj->symtb[idx].st_shndx].sh_name);
				if (sec_name == ".text") {
					rel_offset += (int64_t)(obj->code);
					DEBUGPRINT("Reloced .text section\n");
				} else if (sec_name == ".bss") {
					rel_offset += (int64_t)(obj->bss);
					DEBUGPRINT("Reloced .bss section\n");
				} else if (sec_name == ".data") {
					DEBUGPRINT("No .data section\n");
				}
			} else {		
				string name = string(obj->symstrs + obj->symtb[idx].st_name);
				DEBUGPRINT("Name is %s\n", name.c_str());
				func dest = func_map[name];
				switch (dest.type) {
					case LIB:
						rel_offset += (int64_t)(dest.lib_addr);
						break;
					case TEXT:
						rel_offset += (int64_t)(dest.obj->code) + dest.obj->symtb[dest.idx].st_value;
						break;	     
					case BSS:
					case COM:
						rel_offset += (int64_t)(dest.obj->bss) + dest.obj->symtb[dest.idx].st_value;
						break;
				}
			}	
			*((int32_t *)(obj->code + obj->reloctb[i].r_offset)) = rel_offset; 
			DEBUGPRINT("Value is %d\n", rel_offset);
		}
	}	
	
	string name = "main";
	func dest_func = func_map[name];
	DEBUGPRINT("Offset is %lx size is %lx\n", dest_func.obj->symtb[dest_func.idx].st_value, dest_func.obj->symtb[dest_func.idx].st_size);
        DEBUGPRINT("code block at %p\n", dest_func.obj->code);
	void * entry_point = dest_func.obj->code + dest_func.obj->symtb[dest_func.idx].st_value;	
	return Program(objects, entry_point);	
}

