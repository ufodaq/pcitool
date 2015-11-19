#include <linux/pci.h>

int pcidriver_pcie_get_mps(struct pci_dev *dev)
{
        u16 ctl;

        pcie_capability_read_word(dev, PCI_EXP_DEVCTL, &ctl);

        return 128 << ((ctl & PCI_EXP_DEVCTL_PAYLOAD) >> 5);
}

int pcidriver_pcie_set_mps(struct pci_dev *dev, int mps)
{
        u16 v;

        if (mps < 128 || mps > 4096 || !is_power_of_2(mps))
                return -EINVAL;

        v = ffs(mps) - 8;
        if (v > dev->pcie_mpss)
                return -EINVAL;
        v <<= 5;

        return pcie_capability_clear_and_set_word(dev, PCI_EXP_DEVCTL,
                                                  PCI_EXP_DEVCTL_PAYLOAD, v);
}
