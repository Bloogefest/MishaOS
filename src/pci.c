#include "pci.h"
#include "stdlib.h"
#include "terminal.h"
#include "io.h"
#include "driver.h"

uint8_t pci_read8(uint32_t id, uint32_t reg) {
    outl(PCI_CONFIG_ADDR, 0x80000000 | id | (reg & 0xFC));
    return inb(PCI_CONFIG_DATA + (reg & 0x03));
}

void pci_write8(uint32_t id, uint32_t reg, uint8_t data) {
    outl(PCI_CONFIG_ADDR, 0x80000000 | id | (reg & 0xFC));
    outb(PCI_CONFIG_DATA + (reg & 0x03), data);
}

uint16_t pci_read16(uint32_t id, uint32_t reg) {
    outl(PCI_CONFIG_ADDR, 0x80000000 | id | (reg & 0xFC));
    return inw(PCI_CONFIG_DATA + (reg & 0x02));
}

void pci_write16(uint32_t id, uint32_t reg, uint16_t data) {
    outl(PCI_CONFIG_ADDR, 0x80000000 | id | (reg & 0xFC));
    outw(PCI_CONFIG_DATA + (reg & 0x02), data);
}

uint32_t pci_read32(uint32_t id, uint32_t reg) {
    outl(PCI_CONFIG_ADDR, 0x80000000 | id | (reg & 0xFC));
    return inl(PCI_CONFIG_DATA);
}

void pci_write32(uint32_t id, uint32_t reg, uint32_t data) {
    outl(PCI_CONFIG_ADDR, 0x80000000 | id | (reg & 0xFC));
    outl(PCI_CONFIG_DATA, data);
}

static void pci_read_bar(uint32_t id, uint32_t index, uint32_t* address, uint32_t* mask) {
    uint32_t reg = PCI_CONFIG_BAR0 + index * sizeof(uint32_t);
    *address = pci_read32(id, reg);

    pci_write32(id, reg, 0xFFFFFFFF);
    *mask = pci_read32(id, reg);

    pci_write32(id, reg, *address);
}

void pci_get_bar(pci_bar_t* bar, uint32_t id, uint32_t index) {
    uint32_t address_low;
    uint32_t mask_low;
    pci_read_bar(id, index, &address_low, &mask_low);

    if (address_low & PCI_BAR_64) {
        uint32_t address_high;
        uint32_t mask_high;
        pci_read_bar(id, index + 1, &address_high, &mask_high);

        bar->_.address = (void*) (((uint32_t) address_high << 32) | (address_low & ~0xF));
        bar->size = ~(((uint64_t) mask_high << 32) | (mask_low & ~0xF)) + 1;
        bar->flags = address_low & 0xF;
    } else if (address_low & PCI_BAR_IO) {
        bar->_.port = (uint16_t) (address_low & ~0x3);
        bar->size = (uint16_t) (~(mask_low & ~0x3) + 1);
        bar->flags = address_low & 0x3;
    } else {
        bar->_.address = (void*) (address_low & ~0xF);
        bar->size = ~(mask_low & ~0xF) + 1;
        bar->flags = address_low & 0xF;
    }
}

