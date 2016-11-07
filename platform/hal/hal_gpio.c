#include "stdarg.h"
#include "stdio.h"
#include "plat_types.h"
#include "hal_gpio.h"
#include "reg_gpio.h"
#include "hal_trace.h"
#include "cmsis_nvic.h"
#include "hal_uart.h"

#define HAL_GPIO_BANK_NUM 1
#define HAL_GPIO_PORT_NUM 1

#define HAL_GPIO_PIN_NUM_EACH_PORT (32)
#define HAL_GPIO_PIN_NUM_EACH_BANK (HAL_GPIO_PORT_NUM*HAL_GPIO_PIN_NUM_EACH_PORT)

#define HAL_GPIO_PIN_TO_BANK(pin) \
    (pin/HAL_GPIO_PIN_NUM_EACH_BANK)

#define HAL_GPIO_PIN_TO_PORT(pin) \
    ((pin%HAL_GPIO_PIN_NUM_EACH_BANK)/HAL_GPIO_PIN_NUM_EACH_PORT)

typedef void (* _HAL_GPIO_IRQ_HANDLER)(void);

struct GPIO_PORT_T {
    uint32_t GPIO_DR;
    uint32_t GPIO_DDR;
    uint32_t GPIO_CTL;
};

struct GPIO_BANK_T {
    struct GPIO_PORT_T port[HAL_GPIO_PORT_NUM];
    struct GPIO_PORT_T _port_reserved[3];
    uint32_t GPIO_INTEN;
    uint32_t GPIO_INTMASK;
    uint32_t GPIO_INTTYPE_LEVEL;
    uint32_t GPIO_INT_POLARITY;
    uint32_t GPIO_INTSTATUS;
    uint32_t GPIO_RAW_INTSTATUS;
    uint32_t GPIO_DEBOUNCE;
    uint32_t GPIO_PORTA_EOI;
    uint32_t GPIO_EXT_PORT[HAL_GPIO_PORT_NUM];
    uint32_t GPIO_EXT_PORT_reserved[3];
    uint32_t GPIO_IS_SYNC;
};

void _hal_gpio_bank0_irq_handler(void);

static struct GPIO_BANK_T * const gpio_bank[HAL_GPIO_BANK_NUM] = { (struct GPIO_BANK_T *)0x40001000, };
static HAL_GPIO_PIN_IRQ_HANDLER gpio_irq_handler[HAL_GPIO_PIN_NUM] = {0};
static _HAL_GPIO_IRQ_HANDLER _gpio_irq_handler[HAL_GPIO_BANK_NUM] = {_hal_gpio_bank0_irq_handler, };

enum HAL_GPIO_DIR_T hal_gpio_pin_get_dir(enum HAL_GPIO_PIN_T pin)
{
    int pin_offset = 0;
    int bank = 0, port = 0;
    enum HAL_GPIO_DIR_T dir = 0;

    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin%HAL_GPIO_PIN_NUM_EACH_PORT;

    if (gpio_bank[bank]->port[port].GPIO_DDR & (0x1<<pin_offset))
        dir = HAL_GPIO_DIR_OUT;
    else
        dir = HAL_GPIO_DIR_IN;

    return dir;
}

void hal_gpio_pin_set_dir(enum HAL_GPIO_PIN_T pin, enum HAL_GPIO_DIR_T dir, uint8_t val_for_out)
{
    int pin_offset = 0;
    int bank = 0, port = 0;

    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin%HAL_GPIO_PIN_NUM_EACH_PORT;

    if(dir == HAL_GPIO_DIR_OUT)
        gpio_bank[bank]->port[port].GPIO_DDR |= 0x1<<pin_offset;
    else
        gpio_bank[bank]->port[port].GPIO_DDR &= ~(0x1<<pin_offset);

    if(val_for_out)
        hal_gpio_pin_set(pin);
    else
        hal_gpio_pin_clr(pin);
}

