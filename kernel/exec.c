#include "exec.h"
#include "proc.h"
#include "vfs.h"
#include "mutex.h"
#include "vmem.h"
#include "pe.h"
#include "kheap.h"
#include "string.h"
#include "macros.h"

vaddr_t randomize_stacktop(){
    return DEFAULT_STACKTOP;
}

Process* RunPe64(File *f){
    ImageDosHeader* idh = kheap_alloc(sizeof(*idh));
    read(f, idh, sizeof(*idh));
    lseek(f, idh->e_lfanew, SEEK_SET);
    kheap_free(idh);

    ImageNtHeaders64* inh = kheap_alloc(sizeof(*inh));
    read(f, inh, sizeof(*inh));
    if(inh->Signature != IMAGE_NT_SIGNATURE){
        kheap_free(inh);
        return NULL;
    }

    vaddr_t base = inh->OptionalHeader.ImageBase;
    Process* p = create_process(
        f->node->name, 
        NULL, 
        inh->OptionalHeader.SizeOfStackReserve >> 12, 
        inh->OptionalHeader.SizeOfStackCommit >> 12, 
        randomize_stacktop(),
        inh->OptionalHeader.AddressOfEntryPoint + base
    );

    for(int i = 0; i < inh->FileHeader.NumberOfSections; i++){
        ImageSectionHeader* ish = kheap_alloc(sizeof(*ish));
        read(f, ish, sizeof(*ish));
        
        u32 pages = pages4k(ish->VirtualSize);
        vaddr_t lin = malloc_page4k(pages);
        paddr_t phy = kernel_v2p(lin);

        u32 sch = ish->Characteristics;
        u16 flags = 0;
        if(sch & IMAGE_SCN_MEM_READ) flags |= VM_READ;
        if(sch & IMAGE_SCN_MEM_WRITE) flags |= VM_WRITE;
        if(sch & IMAGE_SCN_MEM_EXECUTE) flags |= VM_EXECUTE;
        insert_vmarea(p->vm, ish->VirtualAddress + base, phy, pages, VM_IMAGE, flags);

        if(sch & IMAGE_SCN_BSS) memset(lin, 0, ish->VirtualSize);
        else{
            u64 off = f->off;
            lseek(f, ish->PointerToRawData, SEEK_SET);
            read(f, lin, ish->VirtualSize);
            lseek(f, off, SEEK_SET);
        }
        if(ish->VirtualSize > ish->SizeOfRawData){
            memset(lin + ish->SizeOfRawData, 0, ish->VirtualSize - ish->SizeOfRawData);
        }

        kheap_free(ish);
    }
    kheap_free(inh);

    ready_process(p);
    return p;
}
Process* RunElf64(File* f){
    return NULL;
}
Process* ExecuteFile(char* path){
    File* f = open(path, 0);
    if(f == NULL) return NULL;
    char cont[4];
    read(f, cont, 4);
    lseek(f, 0, SEEK_SET);
    Process* p;
    if(*(short*)cont == *(short*)"MZ") p = RunPe64(f);
    if(*(int*)cont == *(int*)"\177ELF") p = RunElf64(f);
    close(f);
    return p;
}
