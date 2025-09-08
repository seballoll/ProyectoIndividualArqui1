ENTRY(_start)
SECTIONS
{
  . = 0x80000000;    /* QEMU virt machine reset vector */
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .bss : { *(.bss*) }
}
