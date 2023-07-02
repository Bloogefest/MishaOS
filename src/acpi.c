#include "acpi.h"
#include "string.h"
#include "panic.h"
#include "kprintf.h"

uint8_t acpi_cpu_ids[16];
uint8_t acpi_cpu_count;
uint8_t* io_apic_address;

rsdp_t* rsdp_locate(struct multiboot* multiboot) {
    uint32_t lookup_addr = 0x000E0000;
    if (multiboot->flags & MULTIBOOT_FLAG_CONFIG) {
        lookup_addr = multiboot->config_table;
    }

    rsdp_t* rsdp = (rsdp_t*) _strstr((void*) lookup_addr, "RSD PTR ", 0x000FFFFF - 0x000E0000);
    if (!rsdp) {
        return rsdp;
    }

    uint8_t sum = 0;
    for (uint8_t i = 0; i < 20; i++) {
        sum += ((uint8_t*) rsdp)[i];
    }

    if (sum) {
        panic("RSDP checksum failed");
    }

    return rsdp;
}

void acpi_parse_apic(acpi_madt_t* madt) {
    uint8_t* it = (uint8_t*) (madt + 1);
    uint8_t* end = (uint8_t*) madt + madt->parent.length;

    while (it < end) {
        apic_header_t* header = (apic_header_t*) it;

        switch (header->type) {
            case LOCAL_APIC: {
                puts("[ACPI] Found CPU");
                apic_local_apic_t* local_apic = (apic_local_apic_t*) header;
                if (acpi_cpu_count < sizeof(acpi_cpu_ids)) {
                    acpi_cpu_ids[acpi_cpu_count++] = local_apic->apic_id;
                }

                break;
            }

            case IO_APIC: {
                puts("[ACPI] Found I/O APIC");
                apic_io_apic_t* io_apic = (apic_io_apic_t*) header;
                io_apic_address = (uint8_t*) io_apic->io_apic_address;
                break;
            }
        }

        it += header->length;
    }
}

void acpi_parse_facp(acpi_fadt_t* fadt) {
    // TODO: ?
}

void acpi_parse_dt(sdt_header_t* header) {
    char signature_string[5];
    memcpy(signature_string, header->signature, 4);
    signature_string[4] = 0;

    kprintf("[ACPI] Found %s header\n", signature_string);
    if (memcmp(header->signature, "FACP", 4) == 0) {
        acpi_parse_facp((acpi_fadt_t*) header);
    } else if (memcmp(header->signature, "APIC", 4) == 0) {
        acpi_parse_apic((acpi_madt_t*) header);
    }
}

void acpi_parse_xsdt(sdt_header_t* header) {
    uint64_t* it = (uint64_t*) (header + 1);
    uint64_t* end = (uint64_t*) ((uint8_t*) header + header->length);
    while (it < end) {
        acpi_parse_dt((sdt_header_t*) *it++);
    }
}

void acpi_parse_rsdt(sdt_header_t* header) {
    uint32_t* it = (uint32_t*) (header + 1);
    uint32_t* end = (uint32_t*) ((uint8_t*) header + header->length);
    while (it < end) {
        acpi_parse_dt((sdt_header_t*) *it++);
    }
}

uint8_t acpi_get_max_cpu_count() {
    return sizeof(acpi_cpu_ids);
}

uint8_t acpi_get_cpu_count() {
    return acpi_cpu_count;
}

uint8_t* acpi_get_cpu_ids() {
    return acpi_cpu_ids;
}

uint8_t* acpi_get_io_apic_address() {
    return io_apic_address;
}