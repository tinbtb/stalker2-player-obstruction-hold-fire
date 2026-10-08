#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// This is an exact-build proof of concept; no Unreal SDK or Lua ABI imports.
static HMODULE self;
static volatile LONG once;
static unsigned char *cave;
static char log_path[MAX_PATH];
static const unsigned char expected_hook[8]={0xe8,0x9b,0x3d,0xe3,0xff,0x84,0xc0,0x75};
static const unsigned char expected_sha[32]={0x61,0xbc,0x1e,0x03,0x07,0x40,0xce,0xbc,0x30,0xcf,0x1d,0xad,0x0c,0x86,0xcf,0x65,0xe3,0x9e,0x12,0xff,0x05,0x00,0x22,0x58,0x21,0xd6,0x84,0x18,0x1e,0x08,0xd5,0x6b};
static void log_message(const char *s) {
 FILE *f=fopen(log_path,"w"); if(f){fprintf(f,"%s\n",s);fclose(f);}
}
static int fingerprint(void) {
 wchar_t path[MAX_PATH]; DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);
 if(!n || n>=MAX_PATH)return 0;
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(f==INVALID_HANDLE_VALUE)return 0;
 LARGE_INTEGER size; if(!GetFileSizeEx(f,&size)||size.QuadPart!=174798384){CloseHandle(f);return 0;}
 BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_HASH_HANDLE hash=NULL; unsigned char result[32],buf[65536]; DWORD got; int ok=0;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)goto done;
 if(BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)<0)goto done;
 for(;;){if(!ReadFile(f,buf,sizeof(buf),&got,NULL))goto done;if(!got)break;if(BCryptHashData(hash,buf,got,0)<0)goto done;}
 if(BCryptFinishHash(hash,result,sizeof(result),0)<0)goto done;
 ok=!memcmp(result,expected_sha,32);