void pci_visit(uint32_t bus, uint32_t dev, uint32_t func) {
    uint32_t id = pci_get_id(bus, dev, func);

    pci_device_info_t info;
    info.vendor_id = pci_read16(id, PCI_CONFIG_VENDOR_ID);
    if (info.vendor_id == 0xFFFF) {
        return;
    }

    info.device_id = pci_read16(id, PCI_CONFIG_DEVICE_ID);
    info.prog_intf = pci_read8(id, PCI_CONFIG_PROG_INTF);
    info.subclass = pci_read8(id, PCI_CONFIG_SUBCLASS);
    info.class_code = pci_read8(id, PCI_CONFIG_CLASS_CODE);

    char buf[11];
    itoa(bus, buf, 16);
    terminal_putstring(buf);
    terminal_putchar(':');
    itoa(dev, buf, 16);
    terminal_putstring(buf);
    terminal_putchar(':');
    itoa(func, buf, 10);
    terminal_putstring(buf);
    terminal_putstring(" 0x");
    itoa(info.vendor_id, buf, 16);
    terminal_putstring(buf);
    terminal_putstring("/0x");
    itoa(info.device_id, buf, 16);
    terminal_putstring(buf);
    terminal_putstring(": ");
    terminal_putstring(pci_class_name(info.class_code, info.subclass, info.prog_intf));
    terminal_putchar('\n');

    if (info.class_code == 0xFF || (info.subclass & 0xFF) == 0x80) {
        return;
    }

    for (const driver_t** driver = PCI_DRIVERS; *driver; driver++) {
        (*driver)->init(&info, bus, dev, func);
    }
}

