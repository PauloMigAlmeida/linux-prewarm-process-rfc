#----------------------------------------------------------------------------
# CLI utilities  makefile
#----------------------------------------------------------------------------

DIR_ROOT	:= $(CURDIR)/../../

include $(DIR_ROOT)/scripts/config.mk

DIR_TARGET	:= $(DIR_BUILD)/cli_utilities

SRC_ASM_FILES	:= $(wildcard *.asm)
BIN_ASM_FILES	:= $(SRC_ASM_FILES:%.asm=$(DIR_TARGET)/%.o)

TAG 		:= [cli_utilities]

all: mkdir compile 
	@echo "$(TAG) Compiled successfully"

.PHONY: mkdir
mkdir:
	@mkdir -p $(DIR_TARGET)

.PHONY: compile
compile: $(BIN_ASM_FILES)

$(BIN_ASM_FILES): $(DIR_TARGET)/%.o: %.asm
	@echo "$(TAG) Assembling $<"
	@$(AS) $(ASFLAGS) $< -o $@