void hal_gpio_pin_set(enum HAL_GPIO_PIN_T pin)
{
    int pin_offset = 0;
    int bank = 0, port = 0;

    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin%HAL_GPIO_PIN_NUM_EACH_PORT;

    gpio_bank[bank]->port[port].GPIO_DR |= 0x1<<pin_offset;
}

void hal_gpio_pin_clr(enum HAL_GPIO_PIN_T pin)
{
    int pin_offset = 0;
    int bank = 0, port = 0;

    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin%HAL_GPIO_PIN_NUM_EACH_PORT;

    gpio_bank[bank]->port[port].GPIO_DR &= ~(0x1<<pin_offset);
}

uint8_t hal_gpio_pin_get_val(enum HAL_GPIO_PIN_T pin)
{
    int pin_offset = 0;
    int bank = 0, port = 0;

    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin%HAL_GPIO_PIN_NUM_EACH_PORT;

    /* when as input : read back outside signal value */
    /* when as output: read back DR register value ,same as read back DR register */

    return (((gpio_bank[bank]->GPIO_EXT_PORT[port]) & (0x1<<pin_offset)) >> pin_offset);
}

void _hal_gpio_bank0_irq_handler(void)
{
    uint32_t raw_status = 0, bank = 0, pin_offset = 0;
    raw_status = gpio_bank[bank]->GPIO_RAW_INTSTATUS;

    if (raw_status == 0)
        return;

    while ((raw_status>>pin_offset & 0x1) != 0x1) ++pin_offset;

    /* clear irq */
    gpio_bank[bank]->GPIO_PORTA_EOI |= 0x1<<pin_offset;
    
    if (gpio_irq_handler[pin_offset])
        gpio_irq_handler[pin_offset](pin_offset + bank*HAL_GPIO_PIN_NUM_EACH_BANK);
}

uint8_t hal_gpio_setup_irq(enum HAL_GPIO_PIN_T pin, struct HAL_GPIO_IRQ_CFG_T *cfg)
{
    int pin_offset = 0;
    int bank = 0, port = 0;

    bank = HAL_GPIO_PIN_TO_BANK(pin);
    port = HAL_GPIO_PIN_TO_PORT(pin);
    pin_offset = pin%HAL_GPIO_PIN_NUM_EACH_PORT;

    /* only port A support irq */
    if (port != 0)
        return 0;

    if (cfg->irq_enable) {
        gpio_bank[bank]->GPIO_INTMASK |= (0x1<<pin_offset);

        if (cfg->irq_debounce)
            gpio_bank[bank]->GPIO_DEBOUNCE |= 0x1<<pin_offset;
        else
            gpio_bank[bank]->GPIO_DEBOUNCE &= ~(0x1<<pin_offset);

        if (cfg->irq_type == HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE)
            gpio_bank[bank]->GPIO_INTTYPE_LEVEL |= 0x1<<pin_offset;
        else
            gpio_bank[bank]->GPIO_INTTYPE_LEVEL &= ~(0x1<<pin_offset);

        if (cfg->irq_porlariry == HAL_GPIO_IRQ_POLARITY_HIGH_RISING)
            gpio_bank[bank]->GPIO_INT_POLARITY |= 0x1<<pin_offset;
        else
            gpio_bank[bank]->GPIO_INT_POLARITY &= ~(0x1<<pin_offset);

        gpio_irq_handler[pin] = cfg->irq_handler;

        NVIC_SetVector(GPIO_IRQn, (uint32_t)_gpio_irq_handler[bank]);
        NVIC_EnableIRQ(GPIO_IRQn);

        gpio_bank[bank]->GPIO_INTMASK &= ~(0x1<<pin_offset);
        gpio_bank[bank]->GPIO_INTEN |= 0x1<<pin_offset;
    }
    else {
        gpio_bank[bank]->GPIO_INTMASK |= (0x1<<pin_offset);
        gpio_bank[bank]->GPIO_INTEN &= ~(0x1<<pin_offset);
        gpio_irq_handler[pin] = 0;
    }

    return 0;
}
