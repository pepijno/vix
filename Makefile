.DEFAULT_GOAL = all

GREEN := $(shell tput -Txterm setaf 2)
RESET := $(shell tput -Txterm sgr0)

TARGET := vix
BINDIR := ./bin
SRCDIR := ./src
DEPDIR := ./.deps
OBJDIR := ./.objs

CC := gcc
CFLAGS := -g -std=c2x -Wall -Wextra -Werror -Wpedantic

SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst $(SRCDIR)/%.c,$(DEPDIR)/%.d,$(SOURCES))

.PHONY: all
all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS) | $(BINDIR)
	@printf '[$(GREEN)LD$(RESET)]\t%s\n' '$@'
	@$(CC) $(CFLAGS) $^ -o $@

-include $(DEPENDS)

$(OBJDIR)/%.o : $(SRCDIR)/%.c Makefile | $(DEPDIR) $(OBJDIR)
	@printf '[$(GREEN)CC$(RESET)]\t%s\n' $<
	@$(CC) $(CFLAGS) -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d -c -o $@ $<

$(BINDIR) $(DEPDIR) $(OBJDIR):
	@mkdir -p $@

.PHONY: clean
clean:
	@rm -rf -- $(BINDIR) $(OBJDIR) $(DEPDIR)
