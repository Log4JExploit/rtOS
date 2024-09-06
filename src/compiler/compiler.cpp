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
   Comma,

   And,
   Or,
   Equals,

   Separator,
   NewLine, 
   Other,
   EoF
};

enum class TokenMode {
   ONCE,
   ONCE_OR_NONE,
   ONCE_OR_MORE,
   MORE_OR_NONE,
   BRANCH
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
      this->mode = TokenMode::ONCE;
   }

   TokenNode(vector<variant<TokenNode, TokenType, const char*>> nodes, TokenMode mode) {
      this->list = nodes;
      this->mode = mode;
   }

   vector<variant<TokenNode, TokenType, const char*>>* getList() {
      return &(this->list);
   }

   void setMode(TokenMode mode) {
      this->mode = mode;
   }

   TokenMode getMode() {
      return this->mode;
   }
private:
   TokenMode mode;
   vector<variant<TokenNode, TokenType, const char*>> list;
};

class ContextContainer {
   public:
      ContextContainer(TokenNode *statement) {
         this->statement = statement;
	 this->statements = vector<ContextContainer>();
      }

      void addStatement(ContextContainer container) {
         this->statements.push_back(container);
      }

      TokenNode getStatement() {
         return *(this->statement);
      }
      
      void setName(char *name) {
         this->name = name;
      }
   private:
      TokenNode *statement;
      vector<ContextContainer> statements;
      char *name;
};

class TokenResult {
   public:
      TokenResult(TokenNode* node, bool satisfied) {
         this->node = node;
	 this->satisfied = satisfied;
	 this->tokens = {};
      }

      bool isEmpty() {
         return this->tokens.size() < 1;
      }

      bool isSatisfied() {
         return this->satisfied;
      }

      TokenNode* getNode() {
         return this->node;
      }

      int getTokenCount() {
         int count = 0;
	 for(variant<TokenResult, Token> v : this->tokens) {
            if(is_same_v<decltype(v), TokenResult>) {
               TokenResult result = get<TokenResult>(v);
	       count += result.getTokenCount();
	    } else {
               count++;
	    }  
	 }
	 return count;
      }

      vector<variant<TokenResult, Token>>* getTokens() {
         return &(this->tokens);
      }

      void add(variant<TokenResult, Token> element) {
         this->tokens.push_back(element);
      }

   private:
      TokenNode *node;
      bool satisfied;
      vector<variant<TokenResult, Token>> tokens;
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
vector<TokenNode> *ast;

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
void verifyContext(ContextContainer *context, vector<Token> *list, int *index);
int verifyStatement(TokenNode *node, vector<Token> *list, int index);
void verifyError(vector<Token> *tokens, int index);
TokenResult verifyEachVariant(TokenNode *node, vector<Token> *tokens, int index);
TokenResult verifyVariant(variant<TokenNode, TokenType, const char*> *variant, vector<Token> *tokens, int index);
TokenResult verifyMoreOrNone(TokenNode *node, vector<Token> *tokens, int index);
TokenResult verifyOnceOrMore(TokenNode *node, vector<Token> *tokens, int index);
TokenResult verifyOnceOrNone(TokenNode *node, vector<Token> *tokens, int index);
TokenResult verifyOnce(TokenNode *node, vector<Token> *tokens, int index);
TokenResult verifyBranch(TokenNode *node, vector<Token> *tokens, int index);

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
 
   verifyError(&tokens, 6);

   for(Token token : tokens) {
      cout << "\"" << token.text << "\", ";
   }

   return 0;
}

/* tokenizer */

