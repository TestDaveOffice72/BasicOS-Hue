#include "memory.h"
#include "serial.h"
#include "kernel.h"
#include "uefi.h"
#include "cpu.h"

const uint64_t CR0_WP_BIT = (1ull << 16);
const uint64_t CR0_PG_BIT = (1ull << 31);
const uint64_t CR4_PSE_BIT = (1ull << 4);
const uint64_t CR4_PAE_BIT = (1ull << 5);
const uint64_t CR4_PGE_BIT = (1ull << 7);
const uint64_t CR4_PCIDE_BIT = (1ull << 17);
const uint64_t CR4_SMEP_BIT = (1ull << 20);
const uint64_t CR4_SMAP_BIT = (1ull << 21);
const uint64_t CR4_PKE_BIT = (1ull << 22);
const uint64_t IA32_EFER_LME_BIT = (1ull << 8);
const uint64_t IA32_EFER_NXE_BIT = (1ull << 11);
const uint64_t EFLAGS_AC_BIT = (1ull << 18);
const uint32_t IA32_EFER_MSR = 0xC0000080;

KAPI EFI_STATUS setup_paging(struct kernel *kernel);
KAPI uint64_t *allocate_tables(struct kernel *, uint64_t offset, uint64_t max_address,
                               uint8_t level);
KAPI uint64_t *get_memory_entry_for(struct kernel *, uint64_t address, uint8_t level);
KAPI uint64_t *create_memory_entry_for(struct kernel *k, uint64_t address, uint8_t level);
KAPI void set_entry_present(uint64_t *entry, bool on);
KAPI void set_entry_writeable(uint64_t *entry, bool on);
KAPI void set_entry_pagesize(uint64_t *entry, bool on);
KAPI void set_entry_supervisor(uint64_t *entry, bool on);
KAPI void set_entry_physical(uint64_t *entry, uint64_t physical);
KAPI uint64_t get_entry_physical(uint64_t *entry);
KAPI void set_entry_exec(uint64_t *entry, bool on);
KAPI void init_gdt();

struct GDT {
    uint16_t limit;
    uint64_t offset;
} __attribute__((packed));

