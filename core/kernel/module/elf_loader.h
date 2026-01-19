#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <outputs/types.h>

// ELF types
typedef u32 Elf32_Addr;
typedef u16 Elf32_Half;
typedef u32 Elf32_Off;
typedef int Elf32_Sword;
typedef u32 Elf32_Word;

// ELF header
#define EI_NIDENT 16
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

// Section header
typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

// Symbol table entry
typedef struct {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    Elf32_Word st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half st_shndx;
} Elf32_Sym;

// Relocation entry
typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
} Elf32_Rel;

// Constants
#define ET_REL 1  // Relocatable file
#define EM_386 3  // Intel 80386

#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_REL 9

#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_WRITE 0x1

#define STT_FUNC 2
#define STT_OBJECT 1

#define ELF_ST_BIND(i) ((i) >> 4)
#define ELF_ST_TYPE(i) ((i) & 0xf)
#define ELF32_R_SYM(i) ((i) >> 8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))

#define R_386_32 1
#define R_386_PC32 2

// Function prototypes
int elf_load_module(const char *path, void **module_base, size_t *module_size, Elf32_Addr *entry_point);
int elf_resolve_symbols(void *module_base, Elf32_Shdr *symtab, Elf32_Shdr *strtab, Elf32_Shdr *rel_sections[], int rel_count);
Elf32_Addr elf_lookup_symbol(const char *name);

#endif