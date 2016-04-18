#include "uefi.h"

KAPI EFI_STATUS
efi_realloc_buffer(const struct uefi *uefi, void **buffer, uint64_t size);

KAPI bool
compare_guid(EFI_GUID *g1, EFI_GUID *g2)
{
    if(*(uint64_t *)g1 != *(uint64_t *)g2) return false;
    return *(uint64_t *)g1->Data4 == *(uint64_t *)g2->Data4;
}

KAPI void *
find_configuration_table(const struct uefi *uefi, EFI_GUID *guid)
{
    for(uint32_t i = 0; i < uefi->system_table->NumberOfTableEntries; i += 1) {
        if(compare_guid(guid, &uefi->system_table->ConfigurationTable[i].VendorGuid)) {
            return uefi->system_table->ConfigurationTable[i].VendorTable;
        }
    }
    return NULL;
}


KAPI EFI_STATUS
efi_memory_map(const struct uefi *uefi, struct efi_memory_map *map_info)
{
    EFI_STATUS              status;
    EFI_MEMORY_DESCRIPTOR   *buffer;
    UINTN                   buffer_size;

    status = EFI_SUCCESS;
    buffer = NULL;
    buffer_size = sizeof(EFI_MEMORY_DESCRIPTOR);

    for(int i = 0;
        i < 32 && !EFI_ERROR(efi_realloc_buffer(uefi, (void **)&buffer, buffer_size));
        i++)
    {
        status = uefi->system_table->BootServices->GetMemoryMap(&buffer_size, buffer,
                                                                &map_info->map_key,
                                                                &map_info->descr_size,
                                                                &map_info->descr_version);
        if(!EFI_ERROR(status)) { break; }
    }
    ASSERT_EFI_STATUS(status);
    map_info->entries = buffer_size / map_info->descr_size;
    map_info->descrs = buffer;
    return status;
}


KAPI EFI_STATUS
efi_realloc_buffer(const struct uefi *uefi, void **buffer, uint64_t size)
{
    EFI_STATUS status;
    if (*buffer != NULL) {
        status = uefi->system_table->BootServices->FreePool(*buffer);
        ASSERT_EFI_STATUS(status);
        *buffer = NULL;
    }
    if(size > 0){
        status = uefi->system_table->BootServices->AllocatePool(EfiLoaderData, size, buffer);
        if(EFI_ERROR(status)) {
            uefi->system_table->BootServices->FreePool(*buffer);
            *buffer = NULL;
            return status;
        }
    }
    return status;
}
