#ifndef _PCILIB_PLUGIN_H
#define _PCILIB_PLUGIN_H

void *pcilib_plugin_load(const char *name);
void pcilib_plugin_close(void *plug);
void *pcilib_plugin_get_symbol(void *plug, const char *symbol);
const pcilib_model_description_t *pcilib_get_plugin_model(pcilib_t *pcilib, void *plug, unsigned short vendor_id, unsigned short device_id, const char *model);
const pcilib_model_description_t *pcilib_find_plugin_model(pcilib_t *pcilib, unsigned short vendor_id, unsigned short device_id, const char *model);

#endif /* _PCILIB_PLUGIN_H */
