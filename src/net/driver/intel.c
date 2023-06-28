#include "intel.h"

#include "../buf.h"
#include "../ipv4.h"
#include "../eth.h"
#include "../../io.h"
#include "../../pic.h"
#include "../../isrs.h"
#include "../../paging.h"
#include "../../string.h"
#include "../../terminal.h"

#define RX_DESC_COUNT 32
#define TX_DESC_COUNT 8

#define PACKET_SIZE 2048

#define REG_CTRL 0x0000
#define REG_STATUS 0x0008
#define REG_CTRL_EXT 0x0018
#define REG_IMASK 0x00D0
#define REG_RCTRL 0x0100
#define REG_RXDESCLO 0x2800
#define REG_RXDESCHI 0x2804
#define REG_RXDESCLEN 0x2808
#define REG_RXDESCHEAD 0x2810
#define REG_RXDESCTAIL 0x2818

#define REG_TCTRL 0x0400
#define REG_TXDESCLO 0x3800
#define REG_TXDESCHI 0x3804
#define REG_TXDESCLEN 0x3808
#define REG_TXDESCHEAD 0x3810
#define REG_TXDESCTAIL 0x3818

#define REG_RDTR 0x2820 
#define REG_RXDCTL 0x2828 
#define REG_RADV 0x282C 
#define REG_RSRPD 0x2C00 

#define REG_TIPG 0x0410 
#define ECTRL_SLU 0x40 

#define RCTL_EN (1 << 1) 
#define RCTL_SBP (1 << 2) 
#define RCTL_UPE (1 << 3) 
#define RCTL_MPE (1 << 4) 
#define RCTL_LPE (1 << 5) 
#define RCTL_LBM_NONE (0 << 6) 
#define RCTL_LBM_PHY (3 << 6) 
#define RTCL_RDMTS_HALF (0 << 8) 
#define RTCL_RDMTS_QUARTER (1 << 8) 
#define RTCL_RDMTS_EIGHTH (2 << 8) 
#define RCTL_MO_36 (0 << 12) 
#define RCTL_MO_35 (1 << 12) 
#define RCTL_MO_34 (2 << 12) 
#define RCTL_MO_32 (3 << 12) 
#define RCTL_BAM (1 << 15) 
#define RCTL_VFE (1 << 18) 
#define RCTL_CFIEN (1 << 19) 
#define RCTL_CFI (1 << 20) 
#define RCTL_DPF (1 << 22) 
#define RCTL_PMCF (1 << 23) 
#define RCTL_SECRC (1 << 26) 

#define RCTL_BSIZE_256 (3 << 16)
#define RCTL_BSIZE_512 (2 << 16)
#define RCTL_BSIZE_1024 (1 << 16)
#define RCTL_BSIZE_2048 (0 << 16)
#define RCTL_BSIZE_4096 ((3 << 16) | (1 << 25))
#define RCTL_BSIZE_8192 ((2 << 16) | (1 << 25))
#define RCTL_BSIZE_16384 ((1 << 16) | (1 << 25))

#define CMD_EOP (1 << 0) 
#define CMD_IFCS (1 << 1) 
#define CMD_IC (1 << 2) 
#define CMD_RS (1 << 3) 
#define CMD_RPS (1 << 4) 
#define CMD_VLE (1 << 6) 
#define CMD_IDE (1 << 7) 

#define TCTL_EN (1 << 1) 
#define TCTL_PSP (1 << 3) 
#define TCTL_CT_SHIFT 4 
#define TCTL_COLD_SHIFT 12 
#define TCTL_SWXOFF (1 << 22) 
#define TCTL_RTLC (1 << 24) 

#define TSTA_DD (1 << 0) 
#define TSTA_EC (1 << 1) 
#define TSTA_LC (1 << 2) 
#define LSTA_TU (1 << 3) 

typedef struct eth_intel_rx_desc_s {
    volatile uint64_t addr;
    volatile uint16_t len;
    volatile uint16_t checksum;
    volatile uint8_t status;
    volatile uint8_t errors;
    volatile uint16_t special;
} __attribute__((packed)) eth_intel_rx_desc_t;

typedef struct eth_intel_tx_desc_s {
    volatile uint64_t addr;
    volatile uint16_t len;
    volatile uint8_t cso;
    volatile uint8_t cmd;
    volatile uint8_t status;
    volatile uint8_t css;
    volatile uint16_t special;
} __attribute__((packed)) eth_intel_tx_desc_t;

