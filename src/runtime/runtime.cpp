#include <iostream>
#include <fstream>
#include <string>

struct StackEntry {
   uint8_t info[4];
   long value; // Pointer or actual value
};

const long stackSize = 1024 * 32;
const char *bytecode;

const StackEntry *stack;
uint32_t counter;


void readInputFile(const char *filename);
void initStack();
void initConstants();
void initFunctions();
void beginExecution();

void _add();
void _sub();
void _mul();
void _div();

void _grab();
void _return();
void _create();
void _delete();
void _set();

void _equal();
void _smaller();
void _greater();

void _cjmp();
void _jmp();
void _invoke();


int main(int argsCount, const char **args) {
   if(argsCount < 2) {
      std::cerr << "Runtime Error: Please provide an input file to run." << std::endl;
      return 1;
   }

   // readInputFile(args[1]);
   initStack();

   return 0;
}

void readInputFile(const char* filename) {
   std::ifstream file(filename, std::ios::binary);

   if (!file.is_open()) {
      std::cerr << "Error opening file" << std::endl;
      throw std::runtime_error("Error opening file!");
   }

   std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
   file.close();
   
   bytecode = fileContent.c_str();
   std::cout << "Read bytecode file!" << std::endl;
}

void initStack() {
   stack = reinterpret_cast<StackEntry*>(
	malloc(stackSize * sizeof(StackEntry))
   );
   counter = 0;
   std::cout << "Stack initialized!" << std::endl;
}

void beginExecution() {
   
}
