.DEFAULT_GOAL = all

GREEN := $(shell tput -Txterm setaf 2)
RESET := $(shell tput -Txterm sgr0)

BINDIR := ./bin
CC := gcc

CFLAGS := -g -std=c2x -Wall -Wextra -Werror -Wpedantic

objects := \
	src/analyser.o \
	src/generator.o \
	src/lexer.o \
	src/main.o \
	src/parser.o \
	src/utf8.o \
	src/util.o

.PHONY: all
all: $(BINDIR)/vix

$(BINDIR)/vix: $(objects)
	@mkdir -p -- $(BINDIR)
	@printf '[$(GREEN)LD$(RESET)]\t%s\n' '$(CC) -o $@ $(objects)'
	@$(CC) -o $@ $(objects)

.c.o:
	@printf '[$(GREEN)CC$(RESET)]\t%s\n' '$(CC) $(CFLAGS) -c -o $@ $<'
	@$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@rm -rf -- $(BINDIR) $(objects)
