#ifndef _PCITOOL_SYSINFO_H
#define _PCITOOL_SYSINFO_H

size_t get_free_memory();
int get_file_fs(const char *fname, size_t size, char *fs);

#endif /* _PCITOOL_SYSINFO_H */
