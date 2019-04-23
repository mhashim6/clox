#ifndef clox_parser_h
#define clox_parser_h

#include "common.h"
#include "lexer.h"

typedef struct {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . () []
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

void grouping(bool canAssign);
void expression();
void declaration();
void statement();
void variable(bool canAssign);
void binary(bool canAssign);
void unary(bool canAssign);
void number(bool canAssign);
void string(bool canAssign);
void literal(bool canAssign);

#define PARSE_RULES_LENGTH 40
extern const ParseRule parsRules[PARSE_RULES_LENGTH];

#define PARSE_RULE(type) (&parsRules[type])

#endif
