#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "model.h"
#include "error.h"
#include "pci.h"
#include "config.h"

void *pcilib_plugin_load(const char *name) {
    void *plug;
    char *fullname;
    const char *path;

    path = getenv("PCILIB_PLUGIN_DIR");
    if (!path) path = PCILIB_PLUGIN_DIR;

    fullname = malloc(strlen(path) + strlen(name) + 2);
    if (!fullname) return NULL;

    sprintf(fullname, "%s/%s", path, name);

    plug = dlopen(fullname, RTLD_LAZY|RTLD_LOCAL);
    free(fullname);

    return plug;
}

void pcilib_plugin_close(void *plug) {
    if (plug) 
	dlclose(plug);

}

void *pcilib_plugin_get_symbol(void *plug, const char *symbol) {
    if ((!plug)||(!symbol)) return NULL;
    return dlsym(plug, symbol);
}

const pcilib_model_description_t *pcilib_get_plugin_model(pcilib_t *pcilib, void *plug, unsigned short vendor_id, unsigned short device_id, const char *model) {
    void *get_model;
    const pcilib_model_description_t *model_info;

    if (!plug) return NULL;

    get_model = dlsym(plug, "pcilib_get_event_model");
    if (!get_model) return NULL;

    model_info = ((const pcilib_model_description_t *(*)(pcilib_t *pcilib, unsigned short vendor_id, unsigned short device_id, const char *model))get_model)(pcilib, vendor_id, device_id, model);
    if (!model_info) return model_info;

    if (model_info->interface_version != PCILIB_EVENT_INTERFACE_VERSION) {
	pcilib_warning("Plugin %s exposes outdated interface version (%lu), pcitool supports (%lu)", model_info->name, model_info->interface_version, PCILIB_EVENT_INTERFACE_VERSION);
	return NULL;
    }

    return model_info;
}

const pcilib_model_description_t *pcilib_find_plugin_model(pcilib_t *pcilib, unsigned short vendor_id, unsigned short device_id, const char *model) {
    DIR *dir;
    const char *path;
    struct dirent *entry;

    void *plugin;
    const pcilib_model_description_t *model_info = NULL;


    path = getenv("PCILIB_PLUGIN_DIR");
    if (!path) path = PCILIB_PLUGIN_DIR;

    if (model) {
	plugin = pcilib_plugin_load(model);
	if (plugin) {
	    model_info = pcilib_get_plugin_model(pcilib, plugin, vendor_id, device_id, model);
	    if (model_info) {
		pcilib->event_plugin = plugin;
		return model_info;
	    }
	    pcilib_plugin_close(plugin);
	}
    }

    dir = opendir(path);
    if (!dir) return NULL;

    while ((entry = readdir(dir))) {
	const char *suffix = strstr(entry->d_name, ".so");
	if ((!suffix)||(strlen(suffix) != 3)) continue;

	if ((model)&&(!strcmp(entry->d_name, model))) 
	    continue;

	plugin = pcilib_plugin_load(entry->d_name);
	if (plugin) {
	    model_info = pcilib_get_plugin_model(pcilib, plugin, vendor_id, device_id, model);
	    if (model_info) {
		pcilib->event_plugin = plugin;
		break;
	    }
	    pcilib_plugin_close(plugin);
	}
    }

    closedir(dir);
    return model_info;
}
