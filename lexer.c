#include "list.h"
#include "buffer.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((cold)) __attribute__((noreturn))
__attribute__((format(printf, 1, 2))) void
panic(char *format, ...);

void unreachable(void) { panic("unreachable"); }

typedef enum {
  TOKEN_IDENTIFIER,
  TOKEN_ASSIGN,
  TOKEN_DOT,
  TOKEN_COMMA,
  TOKEN_STRING_LITERAL,
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
  size_t start;
  size_t current;
  size_t line;
} Lexer;

typedef struct {
  size_t size;
  size_t current;
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
  Token token = {.type = TOKEN_STRING_LITERAL, .content = content};
  tokens_add(tokens, &token);
}

bool is_allowed_identifier_char(char c) { return isalnum(c) || c == '_'; }

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
  case TOKEN_STRING_LITERAL:
    printf("TOKEN_STRING_LITERAL: %s\n", token->content);
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

typedef enum {
  NODE_STRING_LITERAL,
  NODE_OBJECT,
  NODE_PROPERTY,
  NODE_IDENTIFIER,
  NODE_FREE_OBJECT_COPY,
  NODE_OBJECT_PROPERTY_ACCESS,
  NODE_ROOT
} NodeType;

typedef struct {
  Tokens *tokens;
  int current;
} Parser;

void ast_error(Parser *parser, Token *token, char *format, ...) {
  va_list ap;
  va_start(ap, format);
  // char *msg = printf(format, ap);
  va_end(ap);
}

struct AstNode;

typedef struct {
  char *content;
} AstStringLiteral;

typedef struct {
  char *content;
} AstIdentifier;

typedef struct {
  LIST_OF(struct AstNode*) free_list;
  LIST_OF(struct AstNode*) property_list;
} AstObject;

typedef struct {
  struct AstNode *identifier;
  struct AstNode *object_result;
} AstProperty;

typedef struct {
  struct AstNode *object;
  struct AstNode *property;
} AstObjectPropertyAccess;

typedef struct {
  struct AstNode *object;
  LIST_OF(struct AstNode*) property_list;
} AstFreeObjectCopy;

typedef struct {
  LIST_OF(struct AstNode*) list;
} AstRoot;

typedef struct AstNode {
  NodeType type;
  int line;
  int column;
  union {
    AstStringLiteral string_literal;
    AstObject object;
    AstProperty property;
    AstIdentifier identifier;
    AstObjectPropertyAccess object_property_access;
    AstFreeObjectCopy free_object_copy;
    AstRoot root;
  } data;
} AstNode;

AstNode* init_node(NodeType type) {
  AstNode* node = malloc(sizeof(AstNode));
  node->type = type;
  node->line = 0;
  node->column = 0;
  return node;
}

Parser init_parser(Tokens *tokens) {
  return (Parser){.tokens = tokens, .current = 0};
}

AstNode *parse_property(Parser *parser, size_t *token_index);
AstNode *parse_object_result(Parser *parser, size_t *token_index);

// identifier ::= [A-Za-z][A-Za-z0-9_]*
AstNode *parse_identifier(Parser *parser, size_t *token_index) {
  Token *token = &(parser->tokens->tokens[*token_index]);
  if (token->type == TOKEN_IDENTIFIER) {
    *token_index += 1;
    AstNode* node = init_node(NODE_IDENTIFIER);
    node->data.identifier.content = token->content;
    return node;
  } else {
    // TODO error;
    return NULL;
  }
}

