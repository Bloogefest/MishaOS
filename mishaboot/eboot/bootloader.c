#include <efi.h>
#include <efilib.h>

#include "elf.h"
#include "util.h"
#include "multiboot.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;

#define LOG_UEFI_CALLS 1

#define strof_(x) #x
#define strof(x) strof_(x)

#define uefi_call(func, va_num, ...)                                                                 \
    status = uefi_call_wrapper(func, va_num, __VA_ARGS__);                                           \
    if (EFI_ERROR(status)) {                                                                         \
        Print(L"[ERROR] L" strof(__LINE__) ": " #func "(" #__VA_ARGS__ ") FAILED (%x)\r\n", status); \
        while (1);                                                                                   \
    } else if(LOG_UEFI_CALLS) {                                                                      \
        Print(L"[STATUS] " #func "(" #__VA_ARGS__ ") OK\r\n");                                       \
    }

static uint32_t get_memory_type(EFI_MEMORY_TYPE type) {
    switch (type) {
        case EfiConventionalMemory:
        case EfiLoaderCode:
        case EfiLoaderData:
        case EfiBootServicesCode:
        case EfiBootServicesData:
        case EfiRuntimeServicesCode:
        case EfiRuntimeServicesData:
            return 1;

        default:
            return 2;
    }
}

void clear_screen() {
    memset((void*)(uintptr_t) gop->Mode->FrameBufferBase, 0, gop->Mode->FrameBufferSize);
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table) {
    InitializeLib(image_handle, system_table);

    UINTN count;
    EFI_HANDLE* handles;
    EFI_STATUS status;

    EFI_GRAPHICS_OUTPUT_PROTOCOL* graphics;

    uefi_call(ST->BootServices->LocateHandleBuffer, 5, ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &count, &handles);
    uefi_call(ST->BootServices->HandleProtocol, 3, handles[0], &gEfiGraphicsOutputProtocolGuid, (void**) &graphics);

    INT64 best_graphics_mode = -1;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* best_graphics_mode_info = NULL;
    for (uint32_t i = 0; i < graphics->Mode->MaxMode; i++) {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* current_mode;
        UINTN current_mode_size;

        uefi_call(graphics->QueryMode, 4, graphics, i, &current_mode_size, &current_mode);

        if (!best_graphics_mode_info || (UINT64) current_mode->HorizontalResolution * current_mode->VerticalResolution
                > (UINT64) best_graphics_mode_info->HorizontalResolution * best_graphics_mode_info->VerticalResolution) {
            best_graphics_mode_info = current_mode;
            best_graphics_mode = i;
        }
    }

    if (best_graphics_mode == -1) {
        Print(L"Graphics mode not found.\r\n");
        while (1);
    }

    uefi_call(graphics->SetMode, 2, graphics, (UINT32) best_graphics_mode);

    gop = graphics;

    clear_screen();

    uefi_call(ST->BootServices->SetWatchdogTimer, 4, 0, 0, 0, NULL);

    EFI_LOADED_IMAGE* loaded_image;
    uefi_call(ST->BootServices->HandleProtocol, 3, image_handle, &gEfiLoadedImageProtocolGuid, (void**) &loaded_image);

    EFI_FILE_IO_INTERFACE* filesystem;
    uefi_call(ST->BootServices->HandleProtocol, 3, loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**) &filesystem);

    EFI_FILE* root;
    uefi_call(filesystem->OpenVolume, 2, filesystem, &root);

    EFI_FILE* kernel_file;
    uefi_call(root->Open, 5, root, &kernel_file, L"KERNEL.ELF", EFI_FILE_MODE_READ, 0);

    Elf32_Ehdr header;

    UINTN read_size = sizeof(header);
    uefi_call(kernel_file->Read, 4, kernel_file, &read_size, &header);

    if (header.e_ident[0] != ELFMAG0 ||
        header.e_ident[1] != ELFMAG1 ||
        header.e_ident[2] != ELFMAG2 ||
        header.e_ident[3] != ELFMAG3) {
        Print(L"Invalid ELF32 magic value.\r\n");
        while (1);
    }

    uintptr_t final_offset = 0;
    Elf32_Phdr* phdrs;
    uefi_call(kernel_file->SetPosition, 2, kernel_file, header.e_phoff);
    read_size = header.e_phnum * header.e_phentsize;
    uefi_call(system_table->BootServices->AllocatePool, 3, EfiLoaderData, read_size, (void**) &phdrs);
    uefi_call(kernel_file->Read, 3, kernel_file, &read_size, phdrs);

    uintptr_t entry = (uintptr_t) header.e_entry;
    for (Elf32_Phdr* phdr = phdrs; (UINT8*) phdr < (UINT8*) phdrs + header.e_phnum * header.e_phentsize; phdr = (Elf32_Phdr*) ((UINT8*) phdr + header.e_phentsize)) {
        if (phdr->p_type == PT_LOAD) {
            Elf32_Word pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000;
            EFI_PHYSICAL_ADDRESS segment = phdr->p_paddr;
            uefi_call(system_table->BootServices->AllocatePages, 4, AllocateAddress, EfiLoaderData, pages, &segment);

            uefi_call(kernel_file->SetPosition, 2, kernel_file, phdr->p_offset);
            read_size = phdr->p_filesz;
            uefi_call(kernel_file->Read, 3, kernel_file, &read_size, (void*)(uintptr_t) segment);

            uintptr_t r = phdr->p_filesz;
            while (r < phdr->p_memsz) {
                *(UINT8*)(phdr->p_vaddr + r) = 0;
                ++r;
            }

            if (phdr->p_vaddr + r > final_offset) {
                final_offset = phdr->p_vaddr + r;
            }
        }
    }

    final_offset = (final_offset & ~0xFFF) + ((final_offset & 0xFFF) ? 0x1000 : 0);
    EFI_PHYSICAL_ADDRESS initrd_address = final_offset;

    EFI_FILE* initrd_file;
    uefi_call(root->Open, 5, root, &initrd_file, L"INITRD.IMG", EFI_FILE_MODE_READ, 0);

    EFI_FILE_INFO* initrd_file_info = LibFileInfo(initrd_file);
    UINTN initrd_file_size = initrd_file_info->FileSize;
    read_size = initrd_file_size;
    uefi_call(system_table->BootServices->AllocatePages, 4, AllocateAddress, EfiLoaderData, (initrd_file_size + 0x1000 - 1) / 0x1000 + 3, &initrd_address);
    uefi_call(initrd_file->Read, 3, initrd_file, &read_size, (void*)(uintptr_t) initrd_address);

    final_offset += initrd_file_size;
    final_offset = (final_offset & ~0xFFF) + ((final_offset & 0xFFF) ? 0x1000 : 0);

    struct multiboot* multiboot_header = (void*) final_offset;
    final_offset += sizeof(struct multiboot);
    memset(multiboot_header, 0, sizeof(struct multiboot));
    multiboot_header->flags = MULTIBOOT_FLAG_MODS | MULTIBOOT_FLAG_MMAP | MULTIBOOT_FLAG_VBE;

    multiboot_module_t* module = (multiboot_module_t*) final_offset;
    final_offset += sizeof(multiboot_module_t);
    memset(module, 0, sizeof(multiboot_module_t));
    module->mod_start = (uint32_t) initrd_address;
    module->mod_end = (uint32_t) (initrd_address + initrd_file_size);

    multiboot_header->mods_count = 1;
    multiboot_header->mods_addr = (uint32_t)(uintptr_t) module;

    vbe_info_t* vbe = (vbe_info_t*) final_offset;
    final_offset += sizeof(vbe_info_t);
    vbe->physbase = (uint32_t) gop->Mode->FrameBufferBase;
    vbe->x_res = gop->Mode->Info->HorizontalResolution;
    vbe->y_res = gop->Mode->Info->VerticalResolution;

    multiboot_header->vbe_mode_info = (uint32_t)(uintptr_t) vbe;

    final_offset = (final_offset & ~(0xFFF)) + ((final_offset & 0xFFF) ? 0x1000 : 0);
    uint32_t* mmap = (void*) final_offset;
    memset(mmap, 0, 1024);
    multiboot_header->mmap_addr = (uint32_t)(uintptr_t) mmap;

    UINTN map_size = 0;
    UINTN map_key;
    UINTN descriptor_size;
    uefi_call_wrapper(ST->BootServices->GetMemoryMap, 5, &map_size, NULL, &map_key, &descriptor_size, NULL);

    EFI_MEMORY_DESCRIPTOR* efi_memory = (void*) final_offset;
    final_offset += map_size;
    while ((uintptr_t) final_offset & 0x3FF) {
        ++final_offset;
    }

    uefi_call(ST->BootServices->GetMemoryMap, 5, &map_size, efi_memory, &map_key, &descriptor_size, NULL);

    UINTN descriptors = map_size / descriptor_size;
    for (UINTN i = 0; i < descriptors; ++i) {
        EFI_MEMORY_DESCRIPTOR* desc = efi_memory;

        mmap[0] = sizeof(uint64_t) * 2 + sizeof(uint32_t);

        mmap[1] = (uint32_t) (desc->PhysicalStart & 0xFFFFFFFF);
        mmap[2] = (uint32_t) (desc->PhysicalStart >> 32);
        UINT64 len = desc->NumberOfPages * 0x1000;
        mmap[3] = (uint32_t) (len & 0xFFFFFFFF);
        mmap[4] = (uint32_t) (len >> 32);
        mmap[5] = get_memory_type(desc->Type);

        mmap = (void*) ((uintptr_t) mmap + 24);
        efi_memory = (EFI_MEMORY_DESCRIPTOR*) ((UINT8*) efi_memory + descriptor_size);

        if (efi_memory->PhysicalStart == 0 && efi_memory->NumberOfPages == 0) {
            break;
        }
    }

    multiboot_header->mmap_length = (uint32_t)(uintptr_t) mmap - multiboot_header->mmap_addr;

    uefi_call_wrapper(ST->BootServices->GetMemoryMap, 5, &map_size, NULL, &map_key, &descriptor_size, NULL);
    status = uefi_call_wrapper(ST->BootServices->ExitBootServices, 2, image_handle, map_key);
    if (status != EFI_SUCCESS) {
        Print(L"[ERROR] ExitBootServices() FAILED (%x)\r\n", status);
        while (1);
    }

    clear_screen();

    if (sizeof(uintptr_t) == sizeof(uint64_t)) {
        extern uint8_t call_kernel64[];
        uint64_t call_kernel_data = ((uint32_t)(uintptr_t) &call_kernel64) | (0x10LL << 32);
        __asm__ __volatile__("push %0\n"
                             "lretl\n" : : "g"(call_kernel_data), "c"(entry), "a"(MULTIBOOT_EAX_MAGIC), "b"(multiboot_header));
        __builtin_unreachable();
    }

    __asm__ __volatile__("jmp call_kernel32" : : "c"(entry), "a"(MULTIBOOT_EAX_MAGIC), "b"(multiboot_header));
    __builtin_unreachable();
}