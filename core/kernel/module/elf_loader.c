#include "elf_loader.h"
#include <string/string.h>
#include <kernel/mem/phys/physmem.h>  // For memory allocation
#include <fs/vfs/vfs.h>  // For file operations
#include <outputs/print.h>  // For debugging
#include <kernel/include/defs.h>
#include <drivers/memory/mem.h>

extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

Elf32_Addr elf_lookup_symbol(const char *name) {
    return (Elf32_Addr)kernel_symbol_lookup(name);
}

int elf_load_module(const char *path, void **module_base, size_t *module_size, Elf32_Addr *entry_point) {
    // Open file
    int fd = fs_open(path, O_RDONLY);
    if (fd < 0) {
        print("Failed to open module file\n", 0);
        return -1;
    }

    // Read ELF header
    Elf32_Ehdr ehdr;
    if (fs_read(fd, &ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        print("Failed to read ELF header\n", 0);
        fs_close(fd);
        return -1;
    }

    // Validate ELF
    if (ehdr.e_ident[0] != 0x7f || ehdr.e_ident[1] != 'E' || ehdr.e_ident[2] != 'L' || ehdr.e_ident[3] != 'F') {
        print("Invalid ELF magic\n", 0);
        fs_close(fd);
        return -1;
    }
    if (ehdr.e_type != ET_REL || ehdr.e_machine != EM_386) {
        print("Unsupported ELF type/machine\n", 0);
        fs_close(fd);
        return -1;
    }

    // Read section headers
    Elf32_Shdr *shdrs = kmalloc(ehdr.e_shnum * sizeof(Elf32_Shdr));
    if (!shdrs) {
        print("Failed to allocate section headers\n", 0);
        fs_close(fd);
        return -1;
    }
    fs_lseek(fd, ehdr.e_shoff, SEEK_SET);
    fs_read(fd, shdrs, ehdr.e_shnum * sizeof(Elf32_Shdr));

    // Read string table
    Elf32_Shdr *strtab_sh = &shdrs[ehdr.e_shstrndx];
    char *strtab = kmalloc(strtab_sh->sh_size);
    if (!strtab) {
        print("Failed to allocate string table\n", 0);
        kfree(shdrs);
        fs_close(fd);
        return -1;
    }
    fs_lseek(fd, strtab_sh->sh_offset, SEEK_SET);
    fs_read(fd, strtab, strtab_sh->sh_size);

    // Calculate total size for module
    size_t total_size = 0;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_flags & SHF_ALLOC) {
            total_size += shdrs[i].sh_size;
        }
    }

    // Allocate memory for module
    void *base = kmalloc(total_size);
    if (!base) {
        print("Failed to allocate module memory\n", 0);
        kfree(strtab);
        kfree(shdrs);
        fs_close(fd);
        return -1;
    }

    // Load sections
    Elf32_Addr current_addr = (Elf32_Addr)base;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_flags & SHF_ALLOC) {
            fs_lseek(fd, shdrs[i].sh_offset, SEEK_SET);
            fs_read(fd, (void*)current_addr, shdrs[i].sh_size);
            shdrs[i].sh_addr = current_addr;  // Update address
            current_addr += shdrs[i].sh_size;
        }
    }

    // Find symbol table and relocations
    Elf32_Shdr *symtab = NULL, *symstrtab = NULL;
    Elf32_Shdr *rel_sections[10];  // Assume max 10 rel sections
    int rel_count = 0;

    for (int i = 0; i < ehdr.e_shnum; i++) {
        if (shdrs[i].sh_type == SHT_SYMTAB) {
            symtab = &shdrs[i];
            symstrtab = &shdrs[shdrs[i].sh_link];
        } else if (shdrs[i].sh_type == SHT_REL) {
            rel_sections[rel_count++] = &shdrs[i];
        }
    }

    // Resolve symbols and apply relocations
    if (symtab && rel_count > 0) {
        if (elf_resolve_symbols(base, symtab, symstrtab, rel_sections, rel_count) != 0) {
            print("Failed to resolve symbols\n", 0);
            kfree(base);
            kfree(strtab);
            kfree(shdrs);
            fs_close(fd);
            return -1;
        }
    }

    // Cleanup
    kfree(strtab);
    kfree(shdrs);
    fs_close(fd);

    *module_base = base;
    *module_size = total_size;
    *entry_point = ehdr.e_entry;  // For modules, entry might be init function

    return 0;
}

int elf_resolve_symbols(void *module_base, Elf32_Shdr *symtab, Elf32_Shdr *strtab, Elf32_Shdr *rel_sections[], int rel_count) {
    // Read symbol table
    Elf32_Sym *syms = kmalloc(symtab->sh_size);
    if (!syms) return -1;
    memcpy(syms, (void*)symtab->sh_addr, symtab->sh_size);

    // Read string table for symbols
    char *symstr = kmalloc(strtab->sh_size);
    if (!symstr) {
        kfree(syms);
        return -1;
    }
    memcpy(symstr, (void*)strtab->sh_addr, strtab->sh_size);

    for (int r = 0; r < rel_count; r++) {
        Elf32_Shdr *rel_sh = rel_sections[r];
        Elf32_Rel *rels = (Elf32_Rel*)rel_sh->sh_addr;
        int rel_num = rel_sh->sh_size / sizeof(Elf32_Rel);

        for (int i = 0; i < rel_num; i++) {
            Elf32_Rel *rel = &rels[i];
            int sym_idx = ELF32_R_SYM(rel->r_info);
            int rel_type = ELF32_R_TYPE(rel->r_info);

            Elf32_Sym *sym = &syms[sym_idx];
            const char *sym_name = &symstr[sym->st_name];

            Elf32_Addr sym_addr = 0;
            if (sym->st_shndx != 0) {  // Local symbol
                sym_addr = sym->st_value;  // Relative to module
            } else {  // External symbol
                sym_addr = elf_lookup_symbol(sym_name);
                if (!sym_addr) {
                    print("Unresolved symbol: ", 0);
                    print(sym_name, 0);
                    print("\n", 0);
                    kfree(symstr);
                    kfree(syms);
                    return -1;
                }
            }

            // Apply relocation
            Elf32_Addr *target = (Elf32_Addr*)(module_base + rel->r_offset);
            switch (rel_type) {
                case R_386_32:
                    *target += sym_addr;
                    break;
                case R_386_PC32:
                    *target += sym_addr - (Elf32_Addr)target;
                    break;
                default:
                    print("Unsupported relocation type\n", 0);
                    kfree(symstr);
                    kfree(syms);
                    return -1;
            }
        }
    }

    kfree(symstr);
    kfree(syms);
    return 0;
}