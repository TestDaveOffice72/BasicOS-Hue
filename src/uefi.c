#include "uefi.h"

EFI_SYSTEM_TABLE *system_table;
EFI_HANDLE image_handle;

KAPI bool
compare_guid(EFI_GUID *g1, EFI_GUID *g2)
{
    if(*(uint64_t *)g1 != *(uint64_t *)g2) return false;
    return *(uint64_t *)g1->Data4 == *(uint64_t *)g2->Data4;
}

KAPI void *
find_configuration_table(EFI_GUID *guid)
{
    for(uint32_t i = 0; i < system_table->NumberOfTableEntries; i += 1) {
        if(compare_guid(guid, &system_table->ConfigurationTable[i].VendorGuid)) {
            return system_table->ConfigurationTable[i].VendorTable;
        }
    }
    return NULL;
}
