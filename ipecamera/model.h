#ifndef _IPECAMERA_MODEL_H
#define _IPECAMERA_MODEL_H

#include <stdio.h>

#include "pcilib.h"
#include "image.h"

#define IPECAMERA_REGISTER_SPACE 0xfeaffc00
#define IPECAMERA_REGISTER_WRITE (IPECAMERA_REGISTER_SPACE + 0)
#define IPECAMERA_REGISTER_READ (IPECAMERA_REGISTER_WRITE + 4)

#ifdef _IPECAMERA_MODEL_C
pcilib_register_bank_description_t ipecamera_register_banks[] = {
    { PCILIB_REGISTER_BANK0, 128, IPECAMERA_REGISTER_PROTOCOL, IPECAMERA_REGISTER_READ,	IPECAMERA_REGISTER_WRITE, PCILIB_BIG_ENDIAN, 8, PCILIB_LITTLE_ENDIAN, "cmosis", "CMOSIS CMV2000 Registers" },
    { PCILIB_REGISTER_BANK1, 64, PCILIB_DEFAULT_PROTOCOL, IPECAMERA_REGISTER_SPACE, IPECAMERA_REGISTER_SPACE, PCILIB_BIG_ENDIAN, 32, PCILIB_LITTLE_ENDIAN, "fpga", "IPECamera Registers" },
    { 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL }
};

pcilib_register_description_t ipecamera_registers[] = {
{1, 	0, 	16, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines",  ""},
{3, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start1", ""},
{5, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start2", ""},
{7, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start3", ""},
{9, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start4", ""},
{11,	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start5", ""},
{13, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start6", ""},
{15, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start7", ""},
{17, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "start8", ""},
{19, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines1", ""},
{21, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines2", ""},
{23, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines3", ""},
{25, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines4", ""},
{27, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines5", ""},
{29, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines6", ""},
{31, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines7", ""},
{33, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_lines8", ""},
{35, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "sub_s", ""},
{37, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "sub_a", ""},
{39, 	0, 	1, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "color", ""},
{40, 	0, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "image_flipping", ""},
{41, 	0, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_flags", ""},
{42, 	0, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_time", ""},
{45, 	0, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_step", ""},
{48, 	0, 	24, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_kp1", ""},
{51, 	0, 	24, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_kp2", ""},
{54, 	0, 	2, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "nr_slopes", ""},
{55, 	0, 	8, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_seq", ""},
{56, 	0, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_time2", ""},
{59, 	0, 	24, 	1088, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_step2", ""},
{68, 	0, 	2, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "nr_slopes2", ""},
{69, 	0, 	8, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "exp_seq2", ""},
{70, 	0, 	16, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "number_frames", ""},
{72, 	0, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "output_mode", ""},
{78, 	0, 	12, 	85, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "training_pattern", ""},
{80, 	0, 	18, 	0x3FFFF,PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "channel_en", ""},
{89, 	0, 	8, 	96, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "vlow2", ""},
{90, 	0, 	8, 	96, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "vlow3", ""},
{100, 	0, 	14, 	16260, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "offset", ""},
{102, 	0, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "pga", ""},
{103, 	0, 	8, 	32, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "adc_gain", ""},
{111, 	0, 	1, 	1, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "bit_mode", ""},
{112, 	0, 	2, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "adc_resolution", ""},
{126, 	0, 	16, 	0, 	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK0, "temp", ""},
{0,	0, 	32,	0,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK1, "spi_conf_input", ""},
{1,	0, 	32,	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "spi_conf_output", ""},
{2,	0, 	32,	0,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK1, "spi_clk_speed", ""},
{3,	0, 	32,	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "firmware_version", ""},
{4,	0, 	32, 	0,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK1, "control", ""},
{5,	0, 	32, 	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "status", ""},
{6,	0, 	16,	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "cmosis_temperature", ""},
{7,	0, 	32,	0,	PCILIB_REGISTER_RW, PCILIB_REGISTER_BANK1, "temperature_sample_timing", ""},
{8,	0, 	32, 	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "start_address", ""},
{9,	0, 	32, 	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "end_address", ""},
{10,	0, 	32, 	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "last_write_address", ""},
{11,	0, 	32, 	0,	PCILIB_REGISTER_R,  PCILIB_REGISTER_BANK1, "last_write_value", ""},
{0,	0,	0,	0,	0,                  0,                     NULL, NULL}
};

pcilib_register_range_t ipecamera_register_ranges[] = {
    {0, 128, PCILIB_REGISTER_BANK0}, {0, 0, 0}
};

#else
extern pcilib_register_description_t ipecamera_registers[];
extern pcilib_register_bank_description_t ipecamera_register_banks[];
extern pcilib_register_range_t ipecamera_register_ranges[];
#endif 

#ifdef _IPECAMERA_IMAGE_C
pcilib_event_api_description_t ipecamera_image_api = {
    ipecamera_init,
    ipecamera_free,

    ipecamera_reset,
    ipecamera_start,
    ipecamera_stop,
    ipecamera_trigger,
    
    ipecamera_get,
    ipecamera_return
};
#else
extern pcilib_event_api_description_t ipecamera_image_api;
#endif

int ipecamera_read(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t *value);
int ipecamera_write(pcilib_t *ctx, pcilib_register_bank_description_t *bank, pcilib_register_addr_t addr, uint8_t bits, pcilib_register_value_t value);

#endif /* _IPECAMERA_MODEL_H */
