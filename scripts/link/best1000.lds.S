
#define VECTOR_SECTION_SIZE			208
#define REBOOT_PARAM_SECTION_SIZE	48
#define ROM_BUILD_TIME_SECTION_SIZE	24
#define ROM_EXPORT_FN_SECTION_SIZE	128
#define BUILD_TIME_SECTION_SIZE		24

#define HEAP_SECTION_SIZE			0x0004
#define STACK_SECTION_SIZE			0xD00

#define USERDATA_SECTION_SIZE		0x1000
#define FACTORY_SECTION_SIZE		0x1000

#define OVERLAY_DATA_SECTION_SIZE		0x8000

#define FAST_XRAM_SECTION_SIZE		0x8000


#define CACHED_ADDR(a)				((a) | 0x04000000)
#define UNCACHED_ADDR(a)			((a) & ~0x04000000)

#define FLASH_TO_FLASHX(a)			((a) - ORIGIN(FLASH) + ORIGIN(FLASHX))
#define FLASHX_TO_FLASH(a)			((a) - ORIGIN(FLASHX) + ORIGIN(FLASH))
#define RAM_TO_RAMX(a)				((a) - ORIGIN(RAM) + ORIGIN(RAMX))
#define RAMX_TO_RAM(a)				((a) - ORIGIN(RAMX) + ORIGIN(RAM))
#define PSRAM_TO_PSRAMX(a)			((a) - ORIGIN(PSRAM) + ORIGIN(PSRAMX))
#define PSRAMX_TO_PSRAM(a)			((a) - ORIGIN(PSRAMX) + ORIGIN(PSRAM))

/* Linker script to configure memory regions. */
MEMORY
{
  ROM    (rx)  : ORIGIN = 0x00000000, LENGTH = 0x10000   /*  64k       */
  FLASH  (r)   : ORIGIN = 0x6C000000, LENGTH = 0x100000  /*   1m       */
  FLASHX (rx)  : ORIGIN = 0x0C000000, LENGTH = 0x100000  /*   1m       */
  RAM    (rwx) : ORIGIN = 0x20000000, LENGTH = (0x20000)   /* 128k   */
  RAMX   (rx)  : ORIGIN = 0x10000000, LENGTH = (0x20000)   /* 128k   */
  FRAMX  (rwx) : ORIGIN = 0x10020000, LENGTH = FAST_XRAM_SECTION_SIZE   /* 32k */
  PSRAM  (rwx) : ORIGIN = 0x64000000, LENGTH = 0x400000  /*   4m       */
  PSRAMX (rx)  : ORIGIN = 0x1C000000, LENGTH = 0x400000  /*   4m       */
}

/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __export_fn_rom
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
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
 *   __factory_start
 *   __factory_end
 *   __calib_start
 *   __calib_end
 *   __flash_start
 *   __flash_end
 *   __free_flash
 *   __boot_sram_start_flash__
 *   __boot_sram_end_flash__
 *   __boot_sram_start__
 *   __boot_bss_sram_start__
 *   __boot_bss_sram_end__
 *   __sram_text_data_start_flash__
 *   __sram_text_data_end_flash__
 *   __fast_sram_text_data_start__
 *   __fast_sram_text_data_end__
 *   __fast_sram_text_data_start_flash__
 *   __fast_sram_text_data_end_flash__
 *   __sram_text_data_start__
 *   __sram_bss_start__
 *   __sram_bss_end__
 *   __sram_userdata_pool_start__
 *   __overlay_text_start__
 *   __overlay_text_exec_start__
 *   __overlay_data_start__
 */
ENTRY(Boot_Loader)