KAPI EFI_STATUS
init_memory(struct kernel *kernel) {
    uint64_t max_address = 0;
    EFI_STATUS status;

    init_gdt(kernel);
    status = setup_paging(kernel);
    ASSERT_EFI_STATUS(status);
    status = efi_memory_map(&kernel->uefi, &kernel->uefi.boot_memmap);
    ASSERT_EFI_STATUS(status);
    serial_print("EFI memory map has ");
    serial_print_int(kernel->uefi.boot_memmap.entries);
    serial_print(" entries (type pstart~vstart, numpages, attrs):\n");

    // At this point it is completely fine to exit the boot services. We are not doing any
    // allocation with UEFI further ahead and we already got the memory map.
    status = kernel->uefi.system_table->BootServices->ExitBootServices(
        kernel->uefi.image_handle,
        kernel->uefi.boot_memmap.map_key
    );
    ASSERT_EFI_STATUS(status);

    // First of all we make a linked list of all free pages. Our scheme is this:
    // Keep first_free_page noted at all times. This page contains the address of the next unused
    // page.
    //
    // Once a free page is requested we will pull up its descriptor and make page read-writeable,
    // thus available for use, read its next_page pointer. This pointer gets noted as the new
    // first_free_page and the page is returned for caller to use.
    //
    // Once a page is freed (not implemented yet) we put our current first_free_page address to the
    // low address of the page and note the current page’s address as our new first_free_page.
    //
    // While this scheme is extremely simple it is very inefficient in terms of data locality et
    // cetera.
    {FOR_EACH_MEMORY_DESCRIPTOR(kernel, descr) {
        if(descr->Type == _EfiConventionalMemory) {
            for(uint64_t page = 0; page < descr->NumberOfPages; page++) {
                uint64_t *page_address = (uint64_t *)(descr->PhysicalStart + page * 0x1000);
                *page_address = (uint64_t)kernel->memory.first_free_page;
                kernel->memory.first_free_page = page_address;
            }
        }

        switch(descr->Type){
            case _EfiReservedMemoryType: serial_print("R"); break;
            case _EfiLoaderCode: serial_print("L"); break;
            case _EfiLoaderData: serial_print("l"); break;
            case _EfiBootServicesCode: serial_print("B"); break;
            case _EfiBootServicesData: serial_print("b"); break;
            case _EfiRuntimeServicesCode: serial_print("R"); break;
            case _EfiRuntimeServicesData: serial_print("r"); break;
            case _EfiConventionalMemory: serial_print("C"); break;
            case _EfiUnusableMemory: serial_print("U"); break;
            case _EfiACPIReclaimMemory: serial_print("A"); break;
            case _EfiACPIMemoryNVS: serial_print("a"); break;
            case _EfiMemoryMappedIO: serial_print("M"); break;
            case _EfiMemoryMappedIOPortSpace: serial_print("m"); break;
            case _EfiPalCode: serial_print("p"); break;
            case _EfiPersistentMemory: serial_print("P"); break;
            case _EfiMaxMemoryType: serial_print("X"); break;
            default: serial_print("!"); break;
        }
        serial_print(" 0x");
        serial_print_hex(descr->PhysicalStart);
        serial_print("~0x");
        serial_print_hex(descr->VirtualStart);
        serial_print("\t");
        serial_print_int(descr->NumberOfPages);
        serial_print("\t");
#define CHECK_ATTR(ty) ({ if(descr->Attribute | EFI_MEMORY_ ## ty) { serial_print(" " #ty); } })
        CHECK_ATTR(UC);
        CHECK_ATTR(WC);
        CHECK_ATTR(WT);
        CHECK_ATTR(WB);
        CHECK_ATTR(UCE);
        CHECK_ATTR(WP);
        CHECK_ATTR(RP);
        CHECK_ATTR(XP);
        CHECK_ATTR(RUNTIME);
#undef CHECK_ATTR
        serial_print("\n");
        // Within the kernel we use identity mapping.
        descr->VirtualStart = descr->PhysicalStart;
        uint64_t phys_end = descr->PhysicalStart + 0x1000 * descr->NumberOfPages;
        if(phys_end > max_address) {
            max_address = phys_end;
        }
    }}
    serial_print("Maximum address is: ");
    serial_print_hex(max_address);
    serial_print("\n");
    kernel->memory.max_addr = max_address;

    // We can inform UEFI runtime about our identity mapping scheme now.
    status = kernel->uefi.system_table->RuntimeServices->SetVirtualAddressMap(
        kernel->uefi.boot_memmap.entries * kernel->uefi.boot_memmap.descr_size,
        kernel->uefi.boot_memmap.descr_size,
        kernel->uefi.boot_memmap.descr_version,
        kernel->uefi.boot_memmap.descrs
    );
    ASSERT_EFI_STATUS(status);
    serial_print("Boot services done\n");


    // Now we should setup the virtual mapping descriptors (which involve allocations from our page
    // allocator we made just above.
    // Figure out how many of each tables we need and allocate them all.
    kernel->memory.pml4_table = allocate_tables(kernel, 0, max_address, 3);

    // Initialize this table with data from the memory map we got back then.
    {FOR_EACH_MEMORY_DESCRIPTOR(kernel, descr) {
        if(descr->Type == _EfiConventionalMemory) {
            for(uint64_t i = descr->NumberOfPages; i > 0; i -= 1) {
                uint64_t *e = get_memory_entry_for(kernel, descr->VirtualStart + 0x1000 * i, 0);
                if((*e | (1 << 11)) == 0) {
                    // First of all, all the unallocated memory is, obviously, not present.
                    set_entry_present(e, false);
                } else {
                    // We also specially some entries which we marked as used for the tables
                    // themselves. These should stay present, but not executable.
                    *e &= ~(1 << 11); // Also, remove the flag.
                    set_entry_exec(e, false);
                }
            }
            continue;
        }
        // Then, we want to make all non-code sections non-executable and code sections
        // non-writeable.
        bool is_code = descr->Type == _EfiLoaderCode || descr->Type == _EfiPalCode
                     || descr->Type == _EfiRuntimeServicesCode;
        if(!is_code) {
            for(uint64_t i = 0; i < descr->NumberOfPages; i += 1) {
                uint64_t *e = get_memory_entry_for(kernel, descr->VirtualStart + 0x1000 * i, 0);
                set_entry_exec(e, false);
            }
        } else {
            for(uint64_t i = 0; i < descr->NumberOfPages; i += 1) {
                uint64_t *e = get_memory_entry_for(kernel, descr->VirtualStart + 0x1000 * i, 0);
                set_entry_writeable(e, false);
            }
        }
    }}
    // Finally, we want NULL to be not present, so we get pagefault when null is dereferenced.
    uint64_t *e = get_memory_entry_for(kernel, 0, 0);
    set_entry_present(e, false);


    // Then we redo graphics framebuffer mapping
    for(uint64_t i = 0; i <= kernel->graphics.buffer_size; i += 0x200000) {
        uint64_t *e = create_memory_entry_for(kernel,
                                              (uint64_t)kernel->graphics.buffer_base + i, 1);
        *e = (uint64_t)kernel->graphics.buffer_base + i;
        set_entry_present(e, true);
        set_entry_writeable(e, true);
        set_entry_pagesize(e, true);
    }

    // Now we set our cr3 to this PML table thing. Note, that we do not shift anything here,
    // because 4k alligned addresses already have 12 bits zeroed for flags. (this took me some day
    // or two to realise omfg)
    uint64_t cr3_val = ((uint64_t)kernel->memory.pml4_table);
    write_cr3(cr3_val);
    serial_print("Paging setup compiete\n");

    // Finally, we can deallocate the BootServicesCode/Data pages. (NB: page with start at address
    // 0x0 is BootServicesData, thus dereferencing it becomes invalid).
    {FOR_EACH_MEMORY_DESCRIPTOR(kernel, descr) {
        if(descr->Type == _EfiBootServicesCode) {
            for(uint64_t i = 0; i < descr->NumberOfPages; i += 1) {
                deallocate_page(kernel, (void *)descr->VirtualStart + 0x1000 * i);
            }
        }
        // FIXME: Clearing the BootServicesData here causes some very weird timing related failures
        // to happen. I couldn’t manage to finish the investigation, sadly.
        // Namely, about a second later some weird `[=3h`s will appear in the serial output and the
        // system will reset. (not a triple/double/page fault, though)
        // if(descr->Type == _EfiBootServicesData) {
        //     for(uint64_t i = 0; i < descr->NumberOfPages; i += 1) {
        //         deallocate_page(kernel, (void *)descr->VirtualStart + 0x1000 * i);
        //     }
        // }
    }}

    return EFI_SUCCESS;
}


