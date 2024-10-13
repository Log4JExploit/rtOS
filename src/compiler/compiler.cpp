#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <variant>
#include <initializer_list>

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
   Char,
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
   int index;
};

/* Keywords */

vector<const char*> keywords {
                  "create", "set", "for", "delete", "if", "inc", "dec", "exit", "done",
                  "up", "down", "below", "above", "invoke", "function", "while", "to", "until", "return" 
};

vector<const char*> basetypes {
		  "bool", "byte", "char", "short", "int", "float", "double", "long", "array", "void"
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
      this->name = charcpy("no name", 7);
   }

   TokenNode(vector<variant<TokenNode*, TokenType, const char*>> *nodes, TokenMode mode = TokenMode::ONCE) {
      this->list = nodes;
      this->mode = mode;
      this->name = charcpy("no name", 7);
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

   bool isOptional() {
      return this->mode == TokenMode::ONCE_OR_NONE || this->mode == TokenMode::MORE_OR_NONE;
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

	 this->message = charcpy("", 0);
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

      void setNode(TokenNode *node) {
         this->node = node;
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
   
      void setMessage(char *message) {
	 this->message = message;
      }

      char* getMessage() {
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

      int getMatchedTokens() {
         int count = 0;
	 for(variant<TokenResult*, Token>& v : this->tokens) {
            if(holds_alternative<TokenResult*>(v)) {
               TokenResult *result = get<TokenResult*>(v);
	       if(result->isSatisfied()) {
	          count += result->getTokenCount();   
	       }
	    } else {
               count++;
	    }  
	 }
	 return count;
      }

      Token* getFirstToken() {
	 return getFirstForResult(this);
      }

      void setEof() {
	 this->eof_ = true;
      }

      bool isEof() {
	 if(this->getMatchedTokens() < 1) {
            return this->eof_;
         }
         for(variant<TokenResult*, Token>& v : this->tokens) {
            if(holds_alternative<TokenResult*>(v)) {
               TokenResult *result = get<TokenResult*>(v);
               if(result->isEof()) {
                  return true;
               }
            }
         }
         return false;
      }

   private:
      Token* getFirstForResult(TokenResult *result) {
         for(variant<TokenResult*, Token>& v : *(result->getTokens())) {
            if(holds_alternative<Token>(v)) {
               return &(get<Token>(v));
	    } else {
               TokenResult *subResult = get<TokenResult*>(v);
               if(subResult->isSatisfied()) {
                  Token *any = getFirstForResult(subResult);
		  if(any != nullptr) {
                     return any;
		  }
               }
            }
         }
	 return nullptr;
      }

      char *message;
      TokenNode *node;
      bool satisfied;
      int skipped;
      bool eof_;
      vector<variant<TokenResult*, Token>> tokens;
};

enum class PointerType {
   STACK,
   STACK_TOP,
   STACK_VALUE,
   CONSTANT,
   FUNCTION,
   MEMORY
};

enum class BaseType {
   Bool,
   Byte,
   Char,
   Short,
   Int,
   Float,
   Double,
   Long,
   Void
};

enum class Operator {
   Add,
   Sub,
   Mul,
   Div,
   And,
   Or,
   Not,
   Equal,
   Greater,
   Smaller
};

class Type {
   public:
      Type(variant<BaseType, const char*> type) {
         this->type = type;
	 this->genericTypes = new vector<Type*>();
      }

      variant<BaseType, const char*> getBaseType() {
         return this->type;
      }

      bool hasGenericTypes() {
         return this->genericTypes->size() >= 0;
      }

      void setGenericType(Type *type) {
         this->genericTypes->push_back(type);
      }

      vector<Type*>* getGenericTypes() {
         return this->genericTypes;
      }

   private:
      variant<BaseType, const char*> type;
      vector<Type*> *genericTypes;
};

class Variable {
   
   public:
      Variable(PointerType pointerType, Type type, const char* identifier = nullptr, long value = 0)
      : pointerType(pointerType), type(type), value(value) {
         if (identifier != nullptr) {
            identifier = charcpy(identifier, strlen(identifier));
         }
      }

      PointerType getPointerType() {
         return pointerType;
      }

      Type getType() {
         return this->type;
      }

      long getValue() {
         return this->value;
      }

      void setValue(long value) {
         this->value = value;
      }

      const char* getIdentifier() {
         return this->identifier;
      }

      bool hasIdentifier() {
         return (this->identifier) != nullptr;
      }

   private:
      PointerType pointerType;
      char *identifier;
      long value;
      Type type;
};

enum class OperandType {
   Invoke,
   InvokeChain,
   Primitive,
   Identifier,
   Block
};


class Unresolved {
   public:
      Unresolved(TokenResult *value) {
         this->value = value;
	 if(strcmp(value->getNode()->getName(), "value") != 0) {
            throw invalid_argument("Given TokenResult is not a value!");
	 }
      }

      vector<Token>* getReferences() {
         vector<Token> *list = new vector<Token>();
	 resolve(list, this->value, true);
	 return list; 
      }

      vector<Token>* getConstants() {
         vector<Token> *list = new vector<Token>();
	 resolve(list, this->value, false); 
         return list;
      }
   private:
      void resolve(vector<Token> *list, TokenResult *target, bool ids) {
         for(variant<TokenResult*, Token>& next : *(target->getTokens())) {
            
	    if(holds_alternative<TokenResult*>(next)) {
               TokenResult *result = get<TokenResult*>(next);
	       
	       if(strcmp(result->getNode()->getName(), "primitivebare") == 0) {
                  Token token = get<Token>(result->getTokens()->at(0));
                  
		  if(token.type == TokenType::Identifier && ids) {
                     list->push_back(token);
		  } else if(token.type != TokenType::Identifier && !ids) {
	             list->push_back(token);
		  }
	       } else {
                  resolve(list, result, ids);
	       }
	    }
	 }
      }

      TokenResult *value;
};

class Value {
   public:
      Value(OperandType type) {
         this->operandType = type;
      }

      OperandType getOperandType() {
         return this->operandType;
      }

   private:
      OperandType operandType;
};

class Operation {
   public:
      Operation(Value *first, Value *second, Operator op) {
         this->first = first;
	 this->second = second;
	 this->op = op;
      }

      Operator getOperator() {
	 return this->op;
      } 

      Value* getFirst() {
	 return this->first;
      }

      Value* getSecond() {
	 return this->second;
      }
   private:
      Value *first;
      Value *second;
      Operator op;
};

class ValueBlock : public Value {
   public:
      ValueBlock() : Value(OperandType::Block) {
         this->content = new vector<variant<Value*, Operator>>();
      }

      void addElement(variant<Value*, Operator> element) {
	 this->content->push_back(element);
      }

      vector<variant<Value*, Operator>>* getContent() {
	 return this->content;
      }

   private:
      vector<variant<Value*, Operator>>* content;
};

class ValueIdentifier : public Value {
   public:
      ValueIdentifier(const char* name) : Value(OperandType::Identifier) {
      	 this->name = name;   
      } 

      const char* getName() {
         return this->name;
      }

      bool isSet() {
	 return this->name != nullptr && strlen(this->name) > 0;
      }

   private:
      const char *name;
};

class ValueInvoke : public Value {
   public:
      ValueInvoke(ValueIdentifier space, ValueIdentifier name)
	      : Value(OperandType::Invoke), space(space), name(name) {
	 this->parameters = new vector<Value*>();
      }

      void addParameter(Value *value) {
         this->parameters->push_back(value);
      }

      ValueIdentifier getSpace() {
	 return this->space;
      }

      ValueIdentifier getName() {
	 return this->name;
      }
   private:
      ValueIdentifier space;
      ValueIdentifier name;
      vector<Value*> *parameters;
};

class ValueInvokeChain : public Value {
   public:
      ValueInvokeChain() : Value(OperandType::InvokeChain) {
         this->chain = new vector<ValueInvoke*>();
      }

   private:
      vector<ValueInvoke*>* chain;
};

class ValuePrimitive : public Value {
   public:
      ValuePrimitive(Type type) 
	      : Value(OperandType::Primitive), type(type) {
      }

      Type getType() {
	 return this->type;
      }

   private:
      Type type;
};

class UnwrappedOperation {
   public:
      UnwrappedOperation() {
         
      }
};

class Statement {
   public:
      Statement(const char *name) {
         this->name = name;
	 this->statements = new vector<Statement*>();
      }

      const char* getName() {
         return this->name;
      }

      vector<Statement*>* getStatements() {
	 return this->statements;
      }

      void add(Statement *statement) {
	 this->statements->push_back(statement);
      }

   private:
      const char *name;
      vector<Statement*> *statements;
};

class SFunction : public Statement {
   public: 
      SFunction() 
	      : Statement("function"), parameters(new vector<Variable>()) {
      }

      void addParameter(Variable *variable) {
         this->parameters->push_back(*variable);
      }

      vector<Variable>* getParameters() {
	 return this->parameters;
      }
  
   private:
      const char *name;
      vector<Variable> *parameters;
};

class SCreate : public Statement {
   public: 
      SCreate(const char *identifier, Type type) 
	      : Statement("create"), identifier(identifier), type(type) {
      }

      const char* getIdentifier() {
	 return this->identifier;
      }

      Type getType() {
	 return this->type;
      }
   private:
      const char *identifier;
      Type type;
};

class SDelete : public Statement {
   public:
      SDelete(const char *identifer) : Statement("delete") {
         this->identifier = identifier;
      }

      const char* getIdentifier() {
	 return this->identifier;
      }

   private:
      const char *identifier;
};

Token endOfFileToken;
vector<TokenNode> *ast;
TokenNode *contextNode;

SFunction *functions;
int functionsCounter;

Variable *constants;
int constantsCounter;

/* Functions */

// TOKENIZER (LEVEL 1)

vector<Token> tokenize(char *text, int length);

Token nextToken(char *text, int length);
Token nextKeywordToken(char *text, int length);
Token nextIdentifierToken(char *text, int length);
Token nextSpecialToken(char *text, int length);
void evaluateToken(char* text, int *index, vector<Token> *tokens, int length, TokenType *last);
void evaluateNumber(char* text, int *index, vector<Token> *tokens, int length);
void evaluateComment(char* text, int *index, vector<Token> *tokens, int length);
void evaluateString(char* text, int *index, vector<Token> *tokens, int length);
void evaluateChar(char* text, int *index, vector<Token> *tokens, int length);
char* evaluateUnicode(char* text, int *index, vector<Token> *tokens,  int length);
int byEscapedCharacter(char c);

// PARSER (LEVEL 2)

void initStatements();
TokenResult* verify(vector<Token> *tokens);
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

// UNIFY (LEVEL 3)

Statement* unify(Statement* container, TokenResult *context);
Statement* unifyStatement(TokenResult *statement);
Statement* unifyCreate(TokenResult *statement);
Statement* unifyDelete(TokenResult *statement);
Statement* unifySet(TokenResult *statement);
Statement* unifyFor(TokenResult *statement);
Statement* unifyWhile(TokenResult *statement);
Statement* unifyIf(TokenResult *statement);
Statement* unifyFunction(TokenResult *statement);
Statement* unifyLambda(TokenResult *statement);
Statement* unifyInvoke(TokenResult *statement);
Statement* unifyIncrement(TokenResult *statement);
Statement* unifyDecrement(TokenResult *statement);
Statement* unifyExit(TokenResult *statement);
Statement* unifyValue(TokenResult *value);
Type* unifyType(TokenResult *result);
variant<BaseType, const char*> unifyBaseType(Token *basicTypeToken);

// VALIDATOR (LEVEL 3)

void validate(TokenResult *context);
void validateFunctions(TokenResult *context);
void validateVariables(TokenResult *context);
void validateCreate();


// TRANSLATE


// UTIL

bool isAlphabetic(char character);
bool isUpper(char character);
bool isLower(char character);
bool isNumeric(char character);
const char* getTokenTypeByInt(int num);

/* main */

vector<Token> *tokenList;

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
   int size = contentStr.length() + 128;

   char *buf = static_cast<char*>(malloc(size));
   memcpy(buf, contentStr.c_str(), size - 128);

   for(int i = size - 128; i < size; i++) {
      if(i % 2 == 0) {
         buf[i] = '\0';
      } else {
         buf[i] = '%';
      }
   }

   cout << content << endl;
   
   vector<Token> tokens = tokenize(buf, contentStr.length());
   tokenList = &tokens;

   for(Token token : tokens) {
      cout << "\"" << token.text << "\", ";
   }
   
   //verifyError(&tokens, 6, "Test error!");
   
   cout << "Initializing statements..." << endl;
   initStatements();

   cout << "Verifying statements..." << endl;
   TokenResult *result = verify(&tokens);
   
   cout << "Unifying statements..." << endl;
   Statement *context = unify(nullptr, result);

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
      } else if(first == '\'') {
	 evaluateChar(text, &index, &tokens, length);
      } else {
         evaluateToken(text, &index, &tokens, length, &last);
      }
   }

   while(tokens.back().type == TokenType::NewLine) {
      tokens.pop_back();
   }

   Token endToken;
   endToken.type = TokenType::EoF;
   endToken.text = charcpy("<EOF>", 5);
   endToken.length = 5;
   endToken.index = tokens.size();
   tokens.push_back(endToken);

   return tokens;
}