// object ::= (identifier+ ">")? "{" property* "}"
AstNode *parse_object(Parser *parser, size_t *token_index) {
  Token *token = &(parser->tokens->tokens[*token_index]);

  if (token->type != TOKEN_IDENTIFIER && token->type != TOKEN_OPEN_BRACE) {
    return NULL;
  }

  AstNode *node = init_node(NODE_OBJECT);
  LIST_INIT(node->data.object.free_list);
  LIST_INIT(node->data.object.property_list);

  while (token->type == TOKEN_IDENTIFIER) {
    AstNode *identifier = parse_identifier(parser, token_index);
    if (!identifier) {
      // TODO error
      return NULL;
    } else {
      LIST_PUSH_BACK(node->data.object.free_list, identifier);
    }
    token = &(parser->tokens->tokens[*token_index]);
  }

  if (LIST_IS_NON_EMPTY(node->data.object.free_list)) {
    if (token->type == TOKEN_GREATER_THAN) {
      *token_index += 1;
      token = &(parser->tokens->tokens[*token_index]);
    } else {
      // TODO error
      return NULL;
    }
  }

  if (token->type == TOKEN_OPEN_BRACE) {
    *token_index += 1;
  } else {
    // TODO error
    return NULL;
  }

  while (true) {
    AstNode *property = parse_property(parser, token_index);
    if (property) {
      LIST_PUSH_BACK(node->data.object.property_list, property);
    }

    token = &(parser->tokens->tokens[*token_index]);
    if (token->type == TOKEN_CLOSE_BRACE) {
      *token_index += 1;
      return node;
    } else if (property) {
      continue;
    } else {
      return NULL;
      // TODO error
    }
  }

  unreachable();
}

// property ::= identifier assignment object_result
AstNode *parse_property(Parser *parser, size_t *token_index) {
  AstNode* identifier = parse_identifier(parser, token_index);

  if (!identifier) {
    // TODO error;
    return NULL;
  }

  Token *equals_token = &(parser->tokens->tokens[*token_index]);
  if (equals_token->type == TOKEN_ASSIGN) {
    *token_index += 1;
  } else {
    // TODO error
    return NULL;
  }

  AstNode *object_result = parse_object_result(parser, token_index);
  if (!object_result) {
    // TODO error
    return NULL;
  }

  AstNode* node = init_node(NODE_PROPERTY);
  node->data.property.identifier = identifier;
  node->data.property.object_result = object_result;
  return node;
}

// object_property ::= identifier ("(" object_result ("," object_result)+ ")" )
// ("." object_property)?
AstNode *parse_object_property(Parser *parser, size_t *token_index) {
  Token *token = &(parser->tokens->tokens[*token_index]);
  if (token->type == TOKEN_IDENTIFIER) {
    AstNode *node = parse_identifier(parser, token_index);
    if (!node) {
      unreachable();
    }

    Token *next_token = &(parser->tokens->tokens[*token_index]);

    if (next_token->type == TOKEN_OPEN_PAREN) {
      *token_index += 1;
      AstNode *free_copy = init_node(NODE_FREE_OBJECT_COPY);
      LIST_INIT(free_copy->data.free_object_copy.property_list);
      while(true) {
        AstNode* object_result = parse_object_result(parser, token_index);
        if (object_result) {
          LIST_PUSH_BACK(free_copy->data.free_object_copy.property_list, object_result);
        } else {
          // TODO error
          return NULL;
        }

        next_token = &(parser->tokens->tokens[*token_index]);
        if(next_token->type == TOKEN_CLOSE_PAREN) {
          *token_index += 1;
          next_token = &(parser->tokens->tokens[*token_index]);
          node = free_copy;
          break;
        } else if(next_token->type == TOKEN_COMMA) {
          *token_index += 1;
        } else {
          // TODO error
          return NULL;
        }
      } while (next_token->type == TOKEN_COMMA);
    }

    if (next_token->type == TOKEN_DOT) {
      *token_index += 1;
      AstNode *object_property = parse_object_property(parser, token_index);
      if (object_property) {
        AstNode* n = init_node(NODE_OBJECT_PROPERTY_ACCESS);
        n->data.object_property_access.object = node;
        n->data.object_property_access.property = object_property;
        return n;
      } else {
        // TODO error
        return NULL;
      }
    } else {
      return node;
    }
  } else {
    // TODO error
    return NULL;
  }
}

AstNode* parse_string_literal(Parser* parser, size_t *token_index) {
  Token *token = &(parser->tokens->tokens[*token_index]);
  if (token->type == TOKEN_STRING_LITERAL) {
    *token_index += 1;
    AstNode* node = init_node(NODE_STRING_LITERAL);
    node->data.string_literal.content = token->content;
    return node;
  } else {
    // TODO error
    return NULL;
  }
}

