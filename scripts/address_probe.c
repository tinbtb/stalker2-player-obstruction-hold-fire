/* Standalone tests. Maps the workspace copy as DATA; never executes game code. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "address_resolver.h"
#include "address_profile.h"
#include "pe_data_image.h"
static void logger(const char *fmt,...) {
    va_list ap;va_start(ap,fmt);vprintf(fmt,ap);va_end(ap);puts("");
}
static BOOL resolve(unsigned char *image) {
    uint32_t rvas[HF_PROFILE_COUNT];uintptr_t globals[HF_GLOBAL_COUNT];
    return hf_resolve((HMODULE)image,rvas,globals,logger);
}
int wmain(int argc,wchar_t **argv) {
    if(argc!=3)return 2;
    unsigned char *image=load_data(argv[2]);
    if(!image)return 9;
    if(!wcscmp(argv[1],L"--inspect")) {
        uint32_t rvas[HF_PROFILE_COUNT];uintptr_t globals[HF_GLOBAL_COUNT];
        ULONGLONG began=GetTickCount64();
        BOOL ok=hf_resolve((HMODULE)image,rvas,globals,logger);
        printf("Offline development fixture: resolved=%u, time=%llu ms; no code executed or hooks installed\n",ok,GetTickCount64()-began);
        if(ok) {
            for(unsigned i=0;i<HF_PROFILE_COUNT;i++)printf("%s RVA=0x%x\n",hf_profiles[i].name,rvas[i]);
            for(unsigned i=0;i<HF_GLOBAL_COUNT;i++)printf("global %u RVA=0x%llx\n",i,(unsigned long long)(globals[i]-(uintptr_t)image));
        }
        VirtualFree(image,0,MEM_RELEASE);return ok?0:10;
    }
    if(wcscmp(argv[1],L"--image") && wcscmp(argv[1],L"--leaf")){VirtualFree(image,0,MEM_RELEASE);return 2;}
    uint32_t rvas[HF_PROFILE_COUNT];uintptr_t globals[HF_GLOBAL_COUNT];
    ULONGLONG began=GetTickCount64();
    if(!hf_resolve((HMODULE)image,rvas,globals,logger))return 10;
    printf("Original data image resolved in %llu ms\n",GetTickCount64()-began);
    /* Simulate a different executable identity without altering game logic. */
    IMAGE_DOS_HEADER *mapped_dos=(void *)image;
    IMAGE_NT_HEADERS64 *mapped_nt=(void *)(image+mapped_dos->e_lfanew);
    mapped_nt->FileHeader.TimeDateStamp^=0x12345678;
    if(!resolve(image))return 11;
    /* Corrupt a member offset/instruction outside the short anchor. */
    unsigned char saved=image[rvas[HF_SHOT]+100];
    image[rvas[HF_SHOT]+100]^=1;
    if(resolve(image))return 12;
    image[rvas[HF_SHOT]+100]=saved;
    /* Duplicate a matching unwind entry; even identical code must be rejected. */
    IMAGE_DATA_DIRECTORY table=mapped_nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    RUNTIME_FUNCTION *entries=(void *)(image+table.VirtualAddress);
    unsigned count=table.Size/sizeof(*entries),chosen=0;
    for(unsigned i=0;i<count;i++)if(entries[i].BeginAddress==rvas[0])chosen=i;
    RUNTIME_FUNCTION old=entries[0];entries[0]=entries[chosen];
    if(resolve(image))return 13;
    entries[0]=old;
    if(!resolve(image))return 14;
    /* Relocate the transfer function into an extra synthetic executable section.
       Repair its PC-relative operands and callers. No game code is executed. */
    IMAGE_SECTION_HEADER *mapped_sections=IMAGE_FIRST_SECTION(mapped_nt);
    IMAGE_SECTION_HEADER *extra=&mapped_sections[mapped_nt->FileHeader.NumberOfSections];
    if((uintptr_t)(extra+1)-(uintptr_t)image>mapped_nt->OptionalHeader.SizeOfHeaders)return 15;
    uint32_t relocated=mapped_nt->OptionalHeader.SizeOfImage;
    unsigned moved=!wcscmp(argv[1],L"--leaf")?HF_PAWN:HF_READY;
    uint32_t original=rvas[moved];
    const HfProfile *profile=&hf_profiles[moved];
    memcpy(image+relocated,image+original,profile->size);
    int32_t movement=(int32_t)(relocated-original);
    for(unsigned i=0;i<profile->mask_count;i++) {
        if(profile->masks[i].size!=4)return 16;
        int32_t value;memcpy(&value,image+relocated+profile->masks[i].offset,4);
        value-=movement;memcpy(image+relocated+profile->masks[i].offset,&value,4);
    }
    for(unsigned i=0;i<sizeof(hf_edges)/sizeof(*hf_edges);i++)if(hf_edges[i].to==moved) {
        const HfReference *ref=&hf_edges[i];int32_t value;
        memcpy(&value,image+rvas[ref->from]+ref->displacement,4);value+=movement;
        memcpy(image+rvas[ref->from]+ref->displacement,&value,4);
    }
    for(unsigned i=0;i<count;i++)if(entries[i].BeginAddress==original) {
        entries[i].BeginAddress=relocated;entries[i].EndAddress=relocated+profile->size;
    }
    if(profile->leaf)memset(image+original,0xcc,profile->size);
    memset(extra,0,sizeof(*extra));memcpy(extra->Name,".ststest",8);
    extra->VirtualAddress=relocated;extra->Misc.VirtualSize=0x10000;
    extra->Characteristics=IMAGE_SCN_MEM_EXECUTE|IMAGE_SCN_MEM_READ|IMAGE_SCN_CNT_CODE;
    mapped_nt->FileHeader.NumberOfSections++;mapped_nt->OptionalHeader.SizeOfImage+=0x10000;
    if(!hf_resolve((HMODULE)image,rvas,globals,logger) || rvas[moved]!=relocated)return 17;
    /* A masked relative displacement still has to satisfy the call graph. */
    for(unsigned i=0;i<sizeof(hf_edges)/sizeof(*hf_edges);i++)if(hf_edges[i].to==moved) {
        const HfReference *ref=&hf_edges[i];
        image[rvas[ref->from]+ref->displacement]^=1;
        if(resolve(image))return 18;
        image[rvas[ref->from]+ref->displacement]^=1;
        break;
    }
    for(unsigned i=0;i<sizeof(hf_global_refs)/sizeof(*hf_global_refs);i++) {
        const HfReference *ref=&hf_global_refs[i];int32_t value;
        memcpy(&value,image+rvas[ref->from]+ref->displacement,4);value+=16;
        memcpy(image+rvas[ref->from]+ref->displacement,&value,4);
    }
    uintptr_t old_manager=globals[HF_PLAYER_ID];
    if(!hf_resolve((HMODULE)image,rvas,globals,logger) || globals[HF_PLAYER_ID]!=old_manager+16)return 19;
    const HfReference *ref=&hf_global_refs[0];
    image[rvas[ref->from]+ref->displacement]^=8;
    if(resolve(image))return 20;
    /* A class registration name must not be silently remapped. */
    image[rvas[ref->from]+ref->displacement]^=8;
    int32_t name_delta;memcpy(&name_delta,image+rvas[hf_capsule_name_ref.from]+hf_capsule_name_ref.displacement,4);
    uint32_t name=rvas[hf_capsule_name_ref.from]+hf_capsule_name_ref.next+name_delta;
    image[name]^=1;if(resolve(image))return 21;image[name]^=1;
    mapped_dos->e_magic=0;if(resolve(image))return 22;
    puts("Resolver tests passed: identity, function/global relocation, altered code, ambiguity, call graph, manager agreement.");
    VirtualFree(image,0,MEM_RELEASE);return 0;
}
