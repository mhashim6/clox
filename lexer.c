#include <stdio.h>
#include <string.h>

#include "common.h"
#include "lexer.h"

Lexer lexer;

void initLexer(const char *source) {
  lexer.start = source;
  lexer.current = source;
  lexer.line = 1;
}

static bool isAtEnd() { return *lexer.current == '\0'; }

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = lexer.start;
  token.length = (int)(lexer.current - lexer.start);
  token.line = lexer.line;

  return token;
}
static Token errorToken(char *message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = lexer.line;

  return token;
}

static char advance() {
  lexer.current++;
  return lexer.current[-1];  // who'd have thought C supports this!
}

static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*lexer.current != expected) return false;

  lexer.current++;
  return true;
}

static char peek() { return *lexer.current; }
static char peekNext() {
  if (isAtEnd()) return '\0';
  return lexer.current[1];
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        lexer.line++;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {  // TODO: support `/**/ ` comments.
          return;
        }
        break;

      default:
        return;
    }
  }
}

static Token string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '/n') lexer.line++;
    advance();
  }
  if (isAtEnd()) return errorToken("Unterminated string.");

  return makeToken(TOKEN_STRING);
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }

static Token number() {
  while (isDigit(peek())) advance();
  if (peek() == '.' && isDigit(peekNext())) {
    advance();
    while (isDigit(peek())) advance();
  }
  return makeToken(TOKEN_NUMBER);
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {
  if (lexer.current - lexer.start == start + length &&
      memcmp(lexer.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  switch (lexer.start[0]) {
    case 'a':
      return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c':
      return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
      return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'i':
      return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n':
      return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
      return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p':
      return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
      return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
      return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 'v':
      return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
      return checkKeyword(1, 4, "hile", TOKEN_WHILE);

    case 'f':
      switch (lexer.start[1]) {
        case 'a':
          return checkKeyword(2, 3, "lse", TOKEN_FALSE);
        case 'o':
          return checkKeyword(2, 1, "r", TOKEN_FOR);
        case 'u':
          return checkKeyword(2, 1, "n", TOKEN_FUN);
      }
    case 't':
      switch (lexer.start[1]) {
        case 'h':
          return checkKeyword(2, 2, "is", TOKEN_THIS);
        case 'r':
          return checkKeyword(2, 2, "ue", TOKEN_TRUE);
      }
  }
  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlpha(peek()) && isDigit(peek())) advance();
  return makeToken(identifierType());
}

Token scanToken() {
  skipWhitespace();

  lexer.start = lexer.current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  if (isDigit(c)) return number();
  if (isAlpha(c)) return identifier();
  switch (c) {
    case '(':
      return makeToken(TOKEN_LEFT_PAREN);
      break;
    case ')':
      return makeToken(TOKEN_RIGHT_PAREN);
      break;
    case '{':
      return makeToken(TOKEN_LEFT_BRACE);
      break;
    case '}':
      return makeToken(TOKEN_RIGHT_BRACE);
      break;
    case ';':
      return makeToken(TOKEN_SEMICOLON);
      break;
    case ',':
      return makeToken(TOKEN_COMMA);
      break;
    case '.':
      return makeToken(TOKEN_DOT);
      break;
    case '-':
      return makeToken(TOKEN_MINUS);
      break;
    case '+':
      return makeToken(TOKEN_PLUS);
      break;
    case '/':
      return makeToken(TOKEN_SLASH);
      break;
      /******************************/

    case '*':
      return makeToken(TOKEN_STAR);
      break;
    case '!':
      return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
      break;
    case '=':
      return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
      break;
    case '<':
      return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
      break;
    case '>':
      return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
      break;

      /******************************/

    case '"':
      return string();
      break;

    default:
      return errorToken("Unexpected character.");
  }
}