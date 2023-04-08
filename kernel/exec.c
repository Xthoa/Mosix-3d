#include "exec.h"
#include "proc.h"
#include "vfs.h"
#include "mutex.h"
#include "vmem.h"
#include "pe.h"
#include "kheap.h"
#include "string.h"
#include "macros.h"
#include "spin.h"
#include "asm.h"
#include "pmem.h"

vaddr_t randomize_stacktop(){
    return DEFAULT_STACKTOP;
}

Freelist drvflist;
void ePeSeekLfanew(File* f){
    ImageDosHeader* idh = kheap_alloc(sizeof(*idh));
    read(f, idh, sizeof(*idh));
    lseek(f, idh->e_lfanew, SEEK_SET);
    kheap_free(idh);
}
u32 ePeConvertFlags(u32 sch){
    u32 flags = 0;
    if(sch & IMAGE_SCN_MEM_READ) flags |= VM_READ;
    if(sch & IMAGE_SCN_MEM_WRITE) flags |= VM_WRITE;
    if(sch & IMAGE_SCN_MEM_EXECUTE) flags |= VM_EXECUTE;
    return flags;
}
void ePeMakePeinfo(Process* p, ImageNtHeaders64* inh, u64 base){
    ImageDataDirectory* idd = inh->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_IMPORT;
    u32 dllcnt = idd->Size / 20;
    p->peinfo.dlls = kheap_alloc(sizeof(ActivePedll) * (dllcnt + 8));
    p->peinfo.count = 0;
    p->peinfo.base = base;
    p->peinfo.idata = idd->VirtualAddress;
    init_spinlock(&p->peinfo.lock);
}
vaddr_t ePeDriverRelocation(ImageNtHeaders64* inh){
    u32 imgpages = inh->OptionalHeader.SizeOfImage >> 12;
    u64 base = flist_alloc(&drvflist, imgpages) << 12;
    return base;
}
Process* RunPe64(File *f){
    ePeSeekLfanew(f);

    ImageNtHeaders64* inh = kheap_alloc(sizeof(*inh));
    read(f, inh, sizeof(*inh));
    if(inh->Signature != IMAGE_NT_SIGNATURE){
        kheap_free(inh);
        return NULL;
    }

    vaddr_t base = inh->OptionalHeader.ImageBase;
    u32 seccnt = inh->FileHeader.NumberOfSections;
    Process* p = create_process(
        f->node->name, 
        NULL, 
        inh->OptionalHeader.SizeOfStackReserve >> 12, 
        inh->OptionalHeader.SizeOfStackCommit >> 12, 
        randomize_stacktop(),
        inh->OptionalHeader.AddressOfEntryPoint + base
    );
    p->env = PENV_PE64;

    ePeMakePeinfo(p, inh, base);
    kheap_free(inh);

    for(int i = 0; i < seccnt; i++){
        ImageSectionHeader* ish = kheap_alloc(sizeof(*ish));
        read(f, ish, sizeof(*ish));
        
        u32 pages = pages4k(ish->VirtualSize);
        vaddr_t lin = malloc_page4k(pages);
        paddr_t phy = kernel_v2p(lin);

        u32 sch = ish->Characteristics;
        u16 flags = ePeConvertFlags(sch);
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
    return p;
}
Process* RunElf64(File* f){
    return NULL;
}
Process* ExecuteFileSuspend(char* path){
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
Process* ExecuteFile(char* path){
    Process* p = ExecuteFileSuspend(path);
    ready_process(p);
    return p;
}

Pedll* dllroot;
Spinlock dlllock;
Pedll* LoadPedll64(File* f){
    ePeSeekLfanew(f);

    ImageNtHeaders64* inh = kheap_alloc(sizeof(*inh));
    read(f, inh, sizeof(*inh));
    if(inh->Signature != IMAGE_NT_SIGNATURE){
        kheap_free(inh);
        return NULL;
    }

    Pedll* dll = kheap_alloc(sizeof(Pedll));
    dll->seccnt = inh->FileHeader.NumberOfSections;
    dll->imgsize = inh->OptionalHeader.SizeOfImage;
    dll->entry = inh->OptionalHeader.AddressOfEntryPoint;
    dll->idata = inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    dll->edata = inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    dll->name = kheap_clonestr(f->node->name);
    dll->sec = kheap_alloc(sizeof(Pesection) * dll->seccnt);
    dll->actual = f->node;

    if(inh->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_MOSIX_DRV){
        u64 nbase = ePeDriverRelocation(inh);
        ImageDataDirectory* idd = inh->OptionalHeader.DataDirectory + IMAGE_DIRECTORY_ENTRY_BASERELOC;
        dll->relocs = idd->VirtualAddress;
        dll->relocsz = idd->Size;
        dll->base = nbase;
        dll->relocfix = nbase - inh->OptionalHeader.ImageBase;
    }
    kheap_free(inh);

    for(int i = 0; i < dll->seccnt; i++){
        Pesection* sec = dll->sec + i;
        ImageSectionHeader* ish = kheap_alloc(sizeof(*ish));
        read(f, ish, sizeof(*ish));
        
        u32 sch = ish->Characteristics;
        u16 flags = ePeConvertFlags(sch);
        sec->flags = flags;
        sec->voff = ish->VirtualAddress;

        u32 pages = pages4k(ish->VirtualSize);
        sec->pages = pages;

        if(sch & IMAGE_SCN_BSS){
            sec->size = 0;
            sec->bsssz = ish->VirtualSize;
            sec->paddr = 0;
        }
        else{
            vaddr_t lin = malloc_page4k(pages4k(ish->SizeOfRawData));
            paddr_t phy = kernel_v2p(lin);
            u64 off = f->off;
            lseek(f, ish->PointerToRawData, SEEK_SET);
            read(f, lin, ish->VirtualSize);
            lseek(f, off, SEEK_SET);
            sec->size = ish->SizeOfRawData;
            sec->bsssz = 0;
            sec->paddr = phy;
        }
        if(ish->VirtualSize > ish->SizeOfRawData){
            sec->bsssz = ish->VirtualSize - ish->SizeOfRawData;
        }

        if((flags & VM_WRITE) == 0){
            sec->flags |= VM_SHARE;
            SharedVmarea* sa = kheap_alloc(sizeof(SharedVmarea));
            sa->paddr = sec->paddr;
            sa->ref = 0;
            sec->paddr = sa;
        }

        kheap_free(ish);
    }
    return dll;
}
void exec_init(){
    dllroot = NULL;
    init_spinlock(&dlllock);
    drvflist.root = kheap_alloc(PAGE_SIZE / 8);
    drvflist.max = 64;
    drvflist.size = 1;
    init_spinlock(&drvflist.lock);
    Extent* e = drvflist.root;
    e->pos = DRIVER_BOTTOM >> 12;
    e->size = DRIVER_MAX >> 12;
}
void InsertDll(Pedll* d){
    acquire_spin(&dlllock);
    d->next = dllroot;
    if(dllroot) dllroot->prev = d;
    dllroot = d;
    release_spin(&dlllock);
}
Node* FindPedll(char* path){
    Node* n = path_walk(path);
    if(n) return n;
    n = path_walk("/files/boot");
    n = find_node_from(n, path);
    return n;
}
Pedll* LoadPedll(Node* node){
    File* f = open_node(node, 0);
    if(f == NULL) return NULL;
    Pedll* d = LoadPedll64(f);
    close(f);
    InsertDll(d);
    return d;
}
Pedll* LocatePedll(char* path){
    Node* node = FindPedll(path);
    if(!node) return NULL;
    acquire_spin(&dlllock);
    for(Pedll* dll = dllroot; dll; dll = dll->next){
        if(dll->actual == node){
            release_spin(&dlllock);
            return dll;
        }
    }
    release_spin(&dlllock);
    return LoadPedll(node);
}
ActivePedll* MapPedll(Process* p, Pedll* dll){
    u64 top = PEDLL_TOP;
    acquire_spin(&p->peinfo.lock);
    if(p->peinfo.count) top = p->peinfo.dlls[p->peinfo.count - 1].base;
    ActivePedll* adll = p->peinfo.dlls + p->peinfo.count++;
    u64 base = top - dll->imgsize;
    adll->base = base;
    release_spin(&p->peinfo.lock);
    adll->dll = dll;
    for(int i = 0; i < dll->seccnt; i++){
        Pesection* s = dll->sec + i;
        u64 paddr;
        uint32_t pgattr = 0;
        if((s->flags & VM_WRITE) == 0) pgattr |= PGATTR_READONLY;
        if((s->flags & VM_EXECUTE) == 0) pgattr |= PGATTR_NOEXEC;
        if(s->flags & VM_SHARE){
            SharedVmarea* sa = s->paddr;
            lock_inc(sa->ref);
            paddr = sa->paddr;
            insert_vmarea(p->vm, base + s->voff, sa, s->pages, VM_LIBIMAGE, s->flags);
        }
        else{
            u64 klin = malloc_page4k(s->pages);
            paddr = kernel_v2p(klin);
            if(s->paddr != 0) memmove(klin, kernel_p2v(s->paddr), s->size);
            if(s->bsssz) memset(klin + s->size, 0, s->bsssz);
            insert_vmarea(p->vm, base + s->voff, paddr, s->pages, VM_LIBIMAGE, s->flags);
        }
        set_mappings(base + s->voff, paddr, s->pages, pgattr);
    }
    return adll;
}
u64 GetProcAddrByOrd(Process* p, ActivePedll* dll, u16 ord){
	ImageExportDirectory* ied = dll->base + dll->dll->edata;
	if(ord > ied->NumberOfFunctions) return 0;
	u32* f = ied->AddressOfFunctions + dll->base;
	return f[ord] + dll->base;
}
u64 GetProcAddrByName(Process* p, ActivePedll* dll, char* name){
	ImageExportDirectory* ied = dll->base + dll->dll->edata;
	u32* f = ied->AddressOfNames + dll->base;
	u16* w = ied->AddressOfNameOrdinals + dll->base;
	for(int i = 0; i < ied->NumberOfNames; i++){
		char* n = f[i] + dll->base;
		if(!strcmp(name, n)){
			return GetProcAddrByOrd(p, dll, w[i]);
		}
	}
	return 0;
}
void LoadPedllsByImportTable(Process* p, u64 base, u32 idata){
    ImageImportDescriptor* iid = base + idata;
    for(; iid->Characteristics && iid->Name; iid ++){
		char* name = base + iid->Name;
        Pedll* dll = LocatePedll(name);
        ActivePedll* adll = MapPedll(p, dll);
        LoadPedllsByImportTable(p, adll->base, adll->dll->idata);
		ImageThunkData64* t = base + iid->FirstThunk;
		for(; t->Ordinal; t ++){
			if(t->AddressOfData >> 31) t->Function = GetProcAddrByOrd(p, adll, t->Ordinal);
			else t->Function = GetProcAddrByName(p, adll, base + t->AddressOfData + 2);
		}
        void (*dlentry)() = adll->base + dll->entry;
        dlentry();
	}
}
void LoadPedllsForProcess(Process* p){
    LoadPedllsByImportTable(p, p->peinfo.base, p->peinfo.idata);
}

void ePeFixRelocation(Pedll* dll){
    u64 base = dll->base;
	u64 vra = base + dll->relocs;
	u64 dest = vra + dll->relocsz;
	while(vra < dest){
		ImageBaseRelocation* ibr = vra;
		u64 area = base + ibr->VirtualAddress;
		u16 cnt = (ibr->SizeOfBlock - 8) / 2;
		for(int i = 0; i < cnt; i++){
			u16 offset = ibr->TypeOffset[i];
			if((offset >> 12) == 0) break;
			u64 lin = area + (offset & 0xfff);
			u64* fixup = lin;
			if((*fixup >> 48) == 0){
                *fixup += dll->relocfix;
            }
		}
		vra += ibr->SizeOfBlock;
	}
}
void MapDriver(Pedll* dll){
    u64 base = dll->base;
    for(int i = 0; i < dll->seccnt; i++){
        Pesection* s = dll->sec + i;
        u64 paddr;
        uint32_t pgattr = 0;
        if((s->flags & VM_WRITE) == 0) pgattr |= PGATTR_READONLY;
        if((s->flags & VM_EXECUTE) == 0) pgattr |= PGATTR_NOEXEC;
        if(s->flags & VM_SHARE){
            SharedVmarea* sa = s->paddr;
            lock_inc(sa->ref);
            paddr = sa->paddr;
        }
        else{
            u64 klin = kernel_p2v(s->paddr);
            if(s->paddr == 0){
                klin = malloc_page4k(s->pages);
                paddr = kernel_v2p(klin);
            }
            else paddr = s->paddr;
            if(s->bsssz) memset(klin + s->size, 0, s->bsssz);
        }
        set_mappings(base + s->voff, paddr, s->pages, pgattr);
    }
}
Pedll* LoadDriver(char* name){
    Pedll* dll = LocatePedll(name);
    if(dll->relocfix == 0) return dll;
    MapDriver(dll);
    ePeFixRelocation(dll);
    dll->relocfix = 0;
    void (*drvinit)(Bool) = dll->entry + dll->base;
    drvinit(DRIVER_INIT);
    return dll;
}
void UnloadDriver(Pedll* dll){
    void (*drvexit)(Bool) = dll->entry + dll->base;
    drvexit(DRIVER_EXIT);
}
