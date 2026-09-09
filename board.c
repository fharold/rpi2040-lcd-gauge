#include "board.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "lcd.h"
#include "CST816S.h"

bool    rp2040_touch = false;
uint8_t CBUT0        = CBUT_PLAIN;

/* Addresses the I2C spec reserves; probing them is meaningless. */
static bool reserved_addr(uint8_t addr) {
  return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

/* Nothing to do yet: the interrupts are enabled so the pins wake the core,
   the state is read in the main loop. */
static void gpio_callback(uint gpio, uint32_t events) {
  (void)gpio;
  (void)events;
}

void board_init_gpio(void) {
  i2c_init(I2C_PORT, 100 * 1000);

  gpio_set_function(DEV_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(DEV_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(DEV_SDA_PIN);
  gpio_pull_up(DEV_SCL_PIN);

  gpio_init(RELAY1_COMMAND_PIN);
  gpio_set_dir(RELAY1_COMMAND_PIN, GPIO_OUT);

  gpio_init(RELAY2_COMMAND_PIN);
  gpio_set_dir(RELAY2_COMMAND_PIN, GPIO_OUT);

  gpio_init(BUTTON_INPUT);
  gpio_set_dir(BUTTON_INPUT, GPIO_IN);
  gpio_pull_up(BUTTON_INPUT);

  gpio_init(SHIFTED_DIGITAL_IN1);
  gpio_set_dir(SHIFTED_DIGITAL_IN1, GPIO_IN);

  gpio_init(SHIFTED_DIGITAL_IN2);
  gpio_set_dir(SHIFTED_DIGITAL_IN2, GPIO_IN);

  /* ALS_OUT, SENSOR1_FILTERED and SENSOR2_FILTERED are analog inputs and are
     configured by sensors_init(), not here. */
}

void board_i2c_scan(void) {
  printf("\nI2C Bus Scan \n");
  printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

  for (int addr = 0; addr < (1 << 7); ++addr) {
    if (addr % 16 == 0) { printf("%02x ", addr); }

    int ret;
    uint8_t rxdata;
    if (reserved_addr(addr)) {
      ret = PICO_ERROR_GENERIC;
    } else {
      ret = i2c_read_blocking(I2C_PORT, addr, &rxdata, 1, false);
    }

    if (ret >= 0 && addr == CST816_ADDR) {
      rp2040_touch = true;
      CBUT0        = CBUT_TOUCH;
      LCD_RST_PIN  = 13;
    }

    printf(ret < 0 ? "." : "@");
    printf(addr % 16 == 15 ? "\n" : "  ");
  }
}

void board_init_module_inputs(void) {
  gpio_init(QMIINT1);
  gpio_set_dir(QMIINT1, GPIO_IN);
  gpio_set_irq_enabled_with_callback(QMIINT1,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, &gpio_callback);

  gpio_init(CBUT0);
  gpio_set_dir(CBUT0, GPIO_IN);
  gpio_pull_up(CBUT0);
  gpio_set_irq_enabled(CBUT0, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
}
