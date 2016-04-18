#pragma once
#include "uefi.h"
#include <inttypes.h>

// ACPI specification, 5.2.5.3
struct RSDP {
    // “RSD PTR ”
    uint8_t signature[8];
    // This is the checksum of the fields defined in the ACPI 1.0 specification. This includes only
    // the first 20 bytes of this table, bytes 0 to 19, including the checksum field. These bytes
    // must sum to zero.
    uint8_t checksum;
    // An OEM-supplied string that identifies the OEM.
    uint8_t oem_id[6];
    // The revision of this structure. Larger revision numbers are backward compatible to lower
    // revision numbers. The ACPI version 1.0 revision number of this table is zero. The current
    // value for this field is 2.
    uint8_t revision;
    // 32 bit physical address of the RSDT.
    uint32_t rsdt;
    // The length of the table, in bytes, including the header, starting from offset 0. This field
    // is used to record the size of the entire table.
    uint32_t length;
    // 64 bit physical address of the XSDT.
    uint64_t xsdt;
    // This is a checksum of the entire table, including both checksum fields.
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

// ACPI specification, 5.2.8
struct XSDTHeader {
    // ‘XSDT’. Signature for the Extended System Description Table.
    uint8_t signature[4];
    // Length, in bytes, of the entire table. The length implies the number of Entry fields (n) at
    // the end of the table.
    uint32_t length;
    uint8_t revision;
    // Entire table must sum to zero.
    uint8_t checksum;
    // OEM ID
    uint8_t oem_id[6];
    // For the XSDT, the table ID is the manufacture model ID. This field must match the OEM Table
    // ID in the FADT.
    uint8_t oem_table_id[8];
    // EM revision of XSDT table for supplied OEM Table ID.
    uint32_t oem_revision;
    // Vendor ID of utility that created the table. For tables containing Definition Blocks, this
    // is the ID for the ASL Compiler.
    uint32_t creator_id;
    // Revision of utility that created the table. For tables containing Definition Blocks, this is
    // the revision for the ASL Compiler.
    uint32_t creator_revision;
    // Data.
} __attribute__ ((packed));

// See table 5-5 in ACPI specification for signatures.
// More common are: "APIC", "HPET", "FACP" and "SSDT"
KAPI struct XSDTHeader*
find_acpi_table(const struct uefi *uefi, uint8_t signature[4]);
