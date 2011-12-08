#ifndef _IPECAMERA_H
#define _IPECAMERA_H

typedef struct ipecamera_s ipecamera_t;

typedef  struct {
    int bpp;			/*<< Bits per pixel (8, 16, or 32) as returned by IPECAMERA_IMAGE_DATA */
    int real_bpp;		/*<< Bits per pixel as returned by camera and IPECAMERA_PACKED_IMAGE */
    int width, height;
} ipecamera_image_dimensions_t;

typedef enum {
    IPECAMERA_RAW_DATA = 0,
    IPECAMERA_IMAGE_DATA = 1,
    IPECAMERA_DIMENSIONS = 0x8000,
    IPECAMERA_IMAGE_REGION = 0x8010,
    IPECAMERA_PACKED_IMAGE = 0x8020,
    IPECAMERA_PACKED_LINE = 0x8021,
    IPECAMERA_PACKED_PAYLOAD = 0x8022,
    IPECAMERA_CHANGE_MASK = 0x8030
} ipecamera_data_type_t;

typedef uint16_t ipecamera_change_mask_t;
typedef uint16_t ipecamera_pixel_t;


int ipecamera_set_buffer_size(ipecamera_t *ctx, int size);

#endif /* _IPECAMERA_H */