void evaluateToken(char* text, int *index, vector<Token> *tokens, int length, TokenType *last) {
   Token token = nextToken(&text[*index], length - (*index));
   *last = token.type;
   *index += token.length;
   token.index = tokens->size();
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
   numberToken.length = buffer.length();
   numberToken.index = tokens->size();
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
   stringToken.length = strText.length();
   stringToken.text = charcpy(strText.c_str(), strText.length());
   stringToken.index = tokens->size();
   tokens->push_back(stringToken);
}

void evaluateChar(char* text, int *index, vector<Token> *tokens, int length) {
   Token charToken;
   charToken.type = TokenType::Char;
   charToken.index = tokens->size();

   int end = *index;
   for(int i = end; i < length; i++) {
      char c = text[i];
      end = i;
      if(c == '\0' || c == '\n') {
         break;
      }
   }

   charToken.text = charcpy(&text[(*index)], end - *index);
   charToken.length = end - *index;
   tokens->push_back(charToken);
   
   (*index)++;
   char *character;
   
   if(*index + 1 >= length) {
      verifyError(tokens, tokens->size() - 1, "Char was never terminated!");
   } else if(text[(*index)] == '\'') {
      verifyError(tokens, tokens->size() - 1, "Char was terminated without specifying a character!");
   }   

   if(text[(*index)] == '\\') {
      char next = text[(*index) + 1];
      if(next == 'u') {
         character = evaluateUnicode(text, index, tokens, length);
	 (*index) += 6;
      } else {
	 int escapedValue = byEscapedCharacter(next);
         if(escapedValue == -1) {
            verifyError(tokens, tokens->size() - 1, "Unknown escaped character!");
         }
         character = reinterpret_cast<char*>(malloc(2));
         character[0] = (char) escapedValue;
	 character[1] = '\0';
         (*index) += 2; 
      }
   } else {
      character = reinterpret_cast<char*>(malloc(2));
      character[0] = text[(*index)];
      character[1] = '\0';
      (*index)++;
   }
  
   if(text[(*index)] != '\'') {
      verifyError(tokens, tokens->size() - 1, "Expected single quote to terminate the character!");
   }

   (*index)++;
   
   charToken.length = strlen(character);
   charToken.text = charcpy(character, charToken.length);
   tokens->pop_back();
   tokens->push_back(charToken);
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
      case '\'': return '\'';
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

   char *placeholder = reinterpret_cast<char*>(malloc(3));

   if(value > 127) {
      placeholder[0] = (char) 194; // UTF-8 2 byte encoding
      placeholder[1] = (char) value;
      placeholder[2] = '\0';
   } else {
      placeholder[0] = (char) value;
      placeholder[1] = '\0';
   }

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
      case '<': token.type = TokenType::AngleBracketOpen; break;
      case '>': token.type = TokenType::AngleBracketClose; break;

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
   int basetypeLength = arrayFind(&basetypes, text, length);
   int keywordLength = arrayFind(&keywords, text, length);

   if(keywordLength + basetypeLength < 1) {
      return nextIdentifierToken(text, length);
   }

   Token token;
   token.type = basetypeLength == 0 ? TokenType::Keyword : TokenType::BaseType;
   token.length = keywordLength + basetypeLength;
   token.text = charcpy(text, token.length);
   return token;
}