done:
 if(hash)BCryptDestroyHash(hash);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);return ok;
}
static unsigned char *allocate_near(uintptr_t hook) {
 SYSTEM_INFO si;GetSystemInfo(&si);uintptr_t step=si.dwAllocationGranularity;
 uintptr_t aligned=hook & ~(step-1);
 for(uintptr_t d=step;d<0x70000000;d+=step){
  uintptr_t choices[2]={aligned+d,aligned>d?aligned-d:0};
  for(int k=0;k<2;k++)if(choices[k]){
   unsigned char *p=VirtualAlloc((void*)choices[k],8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
   if(p)return p;
  }
 }
 return NULL;
}
// Prepare thread handles before suspension. Abort if any thread cannot be checked,
// or is currently executing the eight bytes that will change. No late hot reload.
static HANDLE threads[512];static unsigned count;
static void release_threads(unsigned suspended){
 for(unsigned i=0;i<suspended;i++)ResumeThread(threads[i]);
 for(unsigned i=0;i<count;i++)CloseHandle(threads[i]);count=0;
}
static int patch_safely(unsigned char *hook,const unsigned char *patch){
 HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);if(snap==INVALID_HANDLE_VALUE)return 0;
 THREADENTRY32 te;te.dwSize=sizeof(te);int valid=1;
 if(!Thread32First(snap,&te)){CloseHandle(snap);return 0;}
 do{if(te.th32OwnerProcessID==GetCurrentProcessId()&&te.th32ThreadID!=GetCurrentThreadId()){
  if(count==512){valid=0;break;}
  HANDLE t=OpenThread(THREAD_SUSPEND_RESUME|THREAD_GET_CONTEXT|THREAD_QUERY_INFORMATION,FALSE,te.th32ThreadID);
  if(!t){valid=0;break;}threads[count++]=t;
 }}while(Thread32Next(snap,&te));CloseHandle(snap);
 unsigned suspended=0;
 if(!valid){release_threads(0);return 0;}
 for(unsigned i=0;i<count;i++){
  if(SuspendThread(threads[i])==(DWORD)-1){release_threads(suspended);return 0;}suspended++;
  CONTEXT c;memset(&c,0,sizeof(c));c.ContextFlags=CONTEXT_CONTROL;
  if(!GetThreadContext(threads[i],&c)||(c.Rip>=(uintptr_t)hook && c.Rip<(uintptr_t)hook+8)){
   release_threads(suspended);return 0;
  }
 }
 DWORD old;int ok=0;
 if(!memcmp(hook,expected_hook,8)&&VirtualProtect(hook,8,PAGE_EXECUTE_READWRITE,&old)){
  memcpy(hook,patch,8);FlushInstructionCache(GetCurrentProcess(),hook,8);
  DWORD ignored;if(VirtualProtect(hook,8,old,&ignored))ok=1;
  else{memcpy(hook,expected_hook,8);FlushInstructionCache(GetCurrentProcess(),hook,8);VirtualProtect(hook,8,old,&ignored);}
 }
 release_threads(suspended);return ok;
}
#include <math.h>
static uintptr_t game_base, player_actor;
static DWORD snapshot_thread;
static ULONGLONG snapshot_time;
static int player_index, player_serial;
static double radius, halfheight;
static char snapshot_path[MAX_PATH];
static volatile LONG64 checks, blocked, no_player, unsupported;
static int readable(uintptr_t p,size_t n){
 MEMORY_BASIC_INFORMATION m;
 if(!p||!VirtualQuery((void*)p,&m,sizeof(m)))return 0;
 return m.State==MEM_COMMIT && !(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) && p+n>=p && p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
static uintptr_t object_item(int index){
 if(index<0||index>=*(int*)(game_base+0xa0e68e4))return 0;
 uintptr_t chunks=*(uintptr_t*)(game_base+0xa0e68d0);
 if(!readable(chunks+(unsigned)(index>>16)*8,8))return 0;
 uintptr_t chunk=*(uintptr_t*)(chunks+(unsigned)(index>>16)*8);
 uintptr_t item=chunk+(index&65535)*24;
 return readable(item,24)?item:0;
}
// Exact distance from a segment to the capsule's vertical center segment.
static double clamp01(double x){return x<0?0:x>1?1:x;}
static int intersects(const double o[3],const double d[3],double length,const double p[3],double r,double h){
 double a[3]={o[0]-p[0],o[1]-p[1],o[2]-(p[2]-h+r)};
 double v[3]={d[0]*length,d[1]*length,d[2]*length}, z=2*(h-r);
 double A=v[0]*v[0]+v[1]*v[1]+v[2]*v[2], B=v[2]*z,C=z*z;
 double D=v[0]*a[0]+v[1]*a[1]+v[2]*a[2],E=z*a[2];
 if(!isfinite(A)||A<1||!isfinite(C)||C<0)return 0;
 double den=A*C-B*B, t=den>1e-9?clamp01((B*E-C*D)/den):0;
 double u=C>1e-9?(B*t+E)/C:0;
 if(u<0){u=0;t=clamp01(-D/A);}else if(u>1){u=1;t=clamp01((B-D)/A);}
 double x=a[0]+t*v[0],y=a[1]+t*v[1],q=a[2]+t*v[2]-u*z;
 return x*x+y*y+q*q<=r*r;
}
static unsigned char gate(void *controller,void *actor){
 unsigned char ready=((unsigned char(*)(void*))(game_base+0x44041c))(controller);
 if(ready)return ready;
 InterlockedIncrement64(&checks);
 if(snapshot_thread!=GetCurrentThreadId() || GetTickCount64()-snapshot_time>250){InterlockedIncrement64(&no_player);return 0;}
 uintptr_t item=object_item(player_index);
 if(!item||*(uintptr_t*)item!=player_actor||*(int*)(item+16)!=player_serial||(*(unsigned*)(item+8)&0x10200000)){InterlockedIncrement64(&no_player);return 0;}
 uintptr_t npc=(uintptr_t)actor;
 if(npc==player_actor||!readable(npc,0x658)||!readable(player_actor,0x658))return 0;
 uintptr_t ng=*(uintptr_t*)(npc+0x650),pg=*(uintptr_t*)(player_actor+0x650);
 if(!readable(ng,24)||!readable(pg,24)||*(unsigned*)(pg+16)!=*(unsigned*)(game_base+0x9eec140)||*(unsigned*)(ng+16)==*(unsigned*)(game_base+0x9eec140))return 0;
 uintptr_t vt=*(uintptr_t*)npc;
 if(!readable(vt+0xaa0,8)||*(uintptr_t*)(vt+0xaa0)!=game_base+0xa66c56){InterlockedIncrement64(&unsupported);return 0;}
 uintptr_t root=*(uintptr_t*)(player_actor+0x1c0);
 if(!readable(root+0x1f0,24))return 0;
 double p[3];memcpy(p,(void*)(root+0x1f0),24);
 _Alignas(16) double transform[12]={0};
 ((void*(*)(void*,void*,unsigned char))(game_base+0xa66c56))(actor,transform,*((unsigned char*)controller+0x10));
 double x=transform[0],y=transform[1],z=transform[2],w=transform[3];
 double norm=x*x+y*y+z*z+w*w;
 if(!isfinite(norm)||fabs(norm-1)>0.02)return 0;
 double direction[3]={1-2*(y*y+z*z),2*(x*y+w*z),2*(x*z-w*y)};
 double length=*(float*)((unsigned char*)controller+8);
 if(!isfinite(length)||length<1||length>1000000)return 0;
 if(!intersects(transform+4,direction,length,p,radius+15,halfheight+15))return 0;
 unsigned char relation=((unsigned char(*)(void*,void*))(game_base+0x235809b))((void*)ng,(void*)pg);
 if(relation!=2&&relation!=3)return 0;
 InterlockedIncrement64(&blocked);return 1;
}
__declspec(dllexport) int luaopen_PlayerObstructionSnapshot(void *L){
 (void)L;if(!cave)return 0;
 FILE *f=fopen(snapshot_path,"r");unsigned long long address=0;double r=0,h=0;
 if(!f)return 0;int n=fscanf(f,"%llx %lf %lf",&address,&r,&h);fclose(f);
 snapshot_time=0;
 if(n!=3||r<1||r>200||h<r||h>300||!readable((uintptr_t)address,0x658))return 0;
 int index=*(int*)((uintptr_t)address+12);uintptr_t item=object_item(index);
 if(!item||*(uintptr_t*)item!=(uintptr_t)address||(*(unsigned*)(item+8)&0x10200000))return 0;
 player_actor=(uintptr_t)address;player_index=index;player_serial=*(int*)(item+16);radius=r;halfheight=h;
 snapshot_thread=GetCurrentThreadId();snapshot_time=GetTickCount64();return 0;
}
__declspec(dllexport) int luaopen_PlayerObstructionHoldFire(void *L){
 (void)L;if(InterlockedCompareExchange(&once,1,0))return 0;
 GetModuleFileNameA(self,log_path,MAX_PATH);char *slash=strrchr(log_path,'\\');if(!slash)return 0;
 strcpy(slash+1,"PlayerObstructionHoldFire.log");strcpy(snapshot_path,log_path);strcpy(strrchr(snapshot_path,'\\')+1,"player_snapshot.txt");
 if(!fingerprint()){log_message("REFUSED: unsupported executable SHA256/size. No patch installed.");return 0;}
 game_base=(uintptr_t)GetModuleHandleW(NULL);unsigned char *hook=(unsigned char*)(game_base+0x60c67c);
 if(memcmp(hook,expected_hook,8)){log_message("REFUSED: pre-shot instruction mismatch.");return 0;}
 cave=allocate_near((uintptr_t)hook);if(!cave){log_message("REFUSED: no near allocation.");return 0;}
 // Tail-call C with controller RDI and shooter RSI. Original CALL supplies ABI stack alignment.
 unsigned char stub[]={0x48,0x89,0xf9,0x48,0x89,0xf2,0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};
 uintptr_t fn=(uintptr_t)&gate;memcpy(stub+8,&fn,8);memcpy(cave,stub,sizeof(stub));DWORD old;
 if(!VirtualProtect(cave,4096,PAGE_EXECUTE_READ,&old))goto fail;
 FlushInstructionCache(GetCurrentProcess(),cave,sizeof(stub));
 unsigned char patch[8];memcpy(patch,expected_hook,8);patch[0]=0xe8;int32_t delta=(int32_t)((uintptr_t)cave-((uintptr_t)hook+5));memcpy(patch+1,&delta,4);
 HMODULE pinned;if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,(LPCSTR)&gate,&pinned)||!patch_safely(hook,patch))goto fail;
 log_message("ACTIVE: experimental pre-shot obstruction gate. Awaiting player snapshot.");return 0;