/// Never returns NULL or otherwise invalid pages.
KAPI void *
allocate_page_inner(struct kernel *k, bool ok)
{
    // Figure out the page we can return.
    uint64_t *ret = k->memory.first_free_page;
    // Perhaps should panic instead?
    if(ret == NULL) {
        serial_print("allocate_page_inner: OOM");
        DEBUG_HALT;
    }
    // Now we note that next first_free_page is recorded in the page being returned.
    // TODO: perhaps something more reliable like checking for existence of page tables and doing
    // smart things depending on that?
    if(ok) k->memory.first_free_page = (uint64_t *)*ret;
    // Return the page.
    return ret;
}

KAPI void *
allocate_page(struct kernel *k)
{
    uint64_t *ret = allocate_page_inner(k, false);
    uint64_t *e = get_memory_entry_for(k, (uint64_t)ret, 0);
    set_entry_present(e, true);
    k->memory.first_free_page = (uint64_t *)*ret;
    set_entry_writeable(e, true);
    set_entry_exec(e, false);
    set_entry_supervisor(e, true);
    // Zero out the next-pointer, in order to not leak the internals.
    *ret = 0;
    return ret;
}

KAPI void
deallocate_page(struct kernel *k, void *p)
{
    // Special case.
    if(p == NULL) return;
    // Re-set the entry for this page.
    uint64_t *entry = get_memory_entry_for(k, (uint64_t)p, 0);
    // Then we re-set the first_free_page pointer for this page.
    *((uint64_t *)p) = (uint64_t)k->memory.first_free_page;
    set_entry_present(entry, false);
    // And now this page is our new first_free_page.
    k->memory.first_free_page = p;
}

