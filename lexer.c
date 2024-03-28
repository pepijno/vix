#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TOKEN_IDENTIFIER,
  TOKEN_ASSIGN,
  TOKEN_DOT,
  TOKEN_COMMA,
  TOKEN_STRING,
  TOKEN_OPEN_BRACE,
  TOKEN_CLOSE_BRACE,
  TOKEN_OPEN_PAREN,
  TOKEN_CLOSE_PAREN,
  TOKEN_GREATER_THAN,
  TOKEN_EOF
} TokenType;

typedef struct {
  TokenType type;
  char *content;
} Token;

typedef struct {
  char *input;
  int start;
  int current;
  int line;
} Lexer;

typedef struct {
  int size;
  int current;
  Token *tokens;
} Tokens;

Tokens init_tokens(int init_size) {
  return (Tokens){.size = init_size,
                  .current = 0,
                  .tokens = malloc(init_size * sizeof(Token))};
}

void tokens_add(Tokens *tokens, Token *token) {
  if (tokens->current == tokens->size) {
    tokens->tokens = realloc(tokens->tokens, tokens->size * 2 * sizeof(Token));
    tokens->size *= 2;
  }
  tokens->tokens[tokens->current] = *token;
  tokens->current++;
}

Lexer init_lexer(char *input) {
  Lexer lexer;
  lexer.input = input;
  lexer.start = 0;
  lexer.current = 0;
  lexer.line = 1;
  return lexer;
}

bool lexer_is_at_end(Lexer *lexer) {
  return lexer->current >= strlen(lexer->input);
}

char lexer_peek(Lexer *lexer) {
  if (lexer_is_at_end(lexer)) {
    return '\0';
  }
  return lexer->input[lexer->current];
}

char lexer_advance(Lexer *lexer) {
  char c = lexer->input[lexer->current];
  lexer->current++;
  return c;
}

void lexer_string(Lexer *lexer, Tokens *tokens) {
  while (lexer_peek(lexer) != '"' && !lexer_is_at_end(lexer)) {
    if (lexer_peek(lexer) == '\n') {
      lexer->line++;
    }
    lexer_advance(lexer);
  }

  if (lexer_is_at_end(lexer)) {
    printf("Unterminated string.");
    return;
  }

  // the closing '"'
  lexer_advance(lexer);

  int size = (lexer->current - 1) - (lexer->start + 1);
  char *content = malloc(size * sizeof(char));
  strncpy(content, lexer->input + lexer->start + 1, size);
  Token token = {.type = TOKEN_STRING, .content = content};
  tokens_add(tokens, &token);
}

bool is_allowed_identifier_char(char c) {
  return isalnum(c) || c == '_';
}

void lexer_identifier(Lexer *lexer, Tokens *tokens) {
  while (is_allowed_identifier_char(lexer_peek(lexer))) {
    lexer_advance(lexer);
  }

  int size = lexer->current - lexer->start;
  char *content = malloc(size * sizeof(char));
  strncpy(content, lexer->input + lexer->start, size);
  Token token = {.type = TOKEN_IDENTIFIER, .content = content};
  tokens_add(tokens, &token);
}

void lexer_next_token(Lexer *lexer, Tokens *tokens) {
  char c = lexer_advance(lexer);
  switch (c) {
  case '{':
    tokens_add(tokens, &(Token){.type = TOKEN_OPEN_BRACE});
    break;
  case '}':
    tokens_add(tokens, &(Token){.type = TOKEN_CLOSE_BRACE});
    break;
  case '=':
    tokens_add(tokens, &(Token){.type = TOKEN_ASSIGN});
    break;
  case '>':
    tokens_add(tokens, &(Token){.type = TOKEN_GREATER_THAN});
    break;
  case '.':
    tokens_add(tokens, &(Token){.type = TOKEN_DOT});
    break;
  case ',':
    tokens_add(tokens, &(Token){.type = TOKEN_COMMA});
    break;
  case '(':
    tokens_add(tokens, &(Token){.type = TOKEN_OPEN_PAREN});
    break;
  case ')':
    tokens_add(tokens, &(Token){.type = TOKEN_CLOSE_PAREN});
    break;
  case '"':
    lexer_string(lexer, tokens);
    break;
  case ' ':
  case '\r':
  case '\t':
    // ignore whitespace
    break;
  case '\n':
    lexer->line++;
    break;
  default:
    if (is_allowed_identifier_char(c)) {
        lexer_identifier(lexer, tokens);
    } else {
      printf("Unexpected character: %c\n", c);
      }
    break;
  }
}

