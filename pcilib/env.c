#include <stdio.h>
#include <stdlib.h>
#include "env.h"


static const char *pcilib_env_cache[PCILIB_MAX_ENV] = {0};

const char *pcilib_getenv(pcilib_env_t env, const char *var) {
    if (!pcilib_env_cache[env]) {
	const char *var_env = getenv(var);
	pcilib_env_cache[env] = var_env?var_env:(void*)-1;
	return var_env;
    }

    return (pcilib_env_cache[env] == (void*)-1)?NULL:pcilib_env_cache[env];
}

