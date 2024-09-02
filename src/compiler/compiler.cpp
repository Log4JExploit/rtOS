#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <variant>

using namespace std;

enum class TokenType {
   Keyword,
   Identifier,
   Digits,
   
   BracketOpen,
   BracketClose,
   SquareBracketOpen,
   SquareBracketClose,
   BraceOpen,
   BraceClose,
   AngleBracketOpen,
   AngleBracketClose,
   
   Plus,
   Minus,
   Slash,
   Star,
   
   QuoteSingle,
   QuoteDouble,
   
   Hashtag,
   Colon,
   Dollar,
   Space,
   Point,

   And,
   Or,
   Equals,

   Separator,
   NewLine, 
   Other,
   EoF
};

const char *keywords[] = {
                  "create", "store", "for", "delete", "if", "calc", "inc", "dec", "exit", "done",
                  "up", "down", "below", "above", "not", "invoke", "function", "void", nullptr
};

struct Token {
   TokenType type;
   char *text;
   int length;
};

class TokenNode {
public:
   TokenNode(vector<variant<TokenNode, TokenType, const char*>> nodes) {
      this->list = nodes;
   }

   TokenNode(vector<variant<TokenNode, TokenType, const char*>> nodes, bool repeat) {
      this->list = nodes;
      this->repeat = repeat;
   }
   
   TokenNode(TokenType type) {
      this->list = { type };
   }
   
   TokenNode(TokenType type, bool repeat) {
      this->list = { type };
      this->repeat = repeat;
   }

   vector<variant<TokenNode, TokenType, const char*>> getList() {
      return this->list;
   }