vector<Token> tokenize(char *text, int length) {
   bool comment = false;
   int index = 0;
   vector<Token> tokens;
   TokenType last = TokenType::NewLine;
   
   while(index < length) {
      Token token = nextToken(&text[index], length - index);

      if(last == TokenType::NewLine && token.type == TokenType::Hashtag) {
         comment = true; 
      }

      if(token.type == TokenType::NewLine) {
         comment = false;
      }

      if(!comment) {
         tokens.push_back(token);
      }
      
      last = token.type;
      index += token.length;
   }
   
   Token endToken;
   endToken.type = TokenType::EoF;
   tokens.push_back(endToken);

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
      case ',': token.type = TokenType::Comma; break;
      
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
      if(numeric ? !isNumeric(c) : !(isAlphabetic(c) || c == '$')) {
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
  ContextContainer cc(nullptr);
  int lineCounter = 0;
  int index = 0; 

  while((*tokens)[index].type != TokenType::EoF) {
     verifyContext(&cc, tokens, &index);
  }
  
/*
   if constexpr (std::is_same_v<decltype(arg), int>) {
      
   }*/
}

void verifyContext(ContextContainer *context, vector<Token> *list, int *index) {
   TokenNode *statement = nullptr;
   int maxLength = 0;

   for(TokenNode& node : *ast) {
      int length = verifyStatement(&node, list, *index);
      if(length > maxLength) {
         maxLength = length;
	 statement = &node;
      }
   }

   if(maxLength == 0 || statement == nullptr) {
      verifyError(list, *index);
   }

   *index += maxLength;
   context->addStatement(ContextContainer(statement));
}

int verifyStatement(TokenNode *node, vector<Token> *list, int index) {
   vector<variant<TokenNode, TokenType, const char*>> *statementTokens = node->getList();
   int statementIndex = 0;

   for(variant<TokenNode, TokenType, const char*> targetToken : *statementTokens) {
      TokenType type = (*list)[statementIndex].type;
      while(type == TokenType::Separator || type == TokenType::NewLine) {
         type = (*list)[++statementIndex].type;
      }

      if(type == TokenType::EoF) {
         return 0;
      }

      if (std::is_same_v<decltype(targetToken), const char*>) {
         const char *keyword = get<const char*>(targetToken);
	 Token token = (*list)[statementIndex];

         if(token.type != TokenType::Keyword || strcmp(keyword, token.text) != 0) {
             return 0;
	 }

         statementIndex++;
      } else if ( std::is_same_v<decltype(targetToken), TokenType>) {
         TokenType targetType = get<TokenType>(targetToken);
	 if(targetType != type) {
            return 0;
	 }
	 
	 statementIndex++; 
      } else {
         
         TokenNode node = get<TokenNode>(targetToken);
         TokenMode mode = node.getMode();
	 return 0;
      }
   }

   return statementIndex;
}

void verifyError(vector<Token> *tokens, int index) {
   int lineCount = 0;
   int tokenIndex = 0;

   for(int i = 0; i < index; i++) {
      if((*tokens)[i].type == TokenType::NewLine) {
          lineCount++;
	  tokenIndex = i;
      }
   }
   
   cerr << endl;
   cerr << "Error, unexpected Token! Line " << lineCount << ": " << endl;
   
   int lineErrorIndex = 1;

   for(int i = tokenIndex; i < index; i++) {
      lineErrorIndex += (*tokens)[i].length;
      cerr << (*tokens)[i].text;
   }
   
   cerr << (*tokens)[index].text;
   cerr << endl;
   
   for(int i = 0; i < lineErrorIndex - 2; i++) {
      cerr << "-";
   }

   cerr << "^" << endl;
   exit(1);
}

TokenResult verifyEachVariant(TokenNode *node, vector<Token> *tokens, int index) {
   TokenResult result(node, true);
   int localIndex = 0;

   for(variant<TokenNode, TokenType, const char*> variantObj : *(node->getList())) {
      TokenResult subResult = verifyVariant(&variantObj, tokens, index + localIndex);
      if(!subResult.isSatisfied()) {
         return TokenResult(node, false);
      }
      if(subResult.getNode() == nullptr) {
         vector<variant<TokenResult, Token>> *tokens = subResult.getTokens();
         result.add((*tokens)[0]);        
      } else {
	 result.add(subResult);
      }
      localIndex = result.getTokenCount();
   }
   return result;
}

TokenResult verifyVariant(variant<TokenNode, TokenType, const char*> *variantObj, vector<Token> *tokens, int index) {
   TokenResult result(nullptr, true);
   TokenResult fail(nullptr, false);
   Token token = (*tokens)[index];
   
   if (std::is_same_v<decltype(*variantObj), const char*>) {
      const char *keyword = get<const char*>(*variantObj);
      
      if(token.type != TokenType::Keyword || strcmp(keyword, token.text) != 0) {
         return fail;
      }

      result.add(token);
   } else if (std::is_same_v<decltype(variantObj), TokenType>) {
      TokenType targetType = get<TokenType>(*variantObj);
      
      if(targetType != token.type) {
         return fail;
      }
	 
      result.add(token);
   } else {     
      TokenNode node = get<TokenNode>(*variantObj);
      return verifyBranch(&node, tokens, index);
   }
   return result;
}

TokenResult verifyBranch(TokenNode *node, vector<Token> *tokens, int index) {
   if(node->getMode() == TokenMode::ONCE) {
      return verifyOnce(node, tokens, index);
   } else if(node->getMode() == TokenMode::ONCE_OR_NONE) {
      return verifyOnceOrNone(node, tokens, index);
   } else if(node->getMode() == TokenMode::ONCE_OR_MORE) {
      return verifyOnceOrMore(node, tokens, index);
   } else {
      return verifyMoreOrNone(node, tokens, index);
   }
}

TokenResult verifyMoreOrNone(TokenNode *node, vector<Token> *tokens, int index) { 
   return TokenResult(nullptr, false);
}

TokenResult verifyOnceOrMore(TokenNode *node, vector<Token> *tokens, int index) {
   TokenResult result(node, true);
   bool failed = false;
   
   do {
      TokenResult resultAny = verifyOnceOrNone(node, tokens, index + result.getTokenCount());
      failed = !resultAny.isSatisfied();
      if(!failed) {
         result->add(resultAny);
      }
   } while(!failed);

   return tokenCount <= 0 ? TokenResult(node, false);
}

TokenResult verifyOnceOrNone(TokenNode *node, vector<Token> *tokens, int index) {
    return TokenResult(nullptr, false);
}

TokenResult verifyOnce(TokenNode *node, vector<Token> *tokens, int index) {
    return verifyEachVariant(node->getList(), tokens, index);
}

void initStatements() {
   *ast = vector<TokenNode>();

   TokenNode typeDeclaration({
      TokenType::Identifier,
      TokenType::Colon,
      TokenType::Identifier,
   }, TokenMode::ONCE);

   TokenNode create({ 
      "create", 
      TokenType::Identifier,
   });

   TokenNode createWithType({
      "create",
      TokenType::Identifier,
      TokenType::Colon,
      TokenType::Identifier
   });

   ast->push_back(createWithType);
   ast->push_back(create);

   TokenNode function({
      "function",
      TokenType::Identifier,
      TokenType::BracketOpen,
      
      TokenNode({
          typeDeclaration,
	  TokenNode({
	     TokenType::Comma
	     typeDeclaration,
	  }, TokenMode::MORE_OR_NONE)
      }, TokenMode::ONCE_OR_NONE),
      
      TokenType::BracketClose,
      TokenType::Colon,
      TokenType::Identifier,
      TokenNode({}),
      "done"
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