KAPI EFI_STATUS
setup_paging(struct kernel *kernel)
{
    // We use IA32e paging, which involves setting following registers:
    // MSR IA32_EFER.LME = 1
    // CR4.PAE = 1
    // We must check if IA32_EFER is available by checking CPUID.80000001H.EDX[bit 20 or 29] == 1.
    if(!kernel->cpu.has_ia32_efer) {
        return EFI_UNSUPPORTED;
    }
    uint64_t efer = cpu_read_msr(IA32_EFER_MSR);
    efer |= IA32_EFER_LME_BIT;
    efer |= IA32_EFER_NXE_BIT;
    cpu_write_msr(IA32_EFER_MSR, efer);
    uint64_t cr4;
    __asm__("movq %%cr4, %0" : "=r"(cr4));
    cr4 |= CR4_PAE_BIT;
    __asm__("movq %0, %%cr4" :: "r"(cr4));
    return EFI_SUCCESS;
}

KAPI uint64_t *allocate_tables(struct kernel *k, uint64_t offset, uint64_t max_address,
                               uint8_t level) {
    uint64_t entry_size;
    switch(level) {
        case 0: entry_size = 0x1000ull; break;
        case 1: entry_size = 0x1000ull * 512; break;
        case 2: entry_size = 0x1000ull * 512 * 512; break;
        case 3: entry_size = 0x1000ull * 512 * 512 * 512; break;
        default: serial_print("allocate_tables: bad level"); DEBUG_HALT;
    }
    uint64_t *new_page = allocate_page_inner(k, true);
    if(level != 0) {
        for(uint64_t i = 0; i < 512 && offset + i * entry_size < max_address; i++) {
            uint64_t *child = allocate_tables(k, offset + i * entry_size, max_address, level - 1);
            // We initialize all intermediate pages as present, writable, executable etc, because
            // book-keeping these is pain.
            new_page[i] = (uint64_t)child | 1ull | 1ull << 1 | 1ull << 2 | 1ull << 11;
        }
    } else {
        for(uint64_t i = 0; i < 512 && offset + i * entry_size < max_address; i++) {
            new_page[i] = offset + i * entry_size | 1ull | 1ull << 1 | 1ull << 2 | 1ull << 11;
        }
    }
    return new_page;
}

KAPI uint64_t *
get_memory_entry_for(struct kernel *k, uint64_t address, uint8_t level)
{
    uint64_t index = address / 0x1000;
    uint64_t indices[4] = {
        index % 512,
        (index >> 9) % 512,
        (index >> 18) % 512,
        (index >> 27) % 512
    };
    uint64_t *current = k->memory.pml4_table;
    for(uint8_t l = 3; l > level; l -= 1) {
        current = (uint64_t *)get_entry_physical(current + indices[l]);
    }
    return current + indices[level];
}

// Gets or creates memory entry for specified address. Cannot be used while the tables for memory
// itself aren’t initialized yet.
KAPI uint64_t *
create_memory_entry_for(struct kernel *k, uint64_t address, uint8_t level)
{
    uint64_t index = address / 0x1000;
    uint64_t indices[4] = {
        index % 512,
        (index >> 9) % 512,
        (index >> 18) % 512,
        (index >> 27) % 512
    };
    uint64_t *current = k->memory.pml4_table;
    for(uint8_t l = 4; l > level;) {
        l -= 1;
        uint64_t *next = current + indices[l];
        if(l == level) return next;
        if((*next & 1) == 0) {
            uint64_t *new_page = allocate_page_inner(k, true);
            kmemset(new_page, 0, 0x1000);
            *next = (uint64_t)new_page;
            set_entry_present(next, true);
            set_entry_writeable(next, true);
            set_entry_supervisor(next, true);
        }
        current = (uint64_t *)get_entry_physical(next);
    }
    return NULL;
}

