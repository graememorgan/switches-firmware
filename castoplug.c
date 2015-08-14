#include <spi.h>
#include "castoplug.h"

void rfm22_csn_setup(void) {
    
}

void rfm22_csn_low(void) {
    PORTBbits.RB4 = 0;
}

void rfm22_csn_high(void) {
    PORTBbits.RB4 = 1;
}

void rfm22_csn_wait(void) {
    Delay10TCYx(1);
}

unsigned char rfm22_read_reg(unsigned char i) {
    unsigned char x;
    rfm22_csn_low();
    rfm22_csn_wait();
    WriteSPI(i);
    x = ReadSPI();
    rfm22_csn_high();
    return x;
}

void rfm22_write_reg(unsigned char i, unsigned char x) {
    rfm22_csn_low();
    rfm22_csn_wait();
    WriteSPI(i | 0x80);
    WriteSPI(x);
    rfm22_csn_high();
}

unsigned char rfm22_read_vc(void) {
    /* vendor code */
    return rfm22_read_reg(0x01);
}

unsigned char rfm22_read_ds(void) {
    /* device status */
    return rfm22_read_reg(0x02);
}

unsigned char rfm22_read_is1(void) {
    /* interrupt status 1 */
    return rfm22_read_reg(0x03);
}

void rfm22_write_ie2(unsigned char x) {
    /* interrupt enable 2 */
    rfm22_write_reg(0x06, x);
}

void rfm22_write_omfc1(unsigned char x) {
    /* operating mode and function control 1 */
    rfm22_write_reg(0x07, x);
}

void rfm22_write_omfc2(unsigned char x) {
    /* operating mode and function control 2 */
    rfm22_write_reg(0x08, x);
}

unsigned char rfm22_read_omfc2(void) {
    /* operating mode and function control 2 */
    return rfm22_read_reg(0x08);
}

void rfm22_write_gc0(unsigned char x) {
    /* gpio configuration 0 */
    rfm22_write_reg(0x0b, x);
}

void rfm22_write_gc1(unsigned char x) {
    /* gpio configuration 1 */
    rfm22_write_reg(0x0c, x);
}

void rfm22_write_dac(unsigned char x) {
    /* data access control */
    rfm22_write_reg(0x30, x);
}

void rfm22_write_hc1(unsigned char x) {
    /* header control 1 */
    rfm22_write_reg(0x32, x);
}

void rfm22_write_hc2(unsigned char x) {
    /* header control 2 */
    rfm22_write_reg(0x33, x);
}

void rfm22_write_pl(unsigned char x) {
    /* preamble length */
    rfm22_write_reg(0x34, x);
}

void rfm22_write_tpow(unsigned char x) {
    /* tx power */
    rfm22_write_reg(0x6d, x);
}

void rfm22_write_trate(unsigned int x) {
    /* tx data rate */
    rfm22_write_reg(0x6e, (unsigned char)(x >> 8));
    rfm22_write_reg(0x6f, (unsigned char)(x >> 0));
}

void rfm22_write_mmc1(unsigned char x) {
    /* modulation mode control 1 */
    rfm22_write_reg(0x70, x);
}

void rfm22_write_mmc2(unsigned char x) {
    /* modulation mode control 2 */
    rfm22_write_reg(0x71, x);
}

void rfm22_write_fo(unsigned int x) {
    /* frequency offset */
    rfm22_write_reg(0x73, (unsigned char)(x >> 0));
    rfm22_write_reg(0x74, (unsigned char)(x >> 8));
}

unsigned char rfm22_read_fbs(void) {
    /* frequency band select */
    return rfm22_read_reg(0x75);
}

void rfm22_write_fbs(unsigned char x) {
    /* frequency band select */
    rfm22_write_reg(0x75, x);
}

void rfm22_write_ncf(unsigned int x) {
    /* nominal carrier frequency */
    rfm22_write_reg(0x76, (unsigned char)(x >> 8));
    rfm22_write_reg(0x77, (unsigned char)(x >> 0));
}

