#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "pcilib.h"
#include "value.h"
#include "error.h"
#include "unit.h"
#include "tools.h"

void pcilib_clean_value(pcilib_t *ctx, pcilib_value_t *val) {
    if (!val) return;

    if (val->data) {
        free(val->data);
        val->data = NULL;
    }

    memset(val, 0, sizeof(pcilib_value_t));
}

int pcilib_copy_value(pcilib_t *ctx, pcilib_value_t *dst, const pcilib_value_t *src) {
    if (dst->type != PCILIB_TYPE_INVALID)
        pcilib_clean_value(ctx, dst);

    memcpy(dst, src, sizeof(pcilib_value_t));

    if ((src->size)&&(src->data)) {
        dst->data = malloc(src->size);
        if (!dst->data) {
            dst->type = PCILIB_TYPE_INVALID;
            return PCILIB_ERROR_MEMORY;
        }

        memcpy(dst->data, src->data, src->size);
    }

    return 0;
}

int pcilib_set_value_from_float(pcilib_t *ctx, pcilib_value_t *value, double fval) {
    pcilib_clean_value(ctx, value);

    value->type = PCILIB_TYPE_DOUBLE;
    value->fval = fval;

    return 0;
}

int pcilib_set_value_from_int(pcilib_t *ctx, pcilib_value_t *value, long ival) {
    pcilib_clean_value(ctx, value);

    value->type = PCILIB_TYPE_LONG;
    value->ival = ival;

    return 0;
}

int pcilib_set_value_from_register_value(pcilib_t *ctx, pcilib_value_t *value, pcilib_register_value_t regval) {
    return pcilib_set_value_from_int(ctx, value, regval);
}

int pcilib_set_value_from_static_string(pcilib_t *ctx, pcilib_value_t *value, const char *str) {
    pcilib_clean_value(ctx, value);

    value->type = PCILIB_TYPE_STRING;
    value->sval = str;

    return 0;
}

double pcilib_get_value_as_float(pcilib_t *ctx, const pcilib_value_t *val, int *ret) {
    int err;
    double res;
    pcilib_value_t copy = {0};

    err = pcilib_copy_value(ctx, &copy, val);
    if (err) {
        if (ret) *ret = err;
        return 0.;
    }

    err = pcilib_convert_value_type(ctx, &copy, PCILIB_TYPE_DOUBLE);
    if (err) {
        if (ret) *ret = err;
        return 0.;
    }

    if (ret) *ret = 0;
    res = copy.fval;

    pcilib_clean_value(ctx, &copy);

    return res;
}

long pcilib_get_value_as_int(pcilib_t *ctx, const pcilib_value_t *val, int *ret) {
    int err;
    long res;
    pcilib_value_t copy = {0};

    err = pcilib_copy_value(ctx, &copy, val);
    if (err) {
        if (ret) *ret = err;
        return 0;
    }

    err = pcilib_convert_value_type(ctx, &copy, PCILIB_TYPE_LONG);
    if (err) {
        if (ret) *ret = err;
        return 0;
    }

    if (ret) *ret = 0;
    res = copy.ival;

    pcilib_clean_value(ctx, &copy);

    return res;
}

pcilib_register_value_t pcilib_get_value_as_register_value(pcilib_t *ctx, const pcilib_value_t *val, int *ret) {
    int err;
    pcilib_register_value_t res;
    pcilib_value_t copy = {0};

    err = pcilib_copy_value(ctx, &copy, val);
    if (err) {
        if (ret) *ret = err;
        return 0.;
    }

    err = pcilib_convert_value_type(ctx, &copy, PCILIB_TYPE_LONG);
    if (err) {
        if (ret) *ret = err;
        return 0.;
    }

    if (ret) *ret = 0;
    res = copy.ival;

    pcilib_clean_value(ctx, &copy);

    return res;
}




/*
double pcilib_value_get_float(pcilib_value_t *val) {
    pcilib_value_t copy;

    if (val->type == PCILIB_TYPE_DOUBLE)
        return val->fval;

    err = pcilib_copy_value(&copy, val);
    if (err) ???
}


long pcilib_value_get_int(pcilib_value_t *val) {
}
*/

