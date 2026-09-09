#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

/* ---------- signals added by the HAT ----------
   Same table as the pinout section of README.md. */

#define SHIFTED_DIGITAL_IN1 (16)  /* input                  */
#define SHIFTED_DIGITAL_IN2 (17)  /* input                  */
#define BUTTON_INPUT        (18)  /* input, pull-up         */
#define RELAY1_COMMAND_PIN  (19)  /* output                 */
#define RELAY2_COMMAND_PIN  (20)  /* output                 */
#define ALS_OUT             (26)  /* analog input, ADC0     */
#define SENSOR2_FILTERED    (27)  /* analog input, ADC1     */
#define SENSOR1_FILTERED    (28)  /* analog input, ADC2     */

/* ---------- ADC channels ----------
   The RP2040 channel is not the GPIO number:
   GP26=ADC0, GP27=ADC1, GP28=ADC2, GP29=ADC3 (the module's battery sense). */

#define ALS_ADC_CHANNEL     (0)
#define SENSOR2_ADC_CHANNEL (1)
#define SENSOR1_ADC_CHANNEL (2)

/* ---------- pins of the module itself ----------
   Display, I2C, IMU, backlight and battery are in lib/lcd/lcd.h. */

#define QMIINT1    (23)  /* IMU interrupt                                   */
#define CBUT_PLAIN (22)  /* user button, RP2040-LCD-1.28                    */
#define CBUT_TOUCH (16)  /* user button, RP2040-TOUCH-LCD-1.28. Note that
                            this is also SHIFTED_DIGITAL_IN1: on the touch
                            board the two share GP16.                       */

/* true when the touch controller answered on the I2C bus, i.e. the board is
   an RP2040-TOUCH-LCD-1.28. Set by board_i2c_scan(). */
extern bool rp2040_touch;

/* User button pin, CBUT_PLAIN or CBUT_TOUCH depending on the board. */
extern uint8_t CBUT0;

/* I2C bus, relays, button and digital inputs. */
void board_init_gpio(void);

/* Walks the I2C bus and prints it. Detects the touch board on the way and
   adjusts CBUT0 and LCD_RST_PIN, so it has to run before lcd_init(). */
void board_i2c_scan(void);

/* IMU interrupt and user button, both edge triggered. Runs after
   CST816S_init(), which owns the touch controller. */
void board_init_module_inputs(void);

#endif /* BOARD_H */
