/* Startup-only resolver: no UObject access and no game-thread work. */
#define WIN32_LEAN_AND_MEAN
#include "address_resolver.h"
#include <bcrypt.h>
#include <stddef.h>
#include <string.h>

#include "address_profile.h"
#include "address_profile_development.h"

typedef struct {
    const HfProfile *profiles;
    const HfReference *edges, *globals;
    unsigned edge_count, global_count;
    const HfProfile *helper;
    HfReference helper_call;
    const char *name;
} LayoutFamily;
#define COUNT(a) (sizeof(a)/sizeof(*(a)))


static BOOL read_memory(uintptr_t address, void *out, SIZE_T size) {
    SIZE_T got=0;
    return ReadProcessMemory(GetCurrentProcess(),(void *)address,out,size,&got) && got==size;
}
static BOOL inside(uint32_t start, uint32_t size, uint32_t limit) {
    return start<=limit && size<=limit-start;
}
static BOOL in_section(const IMAGE_SECTION_HEADER *sections, unsigned count,
                      uint32_t rva, uint32_t size, DWORD require, DWORD forbid) {
    for(unsigned i=0;i<count;i++) {
        const IMAGE_SECTION_HEADER *s=&sections[i];
        if((s->Characteristics&require)!=require || (s->Characteristics&forbid))continue;
        if(rva>=s->VirtualAddress && inside(rva-s->VirtualAddress,size,s->Misc.VirtualSize))return TRUE;
    }
    return FALSE;
}
static BOOL fingerprint(BCRYPT_ALG_HANDLE alg, uintptr_t address, const HfProfile *p,
                        unsigned char *buffer) {
    if(!read_memory(address,buffer,p->size))return FALSE;
    for(unsigned i=0;i<32 && i<p->size;i++)if((buffer[i]&p->anchor_mask[i])!=p->anchor[i])return FALSE;
    for(unsigned i=0;i<p->mask_count;i++) {
        if(!inside(p->masks[i].offset,p->masks[i].size,p->size))return FALSE;
        memset(buffer+p->masks[i].offset,0,p->masks[i].size);
    }
    BCRYPT_HASH_HANDLE hash=NULL;
    unsigned char digest[32];
    if(BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)<0)return FALSE;
    BOOL ok=BCryptHashData(hash,buffer,p->size,0)>=0 && BCryptFinishHash(hash,digest,32,0)>=0;
    BCryptDestroyHash(hash);
    return ok && !memcmp(digest,p->digest,32);
}
static BOOL reference_target(uintptr_t base, uint32_t image_size, const uint32_t *rvas,
                             const HfReference *ref, uint32_t *target) {
    int32_t delta;
    if(!read_memory(base+rvas[ref->from]+ref->displacement,&delta,4))return FALSE;
    int64_t value=(int64_t)rvas[ref->from]+ref->next+delta;
    if(value<0 || value>=image_size)return FALSE;
    *target=(uint32_t)value;
    return TRUE;
}
static BOOL resolve_family(HMODULE module, uint32_t rvas[HF_PROFILE_COUNT], uintptr_t globals[HF_GLOBAL_COUNT], HfLog log, const LayoutFamily *family) {
    uintptr_t base=(uintptr_t)module;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS64 nt;
    IMAGE_SECTION_HEADER sections[96];
    RUNTIME_FUNCTION *functions=NULL;
    unsigned char *buffer=NULL;
    BCRYPT_ALG_HANDLE alg=NULL;
    BOOL ok=FALSE;
    unsigned matches[HF_PROFILE_COUNT]={0};
    /* Singleton getters can have many compiler-identical siblings; references
       from the audited native witnesses identify the required one. Still bounded. */
    enum { MAX_CANDIDATES=1024 };
    uint32_t candidates[HF_PROFILE_COUNT][MAX_CANDIDATES]={{0}};
    memset(rvas,0,sizeof(uint32_t)*HF_PROFILE_COUNT);
    memset(globals,0,sizeof(uintptr_t)*HF_GLOBAL_COUNT);
    if(!read_memory(base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE ||
       dos.e_lfanew<=0 || dos.e_lfanew>0x100000 ||
       !read_memory(base+(uint32_t)dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE ||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       nt.FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64) ||
       !nt.FileHeader.NumberOfSections || nt.FileHeader.NumberOfSections>96 ||
       nt.OptionalHeader.SizeOfImage>0x40000000 || nt.OptionalHeader.NumberOfRvaAndSizes<=IMAGE_DIRECTORY_ENTRY_EXCEPTION) {
        log("resolver: invalid/unsupported PE image");return FALSE;
    }
    uint32_t image_size=nt.OptionalHeader.SizeOfImage;
    uint32_t section_offset=(uint32_t)dos.e_lfanew+sizeof(nt);
    uint32_t section_size=nt.FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);
    if(!inside(section_offset,section_size,image_size) ||
       !read_memory(base+section_offset,sections,section_size))goto done;
    IMAGE_DATA_DIRECTORY table=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    if(!table.Size || table.Size>12*1000000 || table.Size%sizeof(RUNTIME_FUNCTION) ||
       !inside(table.VirtualAddress,table.Size,image_size) ||
       !in_section(sections,nt.FileHeader.NumberOfSections,table.VirtualAddress,table.Size,IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_WRITE))goto done;
    functions=HeapAlloc(GetProcessHeap(),0,table.Size);
    uint32_t max_size=0;
    for(unsigned i=0;i<HF_PROFILE_COUNT;i++)if(family->profiles[i].size>max_size)max_size=family->profiles[i].size;
    if(family->helper && family->helper->size>max_size)max_size=family->helper->size;
    buffer=HeapAlloc(GetProcessHeap(),0,max_size);
    if(!functions || !buffer || !read_memory(base+table.VirtualAddress,functions,table.Size) ||
       BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
    for(unsigned i=0;i<table.Size/sizeof(*functions);i++) {
        RUNTIME_FUNCTION f=functions[i];
        if(f.EndAddress<=f.BeginAddress || !inside(f.BeginAddress,f.EndAddress-f.BeginAddress,image_size))continue;
        for(unsigned j=0;j<HF_PROFILE_COUNT;j++) {
            const HfProfile *p=&family->profiles[j];
            if(p->leaf || f.EndAddress-f.BeginAddress!=p->size)continue;
            if(!in_section(sections,nt.FileHeader.NumberOfSections,f.BeginAddress,p->size,
                           IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_WRITE))continue;
            if(fingerprint(alg,base+f.BeginAddress,p,buffer)) {
                if(matches[j]==MAX_CANDIDATES) { log("resolver: %s excessive matching candidates",p->name);goto done; }
                candidates[j][matches[j]++]=f.BeginAddress;
            }
        }
        if((i&8191)==8191)Sleep(1); /* Yield during the one startup pass. */
    }
    /* Leaf functions have no unwind entry. Search executable sections using the
       checked-in masked anchor, then verify the complete function and callers. */
    for(unsigned si=0;si<nt.FileHeader.NumberOfSections;si++) {
        IMAGE_SECTION_HEADER *sec=&sections[si];
        if((sec->Characteristics&(IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_WRITE)) !=
           (IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_EXECUTE))continue;
        uint32_t size=sec->Misc.VirtualSize;
        if(!inside(sec->VirtualAddress,size,image_size) || size>256*1024*1024)goto done;
        unsigned char *text=HeapAlloc(GetProcessHeap(),0,size);
        if(!text)goto done;
        if(!read_memory(base+sec->VirtualAddress,text,size)){HeapFree(GetProcessHeap(),0,text);goto done;}
        BOOL overflow=FALSE;
        for(unsigned j=0;j<HF_PROFILE_COUNT;j++)if(family->profiles[j].leaf) {
            const HfProfile *p=&family->profiles[j];
            for(uint32_t off=0;p->size<=size && off<=size-p->size;off++) {
                if((text[off]&p->anchor_mask[0])!=p->anchor[0])continue;
                unsigned a=1;for(;a<32 && a<p->size;a++)if((text[off+a]&p->anchor_mask[a])!=p->anchor[a])break;
                if(a<32 && a<p->size)continue;
                if(fingerprint(alg,base+sec->VirtualAddress+off,p,buffer)) {
                    if(matches[j]==MAX_CANDIDATES){overflow=TRUE;break;}
                    candidates[j][matches[j]++]=sec->VirtualAddress+off;
                }
            }
        }
        HeapFree(GetProcessHeap(),0,text);
        if(overflow){log("resolver: excessive leaf candidates");goto done;}
    }
    /* Small compiler wrappers can share normalized instruction sequences. Only
       proven references from/to unique witnesses may disambiguate them. */
    for(unsigned pass=0;pass<HF_PROFILE_COUNT;pass++) {
        BOOL changed=FALSE;
        for(unsigned j=0;j<HF_PROFILE_COUNT;j++)if(matches[j]==1)rvas[j]=candidates[j][0];
        for(unsigned j=0;j<HF_PROFILE_COUNT;j++)if(matches[j]>1) {
            unsigned kept=0;
            for(unsigned c=0;c<matches[j];c++) {
                BOOL valid=TRUE;
                rvas[j]=candidates[j][c];
                for(unsigned e=0;e<family->edge_count;e++) {
                    const HfReference *edge=&family->edges[e];uint32_t target;
                    if(edge->from==j && matches[edge->to]==1 &&
                       (!reference_target(base,image_size,rvas,edge,&target) || target!=rvas[edge->to]))valid=FALSE;
                    if(edge->to==j && matches[edge->from]==1 &&
                       (!reference_target(base,image_size,rvas,edge,&target) || target!=rvas[j]))valid=FALSE;
                }
                if(valid)candidates[j][kept++]=candidates[j][c];
            }
            if(kept!=matches[j])changed=TRUE;
            matches[j]=kept;
            if(kept==1)rvas[j]=candidates[j][0];
        }
        if(!changed)break;
    }
    for(unsigned j=0;j<HF_PROFILE_COUNT;j++)if(matches[j]!=1) {
        log("resolver: %s matches=%u; refusing hooks",family->profiles[j].name,matches[j]);goto done;
    }
    for(unsigned j=0;j<HF_PROFILE_COUNT;j++)rvas[j]=candidates[j][0];
    for(unsigned i=0;i<family->edge_count;i++) {
        uint32_t target;
        if(!reference_target(base,image_size,rvas,&family->edges[i],&target) || target!=rvas[family->edges[i].to]) {
            log("resolver: native call relationship changed; refusing hooks");goto done;
        }
    }
    unsigned seen[HF_GLOBAL_COUNT]={0};
    for(unsigned i=0;i<family->global_count;i++) {
        const HfReference *ref=&family->globals[i];uint32_t target;
        unsigned width=ref->to==HF_OBJECT_COUNT || ref->to==HF_PLAYER_ID?4:8;
        if(ref->to>=HF_GLOBAL_COUNT || !reference_target(base,image_size,rvas,ref,&target) ||
           (seen[ref->to] && globals[ref->to]!=base+target) || (target&(width-1)) ||
           !in_section(sections,nt.FileHeader.NumberOfSections,target,width,
                       IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE,IMAGE_SCN_MEM_EXECUTE)) {
            log("resolver: native global references disagree; refusing hooks");goto done;
        }
        globals[ref->to]=base+target;seen[ref->to]++;
    }
    for(unsigned i=0;i<HF_GLOBAL_COUNT;i++)if(!seen[i])goto done;
    if(globals[HF_OBJECT_COUNT]!=globals[HF_OBJECT_CHUNKS]+20)goto done;
    uint32_t name;
    const HfReference *name_ref=family->helper?&dev_hf_capsule_name_ref:&hf_capsule_name_ref;
    unsigned char literal[sizeof(hf_capsule_name)];
    if(!reference_target(base,image_size,rvas,name_ref,&name) ||
       !in_section(sections,nt.FileHeader.NumberOfSections,name,sizeof(literal),IMAGE_SCN_MEM_READ,IMAGE_SCN_MEM_WRITE|IMAGE_SCN_MEM_EXECUTE) ||
       !read_memory(base+name,literal,sizeof(literal)) || memcmp(literal,hf_capsule_name,sizeof(literal))) {
        log("resolver: CapsuleComponent registration name changed");goto done;
    }
    if(family->helper) {
        uint32_t helper_rva;
        if(!reference_target(base,image_size,rvas,&family->helper_call,&helper_rva) ||
           !in_section(sections,nt.FileHeader.NumberOfSections,helper_rva,family->helper->size,
                       IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_EXECUTE,IMAGE_SCN_MEM_WRITE) ||
           !fingerprint(alg,base+helper_rva,family->helper,buffer)) {
            log("resolver: outlined layout helper changed; refusing hooks");goto done;
        }
        BOOL found=FALSE;
        for(unsigned i=0;i<table.Size/sizeof(*functions);i++)
            if(functions[i].BeginAddress==helper_rva && functions[i].EndAddress-helper_rva==family->helper->size)found=TRUE;
        if(!found)goto done;
    }
    log("resolver: %u unique function/layout fingerprints and native references validated",HF_PROFILE_COUNT);
    ok=TRUE;
done:
    if(alg)BCryptCloseAlgorithmProvider(alg,0);
    if(buffer)HeapFree(GetProcessHeap(),0,buffer);
    if(functions)HeapFree(GetProcessHeap(),0,functions);
    if(!ok) { memset(rvas,0,sizeof(uint32_t)*HF_PROFILE_COUNT);memset(globals,0,sizeof(uintptr_t)*HF_GLOBAL_COUNT); }
    return ok;
}

BOOL hf_resolve(HMODULE module,uint32_t rvas[HF_PROFILE_COUNT],uintptr_t globals[HF_GLOBAL_COUNT],HfLog log) {
    static const LayoutFamily families[] = {{hf_profiles,hf_edges,hf_global_refs,COUNT(hf_edges),COUNT(hf_global_refs),NULL,{0},"inline-layout"},{dev_hf_profiles,dev_hf_edges,dev_hf_global_refs,COUNT(dev_hf_edges),COUNT(dev_hf_global_refs),&dev_helper_profile,{HF_AIM,148,152,0},"outlined-layout"}};
    for(unsigned i=0;i<COUNT(families);i++)
        if(resolve_family(module,rvas,globals,log,&families[i])) {
            log("resolver: coherent %s family",families[i].name);return TRUE;
        }
    return FALSE;
}