const char* pci_class_name(uint32_t class_code, uint32_t subclass, uint32_t prog_intf) {
    switch ((class_code << 8) | subclass) {
        case PCI_VGA_COMPATIBLE: return "VGA-Compatible Device";
        case PCI_STORAGE_SCSI: return "SCSI Storage Controller";
        case PCI_STORAGE_IDE: return "IDE Interface";
        case PCI_STORAGE_FLOPPY: return "Floppy Disk Controller";
        case PCI_STORAGE_IPI: return "IPI Bus Controller";
        case PCI_STORAGE_RAID: return "RAID Bus Controller";
        case PCI_STORAGE_ATA: return "ATA Controller";
        case PCI_STORAGE_SATA: return "SATA Controller";
        case PCI_STORAGE_OTHER: return "Mass Storage Controller";
        case PCI_NETWORK_ETHERNET: return "Ethernet Controller";
        case PCI_NETWORK_TOKEN_RING: return "Token Ring Controller";
        case PCI_NETWORK_FDDI: return "FDDI Controller";
        case PCI_NETWORK_ATM: return "ATM Controller";
        case PCI_NETWORK_ISDN: return "ISDN Controller";
        case PCI_NETWORK_WORLDFIP: return "WorldFip Controller";
        case PCI_NETWORK_PICGMG: return "PICMG Controller";
        case PCI_NETWORK_OTHER: return "Network Controller";
        case PCI_DISPLAY_VGA: return "VGA-Compatible Controller";
        case PCI_DISPLAY_XGA: return "XGA-Compatible Controller";
        case PCI_DISPLAY_3D: return "3D Controller";
        case PCI_DISPLAY_OTHER: return "Display Controller";
        case PCI_MULTIMEDIA_VIDEO: return "Multimedia Video Controller";
        case PCI_MULTIMEDIA_AUDIO: return "Multimedia Audio Controller";
        case PCI_MULTIMEDIA_PHONE: return "Computer Telephony Device";
        case PCI_MULTIMEDIA_AUDIO_DEVICE: return "Audio Device";
        case PCI_MULTIMEDIA_OTHER: return "Multimedia Controller";
        case PCI_MEMORY_RAM: return "RAM Memory";
        case PCI_MEMORY_FLASH: return "Flash Memory";
        case PCI_MEMORY_OTHER: return "Memory Controller";
        case PCI_BRIDGE_HOST: return "Host Bridge";
        case PCI_BRIDGE_ISA: return "ISA Bridge";
        case PCI_BRIDGE_EISA: return "EISA Bridge";
        case PCI_BRIDGE_MCA: return "MicroChannel Bridge";
        case PCI_BRIDGE_PCI: return "PCI Bridge";
        case PCI_BRIDGE_PCMCIA: return "PCMCIA Bridge";
        case PCI_BRIDGE_NUBUS: return "NuBus Bridge";
        case PCI_BRIDGE_CARDBUS: return "CardBus Bridge";
        case PCI_BRIDGE_RACEWAY: return "RACEway Bridge";
        case PCI_BRIDGE_OTHER: return "Bridge Device";
        case PCI_COMM_SERIAL: return "Serial Controller";
        case PCI_COMM_PARALLEL: return "Parallel Controller";
        case PCI_COMM_MULTIPORT: return "Multiport Serial Controller";
        case PCI_COMM_MODEM: return "Modem";
        case PCI_COMM_OTHER: return "Communication Controller";
        case PCI_SYSTEM_PIC: return "PIC";
        case PCI_SYSTEM_DMA: return "DMA Controller";
        case PCI_SYSTEM_TIMER: return "Timer";
        case PCI_SYSTEM_RTC: return "RTC";
        case PCI_SYSTEM_PCI_HOTPLUG: return "PCI Hot-Plug Controller";
        case PCI_SYSTEM_SD:  return "SD Host Controller";
        case PCI_SYSTEM_OTHER: return "System Peripheral";
        case PCI_INPUT_KEYBOARD: return "Keyboard Controller";
        case PCI_INPUT_PEN:  return "Pen Controller";
        case PCI_INPUT_MOUSE: return "Mouse Controller";
        case PCI_INPUT_SCANNER: return "Scanner Controller";
        case PCI_INPUT_GAMEPORT: return "Gameport Controller";
        case PCI_INPUT_OTHER: return "Input Controller";
        case PCI_DOCKING_GENERIC: return "Generic Docking Station";
        case PCI_DOCKING_OTHER: return "Docking Station";
        case PCI_PROCESSOR_386: return "386";
        case PCI_PROCESSOR_486: return "486";
        case PCI_PROCESSOR_PENTIUM: return "Pentium";
        case PCI_PROCESSOR_ALPHA: return "Alpha";
        case PCI_PROCESSOR_MIPS: return "MIPS";
        case PCI_PROCESSOR_CO: return "CO-Processor";
        case PCI_SERIAL_FIREWIRE: return "FireWire (IEEE 1394)";
        case PCI_SERIAL_SSA: return "SSA";
        case PCI_SERIAL_USB:
            switch (prog_intf)
            {
                case PCI_SERIAL_USB_UHCI: return "USB (UHCI)";
                case PCI_SERIAL_USB_OHCI: return "USB (OHCI)";
                case PCI_SERIAL_USB_EHCI: return "USB2";
                case PCI_SERIAL_USB_XHCI: return "USB3";
                case PCI_SERIAL_USB_OTHER: return "USB Controller";
                default: return "Unknown USB Class";
            }
        case PCI_SERIAL_FIBER: return "Fiber Channel";
        case PCI_SERIAL_SMBUS: return "SMBus";
        case PCI_WIRELESS_IRDA: return "iRDA Compatible Controller";
        case PCI_WIRLESSS_IR: return "Consumer IR Controller";
        case PCI_WIRLESSS_RF: return "RF Controller";
        case PCI_WIRLESSS_BLUETOOTH: return "Bluetooth";
        case PCI_WIRLESSS_BROADBAND: return "Broadband";
        case PCI_WIRLESSS_ETHERNET_A: return "802.1a Controller";
        case PCI_WIRLESSS_ETHERNET_B: return "802.1b Controller";
        case PCI_WIRELESS_OTHER: return "Wireless Controller";
        case PCI_INTELLIGENT_I2O: return "I2O Controller";
        case PCI_SATELLITE_TV: return "Satellite TV Controller";
        case PCI_SATELLITE_AUDIO: return "Satellite Audio Controller";
        case PCI_SATELLITE_VOICE: return "Satellite Voice Controller";
        case PCI_SATELLITE_DATA: return "Satellite Data Controller";
        case PCI_CRYPT_NETWORK: return "Network and Computing Encryption Device";
        case PCI_CRYPT_ENTERTAINMENT: return "Entertainment Encryption Device";
        case PCI_CRYPT_OTHER: return "Encryption Device";
        case PCI_SP_DPIO: return "DPIO Modules";
        case PCI_SP_OTHER: return "Signal Processing Controller";
    }

    return "Unknown PCI Class";
}