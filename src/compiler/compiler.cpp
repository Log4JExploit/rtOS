#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <variant>
#include <initializer_list>
#include <algorithm>

using namespace std;

/* Structs */

enum class TokenType {
   BaseType,
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
   Not,

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


struct Token {
   TokenType type;
   char *text;
   int length;
};

/* Keywords */

vector<const char*> keywords {
                  "create", "store", "for", "delete", "if", "calc", "inc", "dec", "exit", "done",
                  "up", "down", "below", "above", "not", "invoke", "function" 
};

vector<const char*> basetypes {
		  "bool", "byte", "char", "short", "int", "float", "double", "long", "string", "void"
};

/* Util Functions */

int arrayFind(vector<const char*> *vec, char* text, int length);
char* charcpy(const char *src, int length);
std::string readFile(const char *file);
void writeFile(const char *src, const char *file);
int hextoint(char *text, int offset, int length);

/* Classes */

class TokenNode {
public:
   TokenNode(initializer_list<variant<TokenNode*, TokenType, const char*>> nodes, TokenMode mode = TokenMode::ONCE) {
      this->list = new vector<variant<TokenNode*, TokenType, const char*>>();
      for (const variant<TokenNode*, TokenType, const char*>& node : nodes) {
            this->list->push_back(node);
      }
      this->mode = mode;
      this->name = charcpy("", 0);
   }

   TokenNode(vector<variant<TokenNode*, TokenType, const char*>> *nodes, TokenMode mode = TokenMode::ONCE) {
      this->list = nodes;
      this->mode = mode;
      this->name = charcpy("", 0);
   }

   vector<variant<TokenNode*, TokenType, const char*>>* getList() {
      return this->list;
   }

   void setMode(TokenMode mode) {
      this->mode = mode;
   }

   TokenMode getMode() {
      return this->mode;
   }

   TokenNode* inMode(TokenMode mode) {
      return (new TokenNode(this->list, mode))->withName(this->name);	
   }

   TokenNode* withName(const char* name) {
      this->name = name;
      return this;
   }

   const char* getName() {
      return this->name;
   }

private:
   const char* name;
   TokenMode mode;
   vector<variant<TokenNode*, TokenType, const char*>> *list;
};


class TokenResult {
   public:
      TokenResult(TokenNode* node, bool satisfied) {
         this->node = node;
	 this->satisfied = satisfied;
	 this->tokens = {};

	 this->message = "\0";
	 this->skipped = 0;
      }

      bool isEmpty() {
         return this->tokens.size() < 1;
      }

      void setSatisfied(bool satisfied) {
	 this->satisfied = satisfied;
      }

      bool isSatisfied() {
         return this->satisfied;
      }

      TokenNode* getNode() {
         return this->node;
      }

      int getTokenCount() {
         int count = 0;
	 for(variant<TokenResult*, Token>& v : this->tokens) {
            if(holds_alternative<TokenResult*>(v)) {
               TokenResult *result = get<TokenResult*>(v);
	       count += result->getTokenCount();
	    } else {
               count++;
	    }  
	 }
	 return count + this->skipped;
      }

      vector<variant<TokenResult*, Token>>* getTokens() {
         return &(this->tokens);
      }

      void add(variant<TokenResult*, Token> element) {
         this->tokens.push_back(element);
      }
   
      void setMessage(const char *message) {
	 this->message = message;
      }

      const char* getMessage() {
	 return this->message;
      }

      bool hasMessage() {
	 return strlen(this->message) > 0;
      }

      void skip() {
	 (this->skipped)++;
      }

      void incSkipped(int skipped) {
	 this->skipped += skipped;
      }

      int getSkipped() {
	 return this->skipped;
      }

      int getUnskipped() {
	 return this->getTokenCount() - this->skipped;
      }

   private:
      const char *message;
      TokenNode *node;
      bool satisfied;
      int skipped;
      vector<variant<TokenResult*, Token>> tokens;
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
TokenNode *contextNode;

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
TokenResult* verifyContext(TokenNode *node, vector<Token> *list, int index);
TokenResult* verifyEachVariant(TokenNode *node, vector<Token> *tokens, int index);
TokenResult* verifyVariant(variant<TokenNode*, TokenType, const char*> *variant, vector<Token> *tokens, int index);
TokenResult* verifyNode(TokenNode *node, vector<Token> *tokens, int index);
TokenResult* verifyBranch(TokenNode *node, vector<Token> *tokens, int index);
TokenResult* verifyMoreOrNone(TokenNode *node, vector<Token> *tokens, int index);
TokenResult* verifyOnceOrMore(TokenNode *node, vector<Token> *tokens, int index);
TokenResult* verifyOnceOrNone(TokenNode *node, vector<Token> *tokens, int index);
TokenResult* verifyOnce(TokenNode *node, vector<Token> *tokens, int index);
void verifyError(vector<Token> *tokens, int index);
void verifyError(vector<Token> *tokens, int index, const char *text);


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
   