void lexer_scan_tokens(Lexer *lexer, Tokens *tokens) {
  while (!lexer_is_at_end(lexer)) {
    lexer->start = lexer->current;
    lexer_next_token(lexer, tokens);
  }
  tokens_add(tokens, &(Token){.type = TOKEN_EOF});
}

void print_token(Token *token) {
  switch (token->type) {
  case TOKEN_IDENTIFIER:
    printf("TOKEN_IDENTIFIER: %s\n", token->content);
    break;
  case TOKEN_ASSIGN:
    printf("TOKEN_ASSIGN\n");
    break;
  case TOKEN_STRING:
    printf("TOKEN_STRING: %s\n", token->content);
    break;
  case TOKEN_OPEN_BRACE:
    printf("TOKEN_OPEN_BRACE\n");
    break;
  case TOKEN_CLOSE_BRACE:
    printf("TOKEN_CLOSE_BRACE\n");
    break;
  case TOKEN_GREATER_THAN:
    printf("TOKEN_GREATER_THAN\n");
    break;
  case TOKEN_OPEN_PAREN:
    printf("TOKEN_OPEN_PAREN\n");
    break;
  case TOKEN_CLOSE_PAREN:
    printf("TOKEN_CLOSE_PAREN\n");
    break;
  case TOKEN_DOT:
    printf("TOKEN_DOT\n");
    break;
  case TOKEN_COMMA:
    printf("TOKEN_COMMA\n");
    break;
  case TOKEN_EOF:
    printf("TOKEN_EOF\n");
    break;
  }
}

typedef struct {
  Tokens* tokens;
  int current;
} Parser;

Parser init_parser(Tokens* tokens) {
  return (Parser){ .tokens = tokens, .current = 0 };
}

bool parser_is_at_end(Parser* parser) {
  return parser->current == parser->tokens->size;
}

Token parser_peek(Parser* parser) {
  return parser->tokens->tokens[parser->current];
}

Token parser_previous(Parser* parser) {
  return parser->tokens->tokens[parser->current - 1];
}

Token parser_advance(Parser* parser) {
  if (!parser_is_at_end(parser)) {
    parser->current++;
  }
  return parser_previous(parser);
}

bool parser_check(Parser* parser, TokenType type) {
  if (parser_is_at_end(parser)) {
    return false;
  } else {
    return parser_peek(parser).type == type;
  }
}

bool match(Parser* parser, int count, ...) {
  va_list argp;
  va_start(argp, count);
  for(int i = 0; i < count; ++i) {
    TokenType tokenType = va_arg(argp, TokenType);
    if (parser_check(parser, tokenType)) {
      parser_advance(parser);
      return true;
    }
  }
  va_end(argp);
  return false;
}

void parse_object(Parser* parser) {
}

int main() {
  char input[] = "person = age > { first_name_length = \"John\".length\n    last_name = my_last_name(\"Doe\", \"Doe\") }";
  Lexer lexer = init_lexer(input);
  Tokens tokens = init_tokens(1);
  lexer_scan_tokens(&lexer, &tokens);
  for (int i = 0; i < tokens.size; ++i) {
    print_token(&tokens.tokens[i]);
    if (tokens.tokens[i].type == TOKEN_EOF) {
      break;
    }
  }
  Parser parser = init_parser(&tokens);
  return 0;
}
