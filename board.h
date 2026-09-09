#ifndef BOARD_H
#define BOARD_H

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
   Display, I2C, IMU, backlight and battery are in lib/lcd/lcd.h.

   This firmware targets the RP2040-LCD-1.28 only. The touch variant, whose
   button sits on GP16 and would collide with SHIFTED_DIGITAL_IN1, is not
   supported. */

#define QMIINT1       (23)  /* IMU interrupt                       */
#define MODULE_BUTTON (22)  /* button on the module, distinct from
                               BUTTON_INPUT which is SW1 on the HAT */

/* I2C bus, relays, button and digital inputs. */
void board_init_gpio(void);

/* Walks the I2C bus and prints it. A boot diagnostic: the IMU should answer. */
void board_i2c_scan(void);

/* IMU interrupt and module button, both edge triggered. */
void board_init_module_inputs(void);

#endif /* BOARD_H */