/* verify */

TokenResult* verify(vector<Token> *tokens) {
   cout << "verify: " << endl;
   TokenResult *result = verifyContext(contextNode, tokens, 0);
   cout << "Parsing success: " << result->isSatisfied() << endl;
   return result;
}

TokenResult* verifyContext(TokenNode *node, vector<Token> *list, int index) {
   cout << "verifyContext: " << endl;
   TokenResult *result = new TokenResult(node, true);
   variant<TokenNode*, TokenType, const char*> terminator = "done";
   bool terminated = false;

   const char* error_eof = "Expected a new statement, but the file suddenly ended! What have you done?! °–°";
   const char* error_invalid_statement = "Expected a new statement, but the first token doesn't make any sense :c";
   const char* error_generic = "Invalid statement! An error occurred!";

   do {
      bool success = false;
      TokenResult *max = new TokenResult(nullptr, false);

      Token next = list->at(result->getTokenCount() + index);
      if(next.type == TokenType::EoF) {
	 if(index == 0) {
            return result;
	 }
         verifyError(list, index + result->getTokenCount(), error_eof);
      }

      TokenResult *terminatorResult = verifyVariant(&terminator, list, index + result->getTokenCount());

      if(terminatorResult->isSatisfied()) { 
	 terminated = true;
	 result->add(terminatorResult);
	 continue;
      }
      
      for(TokenNode& node : *ast) {
	 cout << "Trying to detect statement" << node.getName() << endl;
         TokenResult *subResult = verifyEachVariant(&node, list, index + result->getTokenCount());
         if(!subResult->isSatisfied()) {
	    if(subResult->getMatchedTokens() > max->getMatchedTokens()) {
	       max = subResult;
	       cout << "FAIL: UPDATED MAX: " << max->getMatchedTokens() << "TOKENS IN SUBRESULT" << endl;
	    } else {
               cout << "FAIL, BUT NOT UPDATING: UNSKIPPED: " << subResult->getMatchedTokens()  << endl; 
	    }   
	 } else {
	    cout << "STATEMENT SUCCESS: " << node.getName() << endl;
            success = true;
	    result->add(subResult);
	    break;
	 }
      }

      if(!success) {
	 if(max->isEof()) {
            verifyError(list, index + result->getTokenCount() + max->getTokenCount(), error_eof);
	 } else if(result->getTokenCount() < 1) {
            verifyError(list, index + result->getTokenCount() + max->getTokenCount(), error_invalid_statement);
	 } else {
            verifyError(list, index + result->getTokenCount() + max->getTokenCount(), max->hasMessage() ? max->getMessage() : error_generic);
	 }
      }

   } while(!terminated);

   cout << "MOVING 1 LAYER UP" << endl;
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

      if(subResult->isEof()) {
         result->setEof();
      }

      if(!subResult->isSatisfied()) {
	 cout << "RETURNING FAIL" << endl;
	 result->add(subResult);
	 result->setSatisfied(false);
	 if(subResult->hasMessage()) {
            result->setMessage(subResult->getMessage());
         }
	 return result;
      }
      
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

      if(tokens->at(index).type == TokenType::EoF) {
         cout << "RAN INTO EOF: THIS PART IS OPTIONAL THOUGH: IGNORING" << endl;
         result->setSatisfied(false);
         result->setEof();
	 return result;
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
                result->setMessage(charcpy(msg.c_str(), msg.length()));
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
            const char *nameOfType = getTokenTypeByInt(static_cast<int>(targetType)); 
	    cout << "'" << token.text << "' IS NOT CORRECT TOKENTYPE: " << nameOfType <<  endl;
            string msg;
	    msg += "This is not the correct token, expected type: ";
	    msg += static_cast<int>(targetType);
            result->setMessage(charcpy(msg.c_str(), msg.length()));
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

void debugBranchFail(variant<TokenNode*, TokenType, const char*> variantObj, TokenNode *node) {
   if(holds_alternative<TokenNode*>(variantObj)) {
      cout << "BRANCH FAIL ON: " << node->getName() << ": COULDN'T VERIFY BRANCH: " << get<TokenNode*>(variantObj)->getName() << endl;
   } else if(holds_alternative<TokenType>(variantObj)) {
      cout << "BRANCH FAIL ON: " << node->getName() << ": COULDN'T VERIFY BRANCH: Expected Token to be: " << getTokenTypeByInt(static_cast<int>(get<TokenType>(variantObj))) << endl;
   } else {
      cout << "BRANCH FAIL ON: " << node->getName() << ": COULDN'T VERIFY BRANCH: Expected Token to be keyword: " << get<const char*>(variantObj) << endl;
   }
}

TokenResult* verifyBranch(TokenNode *node, vector<Token> *tokens, int index) {
   TokenResult *max = new TokenResult(nullptr, false);
   for(variant<TokenNode*, TokenType, const char*> variantObj : *(node->getList())) {
      TokenResult *branchResult = verifyVariant(&variantObj, tokens, index);
      if(branchResult->isSatisfied()) {
         TokenResult *result = new TokenResult(node, true);
         result->add(branchResult);
	 return result;
      } else if(branchResult->getTokenCount() > max->getTokenCount()) {
	 max = branchResult;
         debugBranchFail(variantObj, node);
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
         if(resultAny->isEof()) {
            result->setEof();
	 }
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
   contextNode = (new TokenNode {})->withName("context"); 

   TokenNode *primitiveBare = (new TokenNode {
      "true",
      "false",
      TokenType::Identifier,
      TokenType::Number,
      TokenType::String,
      TokenType::Char
   })->inMode(TokenMode::BRANCH)->withName("primitivebare");

   TokenNode *primitive = (new TokenNode {
      primitiveBare
   })->withName("primitive")->inMode(TokenMode::BRANCH);
   
   TokenNode *enclosedPrimitive = (new TokenNode {
      TokenType::BracketOpen,
      primitive,
      TokenType::BracketClose
   })->withName("primitiveenclosed");

   primitive->getList()->insert(primitive->getList()->begin(), enclosedPrimitive);
   
   
   // ADDON OF TYPE
   
   TokenNode *type = (new TokenNode {})->inMode(TokenMode::BRANCH)->withName("type");

   TokenNode *typeGeneric = (new TokenNode {
      (new TokenNode {
         TokenType::BaseType,
	 TokenType::Identifier
      })->inMode(TokenMode::BRANCH),
      TokenType::AngleBracketOpen,
      (new TokenNode {
	      type,
	      (new TokenNode {
		 TokenType::Comma,
		 type
	      })->inMode(TokenMode::MORE_OR_NONE)
      })->inMode(TokenMode::ONCE_OR_NONE),
      TokenType::AngleBracketClose
   })->withName("typegeneric");

   type->getList()->push_back(typeGeneric);
   type->getList()->push_back(TokenType::BaseType);

   TokenNode *addonType = (new TokenNode {
      TokenType::Colon,
      type->inMode(TokenMode::BRANCH)
   })->withName("addontype");
 
   // FUNCTION

   TokenNode *parameterDeclaration = (new TokenNode {
      TokenType::Identifier,
      addonType->inMode(TokenMode::ONCE)
   })->withName("parameter");
   
   TokenNode *addonFunctionParameter = (new TokenNode {
      TokenType::Comma,
      parameterDeclaration
   })->withName("parameterseparated");

   TokenNode *functionArgumentList = (new TokenNode {
      parameterDeclaration,
      addonFunctionParameter->inMode(TokenMode::MORE_OR_NONE)
   })->withName("parameterlist");

   TokenNode *lambda = (new TokenNode {
      TokenType::BracketOpen,
      functionArgumentList->inMode(TokenMode::ONCE_OR_NONE),
      TokenType::BracketClose,
      TokenType::Colon,
      type->inMode(TokenMode::BRANCH),
      contextNode
   })->withName("lambda");
   
   TokenNode *function_ = (new TokenNode {
      "function",
      TokenType::Identifier,
      lambda
   })->withName("function");

   primitive->getList()->insert(primitive->getList()->begin(), lambda);

   // VALUE - Section Math

   
   TokenNode *mathOperand = (new TokenNode {
      // functionCallChain added here retroactively
      primitive
   })->inMode(TokenMode::BRANCH)->withName("operandmath");

   TokenNode *mathOperator = (new TokenNode {
		   TokenType::Plus,
		   TokenType::Minus,
		   TokenType::Star,
		   TokenType::Slash
   })->inMode(TokenMode::BRANCH)->withName("operatormath");

   TokenNode *mathConstructBare = (new TokenNode {
      mathOperand,
      mathOperator,
      mathOperand,
      (new TokenNode {
         mathOperator,
	 mathOperand
      })->inMode(TokenMode::MORE_OR_NONE)->withName("addonoperatorandoperandpair")
   })->withName("constructmath");

   TokenNode *mathWrapper = (new TokenNode {
      mathConstructBare
   })->withName("mathwrapper")->inMode(TokenMode::BRANCH);

   TokenNode *mathConstructEnclosed = (new TokenNode {
      TokenType::BracketOpen,
      mathWrapper,
      TokenType::BracketClose
   })->withName("constructmathenclosed");

   mathWrapper->getList()->insert(mathWrapper->getList()->end(), mathConstructEnclosed);
   mathOperand->getList()->insert(mathOperand->getList()->end(), mathConstructEnclosed);

   TokenNode *math = (new TokenNode {
      mathConstructBare,
      mathConstructEnclosed
   })->withName("math")->inMode(TokenMode::BRANCH);


   // Value - Section Compare


   TokenNode *compareOperand = (new TokenNode {
      math,
      mathOperand
   })->withName("operandcompare")->inMode(TokenMode::BRANCH);

   TokenNode *compareOperator = (new TokenNode {
      (new TokenNode { TokenType::Equals, TokenType::Equals })->withName("equals"),
      (new TokenNode { TokenType::AngleBracketClose, TokenType::Equals })->withName("greaterequals"),
      (new TokenNode { TokenType::AngleBracketOpen, TokenType::Equals })->withName("smallerequals"),
      (new TokenNode { TokenType::AngleBracketClose })->withName("greater"),
      (new TokenNode { TokenType::AngleBracketOpen })->withName("smaller")
   })->withName("operatorcompare")->inMode(TokenMode::BRANCH);

   TokenNode *compareConstructBare = (new TokenNode {
      compareOperand,
      compareOperator,
      compareOperand
   })->withName("constructcomparebare");

   TokenNode *compare = (new TokenNode {
      compareConstructBare
   })->withName("compare")->inMode(TokenMode::BRANCH);
   
   TokenNode *compareConstructEnclosed = (new TokenNode {
      TokenType::BracketOpen,
      compare,
      TokenType::BracketClose
   })->withName("constructcompareenclosed");

   compare->getList()->insert(compare->getList()->end(), compareConstructEnclosed);
   
   // Value - Section Logic

   TokenNode *not_ = (new TokenNode {
      TokenType::Not
   })->inMode(TokenMode::ONCE_OR_NONE)->withName("not");

   TokenNode *logicOperand = (new TokenNode {
      // functionCallChain added here retroactively
      compare,
      "true",
      "false",
      TokenType::Identifier
   })->inMode(TokenMode::BRANCH)->withName("operandlogic");

   TokenNode *logicOperator = (new TokenNode {
      (new TokenNode { TokenType::And, TokenType::And })->withName("and"),
      (new TokenNode { TokenType::Or, TokenType::Or })->withName("or")
   })->withName("operatorlogic")->inMode(TokenMode::BRANCH);

   TokenNode *logicConstructBare = (new TokenNode {
      not_,
      logicOperand,
      logicOperator,
      not_,
      logicOperand,
      (new TokenNode {
         logicOperator,
	 not_,
	 logicOperand
      })->inMode(TokenMode::MORE_OR_NONE)
   })->withName("constructlogic");

   TokenNode *logic = (new TokenNode {
      logicConstructBare
   })->withName("logic")->inMode(TokenMode::BRANCH);
   
   TokenNode *logicConstructEnclosed = (new TokenNode {
      TokenType::BracketOpen,
      logic,
      TokenType::BracketClose
   })->withName("constructlogicenclosed");

   logic->getList()->insert(logic->getList()->end(), logicConstructEnclosed);
   logicOperand->getList()->insert(logicOperand->getList()->end(), logicConstructEnclosed);


   // Value - Section Value Object 

   TokenNode *valueBare = (new TokenNode {
      logic,
      compare,
      math,
      primitive
   })->inMode(TokenMode::BRANCH)->withName("valuebare");
  
   TokenNode *value = (new TokenNode {
      valueBare
   })->withName("value")->inMode(TokenMode::BRANCH);
   
   TokenNode *enclosedValue = (new TokenNode {
      TokenType::BracketOpen,
      value,
      TokenType::BracketClose
   })->withName("valueenclosed");

   value->getList()->insert(value->getList()->end(), enclosedValue);

   // ARRAY
   
   TokenNode *arrayArgument = (new TokenNode {
      TokenType::Comma,
      value
   })->withName("arrayargument")->inMode(TokenMode::MORE_OR_NONE);

   TokenNode *arrayBody = (new TokenNode {
      value,
      arrayArgument
   })->withName("arraybody")->inMode(TokenMode::ONCE_OR_NONE);

   TokenNode *array = (new TokenNode {
      TokenType::SquareBracketOpen,
      arrayBody,
      TokenType::SquareBracketClose
   })->withName("array");

   primitive->getList()->insert(primitive->getList()->end(), array);

   // INVOKE

   TokenNode *addonFunctionArgument = (new TokenNode {
      TokenType::Comma,
      value
   })->withName("functionargumentaddon");


   TokenNode *functionArgument = (new TokenNode {
      value,
      addonFunctionArgument->inMode(TokenMode::MORE_OR_NONE)
   })->withName("functionArgument");

   TokenNode *functionCall = (new TokenNode {
      TokenType::Colon,
      TokenType::Colon,
      TokenType::Identifier,
      TokenType::BracketOpen,
      functionArgument->inMode(TokenMode::ONCE_OR_NONE),
      TokenType::BracketClose
   })->withName("functioncallsingle");
   
   TokenNode *domainOrValue = (new TokenNode {
      (new TokenNode {
         TokenType::Identifier,
         TokenType::BaseType
      })->inMode(TokenMode::BRANCH)
   })->withName("domainorvalue");

   TokenNode *functionCallChain = (new TokenNode {
      domainOrValue->inMode(TokenMode::ONCE_OR_NONE),
      functionCall->inMode(TokenMode::ONCE_OR_MORE)
   })->withName("functionCallChain");
   
   TokenNode *invoke_ = (new TokenNode {
      "invoke",
      functionCallChain->inMode(TokenMode::ONCE)
   })->withName("invoke");
   
   mathOperand->getList()->insert(mathOperand->getList()->begin(), functionCallChain);
   compareOperand->getList()->insert(compareOperand->getList()->begin(), functionCallChain);
   logicOperand->getList()->insert(logicOperand->getList()->begin(), functionCallChain);

   // IF (ELSE)

   TokenNode *else_ = (new TokenNode {
      "else",
      contextNode
   })->withName("else");

   TokenNode *if_ = (new TokenNode {
      "if",
      value,
      TokenType::Colon,
      contextNode
   })->withName("if");

   TokenNode *elseIf = (new TokenNode {
      "else",
      if_,
   })->withName("elseif");

   if_->getList()->insert(if_->getList()->end(), elseIf->inMode(TokenMode::MORE_OR_NONE));
   if_->getList()->insert(if_->getList()->end(), else_->inMode(TokenMode::ONCE_OR_NONE));

   // WHILE
   
   TokenNode *while_ = (new TokenNode {
      "while",
      value,
      TokenType::Colon,
      contextNode
   })->withName("while");

   // SET

   TokenNode *set_ = (new TokenNode {
      "set",
      TokenType::Identifier,
      "to",
      value
   })->withName("set");

   // CREATE

   TokenNode *addonValueAssign = (new TokenNode {
      "set",
      "to",
      value
   })->withName("createset");

   TokenNode *create_ = (new TokenNode {
      "create",
      TokenType::Identifier,
      (new TokenNode {
         addonType->inMode(TokenMode::ONCE_OR_NONE),
         addonValueAssign->inMode(TokenMode::ONCE_OR_NONE)
      })->inMode(TokenMode::BRANCH)
   })->withName("create");

   // DELETE

   TokenNode *delete_ = (new TokenNode {
      "delete",
      TokenType::Identifier
   })->withName("delete");

   // FOR

   TokenNode *for_ = (new TokenNode {
      "for",
      "until",
      (new TokenNode {
         (new TokenNode { "above", "below" })->inMode(TokenMode::BRANCH)
      })->inMode(TokenMode::ONCE_OR_NONE),
      value,
      (new TokenNode {
         (new TokenNode { "up", "down" })->inMode(TokenMode::BRANCH)
      })->inMode(TokenMode::ONCE_OR_NONE),
      value,
      (new TokenNode { 
         "set",
         TokenType::Identifier,
      })->inMode(TokenMode::ONCE_OR_NONE)->withName("forset"),
      TokenType::Colon,
      contextNode
   })->withName("for");

   // INC(REMENT)
   
   TokenNode *inc_ = (new TokenNode {
      (new TokenNode {
         "inc", "increment"
      })->inMode(TokenMode::BRANCH),
      TokenType::Identifier
   })->withName("increment");

   // DEC(REMENT)
   
   TokenNode *dec_ = (new TokenNode {
      (new TokenNode {
         "dec", "decrement"
      })->inMode(TokenMode::BRANCH),
      TokenType::Identifier
   })->withName("decrement");

   // EXIT

   TokenNode *exit_ = (new TokenNode {
      "exit",
      value
   })->withName("exit");

   // RETURN
   
   TokenNode *return_ = (new TokenNode {
      "return",
      value
   })->withName("return");

   ast->push_back(*return_);
   ast->push_back(*exit_);
   ast->push_back(*dec_);
   ast->push_back(*inc_);
   ast->push_back(*for_);
   ast->push_back(*delete_);
   ast->push_back(*create_);
   ast->push_back(*set_);
   ast->push_back(*while_);
   ast->push_back(*if_);
   ast->push_back(*invoke_);
   ast->push_back(*function_);
}

void verifyError(vector<Token> *tokens, int index, const char *text) {
   cout << endl;
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
   cerr << "Error is located in line " << lineCount << ": " << endl << endl;
   
   int lineErrorIndex = 0;

   for(int i = tokenIndex; i < tokens->size(); i++) {
      if(i <= index) {
         lineErrorIndex += (*tokens)[i].length;
      }
      if(tokens->at(i).type == TokenType::NewLine || tokens->at(i).type == TokenType::EoF) {
	 if(tokens->at(i).type == TokenType::EoF) {
            cerr << tokens->at(i).text;
	 }
	 break;
      }
      if(tokens->at(i).type == TokenType::String) {
         cerr << "\"" <<(*tokens)[i].text << "\"";
      } else if(tokens->at(i).type == TokenType::Char) {
         cerr << "'" <<(*tokens)[i].text << "'";
      } else {
         cerr << (*tokens)[i].text;
      }
   }

   cerr << endl;

   for(int i = 0; i < lineErrorIndex - 1; i++) {
      cerr << "-";
   }

   cerr << "^" << endl;
   exit(1);
}

/* unify */

Statement* unify(Statement* owner, TokenResult *result) {
  if(owner == nullptr) {
     owner = new Statement("root");
  }

  for(variant<TokenResult*, Token> statement : *(result->getTokens())) {
    if(holds_alternative<Token>(statement)) {
       verifyError(tokenList, get<Token>(statement).index, "Expected statement, not individual token!");
    }
    unifyStatement(get<TokenResult*>(statement));
  }
  return nullptr;
}

Type unifyType(TokenResult *result) {
   
}

Statement* unifyStatement(TokenResult *result) {
   //TokenNode node; 
   string name = result->getNode()->getName();

   if("create" == name) { return unifyCreate(result); } else  
   if("delete" == name) { return unifyCreate(result); } else  
   if("for" == name) { return unifyCreate(result); } else  
   if("set" == name) { return unifyCreate(result); } else  
   if("while" == name) { return unifyCreate(result); } else  
   if("if" == name) { return unifyCreate(result); } else  
   if("invoke" == name) { return unifyCreate(result); } else  
   if("function" == name) { return unifyCreate(result); } else  
   if("return" == name) { return unifyCreate(result); } else  
   if("exit" == name) { return unifyCreate(result); } else  
   if("increment" == name) { return unifyCreate(result); } else  
   if("decrement" == name) { return unifyCreate(result); } else {
      verifyError(tokenList, result->getFirstToken()->index, "Unexpected statement!");
   }
   return nullptr;
}

Statement* unifyCreate(TokenResult *result) {
   cout << "Unify: " << "create" << endl;
   verifyError(tokenList, result->getFirstToken()->index, "Unexpected statement!");
   Token *identifier = &(get<Token>(result->getTokens()->at(1)));
   
   return nullptr;
}

Statement* unifyDelete(TokenResult *result);
Statement* unifySet(TokenResult *result);
Statement* unifyFor(TokenResult *result);
Statement* unifyWhile(TokenResult *result);
Statement* unifyIf(TokenResult *result);
Statement* unifyFunction(TokenResult *result);
Statement* unifyLambda(TokenResult *result);
Statement* unifyInvoke(TokenResult *result);
Statement* unifyIncrement(TokenResult *result);
Statement* unifyDecrement(TokenResult *result);
Statement* unifyExit(TokenResult *context);

Statement* unifyValue(TokenResult *value) {
   cout << "Unifying value..." << endl;
   Unresolved unresolved(value);
   vector<Token>* references = unresolved.getReferences();
   for(Token& t : *references) {
      cout << t.text << endl;
   }
   return nullptr;
}

Type* unifyType(TokenResult *result) {
   TokenNode *node = &(result->getNode());
   
   if(node == nullptr) { 
      Token typeToken = result->getFirstToken();
      Type *type = new Type(unifyBaseType(typeToken));
      
   } else if(string(node->getName()) == "type") {
      Token typeToken = result->getFirstToken();
      return new Type(unifyBaseType(typeToken));
   } else {
      verifyError(tokenList, result->getFirstToken()->index, "Error unifying Type: Expected type or generic type!");
      return nullptr;
   }
}

Type* unfiyTypeGeneric(TokenResult *result) {
   Token typeToken = result->getFirstToken();
   Type *type = new Type(unifyBaseType(typeToken));
   TokenResult* 
}

variant<BaseType, const char*>* unifyBaseType(Token *typeToken) {
   if(typeToken->type == TokenType::Identifier) {
      return new variant<BaseType, const char*>(typeToken->text);
   } else {
      string type = typeToken->text;
      if(type == "bool") { return BaseType::Bool; } else
      if(type == "char") { return BaseType::Char; } else
      if(type == "byte") { return BaseType::Byte; } else
      if(type == "int") { return BaseType::Int; } else
      if(type == "float") { return BaseType::Float; } else
      if(type == "double") { return BaseType::Double; } else
      if(type == "long") { return BaseType::Long; } else
      if(type == "void") { return BaseType::Void; } else {
         verifyError(tokenList, typeToken->index, "Error unifying BaseType: Expected one of [bool, char, byte, int, float, double, long, void]!");
         return nullptr;
      }
   }
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

const char* getTokenTypeByInt(int num) {
   const char* text = "UNKNOWN_TOKEN_TYPE_BY_INT";
   
   switch(num) {
      case 0: text = "BaseType"; break;
      case 1: text = "Keyword"; break;
      case 2: text = "Identifier"; break;
      case 3: text = "Number"; break;
      case 4: text = "String"; break;
      case 5: text = "BracketOpen"; break;
      case 6: text = "BracketClose"; break;
      case 7: text = "SquareBracketOpen"; break;
      case 8: text = "SquareBracketClose"; break;
      case 9: text = "BraceOpen"; break;
      case 10: text = "BraceClose"; break;
      case 11: text = "AngleBracketOpen"; break;
      case 12: text = "AngleBracketClose"; break;
      case 13: text = "Plus"; break;
      case 14: text = "Minus"; break;
      case 15: text = "Slash"; break;
      case 16: text = "Star"; break;
      case 17: text = "QuoteSingle"; break;
      case 18: text = "QuoteDouble"; break;
      case 19: text = "Hashtag"; break;
      case 20: text = "Colon"; break;
      case 21: text = "Dollar"; break;
      case 22: text = "Space"; break;
      case 23: text = "Point"; break;
      case 24: text = "Comma"; break;
      case 25: text = "And"; break;
      case 26: text = "Or"; break;
      case 27: text = "Equals"; break;
      case 28: text = "Not"; break;
      case 29: text = "Separator"; break;
      case 30: text = "NewLine"; break;
      case 31: text = "Char"; break;
      case 32: text = "Other"; break;
      case 33: text = "EoF"; break;
   }

   return text;
}
/* todo */
