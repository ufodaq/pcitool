#ifndef _PCIDRIVER_PCIBUS_H
#define _PCIDRIVER_PCIBUS_H

int pcidriver_pcie_get_mps(struct pci_dev *dev);
int pcidriver_pcie_set_mps(struct pci_dev *dev, int mps);

#endif /* _PCIDRIVER_PCIBUS_H */
