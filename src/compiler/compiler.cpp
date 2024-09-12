#include <cmath>
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
   Number,
   String,
   
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

      vector<variant<TokenResult, Token>*>* getTokens() {
         return &(this->tokens);
      }

      void add(variant<TokenResult, Token> *element) {
         this->tokens.push_back(element);
      }

      TokenResult* persist() {
	 TokenResult *storage = reinterpret_cast<TokenResult*>(malloc(sizeof(TokenResult)));
      	 *storage = *this;
	 return storage;
      }
   
   private:
      TokenNode *node;
      bool satisfied;
      vector<variant<TokenResult, Token>*> tokens;
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
Token nextSpecialToken(char *text, int length);
void evaluateToken(char* text, int *index, vector<Token> *tokens, int length, TokenType *last);
void evaluateNumber(char* text, int *index, vector<Token> *tokens, int length);
void evaluateComment(char* text, int *index, vector<Token> *tokens, int length);
void evaluateString(char* text, int *index, vector<Token> *tokens, int length);
char* evaluateUnicode(char* text, int *index, vector<Token> *tokens,  int length);
int byEscapedCharacter(char c);

// PARSER

void initStatements();
void verify(vector<Token> *tokens);
void verifyContext(TokenNode *node, vector<Token> *list, int index);
void verifyError(vector<Token> *tokens, int index);
void verifyError(vector<Token> *tokens, int index, const char *text);
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
int hextoint(char *text, int offset, int length);

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
   
   verifyError(&tokens, 6, "Test error!");
   return 0;
}

/* tokenizer */

vector<Token> tokenize(char *text, int length) {
   bool comment = false;
   int index = 0;
   vector<Token> tokens;
   TokenType last = TokenType::NewLine;
   
   while(index < length) {
      char first = text[index];
      
      if(first == '#' && last == TokenType::NewLine) {
         evaluateComment(text, &index, &tokens, length);
      } else if(isNumeric(first)) {
         evaluateNumber(text, &index, &tokens, length);
      } else if(first == '\"') {
         evaluateString(text, &index, &tokens, length);
      } else {
         evaluateToken(text, &index, &tokens, length, &last);
      }
   }
   
   char *term = reinterpret_cast<char*>(malloc(1));
   *term = '\0';

   Token endToken;
   endToken.type = TokenType::EoF;
   endToken.text = term;
   tokens.push_back(endToken);

   return tokens;
}

void evaluateToken(char* text, int *index, vector<Token> *tokens, int length, TokenType *last) {
   Token token = nextToken(&text[*index], length - (*index));
   *last = token.type;
   *index += token.length;
   tokens->push_back(token);
}

void evaluateNumber(char* text, int *index, vector<Token> *tokens, int length) {
   Token numberToken;
   numberToken.type = TokenType::Number;
   bool terminated = false;
   string buffer;
   
   while(!terminated) {
      char next = text[*index];
      if(isNumeric(next)) {
         buffer += next;
      } else {
         switch(next) {
            case '.':
            case '-':
            case '+':
            case 'e':
	       buffer += next;
	       break;

            case 'd':
            case 'f':
	       buffer += next;
            default: 
	       terminated = true;
	       break;
	 }
      }

      if(!terminated) {
         (*index)++;
      }
   }

   numberToken.text = charcpy(buffer.c_str(), buffer.length());
   tokens->push_back(numberToken);
}

void evaluateComment(char* text, int *index, vector<Token> *tokens, int length) {
   while(text[*index] != '\n') {
      (*index)++;
   }
}

void evaluateString(char* text, int *index, vector<Token> *tokens, int length) {
   (*index)++;
   string strText;
   bool terminated = false;
   
   while(*index < length && !terminated) {
      if(text[*index] == '\\') {
         if((*index) + 1 < length) {
	    char next = text[(*index) + 1];
            if(next == 'u') {
               char *unicode = evaluateUnicode(text, index, tokens, length); 
	       strText += unicode;
	       (*index) += 6;
	       continue;
	    } else {
	       int escapedValue = byEscapedCharacter(next);
	       if(escapedValue == -1) {
                  verifyError(tokens, tokens->size() - 1, "Unknown escaped character!");
	       }
	       strText += (char) escapedValue;
	       (*index)++;
	    }
	 } else {
            break;
	 }
      } else {
	 if(text[*index] == '\"') {
            terminated = true;
	 } else {
            strText += text[*index];
         }
      }
      (*index)++;
   }

   if(!terminated) {
      verifyError(tokens, tokens->size() - 1, "String was never terminated!");
   }

   Token stringToken;
   stringToken.type = TokenType::String;
   stringToken.text = charcpy(strText.c_str(), strText.length());
   tokens->push_back(stringToken);
}

int byEscapedCharacter(char c) {
   switch(c) {
      case 'n': return '\n';
      case 'r': return '\r';
      case 't': return '\t';
      case 'v': return '\v';
      case 'b': return '\b';
      case 'f': return '\f';
      case '\\': return '\\';
      case '\"': return '\"';
      case '?': return '\?';
      case '0': return '\0';
      default:
         return -1;
         break;
   }
}