   //verifyError(&tokens, 6, "Test error!");
   
   initStatements();
   verify(&tokens);

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

   Token endToken;
   endToken.type = TokenType::Keyword;
   endToken.text = charcpy("done", 4);
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
      case '!': token.type = TokenType::Not; break;

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
   cout << "TEXT SEARCH: " << text << endl;
   
   int basetypeLength = arrayFind(&basetypes, text, length);
   int keywordLength = arrayFind(&keywords, text, length);

   if(keywordLength + basetypeLength < 1) {
      cout << "TEXT NOT FOUND: " << text << endl;
      return nextIdentifierToken(text, length);
   }

   Token token;
   token.type = basetypeLength == 0 ? TokenType::Keyword : TokenType::BaseType;
   token.length = keywordLength + basetypeLength;
   token.text = charcpy(text, token.length);
   return token;
}

/* verify */

void verify(vector<Token> *tokens) {
   cout << "verify: " << endl;
   TokenResult *result = verifyContext(contextNode, tokens, 0);
   cout << "Parsing success: " << result->isSatisfied() << endl;
}

TokenResult* verifyContext(TokenNode *node, vector<Token> *list, int index) {
   cout << "verifyContext: " << endl;
   TokenResult *result = new TokenResult(node, true);
   variant<TokenNode*, TokenType, const char*> terminator = "done";
   bool terminated = false;
   
   do {
      if(verifyVariant(&terminator, list, index + result->getTokenCount())->isSatisfied()) {
	 terminated = true;
	 continue;
      }
      
      cout << "1" << endl; 
      bool success = false;
      TokenResult *max = new TokenResult(nullptr, false);
      cout << "2" << endl;
      for(TokenNode& node : *ast) {
	 cout << "Trying to detect statement" << endl;
         TokenResult *subResult = verifyEachVariant(&node, list, index + result->getTokenCount());
         if(!subResult->isSatisfied()) {
	    if(subResult->getUnskipped() > max->getUnskipped()) {
	       max = subResult;
	       cout << "FAIL: UPDATED MAX: " << max->getUnskipped() << "TOKENS IN SUBRESULT" << endl;
	    } else {
               cout << "FAIL, BUT NOT UPDATING!" << subResult->getUnskipped()  << endl; 
	    }   
	 } else {
	    cout << "STATEMENT SUCCESS" << endl;
            success = true;
	    result->add(subResult);
	    break;
	 }
      }

      if(!success) {
	 cout << "THROWING CONTEXT ERROR" << endl << endl;
	 verifyError(list, index + result->getTokenCount() + max->getTokenCount(), max->hasMessage() ? max->getMessage() : "Invalid statement!");
      }
   } while(!terminated);

   return result;
}

TokenResult* verifyEachVariant(TokenNode *node, vector<Token> *tokens, int index) {
   cout << "verifyEachVariant: " << endl;
   TokenResult *result = new TokenResult(node, true);

   for(variant<TokenNode*, TokenType, const char*> variantObj : *(node->getList())) {
      int localIndex = index + result->getTokenCount();
      cout << "INDEX: " << index << ", LOCAL: " << localIndex << endl; 
      TokenResult *subResult = verifyVariant(&variantObj, tokens, localIndex);
      
      cout << "SIZE: " << result->getTokens()->size() << " SKIPPED: " << subResult->getSkipped() << endl;
      cout << "SATISFIED: " << result->isSatisfied() << endl;

      if(!subResult->isSatisfied()) {
	 cout << "RETURNING FAIL" << endl;
	 result->add(subResult);
	 result->setSatisfied(false);
	 if(subResult->hasMessage()) {
            result->setMessage(subResult->getMessage());
         }
	 return result;
      }

      cout << "Tokens available: " << result->getUnskipped()  << endl;
      
      if(subResult->getNode() == nullptr) {
	 result->add(subResult->getTokens()->at(0));
      	 result->incSkipped(subResult->getSkipped());
      } else {
         result->add(subResult);
      }
   }
   return result;
}

TokenResult* verifyVariant(variant<TokenNode*, TokenType, const char*> *variantObj, vector<Token> *tokens, int index) {
   cout << "verifyVariant: " << endl;
   
   TokenResult *result = new TokenResult(nullptr, true);
   Token token = tokens->at(index);

   if(holds_alternative<TokenNode*>(*variantObj)) {
      delete result;
      TokenNode *node = get<TokenNode*>(*variantObj);
      return verifyNode(node, tokens, index);
   } else {
      while(token.type == TokenType::Separator || token.type == TokenType::NewLine) {
	 result->skip();
	 index++;
	 token = tokens->at(index);
      	 cout << "SKIP" << endl;
      }

      if(holds_alternative<const char*>(*variantObj)) {
	  const char *keyword = get<const char*>(*variantObj);
          
	  bool stringsAreEqual = strcmp(keyword, token.text) == 0;
	  if(token.type != TokenType::Keyword || !stringsAreEqual) {
	     cout << "REAL TOKEN NOT OF CORRECT TYPE: " << (token.type != TokenType::Keyword ? "true" : "false") << endl;
	     cout << "'" << token.text << "' IS NOT '" << keyword << "'" <<  endl;
             if(!stringsAreEqual) {
		string msg;
		msg += "This is not the correct keyword, expected: ";
		msg += keyword;
                result->setMessage(msg.c_str());
	     }
	     result->setSatisfied(false);
	  } else {
             cout << "CORRECT TOKEN: " << token.text << endl;
	  }

	  cout << token.text << " ADDED TO RESULT" << endl;
	  result->add(token);
      } else if(holds_alternative<TokenType>(*variantObj)) {
	 TokenType targetType = get<TokenType>(*variantObj);
	 
	 if(targetType != token.type) {
	    cout << "'" << token.text << "' IS NOT CORRECT TOKENTYPE " << static_cast<int>(targetType) <<  endl;
            string msg;
	    msg += "This is not the correct token, expected type: ";
	    msg += static_cast<int>(targetType);
            result->setMessage(msg.c_str());
	    result->setSatisfied(false);
	 } else {
            cout << "CORRECT TOKEN: " << token.text << endl;
	 }
	    
	 cout << token.text << " ADDED TO RESULT" << endl;
	 result->add(token);
      }
   }
   return result;
}

TokenResult* verifyNode(TokenNode *node, vector<Token> *tokens, int index) {

   if(strlen(node->getName()) > 0) {
      cout << "ATTEMPT VERIFY OF: " << node->getName() << endl;
   }

   if(node->getList()->size() == 0) {
      return verifyContext(node, tokens, index);
   }
	
   if(node->getMode() == TokenMode::ONCE) {
      cout << "MODE: ONCE" << endl; 
      return verifyOnce(node, tokens, index);
   } else if(node->getMode() == TokenMode::ONCE_OR_NONE) {
      cout << "MODE: ONCE_OR_NONE" << endl; 
      return verifyOnceOrNone(node, tokens, index);
   } else if(node->getMode() == TokenMode::ONCE_OR_MORE) {
      cout << "MODE: ONCE_OR_MORE" << endl; 
      return verifyOnceOrMore(node, tokens, index);
   } else if(node->getMode() == TokenMode::BRANCH) {
      cout << "MODE: BRANCH" << endl;
      return verifyBranch(node, tokens, index); 
   } else {
      cout << "MODE: MORE_OR_NONE" << endl; 
      return verifyMoreOrNone(node, tokens, index);
   }
}

TokenResult* verifyBranch(TokenNode *node, vector<Token> *tokens, int index) {
   TokenResult *max = new TokenResult(nullptr, false);
   for(variant<TokenNode*, TokenType, const char*> variantObj : *(node->getList())) {
      TokenResult *branchResult = verifyVariant(&variantObj, tokens, index);
      if(branchResult->isSatisfied()) {
	 return branchResult;
      } else if(branchResult->getTokenCount() > max->getTokenCount()) {
	 max = branchResult;
      }
   }

   if(max == nullptr) {
      return new TokenResult(node, false);
   } else {
      return max;
   }
}

TokenResult* verifyMoreOrNone(TokenNode *node, vector<Token> *tokens, int index) { 
   TokenResult *result = verifyOnceOrMore(node, tokens, index);
   result->setSatisfied(true);
   return result;
}

TokenResult* verifyOnceOrMore(TokenNode *node, vector<Token> *tokens, int index) {
   TokenResult *result = new TokenResult(node, true);
   bool failed = false;
    
   do {
      TokenResult *resultAny = verifyOnce(node, tokens, index + result->getTokenCount());
      if(!resultAny->isSatisfied()) {
      	 failed = true;
      } else {
	 result->add(resultAny);
      }
   } while(!failed);

   result->setSatisfied(!result->isEmpty());
   return result;
}

TokenResult* verifyOnceOrNone(TokenNode *node, vector<Token> *tokens, int index) {
   TokenResult *result = verifyOnce(node, tokens, index);
   if(!result->isSatisfied()) {
      result->getTokens()->clear();
   }
   result->setSatisfied(true);
   return result;
}

TokenResult* verifyOnce(TokenNode *node, vector<Token> *tokens, int index) {
    return verifyEachVariant(node, tokens, index);
}

void initStatements() {
   ast = new vector<TokenNode>();
   contextNode = new TokenNode {};

   // CREATE

   TokenNode *basetypeAndIdent = new TokenNode {
      TokenType::Identifier,
      TokenType::BaseType
   };

   TokenNode *addonType = new TokenNode {
      TokenType::Colon,
      basetypeAndIdent->inMode(TokenMode::BRANCH)
   };

   TokenNode *statementCreate = new TokenNode {
      "create",
      TokenType::Identifier,
      addonType->inMode(TokenMode::ONCE_OR_NONE)
   };
 
   // FUNCTION

   TokenNode *parameterDeclaration = new TokenNode {
      TokenType::Identifier,
      addonType->inMode(TokenMode::ONCE)
   };

   TokenNode *addonFunctionParameter = new TokenNode {
      TokenType::Comma,
      parameterDeclaration
   };

   TokenNode *functionArgumentList = new TokenNode {
      parameterDeclaration,
      addonFunctionParameter->inMode(TokenMode::MORE_OR_NONE)
   };
   
   TokenNode *statementFunction = new TokenNode {
      "function",
      TokenType::Identifier,
      TokenType::BracketOpen,
      functionArgumentList->inMode(TokenMode::ONCE_OR_NONE),
      TokenType::BracketClose,
      TokenType::Colon,
      basetypeAndIdent->inMode(TokenMode::BRANCH),
      contextNode
   };

   // VALUE

   TokenNode *value = (new TokenNode {
      TokenType::String,
      TokenType::Number,
      TokenType::BaseType,
      TokenType::Identifier
   })->withName("#value#");

   TokenNode *connector = (new TokenNode {
      TokenType::And,
      TokenType::Or,
      TokenType::Plus,
      TokenType::Minus,
      TokenType::Star,
      TokenType::Slash
   })->withName("#connector#");

   TokenNode *notWrapper = new TokenNode {
      TokenType::Not
   };

   TokenNode *wrapperValue = new TokenNode {
      notWrapper->inMode(TokenMode::ONCE_OR_NONE),
      value->inMode(TokenMode::BRANCH)
   };

   TokenNode *addonConnectingValues = new TokenNode {
      connector->inMode(TokenMode::BRANCH),
      wrapperValue
   };

   TokenNode *construct = (new TokenNode {
      wrapperValue->inMode(TokenMode::ONCE),
      addonConnectingValues->inMode(TokenMode::MORE_OR_NONE)
   })->withName("#construct#");

   TokenNode *enclosedConstruct = new TokenNode {
      TokenType::BracketOpen,
      construct,
      TokenType::BracketClose
   };

   // INVOKE

   TokenNode *domainIdentifier = (new TokenNode {
      TokenType::Identifier
   })->withName("#domainIdentifier#");

   TokenNode *addonFunctionArgument = new TokenNode {
      TokenType::Comma,
      construct
   };

   TokenNode *functionArgument = new TokenNode {
      construct,
      addonFunctionArgument->inMode(TokenMode::MORE_OR_NONE)
   };

   TokenNode *functionCall = new TokenNode {
      TokenType::Colon,
      TokenType::Colon,
      TokenType::Identifier,
      TokenType::BracketOpen,
      functionArgument->inMode(TokenMode::ONCE_OR_NONE),
      TokenType::BracketClose
   };

   TokenNode *functionCallChain = (new TokenNode {
      domainIdentifier->inMode(TokenMode::ONCE_OR_NONE),
      functionCall->inMode(TokenMode::ONCE_OR_MORE)
   })->withName("#functionCallChain#");
   
   TokenNode *statementInvoke = new TokenNode {
      "invoke",
      functionCallChain->inMode(TokenMode::ONCE)
   };

   value->getList()->insert(value->getList()->begin(), functionCallChain);
   value->getList()->insert(value->getList()->begin(), enclosedConstruct);
   
   ast->push_back(*statementInvoke);
   ast->push_back(*statementFunction);
   ast->push_back(*statementCreate);
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
	  tokenIndex = i + 1;
      }
   }
   
   cerr << endl;
   cerr << "Error is located in line " << lineCount << ": " << endl << endl;
   
   int lineErrorIndex = 0;

   for(int i = tokenIndex; i < tokens->size(); i++) {
      if(tokens->at(i).type == TokenType::NewLine) {
	 break;
      }
      if(i <= index) {
         lineErrorIndex += (*tokens)[i].length;
      }
      cerr << (*tokens)[i].text;
   }
   
   cerr << endl;

   for(int i = 0; i < lineErrorIndex - 2; i++) {
      cerr << "-";
   }

   cerr << "^" << endl;
   exit(1);
}

/* util */

int arrayFind(vector<const char*> *vec, char *text, int length) {
   for(const char*& candidate : *vec) {
      //if(strcmp(candidate, text) == 0) {
	// return length;
      //}
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
	 if(isNumeric(text[j]) || isAlphabetic(text[j]) || text[j] == '$') {
	    continue;
	 }
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
