#ifndef TPS65132_H
#define TPS65132_H
#include <mach/mt_gpio.h>

extern int tps65132_write_bytes(unsigned char addr, unsigned char value);
extern int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value);
#endif
