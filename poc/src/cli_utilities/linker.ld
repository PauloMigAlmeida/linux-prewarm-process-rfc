OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

SECTIONS
{  
  . = 0x401000;  

  .text : {
    *(.text)
  }

  . = ALIGN(16);
  .data : {
    *(.data)
  }


  .bss : ALIGN(4K) {
    _BSS_START = ABSOLUTE(.);
    *(.bss)
    *(COMMON)
  }
  
  _BSS_SIZE = ABSOLUTE(.) - _BSS_START;

  .rodata : {
    *(.rodata)
  }   

}
