#ifndef _IPECAMERA_H
#define _IPECAMERA_H

#include <stdio.h>

#include "pcilib.h"

#define IPECAMERA_REGISTER_SPACE 0xfeaffc00
#define IPECAMERA_REGISTER_WRITE (IPECAMERA_REGISTER_SPACE + 0)
#define IPECAMERA_REGISTER_READ (IPECAMERA_REGISTER_WRITE + 4)

#ifdef _IPECAMERA_C
pcilib_register_bank_description_t ipecamera_register_banks[] = {
    { PCILIB_REGISTER_BANK0, 128, IPECAMERA_REGISTER_PROTOCOL, IPECAMERA_REGISTER_READ,	IPECAMERA_REGISTER_WRITE, PCILIB_BIG_ENDIAN, 8, PCILIB_LITTLE_ENDIAN, "cmosis", "CMOSIS CMV2000 Registers" },
    { PCILIB_REGISTER_BANK1, 64, PCILIB_DEFAULT_PROTOCOL, IPECAMERA_REGISTER_SPACE, IPECAMERA_REGISTER_SPACE, PCILIB_BIG_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "fpga", "IPECamera Registers" },
    { 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL }
};

pcilib_register_description_t ipecamera_registers[] = {
{1, 	16, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines",  ""},
{3, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start1", ""},
{5, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start2", ""},
{7, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start3", ""},
{9, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start4", ""},
{11,	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start5", ""},
{13, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start6", ""},
{15, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start7", ""},
{17, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start8", ""},
{19, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines1", ""},
{21, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines2", ""},
{23, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines3", ""},
{25, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines4", ""},
{27, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines5", ""},
{29, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines6", ""},
{31, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines7", ""},
{33, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines8", ""},
{35, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "sub_s", ""},
{37, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "sub_a", ""},
{39, 	1, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "color", ""},
{40, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "image_flipping", ""},
{41, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_flags", ""},
{42, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_time", ""},
{45, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_step", ""},
{48, 	24, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_kp1", ""},
{51, 	24, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_kp2", ""},
{54, 	2, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "nr_slopes", ""},
{55, 	8, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_seq", ""},
{56, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_time2", ""},
{59, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_step2", ""},
{68, 	2, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "nr_slopes2", ""},
{69, 	8, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_seq2", ""},
{70, 	16, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_frames", ""},
{72, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "output_mode", ""},
{78, 	12, 	85, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "training_pattern", ""},
{80, 	18, 	0x3FFFF,PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "channel_en", ""},
{89, 	8, 	96, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "vlow2", ""},
{90, 	8, 	96, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "vlow3", ""},
{100, 	14, 	16260, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "offset", ""},
{102, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "pga", ""},
{103, 	8, 	32, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "adc_gain", ""},
{111, 	1, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "bit_mode", ""},
{112, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "adc_resolution", ""},
{126, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "temp", ""},
{0,	32,	0,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK1, "spi_conf_input", ""},
{1,	32,	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "spi_conf_output", ""},
{2,	32,	0,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK1, "spi_clk_speed", ""},
{3,	32,	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "firmware_version", ""},
{6,	16,	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "cmosis_temperature", ""},
{7,	32,	0,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK1, "temperature_sample_timing", ""},
{0,	0,	0,	0,                  0,                     NULL, NULL}
};

pcilib_register_range_t ipecamera_register_ranges[] = {
    {0, 128, PCILIB_REGISTER_BANK0}, {0, 0, 0}
};

#else
extern pcilib_register_description_t ipecamera_registers[];
extern pcilib_register_bank_description_t ipecamera_register_banks[];
extern pcilib_register_range_t ipecamera_register_ranges[];
#endif 

int ipecamera_read(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t *value);
int ipecamera_write(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t value);

#endif /* _IPECAMERA_H */
