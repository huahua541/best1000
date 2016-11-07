#ifndef __HAL_LOCATION_H__
#define __HAL_LOCATION_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && !(defined(ROM_BUILD) || defined(PROGRAMMER))

#define BOOT_TEXT_SRAM_LOC              __attribute__((section(".boot_text_sram")))
#define BOOT_TEXT_FLASH_LOC             __attribute__((section(".boot_text_flash")))
#define BOOT_RODATA_LOC                 __attribute__((section(".boot_rodata")))
#define BOOT_DATA_LOC                   __attribute__((section(".boot_data")))
#define BOOT_BSS_LOC                    __attribute__((section(".boot_bss")))

#define SRAM_TEXT_LOC                   __attribute__((section(".sram_text")))
#define SRAM_DATA_LOC                   __attribute__((section(".sram_data")))
#define SRAM_STACK_LOC                  __attribute__((section(".sram_data"), align(8)))
#define SRAM_BSS_LOC                    __attribute__((section(".sram_bss")))

#define FLASH_TEXT_LOC                  __attribute__((section(".flash_text")))
#define FLASH_RODATA_LOC                __attribute__((section(".flash_rodata")))

#else // !__GNUC__ || ROM_BUILD || PROGRAMMER

#define BOOT_TEXT_SRAM_LOC
#define BOOT_TEXT_FLASH_LOC
#define BOOT_RODATA_LOC
#define BOOT_DATA_LOC
#define BOOT_BSS_LOC

#define SRAM_TEXT_LOC
#define SRAM_DATA_LOC
#define SRAM_STACK_LOC
#define SRAM_BSS_LOC

#define FLASH_TEXT_LOC
#define FLASH_RODATA_LOC

#endif // !__GNUC__ || ROM_BUILD || PROGRAMMER

#ifdef __cplusplus
}
#endif

#endif
