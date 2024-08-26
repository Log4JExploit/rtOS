#include <iostream>
#include <fstream>
#include <string>

using namespace std;

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

struct ConstantValue {
   BasicType type;
   int sizeInBytes;
   char *data;
   int id;
}

struct Function {
       
};

int valuesCounter;
ConstantValue *values;


int main(int argsCount, char **args) {
   if(argsCount < 3) {
      cerr << "Requires two arguments: 1: input file, 2: output file" << endl;
      return 1;
   }
   
   string inputFileName(args[1]);
   string outputFileName(args[2]);

   ifstream inputFile(inputFileName);
   ofstream outputFile(outputFileName); 

   if(!inputFile.is_open()) {
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
