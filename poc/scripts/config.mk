#--------------------
# Project directories
#--------------------
DIR_SRC     	:= $(DIR_ROOT)/src
DIR_BUILD   	:= $(DIR_ROOT)/build
DIR_SCRIPTS	:= $(DIR_ROOT)/scripts
DIR_INCLUDE	:= $(DIR_ROOT)/include

#-------------------
# Tool configuration
#-------------------
CC		:= gcc

# Notes:
# 	- gnu99 is used instead of c99 so we can use the GCC inline assembly extensions
CCFLAGS     	:= -std=gnu99 -I$(DIR_INCLUDE) -g \
        	       -masm=intel -Wall -Wextra -Wpedantic

AS          := nasm

ASFLAGS     := -f bin

LD          := ld

MAKE_FLAGS  := --quiet --no-print-directory

