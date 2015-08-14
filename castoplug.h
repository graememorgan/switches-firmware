#ifndef CASTOPLUG_H
#define	CASTOPLUG_H

#include <delays.h>

// define our own custom delay
// instruction rate is 12 MHz, so we want to delay for 1.2 * d
#define delay_10us(d)  Delay100TCYx((d) + (d) / 5)
#define delay_ms(d)    Delay10KTCYx((d) + (d) / 5)

void castoplug_setup(void);

void castoplug_send(unsigned char g, unsigned char d, unsigned char c);

#endif

