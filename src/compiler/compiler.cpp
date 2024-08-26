#include <iostream>
#include <fstream>
#include <string>

using namespace std;

enum class Token {
   Keyword, 
   Identifier, 
   Value  
};

enum class BasicType {
   Bool,
   Byte,
   Char,
   Short,
   Int,
   Float,
   Double,
   Long,
   String
};

struct Variable {
   BasicType type;
   int sizeInBytes;
   char *data;
   int id;
   bool readOnly;
};

struct Parameter {
   BasicType type;
   int id;
};

struct Function {
   Parameter *parameters;
   int parameterCount;
   int id;
};

Function *functions;
int functionsCounter;

Variable *constants;
int constantsCounter;


int main(int argsCount, char **args) {
   if(argsCount < 3) {
      cerr << "Requires two arguments: 1: input file, 2: output file" << endl;
      return 1;
   }
   
   string inputFileName(args[1]);
   string outputFileName(args[2]);

   ifstream inputFile(inputFileName);
   ofstream outputFile(outputFileName); 

   if(!inputFile.is_open()){
      return 2;
   }

   std::string inputContent;

   while(inputFile) {
       char c = inputFile.get();
       if(c == '\r') {
          continue;
       }
       if(c == '\n') {
          c = ' ';
       }
       inputContent.push_back(c);
   } 

   cout << inputContent << endl;

   inputFile.close();
   outputFile.close();

   return 0;
}
