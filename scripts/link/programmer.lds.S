
#define VECTOR_SECTION_SIZE			208
#define REBOOT_PARAM_SECTION_SIZE	48
#define ROM_BUILD_TIME_SECTION_SIZE	24
#define ROM_EXPORT_FN_SECTION_SIZE	128
#define BUILD_TIME_SECTION_SIZE		24

#define HEAP_SECTION_SIZE			0x400
#define STACK_SECTION_SIZE			0x1000

#define CODE_MSG_OVERHEAD			8

/* Linker script to configure memory regions. */
MEMORY
{
  ROM (rx)  : ORIGIN = 0x00000000, LENGTH = 0x10000   /*  64k       */
  RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 0x28000   /* 128k + 32k */
}

/* Library configurations */
GROUP(libgcc.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions ROM and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exec_struct_start
 *   __exidx_start
 *   __exidx_end
 *   __etext
 *   __data_start__
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __free_ram
 */
ENTRY(programmer_start)

SECTIONS
{
	__export_fn_rom = (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_TIME_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE);

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

	.burn_buffer (NOLOAD) :
	{
		*(.burn_buffer)
		. = ALIGN(4);
	} > RAM
/*
	.code_dummy (NOLOAD) :
	{
		. = CODE_MSG_OVERHEAD;
		. = ALIGN(4);
	} > RAM
*/
	.text :
	{
		KEEP(*(.boot_struct))

		__exec_struct_start = .;
		KEEP(*(.exec_struct))

		*(.text*)

		*(.rodata*)
		*(.note.gnu.build-id)
	} > RAM

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > RAM

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > RAM

	__exidx_end = .;

	__etext = .;

	.data :
	{
		__data_start__ = .;
		*(.data*)

		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	/* The following section be the last loaded section */
	.code_start_addr :
	{
		LONG(ABSOLUTE(__exec_struct_start));
	} > RAM

	.bss (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.heap (NOLOAD) :
	{
		. = ALIGN(8);
		__HeapBase = .;
		__end__ = .;
		end = __end__;
		. = HEAP_SECTION_SIZE;
		. = ALIGN(8);
		__HeapLimit = .;
	} > RAM

	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later */
	.stack_dummy (COPY) :
	{
		. = STACK_SECTION_SIZE;
		. = ALIGN(8);
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
	 * size of stack_dummy section */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);
	__StackLimit = __StackTop - SIZEOF(.stack_dummy);
	PROVIDE(__stack = __StackTop);

	/* Check if data + heap + stack exceeds RAM limit */
	ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
	__free_ram = __StackLimit - __HeapLimit;
}