KAPI void
map_page(struct kernel *k, uint64_t address, uint8_t level, uint64_t phys_addr)
{
    serial_print("Mapped "); serial_print_hex(address); serial_print(" to ");
    serial_print_hex(phys_addr); serial_print("\n");
    uint64_t *entry = create_memory_entry_for(k, address, level);
    set_entry_physical(entry, phys_addr);
    set_entry_present(entry, true);
    set_entry_exec(entry, true);
    set_entry_writeable(entry, true);
}


KAPI void
set_entry_present(uint64_t *entry, bool on)
{
    const uint64_t flag = 1ull;
    if(on) { *entry |= flag; } else { *entry &= ~flag; }
}

KAPI void
set_entry_writeable(uint64_t *entry, bool on)
{
    const uint64_t flag = 1ull << 1;
    if(on) { *entry |= flag; } else { *entry &= ~flag; }
}

KAPI void
set_entry_supervisor(uint64_t *entry, bool on)
{
    const uint64_t flag = 1ull << 2;
    if(on) { *entry |= flag; } else { *entry &= ~flag; }
}

KAPI void
set_entry_pagesize(uint64_t *entry, bool on)
{
    const uint64_t flag = 1ull << 7;
    if(on) { *entry |= flag; } else { *entry &= ~flag; }
}

KAPI void
set_entry_physical(uint64_t *entry, uint64_t physical)
{
    if((physical & 0xFFF) != 0) {
        serial_print("set_entry_physical: bad physical address: ");
        DEBUG_HALT;
    }
    *entry |= physical;
}

KAPI uint64_t
get_entry_physical(uint64_t *entry)
{
    return *entry & 0x0007FFFFFFFFF000;
}

KAPI void
set_entry_exec(uint64_t *entry, bool on)
{
    const uint64_t flag = 1ull << 63;
    if(!on) { *entry |= flag; } else { *entry &= ~flag; }
}

KAPI void
init_gdt()
{
    // we have segments set up already, we just want to replace them with our own (e.g. no 32bit
    // mode). All we got to do is basicaly replace the known-used descriptors with our own and
    // reload the segments afterwards.
    struct GDT *gdt;
    uint16_t data_segment, code_segment;
    __asm__("cli; sgdt (%0);"
            "movw %%ds, %1;"
            "movw %%cs, %2;" : "=r"(gdt), "=r"(data_segment), "=r"(code_segment));
    data_segment /= 8;
    code_segment /= 8;
    kmemset((uint8_t *)gdt->offset, gdt->limit + 1, 0);
    ((uint64_t *)gdt->offset)[data_segment] = 0xFFFF | 0xFull << 48
                                           | 1ull << 55 // this is a number of pages, not bytes
                                           | 1ull << 47 // present
                                           | 1ull << 44 // 1 for code and data segments
                                           | 1ull << 41 // writeable
                                           ;
    ((uint64_t *)gdt->offset)[code_segment] = 0xFFFF | 0xFull << 48
                                           | 1ull << 55
                                           | 1ull << 53 // executable
                                           | 1ull << 47
                                           | 1ull << 44
                                           | 1ull << 43 // executable
                                           | 1ull << 41 // readable
                                           ;
    // Set the other selectors to new segments and load up our “new” global descriptor table.
    __asm__("lgdt (%0);"
            // We didn’t change the index of data segment descriptor, thus reloading it like this
            // should suffice.
            "mov %%ds, %%ax;"
            "mov %%ax, %%ds;"
            "mov %%ax, %%ss;"
            "mov %%ax, %%es;"
            "mov %%ax, %%fs;"
            "mov %%ax, %%gs;"
            // Now, reload the code segment. Calling our no-op interrupt should suffice.
            "sti; int $32;" :: "r"(gdt) : "%ax");
}

KAPI EFI_STATUS
add_gdt_entry(uint64_t entry, uint16_t *byte)
{
    struct GDT *gdt;
    __asm__("cli; sgdt (%0);" : "=r"(gdt));
    for(uint16_t i = 1; i < gdt->limit + 1 / 8; i += 1) {
        uint64_t *e = ((uint64_t *)gdt->offset) + i;
        if(*e == 0) {
            *e = entry;
            *byte = i * 8;
            __asm__("lgdt (%0); sti;" : "=r"(gdt));
            return EFI_SUCCESS;
        }
    }
    return EFI_OUT_OF_RESOURCES;
}


