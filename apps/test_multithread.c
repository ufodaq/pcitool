#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#include "pcilib.h"
#include "pcilib/error.h"

const char* prop = "/registers/fpga/reg1";
char* reg;
int stop = 0;

/*
void *get_prop(void *arg)
{
    pcilib_t *ctx = (pcilib_t*)arg;

    while(!stop)
    {
        int err;
        pcilib_value_t val = {0};
        err = pcilib_get_property(ctx, prop, &val);
        if(err)
        {
            pcilib_error("Error in pcilib_read_register");
            return NULL;
        }
        long value = pcilib_get_value_as_int(ctx, &val, &err);
        pcilib_clean_value(ctx, &val);
        if(err)
        {
            pcilib_error("Error in pcilib_get_value_as_int");
            return NULL;
        }
        printf("reg = %li\n", value);
    }
    return NULL;
}
*/

void *read_reg(void *arg)
{
    pcilib_t *ctx = (pcilib_t*)arg;

    while(!stop)
    {
        int err;
        pcilib_register_value_t reg_val = {0};
        pcilib_value_t val = {0};

        err = pcilib_read_register(ctx, NULL, reg, &reg_val);

        if(err)
        {
            pcilib_error("Error in pcilib_read_register");
            return NULL;
        }
        err = pcilib_set_value_from_register_value(ctx, &val, reg_val);
        if(err)
        {
            pcilib_error("Error in pcilib_set_value_from_register_value");
            return NULL;
        }
        long value = pcilib_get_value_as_int(ctx, &val, &err);
        pcilib_clean_value(ctx, &val);
        if(err)
        {
            pcilib_error("Error in pcilib_get_value_as_int");
            return NULL;
        }
        printf("reg = %li\n", value);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 5) {
        printf("Usage:\n\t\t%s <device> <model> <register> <num_threads>\n", argv[0]);
        exit(0);
    }

    reg = argv[3];
    int threads = atoi( argv[4] );

    pcilib_t *ctx = pcilib_open(argv[1], argv[2]);

    for(int i = 0; i < threads; i++)
    {
        pthread_t pth;
        pthread_create(&pth, NULL, read_reg, ctx);
    }

    getchar();
    stop = 1;
    return 0;
}