// object_result ::= object | STRING | object_property
AstNode *parse_object_result(Parser *parser, size_t *token_index) {
  AstNode *node = parse_object(parser, token_index);
  if (!node) {
    node = parse_string_literal(parser, token_index);
  }
  if (!node) {
    node = parse_object_property(parser, token_index);
  }

  if (node) {
    return node;
  } else {
    // TODO error
    return NULL;
  }
  unreachable();
}

// root ::= property*
AstNode *parse_root(Parser* parser, size_t *token_index) {
  Token *token = &(parser->tokens->tokens[*token_index]);
  AstNode* node = init_node(NODE_ROOT);
  LIST_INIT(node->data.root.list);
  while (token->type != TOKEN_EOF) {
    AstNode *property = parse_property(parser, token_index);
    if (property) {
      LIST_PUSH_BACK(node->data.root.list, property);
    } else {
      // TODO error
      return NULL;
    }
    token = &(parser->tokens->tokens[*token_index]);
  }
  return node;
}

void printAst(AstNode* node, int indent) {
  if (node->type == NODE_ROOT) {
    printf("%*sNODE_ROOT(\n", indent, "");
    for (size_t i = 0; i < LIST_SIZE(node->data.root.list); ++i) {
      printAst(LIST_AT(node->data.root.list, i), indent + 2);
    }
    printf("%*s)\n", indent, "");
  } else if (node->type == NODE_PROPERTY) {
    printf("%*sNODE_PROPERTY(", indent, "");
    printAst(node->data.property.identifier, indent);
    printf(", ");
    printAst(node->data.property.object_result, indent);
    printf(")\n");
  } else if (node->type == NODE_IDENTIFIER) {
    printf("NODE_IDENTIFIER(%s)", node->data.identifier.content);
  } else if (node->type == NODE_STRING_LITERAL) {
    printf("NODE_STRING_LITERAL(%s)", node->data.string_literal.content);
  } else if (node->type == NODE_OBJECT) {
    printf("NODE_OBJECT(");
    for (size_t i = 0; i < LIST_SIZE(node->data.object.free_list); ++i) {
      if (i != 0) {
        printf(" ");
      }
      printAst(LIST_AT(node->data.object.free_list, i), indent + 2);
    }
    printf("\n");
    for (size_t i = 0; i < LIST_SIZE(node->data.object.property_list); ++i) {
      printAst(LIST_AT(node->data.object.property_list, i), indent + 2);
    }
    printf("%*s)", indent, "");
  } else if (node->type == NODE_FREE_OBJECT_COPY) {
    printf("NODE_FREE_OBJECT_COPY(");
    for (size_t i = 0; i < LIST_SIZE(node->data.free_object_copy.property_list); ++i) {
      if (i != 0) {
        printf(", ");
      }
      printAst(LIST_AT(node->data.free_object_copy.property_list, i), indent);
    }
    printf(")");
  } else if (node->type == NODE_OBJECT_PROPERTY_ACCESS) {
    printf("NODE_OBJECT_PROPERTY_ACCESS(");
    printAst(node->data.object_property_access.object, indent);
    printf(", ");
    printAst(node->data.object_property_access.property, indent);
    printf(")");
  }
}

int main(void) {
  char input[] = "person = height length > {"
"    name = \"John\""
"    age = name"
"}";
  Lexer lexer = init_lexer(input);
  Tokens tokens = init_tokens(1);
  lexer_scan_tokens(&lexer, &tokens);
  for (size_t i = 0; i < tokens.size; ++i) {
    print_token(&tokens.tokens[i]);
    if (tokens.tokens[i].type == TOKEN_EOF) {
      break;
    }
  }
  Parser parser = init_parser(&tokens);
  size_t token_index = 0;
  AstNode* root = parse_root(&parser, &token_index);
  printAst(root, 0);
  return 0;
}
