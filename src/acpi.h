#pragma once

#include <stdint.h>

#include "multiboot.h"

#define LOCAL_APIC 0
#define IO_APIC 1
#define INTERRUPT_OVERRIDE 2

typedef struct rsdp_s {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} rsdp_t;

typedef struct rsdp2_s {
    rsdp_t parent;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} rsdp2_t;

typedef struct sdt_header_s {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} sdt_header_t;

typedef struct gas_s {
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
} gas_t;

typedef struct acpi_fadt_s {
    sdt_header_t parent;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;
    uint16_t boot_arch_flags;
    uint8_t reserved2;
    uint32_t flags;
    gas_t reset_reg;
    uint8_t reset_value;
    uint8_t reserved3[3];
    uint64_t x_firmware_control;
    uint64_t x_dsdt;
    gas_t x_pm1a_event_block;
    gas_t x_pm1b_event_block;
    gas_t x_pm1a_control_block;
    gas_t x_pm1b_control_block;
    gas_t x_pm2_control_block;
    gas_t x_gpe0_block;
    gas_t x_gpe1_block;
} acpi_fadt_t;

typedef struct acpi_madt_s {
    sdt_header_t parent;
    uint32_t local_apic_address;
    uint32_t flags;
} acpi_madt_t;

typedef struct apic_header_s {
    uint8_t type;
    uint8_t length;
} apic_header_t;

typedef struct apic_local_apic_s {
    apic_header_t parent;
    uint8_t apic_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} apic_local_apic_t;

typedef struct apic_io_apic_s {
    apic_header_t parent;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} apic_io_apic_t;

rsdp_t* rsdp_locate(struct multiboot* multiboot);

void acpi_parse_xsdt(sdt_header_t* header);
void acpi_parse_rsdt(sdt_header_t* header);

uint8_t acpi_get_max_cpu_count();
uint8_t acpi_get_cpt_count();
uint8_t* acpi_get_cpu_ids();
uint8_t* acpi_get_io_apic_address();