SECTIONS
{
	__export_fn_rom = (ORIGIN(ROM) + LENGTH(ROM) - ROM_BUILD_TIME_SECTION_SIZE - ROM_EXPORT_FN_SECTION_SIZE);

	.boot_struct (ORIGIN(FLASH)) :
	{
		__flash_start = .;
		KEEP(*(.boot_struct))
		. = ALIGN(4);
	} > FLASH

	.boot_text_flash (FLASH_TO_FLASHX(LOADADDR(.boot_struct) + SIZEOF(.boot_struct))) :
						AT (FLASHX_TO_FLASH(ADDR(.boot_text_flash)))
	{
		*(.boot_loader)
		*(.boot_text_flash)
		*(.boot_rodata)
		. = ALIGN(4);
	} > FLASHX

	.vector_table (ORIGIN(RAM)) (NOLOAD) :
	{
		KEEP(*(.vector_table))
		. = VECTOR_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.reboot_param (.) (NOLOAD) :
	{
		KEEP(*(.reboot_param))
		. = REBOOT_PARAM_SECTION_SIZE;
		. = ALIGN(4);
	} > RAM

	.userdata_pool (.) (NOLOAD) :
	{
		*(.userdata_pool)
		. = ALIGN(4);
	} > RAM

	__boot_sram_start_flash__ = LOADADDR(.boot_text_flash) + SIZEOF(.boot_text_flash);
	__boot_sram_start__ = ADDR(.userdata_pool) + SIZEOF(.userdata_pool);
	.boot_text_sram (RAM_TO_RAMX(__boot_sram_start__)) : AT (__boot_sram_start_flash__)
	{
		*:hal_norflash.o(.text*)
		*:norflash_*.o(.text*)
		/* libc_nano.a(lib_a-memcpy.o) or memcpy.o */
		*memcpy.o(.text*)
		/* libc_nano.a(lib_a-memset.o) or memset.o */
		*memset.o(.text*)
		*(.boot_text_sram)
		. = ALIGN(4);
	} > RAMX

	.boot_data_sram (RAMX_TO_RAM(ADDR(.boot_text_sram) + SIZEOF(.boot_text_sram))) :
					AT (__boot_sram_start_flash__ + SIZEOF(.boot_text_sram))
	{
		*:hal_norflash.o(.data* .rodata*)
		*:norflash_*.o(.data* .rodata*)
		*:hal_psram.o(.data* .rodata*)
		*(.boot_data)
		. = ALIGN(4);
	} > RAM

	__boot_sram_end_flash__ = __boot_sram_start_flash__ +
								SIZEOF(.boot_text_sram) + SIZEOF(.boot_data_sram);

	.boot_bss_sram (.) (NOLOAD) :
	{
		__boot_bss_sram_start__ = .;
		*:hal_norflash.o(.bss*)
		*:norflash_*.o(.bss*)
		*:hal_psram.o(.bss*)
		*(.boot_bss)
		. = ALIGN(4);
		__boot_bss_sram_end__ = .;
	} > RAM

	__sram_text_data_start_flash__ = __boot_sram_end_flash__;
	__sram_text_data_start__ = __boot_bss_sram_end__;
	.sram_text (RAM_TO_RAMX(__sram_text_data_start__)) :
				AT (__sram_text_data_start_flash__)
	{
		*:filter_process.o(.text*)
		*:utils.o(.text*)
		*:division_operations.o(.text*)
		*:cqueue.o(.text*)
		*(.sram_text)
		. = ALIGN(4);
	} > RAMX

	.sram_data (RAMX_TO_RAM(ADDR(.sram_text) + SIZEOF(.sram_text))) :
				AT (__sram_text_data_start_flash__ + SIZEOF(.sram_text))
	{
		*:utils.o(.data* .rodata*)
		*:vad_core.o(.rodata*)
		*:vad_filterbank.o(.rodata*)
		*(.sram_data)
		. = ALIGN(4);
	} > RAM

	__sram_text_data_end_flash__ = __sram_text_data_start_flash__ +
								SIZEOF(.sram_text) + SIZEOF(.sram_data);

	.sram_bss (.) (NOLOAD) :
	{
		__sram_bss_start__ = .;
		*:utils.o(.bss*)
		*(.sram_bss)
		. = ALIGN(4);
		__sram_bss_end__ = .;
	} > RAM

	__fast_sram_text_data_start_flash__ = __sram_text_data_end_flash__;
	.fast_text_sram (ORIGIN(FRAMX)) : AT (__fast_sram_text_data_start_flash__)
	{
		__fast_sram_text_data_start__ = .;
		/* libc_nano.a(lib_a-memmove.o) or memmove.o */
		*memmove.o(.text*)
		*(.fast_text_sram)
		. = ALIGN(4);
		__fast_sram_text_data_end__ = .;
	} > FRAMX
	__fast_sram_text_data_end_flash__ = __fast_sram_text_data_start_flash__ + SIZEOF(.fast_text_sram);

	__overlay_text_start__ = RAMX_TO_RAM(__fast_sram_text_data_end__);
	__overlay_text_exec_start__ = RAM_TO_RAMX(__overlay_text_start__);
	OVERLAY (__overlay_text_exec_start__) : NOCROSSREFS AT (__fast_sram_text_data_end_flash__)
	{
		.overlay_text0
		{
			*:preprocess.o(.text*)
			*:filterbank.o(.text*)
			*:kiss_fft.o(.text*)
			*:kiss_fftr.o(.text*)
			*:fftwrap.o(.text*)
			*:mdf.o(.text*)
			*:plc.o(.text*)
			*(.overlay_text0)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_text1
		{
			*:audio_resample.o(.text*)
			*:sbc.o(.text*)
			*:sbc_math.o(.text*)
			*:fixedpoint.o(.text*)
			*:replaygain.o(.text*)
			*:dsp_core.o(.text*)
			*:dsp_misc.o(.text*)
			*:dsp_filter.o(.text*)
			*:dsp_sample_input.o(.text*)
			*:dsp_sample_output.o(.text*)
			*:eq.o(.text*)
			*:pga.o(.text*)
			*:audio_eq.o(.text*)
			*:filter_iir.o(.text*)
			*(.overlay_text1)
			. = ALIGN(4);
		}
		.overlay_text2
		{
			*:fmdec.o(.text* .data* .rodata* .bss*)
			*(.overlay_text2)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_text3
		{
			*:fft128dot.o(.text*)
			*(.overlay_text3)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_text4
		{
			*(.overlay_text4)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_text5
		{
			*(.overlay_text5)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_text6
		{
			*(.overlay_text6)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_text7
		{
			*(.overlay_text7)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_text_end
		{
			BYTE(0);
			. = ALIGN(4);
		}
	} > FRAMX

	.dummy_overlay_text (.) (NOLOAD) :
	{
		__dummy_overlay_text_end__ = .;
	} > FRAMX

	ASSERT(__dummy_overlay_text_end__ - __fast_sram_text_data_start__ <= FAST_XRAM_SECTION_SIZE, "fast xram sections too large")
	__overlay_data_start__ = __sram_bss_end__;
	OVERLAY (__overlay_data_start__) : NOCROSSREFS AT (__load_stop_overlay_text_end)
	{
		.overlay_data0
		{
			*:preprocess.o(.data* .rodata*)
			*:filterbank.o(.data* .rodata*)
			*:kiss_fft.o(.data* .rodata*)
			*:kiss_fftr.o(.data* .rodata*)
			*:fftwrap.o(.data* .rodata*)
			*:mdf.o(.data* .rodata*)
			*:plc.o(.data* .rodata*)
			*(.overlay_data0 .overlay_rodata0)
			BYTE(0);
			. = ALIGN(4);
		}
		
		.overlay_data1
		{
			*:audio_resample.o(.data* .rodata*)
			*:sbc.o(.data* .rodata*)
			*:sbc_math.o(.data* .rodata*)
			*:fixedpoint.o(.data* .rodata*)
			*:replaygain.o(.data* .rodata*)
			*:dsp_core.o(.data* .rodata*)
			*:dsp_misc.o(.data* .rodata*)
			*:dsp_filter.o(.data* .rodata*)
			*:dsp_sample_input.o(.data* .rodata*)
			*:dsp_sample_output.o(.data* .rodata*)
			*:eq.o(.data* .rodata*)
			*:pga.o(.data* .rodata*)
			*:audio_eq.o(.data* .rodata*)
			*:filter_iir.o(.data* .rodata*)	
			*(.overlay_data1 .overlay_rodata1)
			. = ALIGN(4);
		}
		.overlay_data2
		{
			*(.overlay_data2 .overlay_rodata2)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_data3
		{
			*:fft128dot.o(.data* .rodata*)
			*(.overlay_data3 .overlay_rodata3)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_data4
		{
			*(.overlay_data4 .overlay_rodata4)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_data5
		{
			*(.overlay_data5 .overlay_rodata5)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_data6
		{
			*(.overlay_data6 .overlay_rodata6)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_data7
		{
			*(.overlay_data7 .overlay_rodata7)
			BYTE(0);
			. = ALIGN(4);
		}
		.overlay_data_end
		{
			BYTE(0);
			. = ALIGN(4);
		}
	} > RAM

	.dummy_overlay_data (.) (NOLOAD) :
	{
		__dummy_overlay_data_end__ = .;
	} > RAM

	ASSERT(__dummy_overlay_data_end__ - __overlay_data_start__ <= OVERLAY_DATA_SECTION_SIZE, "overlay sections too large")

	.text (FLASH_TO_FLASHX(__load_stop_overlay_data_end)) : AT (FLASHX_TO_FLASH(ADDR(.text)))
	{
		*(.text*)
		*(.flash_text)

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
	} > FLASHX

	.ARM.extab (.) : AT (FLASHX_TO_FLASH(ADDR(.ARM.extab)))
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASHX

	__exidx_start = .;
	.ARM.exidx (.) : AT (FLASHX_TO_FLASH(ADDR(.ARM.exidx)))
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASHX
	__exidx_end = .;

	.rodata (FLASHX_TO_FLASH(__exidx_end)) :
	{
		*(.rodata*)
		*(.flash_rodata)
		*(.note.gnu.build-id)

		KEEP(*(.eh_frame*))
	} > FLASH

	/* To copy multiple FLASH to RAM sections,
	 * uncomment .copy.table section and,
	 * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
	/*
	.copy.table (.) :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;
		LONG (__etext)
		LONG (__data_start__)
		LONG (__data_end__ - __data_start__)
		LONG (__etext2)
		LONG (__data2_start__)
		LONG (__data2_end__ - __data2_start__)
		__copy_table_end__ = .;
	} > FLASH
	*/

	/* To clear multiple BSS sections,
	 * uncomment .zero.table section and,
	 * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
	/*
	.zero.table (.) :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;
		LONG (__bss_start__)
		LONG (__bss_end__ - __bss_start__)
		LONG (__bss2_start__)
		LONG (__bss2_end__ - __bss2_start__)
		__zero_table_end__ = .;
	} > FLASH
	*/

	__etext = .;

	.data : AT (__etext)
	{
		__data_start__ = .;
		*(vtable)
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	__overlay_bss_start__ = .;
	.overlay_local_player (__overlay_bss_start__) (NOLOAD) :
	{
		__overlay_local_player_start = .;
		*(.overlay_local_player)
	        *usb_audio_test.o(.bss.os_thread_def_stack_app_uaudio_thread)  /*  0x1b68 */
		*:usb_audio_test.o(.bss.os_mailQ_q_app_uaudio_mailbox)  /* 0x1200 */



		. = ALIGN(4);
		__overlay_local_player_end = .;
	} > RAM

	.overlay_bt_player (__overlay_bss_start__) (NOLOAD) :
	{
		__overlay_bt_player_start = .;
		*(.overlay_bt_player)
		*:app_bt_stream.o(.bss.bt_audio_buff)


		. = ALIGN(4);
		__overlay_bt_player_end = .;
	} > RAM

	__overlay_bss_size = MAX(SIZEOF(.overlay_local_player), SIZEOF(.overlay_bt_player));
	. = __overlay_bss_start__ + __overlay_bss_size;
	.bss (.) (NOLOAD) :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM

	.heap (.) (NOLOAD) :
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
	.stack_dummy (.) (COPY) :
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

	. = __etext + SIZEOF(.data);
	.build_time (.) :
	{
		KEEP(*(.build_time))
		. = BUILD_TIME_SECTION_SIZE;
	} > FLASH = 0x00000000

	/* The following section be the last loaded section */
	.code_start_addr (.) :
	{
		LONG(ABSOLUTE(__flash_start));
	} > FLASH

	__flash_end = .;
	__flash_other_partition_start = (ORIGIN(FLASH) + LENGTH(FLASH) - FACTORY_SECTION_SIZE - USERDATA_SECTION_SIZE) ;
	.userdata (ORIGIN(FLASH) + LENGTH(FLASH) - FACTORY_SECTION_SIZE - USERDATA_SECTION_SIZE) (NOLOAD) :
	{
		__userdata_start = . - 0x04000000;
		. = USERDATA_SECTION_SIZE;
		__userdata_end = . - 0x04000000;
	} > FLASH

	.factory (ORIGIN(FLASH) + LENGTH(FLASH) - FACTORY_SECTION_SIZE) (NOLOAD) :
	{
		__factory_start = . - 0x04000000;
		. = FACTORY_SECTION_SIZE;
		__factory_end = . - 0x04000000;
	} > FLASH

	ASSERT(__flash_other_partition_start >= __flash_end, "region FLASH overflowed with factory")
	__free_flash = __flash_other_partition_start - __flash_end;
}
