/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */
#include <autoconf.h>

#include <assembler.h>

.extern efi_early_init

.section ".text.efi.header"

 /*
 * This is PE32+/COFF executable file format for UEFI Spec.
 * See http://www.uefi.org/specifications
 *
 * Note:
 * The .text section of PE32+/COFF executable file should be multiple of
 * PECOFF_FILE_ALIGNMENT. We align to a page so that we could pad _edata to align
 * the image size with PECOFF_FILE_ALIGNMENT.
 */

efi_head:
    .ascii  "MZ"                            // Magic "MZ" signature required by UEFI
    .short  0
    .long   0
    .quad   0
    .quad   0
    .quad   0
    .quad   0
    .quad   0
    .quad   0
    .long   0
    .long   pe_header - efi_head            // Offset to the begining of PE header, ignore DOS stub

pe_header:
    .ascii  "PE"
    .short  0

coff_header:
#ifdef CONFIG_ARCH_AARCH64
    .short  0xaa64                          // AArch64
#else
    .short  0x01c2                          // AArch32
#endif
    .short  2                               // Number of sections
    .long   0                               // TimeStamp
    .long   0                               // PointerToSymbolTable
    .long   1                               // NumberOfSymbolTable
    .short  section_table - optional_header // SizeOfOptionalHeader
#ifdef CONFIG_ARCH_AARCH64
    .short  0x206                           // Characteristics.
                                            // IMAGE_FILE_DEBUG_STRIPPED |
                                            // IMAGE_FILE_EXECUTABLE_IMAGE |
                                            // IMAGE_FILE_LINE_NUMS_STRIPPED
#else
    .short  0x306                           // Characteristics.
                                            // IMAGE_FILE_32BIT_MACHINE |
                                            // IMAGE_FILE_DEBUG_STRIPPED |
                                            // IMAGE_FILE_EXECUTABLE_IMAGE |
                                            // IMAGE_FILE_LINE_NUMS_STRIPPED
#endif
optional_header:
#ifdef CONFIG_ARCH_AARCH64
    .short  0x20b                           // Magic for PE32+ format
#else
    .short  0x10b                           // Magic for PE32 format
#endif
    .byte   0x02                            // MajorLinkerVersion
    .byte   0x14                            // MinorLinkerVersion
    .long   _end - efi_head_end             // SizeOfCode
    .long   0                               // SizeOfInitializedData
    .long   0                               // SizeOfUninitializedData
    .long   efi_stub_entry - efi_head       // AddressOfEntryPoint
    .long   efi_head_end - efi_head         // BaseOfCode
    .long   0                               // BaseOfData

extra_header_fields:
    .long  0                                // Image Base
    .long   0x1000                          // Section Alignment
    .long   PECOFF_FILE_ALIGNMENT           // File Alignment (See linker script)
    .short  0                               // MajorOperatingSystemVersion
    .short  0                               // MinorOperatingSystemVersion
    .short  0                               // MajorImageVersion
    .short  0                               // MinorImageVersion
    .short  0                               // MajorSubsystemVersion
    .short  0                               // MinorSubsystemVersion
    .long   0                               // Win32VersionValue
    .long   _end - efi_head                 // SizeOfImage
    .long   efi_head_end - efi_head         // SizeOfHeaders
    .long   0                               // CheckSum
    .short  0xa                             // Subsystem (EFI application)
    .short  0                               // DllCharacteristics
#ifdef CONFIG_ARCH_AARCH64
    .quad   0                               // SizeOfStackReserve
    .quad   0                               // SizeOfStackCommit
    .quad   0                               // SizeOfHeapReserve
    .quad   0                               // SizeOfHeapCommit
#else
    .long   0                               // SizeOfStackReserve
    .long   0                               // SizeOfStackCommit
    .long   0                               // SizeOfHeapReserve
    .long   0                               // SizeOfHeapCommit
#endif
    .long   0                               // LoaderFlags
    .long   (section_table - .) / 8         // NumberOfRvaAndSizes
    .quad   0                               // ExportTable and SizeOfExportTable
    .quad   0                               // ImportTable and SizeOfImportTable
    .quad   0                               // ResourceTable and SizeOfResourceTable
    .quad   0                               // ExceptionTable and SizeOfExceptionTable
    .quad   0                               // CertificationTable and SizeOfCertificateTable
    .quad   0                               // BaseRelocationTable and SizeOfBaseRelocationTable

section_table:
    .ascii	".reloc"
    .byte	0
    .byte	0                               // end of 0 padding of section name
    .long	0
    .long	0
    .long	0                               // SizeOfRawData
    .long	0                               // PointerToRawData
    .long	0                               // PointerToRelocations
    .long	0                               // PointerToLineNumbers
    .short	0                               // NumberOfRelocations
    .short	0                               // NumberOfLineNumbers
    .long	0x42100040                      // Characteristics (section flags)

    .ascii	".text"
    .byte	0
    .byte	0
    .byte	0                               // end of 0 padding of section name
    .long	_end - efi_head_end             // VirtualSize
    .long	efi_head_end - efi_head         // VirtualAddress
    .long	_edata - efi_head_end           // SizeOfRawData
    .long	efi_head_end - efi_head         // PointerToRawData
    .long	0                               // PointerToRelocations (0 for executables)
    .long	0                               // PointerToLineNumbers (0 for executables)
    .short	0                               // NumberOfRelocations (0 for executables)
    .short	0                               // NumberOfLineNumbers (0 for executables)
    .long	0xe0500020                      // Characteristics (section flags)

    .align 12
efi_head_end:

/*
 * This function is called by UEFI firmware as entry to efi application.
 * We arrive here from the EFI boot manager with:
 * On AArch64:
 *      * x0 as current application handler pointer and x1 as system table pointer.
 *      * MMU on with identity-mapped RAM
 *      * I-cache and D-cache on
 *      * EL2 or EL1 based on firmware configuration
 * On AArch32:
 *      * r0 as current application handler pointer and r1 as system table pointer.
 *      * MMU on with identity-mapped RAM
 *      * I-cache and D-cache on
 *      * A privileged mode (i.e. hyp or svc)
 *
 * Here we can do any UEFI dependant initialization, e.g. pass command line arguments to main
 */

.text

BEGIN_FUNC(efi_stub_entry)
#ifndef CONFIG_ARCH_AARCH64
    bl      arch_early_relocate             // Fixup GOT on AArch32
#endif

    bl      efi_early_init                  // Early efi driver initialization
                                            // we do not return to firmware so FR/LR are ignored!
    b       _start                          // Jump to elfloader entry point
END_FUNC(efi_stub_entry)