unsigned char rfm22_read_ncf1(void) {
    /* nominal carrier frequency 1 */
    return rfm22_read_reg(0x76);
}

void rfm22_write_tfc1(unsigned char x) {
    /* tx fifo control 1 */
    rfm22_write_reg(0x7c, x);
}

void rfm22_write_rfc(unsigned char x) {
    /* rx fifo control */
    rfm22_write_reg(0x7e, x);
}

unsigned char rfm22_read_fa(void) {
    /* fifo access */
    return rfm22_read_reg(0x7f);
}

void rfm22_write_fa(unsigned char x) {
    /* fifo access */
    return rfm22_write_reg(0x7f, x);
}

void rfm22_reset_fifo(void) {
    const unsigned char x =    rfm22_read_omfc2();
    rfm22_write_omfc2(x | (1 << 1) | (1 << 0));
    rfm22_write_omfc2(x & ~((1 << 1) | (1 << 0)));
}

void rfm22_setup(void) {
    OpenSPI(SPI_FOSC_64, MODE_00, SMPEND);
    rfm22_csn_setup();

    /* gpio configuration */
    rfm22_write_gc0(0x12);
    rfm22_write_gc1(0x15);

    /* crystal frequency capacitance */
    rfm22_write_reg(0x09, 0x7f);

    /* pll, tx, xtal on */
    rfm22_write_omfc1((1 << 3) | (1 << 1) | (1 << 0));

    /* automatic transmission */
    rfm22_write_omfc2(1 << 3);

    /* disable interrupts */
    rfm22_write_ie2(0x00);

    /* tx send size to 4 */
    rfm22_write_tfc1(0x04);

    /* tx power */
    rfm22_write_tpow(0x03);

    /* 433.92 MHz carrier */
    /* refer to rfm22.pdf, 3.6 */
    rfm22_write_fo(0);
    rfm22_write_fbs(19);

    /* -40KHz */
    rfm22_write_ncf(25340);

    /* ook modulation */
    /* refer to rfm22.pdf, 7.2 */
    /* direct asynchronous mode */
    /* miso used for tx data clock */
    /* mosi used for tx data */
    rfm22_write_mmc1(0 << 5);
    rfm22_write_mmc2((0 << 6) | (1 << 4) | (1 << 0));

    /* 40Kbps tx data rate */
    rfm22_write_trate(0xffff);


    //rfm22_write_reg(0x35, 0); // preamble length
    /* minimal rx packet handler */
    /* receive anything after preamble and sync */
    rfm22_write_dac(0);
    /* disable checks */
    rfm22_write_hc1(0x00);
    rfm22_write_hc2(0x00);
    rfm22_write_pl(0x00);

    rfm22_reset_fifo();
}

void castoplug_setup(void) {
    rfm22_setup();
    CloseSPI();
}

/* castoplug_send_xxx routines */

#define castoplug_send_pulse(__thigh, __tlow)	\
do {						\
    PORTCbits.RC7 = 1;	                        \
    delay_10us(__thigh/10);			\
    PORTCbits.RC7 = 0;	                        \
    delay_10us(__tlow/10);			\
} while (0)

void castoplug_send_pulse_0(void) {
    castoplug_send_pulse(400, 940);
}

void castoplug_send_pulse_1(void) {
    castoplug_send_pulse(1005, 340);
}

unsigned char groups[] = {0x15, 0x45, 0x51, 0x54};
unsigned char IDs[] = {0x2A, 0x8A, 0xA2};
unsigned char commands[] = {0xA8, 0xAE};

void castoplug_bitblast(unsigned char d) {
    unsigned char i = 8;
    while (i--) {
        if (d & 0x80) {
            castoplug_send_pulse_1();
        } else {
            castoplug_send_pulse_0();
        }
        d <<= 1;
    }
}

void castoplug_send(unsigned char g, unsigned char d, unsigned char c) {
    castoplug_bitblast(groups[g]);
    castoplug_send_pulse_0();
    castoplug_bitblast(IDs[d]);
    castoplug_bitblast(commands[c]);
}