fail:
 VirtualFree(cave,0,MEM_RELEASE);cave=NULL;log_message("REFUSED: safe installation failed.");return 0;
}
__declspec(dllexport) int luaopen_PlayerObstructionStatus(void *L){
 (void)L;if(cave){char s[320];snprintf(s,sizeof(s),"ACTIVE: checks=%lld withheld=%lld unavailable_player=%lld unsupported_aim=%lld snapshot_age_ms=%llu",checks,blocked,no_player,unsupported,(unsigned long long)(GetTickCount64()-snapshot_time));log_message(s);}return 0;
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD why,LPVOID reserved){(void)reserved;if(why==DLL_PROCESS_ATTACH){self=h;DisableThreadLibraryCalls(h);}return TRUE;}
#ifdef GEOMETRY_TEST
#define CHECK(want) do { if(intersects(o,d,length,p,r,h)!=(want)){printf("Geometry failure at line %d\n",__LINE__);return 1;}cases++; }while(0)
int main(void){
 int cases=0;double o[]={0,0,0},d[]={1,0,0},p[]={500,0,0},length=1000,r=40,h=90;
 CHECK(1);p[1]=100;CHECK(0);p[1]=40;CHECK(1);p[1]=40.001;CHECK(0);
 p[0]=-100;p[1]=0;CHECK(0);p[0]=1100;CHECK(0);p[0]=1039;CHECK(1);
 p[0]=500;p[2]=89;CHECK(1);p[2]=91;CHECK(0);p[2]=0;
 p[0]=0;CHECK(1);p[0]=500;length=400;CHECK(0);
 length=1000;d[0]=0;d[2]=1;p[0]=0;p[2]=500;CHECK(1);
 p[0]=41;CHECK(0);p[0]=0;p[2]=-500;CHECK(0);
 p[2]=500;d[2]=-1;CHECK(0);d[2]=1;length=NAN;CHECK(0);
 printf("%d geometry scenarios passed\n",cases);return 0;
}
#endif
