#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "object.h"
#include "parser.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

Parser parser;

Chunk* compilingChunk;
static Chunk* currentChunk() { return compilingChunk; }

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;

  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void error(const char* message) { errorAt(&parser.previous, message); }

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scanToken();
    if (parser.current.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;

    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;

      default:
          // Do nothing.
          ;
    }

    advance();
  }
}

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}
static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void endCompiler() {
  emitByte(OP_RETURN);

#ifdef DEBUG_PRINT_CODE
  if (!parser.hadError) {
    disassembleChunk(currentChunk(), "code");
  }
#endif
}

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = PARSE_RULE(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }
  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= PARSE_RULE(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = PARSE_RULE(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
    expression();
  }
}

void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static uint8_t identifierConstant(Token* name) {
  return makeConstant(LOX_OBJ(copyString(name->start, name->length)));
}

static uint8_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);
  return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
  emitBytes(OP_DEFINE_GLOBAL, global);
}

static void varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global);
}

void expressionStatement() {
  expression();
  emitByte(OP_POP);
  consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
}

static void printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

void declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();
}

void statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else {
    expressionStatement();
  }
}

static void namedVariable(Token name, bool canAssign) {
  int arg = identifierConstant(&name);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_GLOBAL, (uint8_t)arg);
  } else {
    emitBytes(OP_GET_GLOBAL, (uint8_t)arg);
  }
}

void variable(bool canAssign) { namedVariable(parser.previous, canAssign); }

static void emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
  emitConstant(LOX_NUMBER(value));
}

void string(bool canAssign) {
  emitConstant(LOX_OBJ(
      copyString(parser.previous.start + 1, parser.previous.length - 2)));

  // `+1` & `-2` to trim qoutes.
}

void literal(bool canAssign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE:
      emitByte(OP_FALSE);
      break;
    case TOKEN_NIL:
      emitByte(OP_NIL);
      break;
    case TOKEN_TRUE:
      emitByte(OP_TRUE);
      break;
    default:
      return;
  }
}

void binary(bool canAssign) {
  TokenType operator= parser.previous.type;

  // Compile the right operand.
  ParseRule* rule = PARSE_RULE(operator);
  parsePrecedence((Precedence)(rule->precedence + 1));

  // Emit the operator instruction.
  switch (operator) {
    case TOKEN_BANG_EQUAL:
      emitBytes(OP_EQUAL, OP_NOT);
      break;
    case TOKEN_EQUAL_EQUAL:
      emitByte(OP_EQUAL);
      break;
    case TOKEN_GREATER:
      emitByte(OP_GREATER);
      break;
    case TOKEN_GREATER_EQUAL:
      emitBytes(OP_LESS, OP_NOT);
      break;
    case TOKEN_LESS:
      emitByte(OP_LESS);
      break;
    case TOKEN_LESS_EQUAL:
      emitBytes(OP_GREATER, OP_NOT);
      break;

    case TOKEN_PLUS:
      emitByte(OP_ADD);
      break;
    case TOKEN_MINUS:
      emitByte(OP_SUBTRACT);
      break;
    case TOKEN_STAR:
      emitByte(OP_MULTIPLY);
      break;
    case TOKEN_SLASH:
      emitByte(OP_DIVIDE);
      break;
    default:
      return;
  }
}

void unary(bool canAssign) {
  TokenType operator= parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operator) {
    case TOKEN_BANG:
      emitByte(OP_NOT);
      break;

    case TOKEN_MINUS:
      emitByte(OP_NEGATE);
      break;
    default:
      return;
  }
}

void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

bool compile(const char* source, Chunk* chunk) {
  initLexer(source);

  compilingChunk = chunk;

  parser.hadError = false;
  parser.panicMode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  endCompiler();

  return !parser.hadError;
}