typedef struct eth_intel_device_s {
    uint8_t bar_type;
    uint16_t io_base;
    uint8_t* mmio_base;
    uint32_t rx_index;
    uint32_t tx_index;
    eth_intel_rx_desc_t* rx_descs[RX_DESC_COUNT];
    eth_intel_tx_desc_t* tx_descs[TX_DESC_COUNT];
    net_buf_t* rx_bufs[RX_DESC_COUNT];
    net_buf_t* tx_bufs[TX_DESC_COUNT];
    eth_addr_t mac_address;
} eth_intel_device_t;

static eth_intel_device_t device;

extern page_directory_t page_directory;
extern pfa_t pfa;

static void eth_intel_write_command(uint16_t address, uint32_t value) {
    if (device.bar_type == 0) {
        mmio_write32(device.mmio_base + address, value);
    } else {
        outl(device.io_base, address);
        outl(device.io_base + 4, value);
    }
}

static uint32_t eth_intel_read_command(uint16_t address) {
    if (device.bar_type == 0) {
        return mmio_read32(device.mmio_base + address);
    } else {
        outl(device.io_base, address);
        return inl(device.io_base + 4);
    }
}

static uint8_t read_mac_address() {
    uint8_t* mem_base = (uint8_t*) (device.mmio_base + 0x5400);
    if (*(uint32_t*) mem_base) {
        memcpy(device.mac_address.address, mem_base, 6);
    } else {
        return 0;
    }

    return 1;
}

static void eth_intel_rx_init() {
    eth_intel_rx_desc_t* descs = pfa_request_page(&pfa);
    uint32_t phys_addr = (uint32_t) pde_get_phys_addr(&page_directory, descs);

    for (uint32_t i = 0; i < RX_DESC_COUNT; i++) {
        net_buf_t* buf = net_alloc_buf();
        device.rx_bufs[i] = buf;
        device.rx_descs[i] = (eth_intel_rx_desc_t*) ((uint8_t*) descs + i * 16);
        device.rx_descs[i]->addr = (uint64_t)(uintptr_t) pde_get_phys_addr(&page_directory, buf->start);
        device.rx_descs[i]->status = 0;
    }

    eth_intel_write_command(REG_RXDESCLO, phys_addr);
    eth_intel_write_command(REG_RXDESCHI, 0);

    eth_intel_write_command(REG_RXDESCLEN, RX_DESC_COUNT * 16);

    eth_intel_write_command(REG_RXDESCHEAD, 0);
    eth_intel_write_command(REG_RXDESCTAIL, RX_DESC_COUNT - 1);

    device.rx_index = 0;

    eth_intel_write_command(REG_RCTRL, RCTL_EN | RCTL_SBP | RCTL_UPE
                          | RCTL_MPE | RCTL_LBM_NONE | RTCL_RDMTS_HALF
                          | RCTL_BAM | RCTL_SECRC | RCTL_BSIZE_2048);
}

static void eth_intel_tx_init() {
    eth_intel_tx_desc_t* descs = pfa_request_page(&pfa);
    uint32_t phys_addr = (uint32_t) pde_get_phys_addr(&page_directory, descs);

    for (uint32_t i = 0; i < TX_DESC_COUNT; i++) {
        device.tx_descs[i] = (eth_intel_tx_desc_t*) ((uint8_t*) descs + i * 16);
        device.tx_descs[i]->addr = 0;
        device.tx_descs[i]->cmd = 0;
        device.tx_descs[i]->status = TSTA_DD;
    }

    eth_intel_write_command(REG_TXDESCHI, 0);
    eth_intel_write_command(REG_TXDESCLO, phys_addr);

    eth_intel_write_command(REG_TXDESCLEN, TX_DESC_COUNT * 16);

    eth_intel_write_command(REG_TXDESCHEAD, 0);
    eth_intel_write_command(REG_TXDESCTAIL, 0);

    device.tx_index = 0;

    eth_intel_write_command(REG_TCTRL, TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT)
                          | (64 << TCTL_COLD_SHIFT) | TCTL_RTLC);

    eth_intel_write_command(REG_TCTRL, 0b0110000000000111111000011111010);
    eth_intel_write_command(REG_TIPG, 0x0060200A);
}

static void eth_intel_enable_interrupts() {
    eth_intel_write_command(REG_IMASK, 0x1F6DC);
    eth_intel_write_command(REG_IMASK, 0xFF & ~4);
    eth_intel_read_command(0xC0);
}

