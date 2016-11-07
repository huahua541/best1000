
#define VECTOR_SECTION_SIZE			208
#define REBOOT_PARAM_SECTION_SIZE	48
#define BUILD_TIME_SECTION_SIZE		24
#define EXPORT_FUNC_SECTION_SIZE	64

#define HEAP_SECTION_SIZE			0x400
#define STACK_SECTION_SIZE			0x1000

/* Linker script to configure memory regions. */
MEMORY
{
  ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 0x80000   /*  512K */
  RAM (rwx) : ORIGIN = 0x20000400, LENGTH = 0x28000-0x400   /* 128k + 32k */
}

/* Library configurations */
GROUP(libgcc.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __rom_etext
 *   __rom_data_start__
 *   __rom_data_end__
 *   __rom_bss_start__
 *   __rom_bss_end__
 *   __rom_HeapLimit
 *   __rom_StackLimit
 *   __rom_StackTop
 *   __rom_stack
 *   __Vectors_End
 *   __Vectors_Size
 *   __free_rom_ram
 *   __rom_end
 *   __free_rom
 */
ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        __vector_table = .;
        KEEP(*(.vectors_table))
        *(.text.Reset_Handler)
        *(.text.System_Init)
         . = ALIGN(4);

        __Vectors_End = .;
        __Vectors_Size = __Vectors_End - __Vectors;

        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        *(.rodata*)
        *(.note.gnu.build-id)

        KEEP(*(.eh_frame*))
    } > ROM

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > ROM

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > ROM
	__exidx_end = .;

	__rom_etext = .;

	.vector_table (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.reboot_param (NOLOAD) :
	{
		KEEP(*(.reboot_param))
		. = REBOOT_PARAM_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.rom_data : AT (__rom_etext)
	{
		__rom_data_start__ = .;
		*(.data*)
		. = ALIGN(4);
		/* All data end */
		__rom_data_end__ = .;
	} > RAM

	.rom_bss (NOLOAD) :
	{
		. = ALIGN(4);
		__rom_bss_start__ = .;
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		*(AHBSDRAM1)
		. = ALIGN(4);
		__rom_bss_end__ = .;
		__bss_end__ = .;
	} > RAM

	.rom_heap (NOLOAD) :
	{
		. = ALIGN(8);
		__rom_HeapBase = .;
        __end__ = .;
        end = __end__;
		. = HEAP_SECTION_SIZE;
		. = ALIGN(8);
		__rom_HeapLimit = .;
	} > RAM

	.scratch_buf (NOLOAD) :
	{
		. = ALIGN(4);
		*(.scratch_buf)
		. = ALIGN(4);
	} > RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.rom_stack_dummy (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__rom_StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackTop = __rom_StackTop;
	__rom_StackLimit = __rom_StackTop - SIZEOF(.rom_stack_dummy);
	PROVIDE(__rom_stack = __rom_StackTop);
	PROVIDE(__stack = __rom_StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__rom_StackLimit >= __rom_HeapLimit, "region RAM overflowed with stack")
	__free_rom_ram = __rom_StackLimit - __rom_HeapLimit;

	__rom_end = __rom_etext + SIZEOF(.rom_data);

	.rom_export_fn (ORIGIN(ROM) + LENGTH(ROM) - BUILD_TIME_SECTION_SIZE - EXPORT_FUNC_SECTION_SIZE) :
	{
		KEEP(*(.export_fn))
		. = EXPORT_FUNC_SECTION_SIZE;
	} > ROM = 0x00000000

	.rom_build_time (ORIGIN(ROM) + LENGTH(ROM) - BUILD_TIME_SECTION_SIZE) :
	{
		KEEP(*(.build_time))
		. = BUILD_TIME_SECTION_SIZE;
	} > ROM = 0x00000000

	__free_rom = ADDR(.rom_export_fn) - __rom_end;
}
