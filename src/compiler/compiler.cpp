#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace std;

enum class TokenType {
   Keyword,
   Colon,
   DoubleColon,
   BracketOpen,
   BracketClose,
   SquareBracketOpen,
   SquareBracketClose,
   BraceOpen,
   BraceClose,
   AngleBracketOpen,
   AngleBracketClose,
   Digits,
   Alpha,
   Plus,
   Minus,
   Slash,
   Star,
   Hashtag,
   QuoteSingle,
   QuoteDouble,
   Dollar,
   Space,
   Point,
   Other
};

const char *keywords[] = {
	          "bool", "char", "byte", "long", "int", "double", "float", "short", "string",
                  "create", "store", "for", "delete", "if", "calc", "inc", "dec", "exit", "done",
                  "up", "down", "below", "above", "not", "invoke", "function", nullptr
};

const char *operators[] = {
   "+", "-", "*", "/", nullptr
};

const char *brackets[] = {
   "(", ")", "{", "}", "[", "]", nullptr
};

const char *separators[] = {
   " ", "	", "\n", "\r"
}

struct Token {
   TokenType type;
   char *text;
   int length;
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
   char *name;
};

Function *functions;
int functionsCounter;

Variable *constants;
int constantsCounter;

vector<Token> tokenize(char *text, int length);
Token nextToken(char *text, int length);
int arrayFind(const char *array[], char* text, int length);
bool isAlphabetic(char character);
bool isUpper(char character);
bool isLower(char character);
bool isNumeric(char character);

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
       int read = inputFile.get();
       if(read == 13 || read == -1) {
           continue;
       }
       inputContent.push_back((char) read);
       cout << read << endl;
   } 
 
   int length = inputContent.length(); 
   const char *constantStr = inputContent.c_str(); 
   char *content = static_cast<char*>(malloc(length));
   memcpy(content, constantStr, length);

   cout << content << endl;

   inputFile.close();
   
   vector<Token> tokens = tokenize(content, length);
   
   outputFile.close();

   return 0;
}


vector<Token> tokenize(char *text, int length) {
   int index = 0;
   vector<Token> tokens;
   while(index < length) {
      Token token = nextToken(&text[index], length - index);
      tokens.push_back(token);
      index += token.length;
      char *copy = static_cast<char*>(malloc(token.length + 1));
      memcpy(copy, token.text, token.length);
      copy[token.length] = 0;
      cout << "Token Resut: " << copy << endl;
   }
   return tokens;
}

Token nextToken(char *text, int length) {
   char first = text[0];

   if(isAlphabetic(first)) {
      if(isLower(first)) {
         return nextKeywordToken(text, length);
      } else {
         return nextIdentifier(text, length);
      }
   } else if(isNumeric(first)) {
      nextNumericToken(text, length);
   }

   int keywordLength = arrayFind(keywords, text);

   if(keywordLength > 0) {
      token.type = TokenType::Keyword;
      token.length = keywordLength;
      return token;
   }

   int operatorLength = arrayFind(operators, text);

   if(operatorLength > 0) {
      switch(text[0]) {
	      case '+': token.type = TokenType::Plus; break;
	      case '-': token.type = TokenType::Minus; break;
	      case '*': token.type = TokenType::Star; break;
	      case '/': token.type = TokenType::Slash; break;
              default:
			cerr << "UNKNOWN OPERATOR" << endl;
			exit(1);
			break;
      }
      token.length = 1;
      return token;
   }

   return token;
}

Token nextKeywordToken(char *text, int length) {
   int keywordLength = arrayFind(keywords, text);

   if(keywordLength > 0) {
      token.type = TokenType::Keyword;
      token.length = keywordLength;
      return token;
   }
   return nextIdentifierToken(text, length);
}

int arrayFind(const char **array, char *text, int length) {
   for(int i = 0; array[i] != nullptr; i++) {
      const char *candidate = array[i];

      if(strlen(candidate) > length) {
         continue;
      }

      bool finished = false;
      int j = 0;
      
      while(candidate[j] == text[j]) {
         if(text[j] == 0) {
	    finished = true;
	    break;
	 }
	 
         j++;
      }
      
      if(finished || candidate[j] == 0) {
         return j;
      }
   }  
   return 0;
}

bool isAlphabetic(char c) {
   return isUpper(c) || isLower(c);
}

bool isUpper(char c) {
   return (c > 191 && c < 223 ) || (c => 'A' && c <= 'Z'); 
}

bool isLower(char c) {
   return (c > 221 && c < 256) || (c >= 'a' && c <= 'z' );
}

bool isNumeric(char c) {
   return c >= '0' && c <= '9';
}