// Implementation of malloc, from K&R
//
// uint64_t is chosen as an instance of the most restrictive alignment type
typedef union header {
  struct {
    union header *next;
    uint64_t size;       // in bytes
  } s;

  uint64_t _Align; // alignment
} Header;
static Header base;           // Used to get an initial member for free list
static Header *freep = NULL;  // Free list starting point
static uint64_t brk = 0x200000000; // 8GB onwards is heap

KAPI static Header *morecore(uint64_t nblocks);

KAPI void *
kmalloc(uint64_t nbytes)
{
    Header *currp;
    Header *prevp;

    // Number of pages needed to provide at least nbytes of memory.
    uint64_t pages = ((nbytes + sizeof(Header) - 1) / 0x1000) + 1;

    if (freep == NULL) {
        // Create degenerate free list; base points to itself and has size 0
        base.s.next = &base;
        base.s.size = 0;
        // Set free list starting point to base address
        freep = &base;
    }

    // Initialize pointers to two consecutive blocks in the free list, which we
    // call prevp (the previous block) and currp (the current block)
    prevp = freep;
    currp = prevp->s.next;

    // Step through the free list looking for a block of memory large enough.
    for (; ; prevp = currp, currp = currp->s.next) {
        if (currp->s.size >= nbytes) {
            if (currp->s.size == nbytes) {
                // Found exactly sized partition, good for us!
                prevp->s.next = currp->s.next;
            } else {
                uint64_t old_size = currp->s.size;
                void *old_next = currp->s.next;
                // Changes the memory stored at currp to reflect the reduced block size
                currp->s.size = nbytes;
                // Find location at which to create the block header for the new block
                prevp->s.next = (Header *)((uint64_t)currp + nbytes + sizeof(Header));
                // Store the block size in the new header
                prevp->s.next->s.next = old_next;
                prevp->s.next->s.size = old_size - nbytes - sizeof(Header);
            }
            // Set global starting position to the previous pointer
            freep = prevp;
            // Return the location of the start of the memory, not header
            return (void *) (currp + 1);
        }

        // no block found
        if (currp == freep) {
            if ((currp = morecore(pages)) == NULL) {
                return NULL;
            }
        }
    }
}

KAPI static Header *
morecore(uint64_t pages)
{
    void *freemem;    // The address of the newly created memory
    Header *insertp;  // Header ptr for integer arithmatic and constructing header

    freemem = (void *)brk;
    // Request for as many pages as necessary
    for(uint64_t p = 0; p < pages; p += 1) {
        void *page = allocate_page(global_kernel);
        // unable to allocate more memory; allocate_page returns NULL
        if (page == NULL) {
            serial_print("kmalloc: OOM");
            DEBUG_HALT;
        }
        // Map the page into our heap
        map_page(global_kernel, brk, 0, (uint64_t)page);
        brk += 0x1000;
    }
    // Construct new block
    insertp = (Header *)freemem;
    insertp->s.size = 0x1000 * pages;

    kfree((void *) (insertp + 1));

    return freep;
}

KAPI void
kfree(void *ptr)
{
    Header *insertp, *currp;

    // Find address of block header for the data to be inserted
    insertp = ((Header *) ptr) - 1;

    // Step through the free list looking for the position in the list to place
    // the insertion block.
    for (currp = freep; !((currp < insertp) && (insertp < currp->s.next)); currp = currp->s.next) {
        // currp >= currp->s.ptr implies that the current block is the rightmost
        // block in the free list.
        if ((currp >= currp->s.next) && ((currp < insertp) || (insertp < currp->s.next))) {
            break;
        }
    }

    if ((insertp + insertp->s.size) == currp->s.next) {
        insertp->s.size += currp->s.next->s.size;
        insertp->s.next = currp->s.next->s.next;
    } else {
        insertp->s.next = currp->s.next;
    }

    if ((currp + currp->s.size) == insertp) {
        currp->s.size += insertp->s.size;
        currp->s.next = insertp->s.next;
    } else {
        currp->s.next = insertp;
    }

    freep = currp;
}