   bool canRepeat() {
      return this->repeat;
   }

private:
   bool repeat;
   vector<variant<TokenNode, TokenType, const char*>> list;
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

Token endOfFileToken;
Function *functions;
int functionsCounter;

Variable *constants;
int constantsCounter;

/* Functions */

// TOKENIZER

vector<Token> tokenize(char *text, int length);

Token nextToken(char *text, int length);
Token nextKeywordToken(char *text, int length);
Token nextIdentifierToken(char *text, int length);
Token nextNumericToken(char *text, int length);
Token nextSpecialToken(char *text, int length);

// PARSER

void initStatements();
void verify(vector<Token> *tokens);

/* Util Functions */

int arrayFind(const char *array[], char* text, int length);
char* charcpy(const char *src, int length);
std::string readFile(const char *file);
void writeFile(const char *src, const char *file);

bool isAlphabetic(char character);
bool isUpper(char character);
bool isLower(char character);
bool isNumeric(char character);

/* main */

int main(int argsCount, char **args) {
   if(argsCount < 3) {
      cerr << "Requires two arguments: 1: input file, 2: output file" << endl;
      return 1;
   }
   
   endOfFileToken.type = TokenType::EoF;

   string inputFileName(args[1]);
   string outputFileName(args[2]);

   string contentStr = readFile(inputFileName.c_str());
   char *content = charcpy(contentStr.c_str(), contentStr.length());

   cout << content << endl;
   
   vector<Token> tokens = tokenize(content, contentStr.length());
  
   for(Token token : tokens) {
      cout << "\"" << token.text << "\", ";
   }

   return 0;
}

/* tokenizer */

vector<Token> tokenize(char *text, int length) {
   int index = 0;
   vector<Token> tokens;
   TokenType last = TokenType::EoF;
   while(index < length) {
      Token token = nextToken(&text[index], length - index);
      
      if(last == TokenType::NewLine && token.type == TokenType::HashTag) {
          int commentLength = token.length;
      }

      tokens.push_back(token);
      last = token.type;
      index += token.length;
   }
   return tokens;
}

Token nextToken(char *text, int length) {
   char first = text[0];

   if(first == 0 || length == 0) {
      return endOfFileToken;
   }

   if(isAlphabetic(first)) {
      if(isLower(first)) {
         return nextKeywordToken(text, length);
      } else {
         return nextIdentifierToken(text, length);
      }
   } else if(isNumeric(first)) {
      return nextNumericToken(text, length);
   }

   return nextSpecialToken(text, length);
}

Token nextSpecialToken(char *text, int length) {
   char first = text[0];    
  
   Token token;
   token.text = charcpy(text, 1);
   token.length = 1;
   
   switch(text[0]) {
      case '+': token.type = TokenType::Plus; break;
      case '-': token.type = TokenType::Minus; break;
      case '*': token.type = TokenType::Star; break;
      case '/': token.type = TokenType::Slash; break;

      case ' ': 
      case '	': 
      case '\r': token.type = TokenType::Separator; break;
      
      case '(': token.type = TokenType::BracketOpen; break;
      case ')': token.type = TokenType::BracketClose; break;
      case '{': token.type = TokenType::BraceOpen; break;
      case '}': token.type = TokenType::BraceClose; break;
      case '[': token.type = TokenType::SquareBracketOpen; break;
      case ']': token.type = TokenType::SquareBracketClose; break;
      
      case '$': token.type = TokenType::Dollar; break;
      case '#': token.type = TokenType::Hashtag; break;
      case ':': token.type = TokenType::Colon; break;
      case '.': token.type = TokenType::Point; break;
      
      case '\'': token.type = TokenType::QuoteSingle; break;
      case '\"': token.type = TokenType::QuoteDouble; break;
     
      case '&': token.type = TokenType::And; break;
      case '|': token.type = TokenType::Or; break;
      case '=': token.type = TokenType::Equals; break;
      
      case '\n': token.type = TokenType::NewLine; break;

      default:
         token.type = TokenType::Other;
	 break;
   }

   token.length = 1;
   return token;
}

Token nextAlphaOrNumToken(char *text, int length, bool numeric) {
   int index = 0;
   while(text[index] != 0 && index < length) {
      char c = text[index];
      if(numeric ? !isNumeric(c) : !(isAlphabetic(c) || c != '$')) {
         break;
      }
      index++;
   }

   if(index == 0) {
      return endOfFileToken;
   }

   Token token;
   token.type = numeric ? TokenType::Digits : TokenType::Identifier;
   token.text = charcpy(text, index);
   token.length = index;
   return token;
}

Token nextIdentifierToken(char *text, int length) {
   return nextAlphaOrNumToken(text, length, false);
}

Token nextNumericToken(char *text, int length) {
   return nextAlphaOrNumToken(text, length, true);
}

Token nextKeywordToken(char *text, int length) {
   int keywordLength = arrayFind(keywords, text, length);

   if(keywordLength < 1) {
      return nextIdentifierToken(text, length);
   }

   Token token;
   token.text = charcpy(text, keywordLength);
   token.type = TokenType::Keyword;
   token.length = keywordLength;
   return token;
}

/* verify */

void verify(vector<Token> *tokens) {
   
}

void initStatements() {
   TokenNode typeDeclaration({
      TokenType::Identifier,
      TokenType::Colon,
      TokenType::Separator,
      TokenType::Identifier
   });

   TokenNode create({ 
      "create", 
      TokenType::Separator,
      TokenType::Identifier,
      TokenNode({
         TokenType::Colon,
	 TokenType::Keyword
      }, true)
   });

   TokenNode function({
      "function",
      TokenType::Separator,
      TokenType::Identifier,
      TokenType::BracketOpen
   }); 
}

/* util */

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


char* charcpy(const char *src, int length) {
   char *copy = static_cast<char*>(malloc(length + 1));
   memcpy(copy, src, length);
   copy[length] = 0;
   return copy;
}

string readFile(const char *filename) {
   string inputFileName(filename);
   ifstream inputFile(inputFileName);

   if(!inputFile.is_open()){
      cerr << "Unable to open file '" << filename << "' for writing" << endl;
      exit(1);
   }

   string inputContent;

   while(inputFile) {
       int read = inputFile.get();
       if(read == 13 || read == -1) {
           continue;
       }
       inputContent.push_back((char) read);
       cout << read << endl;
   }

   return inputContent; 
}

void writeFile(const char *src, const char *filename) {
   ofstream outputFile(filename); 
   if(!outputFile.is_open()) {
      cout << "Unable to open " << filename << " for writing, exiting!";
      exit(-1);
   }
   cout << "TODO: Writing to file" << endl;
}

bool isAlphabetic(char c) {
   return isUpper(c) || isLower(c);
}

bool isUpper(char c) {
   return (c > 191 && c < 223 ) || (c >= 'A' && c <= 'Z'); 
}

bool isLower(char c) {
   return (c > 221 && c < 256) || (c >= 'a' && c <= 'z' );
}

bool isNumeric(char c) {
   return c >= '0' && c <= '9';
}

/* todo */
