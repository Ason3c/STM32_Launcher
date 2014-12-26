#include "stm32f10x.h"


//I2C1 on pins 6 and 7 - configured in i2c_int.h
#define I2C1_SCL	GPIO_Pin_6
#define I2C1_SDA	GPIO_Pin_7


extern uint8_t bootsource;

void setup_gpio(void);
uint8_t get_wkup(void);