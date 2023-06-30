#include "ide.h"
#include "io.h"
#include "kprintf.h"
#include "stdlib.h"

uint8_t ide_buf[2048] = {0};
static volatile uint8_t ide_irq_invoked = 0;
static uint8_t atapi_packet[12] = {0xA8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
ide_channel_regs_t channels[2];
ide_device_t ide_devices[4];

void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nien);
    }

    if (reg < 0x08) {
        outb(channels[channel].base + reg, data);
    } else if (reg < 0x0C) {
        outb(channels[channel].base + reg - 0x06, data);
    } else if (reg < 0x0E) {
        outb(channels[channel].ctrl + reg - 0x0A, data);
    } else if (reg < 0x16) {
        outb(channels[channel].bmide + reg - 0x0E, data);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nien);
    }
}

uint8_t ide_read(uint8_t channel, uint8_t reg) {
    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nien);
    }

    uint8_t result = 0;
    if (reg < 0x08) {
        result = inb(channels[channel].base + reg);
    } else if (reg < 0x0C) {
        result = inb(channels[channel].base + reg - 0x06);
    } else if (reg < 0x0E) {
        result = inb(channels[channel].ctrl + reg - 0x0A);
    } else if (reg < 0x16) {
        result = inb(channels[channel].bmide + reg - 0x0E);
    }

    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nien);
    }

    return result;
}

void ide_read_buffer(uint8_t channel, uint8_t reg, uint32_t* buffer, uint8_t quads) {
    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nien);
    }

    asm("pushw %es; movw %ds, %ax; movw %ax, %es");
    if (reg < 0x08) {
        insl(channels[channel].base + reg, buffer, quads);
    } else if (reg < 0x0C) {
        insl(channels[channel].base + reg - 0x06, buffer, quads);
    } else if (reg < 0x0E) {
        insl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
    } else if (reg < 0x16) {
        insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
    }

    asm("popw %es");
    if (reg > 0x07 && reg < 0x0C) {
        ide_write(channel, ATA_REG_CONTROL, channels[channel].nien);
    }
}

void ide_io_wait(uint16_t channel) {
    for (uint8_t i = 0; i < 4; i++) {
        ide_read(channel, ATA_REG_ALTSTATUS);
    }
}


uint8_t ide_poll(uint8_t channel, uint8_t advanced) {
    ide_io_wait(channel);
    while (ide_read(channel, ATA_REG_ALTSTATUS) & ATA_SR_BSY);
    if (advanced) {
        uint8_t state = ide_read(channel, ATA_REG_STATUS);
        if (state & ATA_SR_ERR) {
            return 2;
        }

        if (state & ATA_SR_DF) {
            return 1;
        }

        if ((state & ATA_SR_DRQ) == 0) {
            return 3;
        }
    }

    return 0;
}

void ide_init(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4) {
    channels[ATA_PRIMARY].base = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0);
    channels[ATA_PRIMARY].ctrl = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1);
    channels[ATA_SECONDARY].base = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
    channels[ATA_SECONDARY].ctrl = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
    channels[ATA_PRIMARY].bmide = (bar4 & 0xFFFFFFFC);
    channels[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8;

    ide_write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
    ide_write(ATA_SECONDARY, ATA_REG_CONTROL, 2);

    uint8_t count = 0;
    for (uint8_t i = 0; i < 2; i++) {
        for (uint8_t j = 0; j < 2; j++) {
            ide_devices[count].reserved = 0;

            ide_write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
            ide_io_wait(i);

            ide_write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
            ide_io_wait(i);

            if (ide_read(i, ATA_REG_STATUS) == 0) {
                continue;
            }

            uint8_t err = 0;
            uint8_t status;
            while (1) {
                status = ide_read(i, ATA_REG_STATUS);
                if (status & ATA_SR_ERR) {
                    err = 1;
                    break;
                }

                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
                    break;
                }
            }

            uint8_t type = IDE_ATA;
            if (err != 0) {
                uint8_t cl = ide_read(i, ATA_REG_LBA1);
                uint8_t ch = ide_read(i, ATA_REG_LBA1);

                if (cl == 0x14 && ch == 0xEB) {
                    type = IDE_ATAPI;
                } else if (cl == 0x69 && ch == 0x96) {
                    type = IDE_ATAPI;
                } else {
                    continue;
                }

                ide_write(i, ATA_REG_CONTROL, ATA_CMD_IDENTIFY_PACKET);
                ide_io_wait(i);
            }

            ide_read_buffer(i, ATA_REG_DATA, (uint32_t*) ide_buf, 128);

            ide_devices[count].reserved = 1;
            ide_devices[count].type = type;
            ide_devices[count].channel = i;
            ide_devices[count].drive = j;
            ide_devices[count].signature = *((uint16_t*) (ide_buf + ATA_IDENT_DEVICETYPE));
            ide_devices[count].capabilities = *((uint16_t*) (ide_buf + ATA_IDENT_CAPABILITIES));
            ide_devices[count].command_sets = *((uint32_t*) (ide_buf + ATA_IDENT_COMMANDSETS));

            if (ide_devices[count].command_sets & (1 << 26)) {
                ide_devices[count].size = *((uint32_t*) (ide_buf + ATA_IDENT_MAX_LBA_EXT));
            } else {
                ide_devices[count].size = *((uint32_t*) (ide_buf + ATA_IDENT_MAX_LBA));
            }

            for (uint8_t k = 0; k < 40; k += 2) {
                ide_devices[count].model[k] = ide_buf[ATA_IDENT_MODEL + k + 1];
                ide_devices[count].model[k + 1] = ide_buf[ATA_IDENT_MODEL + k];
            }

            ide_devices[count].model[40] = 0;
            count++;
        }
    }

    for (uint8_t i = 0; i < 4; i++) {
        if (ide_devices[i].reserved == 1) {
            kprintf("Found %s drive - %lu MB - %s\n",
                    (const char*[]){"ATA", "ATAPI"}[ide_devices[i].type],
                    ide_devices[i].size / 1024 / 2, ide_devices[i].model);
        }
    }
}