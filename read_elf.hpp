#include <map>
using namespace std;

void *read_object_file(char *filename, size_t code_size);

map<char *, int> load_symbol_table(char *filename);

void update_relocation_record(char *symbol_filename, char *relocation_filename, void *code_start); 