int pcilib_convert_value_unit(pcilib_t *ctx, pcilib_value_t *val, const char *unit_name) {
    if (val->type == PCILIB_TYPE_INVALID) {
        pcilib_error("Can't convert uninitialized value");
        return PCILIB_ERROR_NOTINITIALIZED;
    }

    if ((val->type != PCILIB_TYPE_DOUBLE)&&(val->type != PCILIB_TYPE_LONG)) {
        pcilib_error("Can't convert value of type %u, only values with integer and float types can be converted to different units", val->type);
        return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    if (!val->unit) {
        pcilib_error("Can't convert value with the unspecified unit");
        return PCILIB_ERROR_INVALID_ARGUMENT;
    }

    pcilib_unit_t unit = pcilib_find_unit_by_name(ctx, val->unit);
    if (unit == PCILIB_UNIT_INVALID) {
        pcilib_error("Can't convert unrecognized unit %s", val->unit);
        return PCILIB_ERROR_NOTFOUND;
    }

    return pcilib_transform_unit_by_name(ctx, unit_name, val);
}

int pcilib_convert_value_type(pcilib_t *ctx, pcilib_value_t *val, pcilib_value_type_t type) {
    if (val->type == PCILIB_TYPE_INVALID) {
        pcilib_error("Can't convert uninitialized value");
        return PCILIB_ERROR_NOTINITIALIZED;
    }

    switch (type) {
     case PCILIB_TYPE_STRING:
        switch (val->type) {
         case PCILIB_TYPE_STRING:
            return 0;
         case PCILIB_TYPE_DOUBLE:
            sprintf(val->str, (val->format?val->format:"%lf"), val->fval);
            val->format = NULL;
            break;
         case PCILIB_TYPE_LONG:
            sprintf(val->str, (val->format?val->format:"%li"), val->ival);
            val->format = NULL;
            break;
         default:
            return PCILIB_ERROR_NOTSUPPORTED;
        }
        val->sval = val->str;
        break;
     case PCILIB_TYPE_DOUBLE:
        switch (val->type) {
         case PCILIB_TYPE_STRING:
            if (sscanf(val->sval, "%lf", &val->fval) != 1) {
                pcilib_warning("Can't convert string (%s) to float", val->sval);
                return PCILIB_ERROR_INVALID_DATA;
            }
            val->format = NULL;
            break;
         case PCILIB_TYPE_DOUBLE:
            return 0;
         case PCILIB_TYPE_LONG:
            val->fval = val->ival;
            val->format = NULL;
            break;
         default:
            return PCILIB_ERROR_NOTSUPPORTED;
        }
        break;
     case PCILIB_TYPE_LONG:
        switch (val->type) {
         case PCILIB_TYPE_STRING:
            if (pcilib_isnumber(val->sval)) {
                if (sscanf(val->sval, "%li", &val->ival) != 1) {
                    pcilib_warning("Can't convert string (%s) to int", val->sval);
                    return PCILIB_ERROR_INVALID_DATA;
                }
                val->format = NULL;
            } else if (pcilib_isxnumber(val->sval)) {
                if (sscanf(val->sval, "%lx", &val->ival) != 1) {
                    pcilib_warning("Can't convert string (%s) to int", val->sval);
                    return PCILIB_ERROR_INVALID_DATA;
                }
                val->format = "0x%lx";
            } else {
                pcilib_warning("Can't convert string (%s) to int", val->sval);
                return PCILIB_ERROR_INVALID_DATA;
            }
            break;
         case PCILIB_TYPE_DOUBLE:
            val->ival = round(val->fval);
            val->format = NULL;
            break;
         case PCILIB_TYPE_LONG:
            return 0;
         default:
            return PCILIB_ERROR_NOTSUPPORTED;
        }
     break;
     default:
        return PCILIB_ERROR_NOTSUPPORTED;
    }

    return 0;
}
