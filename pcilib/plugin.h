#ifndef _PCILIB_PLUGIN_H
#define _PCILIB_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Loads the specified plugin
 * The plugin symbols are loaded localy and not available for symbol resolution, but should be requested
 * with pcilib_plugin_get_symbol() instead.
 * @param[in] name	- the plugin name (with extension, but without path)
 * @return		- dlopen'ed plugin or NULL in the case of error
 */
void *pcilib_plugin_load(const char *name);

/**
 * Cleans up the loaded plugin
 * @param[in] plug	- plugin loaded with pcilib_plugin_load()
 */
void pcilib_plugin_close(void *plug);

/**
 * Resolves the specified symbol in the plugin
 * @param[in] plug	- plugin loaded with pcilib_plugin_load()
 * @param[in] symbol	- name of the symbol 
 * @return		- pointer to the symbol or NULL if not found
 */
void *pcilib_plugin_get_symbol(void *plug, const char *symbol);

/**
 * Verifies if plugin can handle the hardware and requests appropriate model description
 * @param[in,out] ctx 	- pcilib context
 * @param[in] plug	- plugin loaded with pcilib_plugin_load()
 * @param[in] vendor_id	- Vendor ID reported by hardware
 * @param[in] device_id	- Device ID reported by hardware
 * @param[in] model	- the requested pcilib model or NULL for autodetction
 * @return		- the appropriate model description or NULL if the plugin does not handle the installed hardware or requested model
 */
const pcilib_model_description_t *pcilib_get_plugin_model(pcilib_t *ctx, void *plug, unsigned short vendor_id, unsigned short device_id, const char *model);

/**
 * Finds the appropriate plugin and returns model description.
 *
 * The function sequentially loads plugins available in ::PCILIB_PLUGIN_DIR and
 * checks if they support the installed hardware and requested model. If hardware
 * is not supported, the plugin is immideately unloaded. On a first success 
 * the model description is returned to caller and no further plguins are loaded. 
 * If no suitable plugin is found, the NULL is returned. 
 *
 * If model is specified, first a plugin with the same name is loaded and check performed
 * if it can handle the installed hardware. If not, we iterate over all available
 * plugins as usual.
 *
 * @param[in,out] ctx 	- pcilib context
 * @param[in] vendor_id	- Vendor ID reported by hardware
 * @param[in] device_id	- Device ID reported by hardware
 * @param[in] model	- the requested pcilib model or NULL for autodetction
 * @return		- the appropriate model description or NULL if no plugin found able to handle the installed hardware or requested model
 */
const pcilib_model_description_t *pcilib_find_plugin_model(pcilib_t *ctx, unsigned short vendor_id, unsigned short device_id, const char *model);

#ifdef __cplusplus
}
#endif

#endif /* _PCILIB_PLUGIN_H */