void eth_intel_poll(net_intf_t* intf) {
    eth_intel_rx_desc_t* desc = device.rx_descs[device.rx_index];
    if (desc->status & 0x1) {
        uint32_t index = device.rx_index;
        eth_intel_rx_desc_t* desc = device.rx_descs[index];
        net_buf_t* buf = device.rx_bufs[device.rx_index];
        buf->end = buf->start + desc->len;

        eth_recv(intf, buf);

        net_free_buf(buf);
        buf = net_alloc_buf();

        device.rx_bufs[device.rx_index] = buf;
        desc->addr = (uint64_t)(uintptr_t) pde_get_phys_addr(&page_directory, buf->start);
        desc->status = 0;

        device.rx_index = (device.rx_index + 1) % RX_DESC_COUNT;
        eth_intel_write_command(REG_RXDESCTAIL, index);
    }
}

void eth_intel_send(net_buf_t* buf) {
    uint32_t index = device.tx_index;
    eth_intel_tx_desc_t* desc = device.tx_descs[index];
    net_buf_t* old_buf = device.tx_bufs[index];
    if (old_buf && old_buf != buf) {
        net_free_buf(old_buf);
    }

    device.tx_bufs[index] = buf;
    desc->addr = (uint64_t)(uintptr_t) pde_get_phys_addr(&page_directory, buf->start);
    desc->len = (uint32_t) buf->end - (uint32_t) buf->start;
    desc->cmd = CMD_EOP | CMD_IFCS | CMD_RS;
    desc->status = 0;
    device.tx_index = (device.tx_index + 1) % TX_DESC_COUNT;
    eth_intel_write_command(REG_TXDESCTAIL, device.tx_index);
    while (!(device.tx_descs[index]->status & 0xFF));
}

void eth_intel_interrupt_handler(struct interrupt_frame* frame) {
    eth_intel_write_command(REG_IMASK, 0x01);

    uint32_t status = eth_intel_read_command(0xC0);

    if (status & 0x10) {
        eth_intel_write_command(REG_CTRL, eth_intel_read_command(REG_CTRL) | ECTRL_SLU);
    }

    pic_master_eoi();
}

void intel_net_driver_init(pci_device_info_t* info, uint32_t bus, uint32_t dev, uint32_t func) {
    if (info->vendor_id != 0x8086) {
        return;
    }

    if (info->device_id != 0x100E && info->device_id != 0x1503) {
        return;
    }

    terminal_putstring("Initializing Intel Gigabit Ethernet...\n");

    uint32_t id = pci_get_id(bus, dev, func);

    pci_bar_t bar;
    pci_get_bar(&bar, id, 0);

    device.bar_type = bar.flags & PCI_BAR_IO;
    device.mmio_base = bar._.address;
    device.io_base = bar._.port;

    if (device.bar_type == 0) {
        for (uint32_t i = 0; i < 0xFFFF; i += 4096) {
            void* ptr = (void*) (device.mmio_base + i);
            pde_map_memory(&page_directory, &pfa, ptr, ptr);
        }
    }

    uint32_t pci_status = pci_read32(id, PCI_CONFIG_COMMAND);
    pci_status |= 4;
    pci_write32(id, PCI_CONFIG_COMMAND, pci_status);

    if (!read_mac_address()) {
        terminal_putstring("Unable to read MAC address.\n");
        return;
    }

    char str[20];
    ethtoa(&device.mac_address, str);
    terminal_putstring("MAC address: ");
    terminal_putstring(str);
    terminal_putchar('\n');

    eth_intel_write_command(REG_CTRL, eth_intel_read_command(REG_CTRL) | ECTRL_SLU);

    for (uint32_t i = 0; i < 0x80; i++) {
        eth_intel_write_command(0x5200 + i * 4, 0);
    }

    uint8_t irq = pci_read8(id, PCI_CONFIG_INTERRUPT_LINE) - 9;
    peripheral_isrs[irq] = eth_intel_interrupt_handler;

    eth_intel_enable_interrupts();
    eth_intel_rx_init();
    eth_intel_tx_init();

    net_intf_t* intf = net_intf_create();
    intf->eth_addr = device.mac_address;
    intf->ip_addr = ipv4_null_addr;
    intf->name = "eth";
    intf->poll = eth_intel_poll;
    intf->send = eth_intf_send;
    intf->dev_send = eth_intel_send;

    net_intf_add(intf);
}
