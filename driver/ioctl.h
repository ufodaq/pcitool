#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,11)
int pcidriver_ioctl(struct inode  *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#else
long pcidriver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif
