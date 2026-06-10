#ifndef __SHT20_H
#define __SHT20_H

#include "main.h"
#include "i2c.h"
#include "sensor_types.h"

#define SHT20_I2C_ADDR      0x80

#define SHT20_CMD_TEMP_NH   0xF3
#define SHT20_CMD_HUMI_NH   0xF5
#define SHT20_CMD_RESET     0xFE

#define SHT20_WAIT_TEMP_MS  85
#define SHT20_WAIT_HUMI_MS  29

uint8_t SHT20_Init(void);
uint8_t SHT20_ReadData(SensorData_t *data);
uint8_t SHT20_CRC8_Check(uint8_t *buf, uint8_t len);

#endif
