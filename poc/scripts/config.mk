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
CCFLAGS     	:= -std=gnu99 -I$(DIR_INCLUDE) -Qn -g \
               		-m64 -mno-red-zone -masm=intel \
	               -ffreestanding -fno-asynchronous-unwind-tables \
        	       -Wall -Wextra -Wpedantic -mcmodel=large -fno-builtin

AS          := nasm

ASFLAGS     := -f bin

LD          := ld

LDFLAGS     := -nostdlib -z max-page-size=0x1000

OBJCOPY     := objcopy

MAKE_FLAGS  := --quiet --no-print-directory