char* evaluateUnicode(char* text, int *index, vector<Token> *tokens, int length) {
   if((*index) + 4 >= length) {
      verifyError(tokens, tokens->size() - 1, "End of file reached during unicode sequence! Expected at least 4 hex characters!");
   }

   int value = hextoint(text, (*index) + 2, 4); 

   if(value == -1) {
      verifyError(tokens, tokens->size() - 1, "Non-hex character found in unicode sequence!");
   } 

   char *placeholder = reinterpret_cast<char*>(malloc(2));
   placeholder[0] = '?';
   placeholder[1] = '\0';

   return placeholder;
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

Token nextIdentifierToken(char *text, int length) {
   int index = 0;
   while(index < length && text[index] != '\0') {
      char c = text[index];
      if(isAlphabetic(c) || (index > 0 && (c == '$' || isNumeric(c)))) {
         index++;
      } else {
         break;
      }
   }

   if(index == 0) {
      return endOfFileToken;
   }

   Token token;
   token.type = TokenType::Identifier;
   token.text = charcpy(text, index);
   token.length = index;
   return token;
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
  int index = 0; 
  TokenResult result = verifyContext(&global, tokens, index);
}

TokenResult verifyContext(TokenNode *node, vector<Token> *list, int index) {
   TokenResult result(node, false);
   bool terminated = false;
   
   while(!terminated) {
      if(verifyVariant("done", list, index).isSatisfied()) {
	 return;
      }
      
      bool success = false;
      TokenResult max(nullptr, false);

      for(TokenNode& node : *ast) {
         TokenResult result = verifyEachVariant(&node, list, *index);
         if(!result.isSatisfied() {
	    if(result.getTokenCount() > max.getTokenCount()) {
	       max = result;
	    }   
	 } else {
	    success = true;
	    max = result;
	 }
      }

      if(success) {
      	 TokenResult *storage = reinterpret_cast<TokenResult*>(malloc(sizeof(TokenResult)));
	 context->add(storage);
      } else {

      }
   }
}

void verifyError(vector<Token> *tokens, int index, const char *text) {
   cout << "Error Message: " << text;
   verifyError(tokens, index);
}

void verifyError(vector<Token> *tokens, int index) {
   int lineCount = 1;
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

   for(variant<TokenNode, TokenType, const char*> variantObj : *(node->getList())) {
      int localIndex = index + result.getTokenCount();
      TokenResult subResult = verifyVariant(&variantObj, tokens, index);
      
      if(!subResult.isSatisfied()) {
         return TokenResult(node, false);
      }

      if(subResult.getNode() == nullptr) {
         vector<variant<TokenResult, Token, ContextContainer>> *tokens = subResult.getTokens();
         result.add((*tokens)[0]);        
      } else {
	 result.add(subResult);
      }
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
   if(node->getList()->size() == 0) {
      ContextContainer *container = reinterpret_cast<ContextContainer*>(malloc(sizeof(ContextContainer)));
      *container = ContextContainer(node);
      verifyContext(&container, tokens, index);
      
   }
	
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
      TokenResult resultAny = verifyOnce(node, tokens, index + result.getTokenCount());
      failed = !resultAny.isSatisfied();
      if(!failed) {
         result.add(resultAny);
      }
   } while(!failed);

   return result.getTokenCount() < 1 ? TokenResult(node, false) : result;
}

TokenResult verifyOnceOrNone(TokenNode *node, vector<Token> *tokens, int index) {
    TokenResult result = verifyOnce(node, tokens, index);
    return result.isSatisfied() ? result : TokenResult(node, true);
}

TokenResult verifyOnce(TokenNode *node, vector<Token> *tokens, int index) {
    return verifyEachVariant(node, tokens, index);
}

void initStatements() {
   *ast = vector<TokenNode>();

   TokenNode* typeDeclaration = new TokenNode({
      TokenType::Identifier,
      TokenType::Colon,
      TokenType::Identifier
   }, TokenMode::ONCE);

   TokenNode* createWithType = new TokenNode({
      "create",
      TokenType::Identifier,
      TokenNode({
         TokenType::Colon,
         TokenType::Identifier
      }, TokenMode::ONCE_OR_NONE)
   });
   
   TokenNode* function = new TokenNode({
      "function",
      TokenType::Identifier,
      TokenType::BracketOpen,
 
      TokenNode({
          *typeDeclaration,
	  TokenNode({
	     TokenType::Comma,
	     *typeDeclaration,
	  }, TokenMode::MORE_OR_NONE)
      }, TokenMode::ONCE_OR_NONE),
      
      TokenType::BracketClose,
      TokenType::Colon,
      TokenType::Identifier,
      TokenNode({}),
      "done"
   });

   ast->push_back(*function);
   ast->push_back(*createWithType);
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

int hextoint(char *text, int offset, int length) {
   int result = 0;
   int exp = 0;
   for(int i = length - 1 + offset; i >= offset; i--) {
      char c = text[i];
      if(!isAlphabetic(c) && !isNumeric(c)) {
         return -1;
      }
      int v = isAlphabetic(c) ? (isUpper(c) ? (int) (c - 'A') + 10 : (int) (c - 'a') + 10) : (int) (c - '0');
      int pw = (int) pow(16, exp);
      result += pw * v;
      exp++;
   }

   return result;
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
