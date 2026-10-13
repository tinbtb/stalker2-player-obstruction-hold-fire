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
static int worker_ready,hold_fire_enabled=1,metrics_enabled,frames_requested,relationships_requested;
#define COUNT(counter) do{if(metrics_enabled)InterlockedIncrement64(&(counter));}while(0)
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
static int patch_safely(unsigned char *hook,const unsigned char *patch,const unsigned char *expected){
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
 if(!memcmp(hook,expected,8)&&VirtualProtect(hook,8,PAGE_EXECUTE_READWRITE,&old)){
  memcpy(hook,patch,8);FlushInstructionCache(GetCurrentProcess(),hook,8);
  DWORD ignored;if(VirtualProtect(hook,8,old,&ignored))ok=1;
  else{memcpy(hook,expected,8);FlushInstructionCache(GetCurrentProcess(),hook,8);VirtualProtect(hook,8,old,&ignored);}
 }
 release_threads(suspended);return ok;
}
#include <math.h>
static uintptr_t game_base, player_actor;
static volatile LONG gate_thread;

static double player_position[3];
static double radius, halfheight;
static volatile LONG64 checks, blocked, no_player, unsupported;
static volatile LONG64 snapshot_attempts, snapshot_successes;
#define SAFETY_MARGIN_CM 40.0
// SEH covers only our byte loads, never game engine calls. No mapped-region cache.
static volatile LONG64 guarded_reads, read_faults, guard_rearms;
static int read_exception(EXCEPTION_POINTERS *e,uintptr_t source,size_t size){
 EXCEPTION_RECORD *r=e->ExceptionRecord;
 if(r->NumberParameters<2 || r->ExceptionInformation[0]!=0 ||
    r->ExceptionInformation[1]<source || r->ExceptionInformation[1]-source>=size)
  return EXCEPTION_CONTINUE_SEARCH;
 if(r->ExceptionCode==EXCEPTION_GUARD_PAGE){
  // A guard access clears PAGE_GUARD. Restore it before abandoning our read.
  MEMORY_BASIC_INFORMATION m;DWORD old;
  uintptr_t page=r->ExceptionInformation[1]&~(uintptr_t)4095;
  if(!VirtualQuery((void*)page,&m,sizeof(m)) ||
     !VirtualProtect((void*)page,4096,m.Protect|PAGE_GUARD,&old))return EXCEPTION_CONTINUE_SEARCH;
  COUNT(guard_rearms);
 }else if(r->ExceptionCode!=EXCEPTION_ACCESS_VIOLATION && r->ExceptionCode!=EXCEPTION_IN_PAGE_ERROR)
  return EXCEPTION_CONTINUE_SEARCH;
 return EXCEPTION_EXECUTE_HANDLER;
}
// Keep the faulting loads in a non-inlined callee so Clang emits an SEH invoke.
__declspec(noinline) static void copy_bytes(void *destination,uintptr_t source,size_t size){
 volatile const unsigned char *p=(volatile const unsigned char*)source;
 unsigned char *q=(unsigned char*)destination;
 for(size_t i=0;i<size;i++)q[i]=p[i];
}
__declspec(noinline) static int safe_copy(void *destination,uintptr_t source,size_t size){
 COUNT(guarded_reads);
 if(!source || size>UINTPTR_MAX-source){COUNT(read_faults);return 0;}
 __try {
  copy_bytes(destination,source,size);
 } __except(read_exception(GetExceptionInformation(),source,size)) {
  COUNT(read_faults);return 0;
 }
 return 1;
}
#define COPY_AT(dst, address) safe_copy(&(dst),(uintptr_t)(address),sizeof(dst))
static uintptr_t object_item(int index){
 int count;uintptr_t chunks,chunk;
 if(!COPY_AT(count,game_base+0xa0e68e4)||index<0||index>=count)return 0;
 if(!COPY_AT(chunks,game_base+0xa0e68d0)||!chunks)return 0;
 if(!COPY_AT(chunk,chunks+(unsigned)(index>>16)*8)||!chunk)return 0;
 uintptr_t offset=(uintptr_t)(index&65535)*24;
 return chunk<=UINTPTR_MAX-offset?chunk+offset:0;
}
// Exact distance from a segment to the capsule's vertical center segment.
static double clamp01(double x){return x<0?0:x>1?1:x;}
static int intersects(const double o[3],const double d[3],double length,const double p[3],double r,double h){
 double a[3]={o[0]-p[0],o[1]-p[1],o[2]-(p[2]-h+r)};
 double v[3]={d[0]*length,d[1]*length,d[2]*length}, z=2*(h-r);
 double A=v[0]*v[0]+v[1]*v[1]+v[2]*v[2], B=v[2]*z,C=z*z;
 double D=v[0]*a[0]+v[1]*a[1]+v[2]*a[2],E=z*a[2];
 if(!isfinite(A)||A<1||!isfinite(C)||C<0)return 0;
 // A capsule with h == r is a sphere: minimize against the point, not t=0.
 if(C<=1e-9){double t=clamp01(-D/A);double x=a[0]+t*v[0],y=a[1]+t*v[1],q=a[2]+t*v[2];return x*x+y*y+q*q<=r*r;}
 double den=A*C-B*B, t=den>1e-9?clamp01((B*E-C*D)/den):0;
 double u=C>1e-9?(B*t+E)/C:0;
 if(u<0){u=0;t=clamp01(-D/A);}else if(u>1){u=1;t=clamp01((B-D)/A);}
 double x=a[0]+t*v[0],y=a[1]+t*v[1],q=a[2]+t*v[2]-u*z;
 return x*x+y*y+q*q<=r*r;
}

// Diagnostic QPC durations include instrumentation overhead; cumulative counters.
typedef struct { volatile LONG64 count, total, maximum; } Metric;
static Metric metrics[10];
static LARGE_INTEGER perf_frequency;

static char metrics_path[MAX_PATH];
static LONG64 stamp(void){ LARGE_INTEGER t;QueryPerformanceCounter(&t);return t.QuadPart; }
static LONG64 metric_stamp(void){return metrics_enabled?stamp():0;}
static void record_metric(int i,LONG64 start){
 if(!metrics_enabled)return;
 LONG64 dt=stamp()-start,old;InterlockedIncrement64(&metrics[i].count);
 InterlockedAdd64(&metrics[i].total,dt);
 do{old=metrics[i].maximum;if(dt<=old)break;}while(InterlockedCompareExchange64(&metrics[i].maximum,dt,old)!=old);
}
// UObject identity is checked on the firing thread before each use. No persistent pawn cache.
static int valid_object(uintptr_t obj,size_t size){
 int index;uintptr_t pointer;unsigned flags;
 if(!obj || size>UINTPTR_MAX-obj || !COPY_AT(index,obj+12))return 0;
 uintptr_t item=object_item(index);
 return item && COPY_AT(pointer,item) && pointer==obj &&
  COPY_AT(flags,item+8) && !(flags&0x10200000);
}
static uintptr_t (*get_controller)(void*,int);
static uintptr_t (*get_pawn)(void*);
static unsigned char (*get_ready)(void*);
// The relationship routine takes identity keys, not gameplay object addresses.
static uintptr_t (*get_relation_identity)(void*);
static unsigned char (*get_relation)(uintptr_t,uintptr_t);
static volatile LONG64 relation_values[256],withheld_values[256];
static void *(*get_aim)(void*,void*,unsigned char);
static int valid_transform(const double t[12]){
 double norm=t[0]*t[0]+t[1]*t[1]+t[2]*t[2]+t[3]*t[3];
 if(!isfinite(norm)||fabs(norm-1)>0.02)return 0;
 for(int i=4;i<7;i++)if(!isfinite(t[i]))return 0;
 return 1;
}
static void forward(const double t[12],double d[3]){
 double x=t[0],y=t[1],z=t[2],w=t[3];
 d[0]=1-2*(y*y+z*z);d[1]=2*(x*y+w*z);d[2]=2*(x*z-w*y);
}
// Bound the segment produced by the game's previous/current transform blend.
// Every blended origin lies between the two origins; the rotation is bounded
// by the full angular separation of their quaternions.
static int predicted_intersection(const double previous[12],const double current[12],
 const double player[3],double r,double h){
 if(!valid_transform(previous)||!valid_transform(current))return 0;
 double origin_delta=0,player_distance=0;
 for(int i=0;i<3;i++){
  double a=previous[4+i]-current[4+i],c=player[i]-current[4+i];
  origin_delta+=a*a;player_distance+=c*c;
 }
 origin_delta=sqrt(origin_delta);player_distance=sqrt(player_distance);
 if(!isfinite(origin_delta)||!isfinite(player_distance)||player_distance>1000000)return 0;
 double dot=0,np=0,nc=0;
 for(int i=0;i<4;i++){dot+=previous[i]*current[i];np+=previous[i]*previous[i];nc+=current[i]*current[i];}
 if(!isfinite(dot))return 0;
 // Quaternion endpoints can differ in sign while representing one rotation.
 dot=fabs(dot)/sqrt(np*nc);if(dot>1)dot=1;
 double angular_sine=sqrt(1-dot*dot);
 double travel_to_player=player_distance+origin_delta+h+r;
 double expansion=origin_delta+2*travel_to_player*angular_sine;
 // The old controller range can be stale before game shot preparation.
 // Limit at the player's far side; this may hold fire behind the intended target.
 double length=player_distance+origin_delta+h+r+SAFETY_MARGIN_CM;
 if(!isfinite(expansion)||!isfinite(length))return 0;
 double d[3];forward(current,d);
 return intersects(current+4,d,length,player,r+SAFETY_MARGIN_CM+expansion,h+SAFETY_MARGIN_CM+expansion);
}
typedef struct { uintptr_t pawn, gameplay; unsigned id; } PlayerIdentity;
static int native_identity(void *actor,PlayerIdentity *out){
 COUNT(snapshot_attempts);
 uintptr_t npc=(uintptr_t)actor,vt,world_getter;
 if(!valid_object(npc,0x658)||!COPY_AT(vt,npc)||!vt||
    !COPY_AT(world_getter,vt+0x198)||!world_getter)return 0;
 LONG64 t=metric_stamp();uintptr_t pc=get_controller(actor,0);record_metric(7,t);
 if(!valid_object(pc,0x6e0))return 0;
 t=metric_stamp();uintptr_t pawn=get_pawn((void*)pc);record_metric(8,t);
 if(!valid_object(pawn,0x658))return 0;
 uintptr_t pg;unsigned player_id,pg_id;
 if(!COPY_AT(pg,pawn+0x650)||!pg||!COPY_AT(pg_id,pg+16)||
    !COPY_AT(player_id,game_base+0x9eec140)||pg_id!=player_id)return 0;
 out->pawn=pawn;out->gameplay=pg;out->id=player_id;return 1;
}
static int native_capsule(uintptr_t pawn){
 uintptr_t cls,capsule,actual_cls;
 if(!COPY_AT(cls,game_base+0xa234940)||!cls)return 0;
 if(!COPY_AT(capsule,pawn+0x340))return 0;
 if(!valid_object(capsule,0x510)||!COPY_AT(actual_cls,capsule+0x10)||actual_cls!=cls){
  if(!COPY_AT(capsule,pawn+0x1c0)||!valid_object(capsule,0x510)||
     !COPY_AT(actual_cls,capsule+0x10)||actual_cls!=cls)return 0;
 }
 double scale[3],position[3];float dimensions[2];
 if(!COPY_AT(scale,capsule+0x210)||!COPY_AT(dimensions,capsule+0x508)||
    !COPY_AT(position,capsule+0x1f0))return 0;
 double r=(float)(scale[0]<scale[1]?scale[0]:scale[1])*dimensions[1];
 double h=(float)scale[2]*dimensions[0];
 if(!isfinite(r)||!isfinite(h)||r<1||r>200||h<r||h>300)return 0;
 for(int i=0;i<3;i++)if(!isfinite(position[i]))return 0;
 memcpy(player_position,position,sizeof(position));player_actor=pawn;radius=r;halfheight=h;
 COUNT(snapshot_successes);return 1;
}
#ifdef NATIVE_TEST
static int native_player(void *actor){
 PlayerIdentity player;
 return native_identity(actor,&player)&&native_capsule(player.pawn);
}
#endif
static unsigned char gate_inner(void *controller,void *actor){
 unsigned char ready=get_ready(controller);
 if(ready)return ready;
 COUNT(checks);
 LONG tid=(LONG)GetCurrentThreadId();
 LONG owner=InterlockedCompareExchange(&gate_thread,tid,0);
 if(owner && owner!=tid){COUNT(no_player);return 0;}
 PlayerIdentity player;
 LONG64 lookup_start=metric_stamp();int found=native_identity(actor,&player);record_metric(0,lookup_start);
 if(!found){COUNT(no_player);return 0;}
 uintptr_t npc=(uintptr_t)actor,ng,vt,aim;unsigned ng_id;
 if(npc==player.pawn)return 0;
 if(!COPY_AT(ng,npc+0x650)||!ng||!COPY_AT(ng_id,ng+16)||ng_id==player.id)return 0;
 if(!COPY_AT(vt,npc)||!vt||!COPY_AT(aim,vt+0xaa0)||aim!=game_base+0xa66c56){
  COUNT(unsupported);return 0;
 }
 // Relationships are fresh per shot. Hostiles never need capsule or aim work.
 LONG64 relation_start=metric_stamp();
 uintptr_t shooter_key=get_relation_identity((void*)ng);
 uintptr_t player_key=get_relation_identity((void*)player.gameplay);
 unsigned char relation=get_relation(shooter_key,player_key);
 record_metric(2,relation_start);
 if(relationships_requested)InterlockedIncrement64(&relation_values[relation]);
 if(relation!=2&&relation!=3)return 0;
 LONG64 capsule_start=metric_stamp();found=native_capsule(player.pawn);record_metric(3,capsule_start);
 if(!found){COUNT(no_player);return 0;}
 double p[3];memcpy(p,player_position,24);
 _Alignas(16) double previous[12],transform[12]={0};
 if(!COPY_AT(previous,(uintptr_t)controller+0x20))return 0;
 LONG64 aim_start=metric_stamp();
 // Shot preparation may replace the mode after this hook. It writes 0/1;
 // preserve an existing non-boolean mode if preparation takes another path.
 get_aim(actor,transform,0);
 int hit=predicted_intersection(previous,transform,p,radius,halfheight);
 if(!hit){get_aim(actor,transform,1);hit=predicted_intersection(previous,transform,p,radius,halfheight);}
 unsigned char existing_mode;
 if(!hit&&COPY_AT(existing_mode,(uintptr_t)controller+0x10)&&existing_mode>1){
  get_aim(actor,transform,existing_mode);
  hit=predicted_intersection(previous,transform,p,radius,halfheight);
 }
 record_metric(1,aim_start);
 if(!hit)return 0;
 if(relationships_requested)InterlockedIncrement64(&withheld_values[relation]);
 COUNT(blocked);return 1;
}
static unsigned char gate(void *controller,void *actor){ LONG64 t=metric_stamp();unsigned char result=gate_inner(controller,actor);record_metric(5,t);return result;}
#ifdef FRAME_CAPTURE
#include "frame_capture.h"
#endif
static int initialize(void *L){
 (void)L;if(InterlockedCompareExchange(&once,1,0))return 0;
 DWORD path_len=GetModuleFileNameA(self,log_path,MAX_PATH);
 if(!path_len||path_len>=MAX_PATH)return 0;
 char *slash=strrchr(log_path,'\\');if(!slash)return 0;
 size_t prefix=(size_t)(slash+1-log_path);
 // The INI has the longest sibling name. Reject paths that cannot fit.
 if(prefix+sizeof("PlayerObstructionHoldFire.ini")>MAX_PATH)return 0;
 strcpy(slash+1,"PlayerObstructionHoldFire.log");
 QueryPerformanceFrequency(&perf_frequency);strcpy(metrics_path,log_path);strcpy(strrchr(metrics_path,'\\')+1,"native_metrics.csv");
 char ini[MAX_PATH];strcpy(ini,log_path);strcpy(strrchr(ini,'\\')+1,"PlayerObstructionHoldFire.ini");
 hold_fire_enabled=GetPrivateProfileIntA("Debug","EnableHoldFire",1,ini)!=0;
 metrics_enabled=GetPrivateProfileIntA("Debug","EnableMetrics",0,ini)!=0;
 relationships_requested=GetPrivateProfileIntA("Debug","CaptureRelationships",0,ini)==1;
 if(relationships_requested)metrics_enabled=1;
#ifdef FRAME_CAPTURE
 frames_requested=GetPrivateProfileIntA("Debug","CaptureEngineFrames",0,ini)==1;
#endif
 if(frames_requested)metrics_enabled=1;
 if(metrics_enabled){FILE *mf=fopen(metrics_path,"w");if(mf){fprintf(mf,"system_uptime_ms,metric,count,total_us,max_us,checks,withheld,snapshot_attempts,snapshot_successes,unavailable_player,unsupported_aim,reserved\n");fclose(mf);}}
 if(!fingerprint()){log_message("REFUSED: unsupported executable SHA256/size. No patch installed.");return 0;}
 game_base=(uintptr_t)GetModuleHandleW(NULL);
 if(!hold_fire_enabled){worker_ready=1;log_message("CONTROL: v0.4.4-prediction-test. Hold-fire hook NOT installed; vanilla firing behavior.");return 0;}
 get_ready=(void*)(game_base+0x44041c);get_relation_identity=(void*)(game_base+0x4d61a4);get_relation=(void*)(game_base+0x235809b);get_aim=(void*)(game_base+0xa66c56);
 get_controller=(void*)(game_base+0x2432506);get_pawn=(void*)(game_base+0x2f8c936);unsigned char *hook=(unsigned char*)(game_base+0x60c67c);
 if(memcmp(hook,expected_hook,8)){log_message("REFUSED: pre-shot instruction mismatch.");return 0;}
 cave=allocate_near((uintptr_t)hook);if(!cave){log_message("REFUSED: no near allocation.");return 0;}
 // Tail-call C with controller RDI and shooter RSI. Original CALL supplies ABI stack alignment.
 unsigned char stub[]={0x48,0x89,0xf9,0x48,0x89,0xf2,0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};
 uintptr_t fn=(uintptr_t)&gate;memcpy(stub+8,&fn,8);memcpy(cave,stub,sizeof(stub));DWORD old;
 if(!VirtualProtect(cave,4096,PAGE_EXECUTE_READ,&old))goto fail;
 FlushInstructionCache(GetCurrentProcess(),cave,sizeof(stub));
 unsigned char patch[8];memcpy(patch,expected_hook,8);patch[0]=0xe8;int32_t delta=(int32_t)((uintptr_t)cave-((uintptr_t)hook+5));memcpy(patch+1,&delta,4);
 HMODULE pinned;if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,(LPCSTR)&gate,&pinned)||!patch_safely(hook,patch,expected_hook))goto fail;
 worker_ready=1;log_message("ACTIVE: experimental pre-shot obstruction gate. v0.4.4-prediction-test conservative shot prediction; guarded memory reads enabled.");return 0;
fail:
 VirtualFree(cave,0,MEM_RELEASE);cave=NULL;log_message("REFUSED: safe installation failed.");return 0;
}
static void write_status(void){
 static const char *names[]={"native_player_identity","aim_transform","relation","player_capsule","unused4","native_gate_including_original","guarded_read_fault","get_player_controller","get_pawn","guarded_read"};
 FILE *f=fopen(metrics_path,"a");
 InterlockedExchange64(&metrics[9].count,guarded_reads);
 InterlockedExchange64(&metrics[6].count,read_faults);
 if(f){for(int i=0;i<10;i++){if(i<4||i>=5)fprintf(f,"%llu,%s,%lld,%.3f,%.3f,%lld,%lld,%lld,%lld,%lld,%lld,0\n",
 (unsigned long long)GetTickCount64(),names[i],InterlockedCompareExchange64(&metrics[i].count,0,0),
 (double)InterlockedCompareExchange64(&metrics[i].total,0,0)*1e6/perf_frequency.QuadPart,
 (double)InterlockedCompareExchange64(&metrics[i].maximum,0,0)*1e6/perf_frequency.QuadPart,
 InterlockedCompareExchange64(&checks,0,0),InterlockedCompareExchange64(&blocked,0,0),
 InterlockedCompareExchange64(&snapshot_attempts,0,0),InterlockedCompareExchange64(&snapshot_successes,0,0),
 InterlockedCompareExchange64(&no_player,0,0),InterlockedCompareExchange64(&unsupported,0,0));}
 if(relationships_requested)for(int v=0;v<256;v++){
  LONG64 n=InterlockedCompareExchange64(&relation_values[v],0,0);
  LONG64 b=InterlockedCompareExchange64(&withheld_values[v],0,0);
  if(n)fprintf(f,"%llu,relationship_value_%d,%lld,0,0,0,0,0,0,0,0,0\n",(unsigned long long)GetTickCount64(),v,n);
  if(b)fprintf(f,"%llu,withheld_relationship_%d,%lld,0,0,0,0,0,0,0,0,0\n",(unsigned long long)GetTickCount64(),v,b);
 }
 fclose(f);}
}
static DWORD WINAPI worker(void *arg){
 (void)arg;initialize(NULL);
#ifdef FRAME_CAPTURE
 if(worker_ready && frames_requested)frame_init();
#endif
 if(worker_ready && (metrics_enabled || frames_requested)){ULONGLONG next=GetTickCount64()+5000;
  for(;;){
#ifdef FRAME_CAPTURE
   Sleep(1000);frame_flush();
#else
   Sleep(5000);
#endif
   ULONGLONG now=GetTickCount64();if(now>=next){if(metrics_enabled)write_status();next=now+5000;}
  }
 }
 return 0;
}
// ASI loader entry point. DllMain only schedules the worker; engine access occurs in the shot hook.
__declspec(dllexport) void InitializeASI(void){
 static volatile LONG started;
 if(InterlockedCompareExchange(&started,1,0))return;
 HMODULE pinned;
 if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,(LPCSTR)&InitializeASI,&pinned))return;
 HANDLE t=CreateThread(NULL,0,worker,NULL,0,NULL);if(t)CloseHandle(t);
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD why,LPVOID reserved){
 (void)reserved;if(why==DLL_PROCESS_ATTACH){self=h;DisableThreadLibraryCalls(h);InitializeASI();}return TRUE;
}
#ifdef GEOMETRY_TEST
#define CHECK(want) do { if(intersects(o,d,length,p,r,h)!=(want)){printf("Geometry failure at line %d\n",__LINE__);return 1;}cases++; }while(0)
int main(void){
 int cases=0;double o[]={0,0,0},d[]={1,0,0},p[]={500,0,0},length=1000,r=40,h=90;
 CHECK(1);p[1]=100;CHECK(0);p[1]=40;CHECK(1);p[1]=40.001;CHECK(0);
 p[1]=0;r=h=80;CHECK(1);p[1]=81;CHECK(0);p[1]=0;r=40;h=90;
 p[0]=-100;p[1]=0;CHECK(0);p[0]=1100;CHECK(0);p[0]=1039;CHECK(1);
 p[0]=500;p[2]=89;CHECK(1);p[2]=91;CHECK(0);p[2]=0;
 p[0]=0;CHECK(1);p[0]=500;length=400;CHECK(0);
 length=1000;d[0]=0;d[2]=1;p[0]=0;p[2]=500;CHECK(1);
 p[0]=41;CHECK(0);p[0]=0;p[2]=-500;CHECK(0);
 p[2]=500;d[2]=-1;CHECK(0);d[2]=1;length=NAN;CHECK(0);
 printf("%d geometry scenarios passed\n",cases);return 0;
}
#endif


#ifdef NATIVE_TEST
static unsigned char npc_test[0x700],pc_test[0x700],pawn_test[0x700],cap_test[0x700],gameplay_test[32],vt_test[0xab0];
static unsigned char items_test[24*4];static uintptr_t chunks_test[1];static int missing;
static uintptr_t mock_controller(void *ctx,int i){return ctx==npc_test && i==0 && !missing?(uintptr_t)pc_test:0;}
static uintptr_t mock_pawn(void *pc){return pc==pc_test?(uintptr_t)pawn_test:0;}
static unsigned char npc_gameplay_test[32],shot_test[0x100];
static int aim_calls,relation_calls,mode1_override,mode2_override;static unsigned char test_relation=2,test_ready;
static double aim_y;
static unsigned char mock_ready(void *c){(void)c;return test_ready;}
static uintptr_t mock_relation_identity(void *obj){
 return obj==npc_gameplay_test?0x1111222233334444ULL:obj==gameplay_test?0x5555666677778888ULL:0;
}
static unsigned char mock_relation(uintptr_t a,uintptr_t b){
 relation_calls++;return a==0x1111222233334444ULL && b==0x5555666677778888ULL?test_relation:0;
}
static void *mock_aim(void *a,void *out,unsigned char mode){
 (void)a;aim_calls++;double *t=out;t[3]=1;t[4]=-200;
 t[5]=((mode==1&&mode1_override)||(mode==2&&mode2_override))?0:aim_y;return out;
}
#define EXPECT(x) do{if(!(x)){printf("Native test failure line %d\n",__LINE__);return 1;}cases++;}while(0)
int main(void){
 int cases=0;metrics_enabled=1;QueryPerformanceFrequency(&perf_frequency);game_base=(uintptr_t)VirtualAlloc(NULL,0xa240000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!game_base)return 2;
 *(uintptr_t*)(game_base+0xa0e68d0)=(uintptr_t)chunks_test;*(int*)(game_base+0xa0e68e4)=4;chunks_test[0]=(uintptr_t)items_test;
 unsigned char *objs[]={npc_test,pc_test,pawn_test,cap_test};
 for(int i=0;i<4;i++){*(int*)(objs[i]+12)=i;*(uintptr_t*)(items_test+i*24)=(uintptr_t)objs[i];}
 *(uintptr_t*)npc_test=(uintptr_t)vt_test;*(uintptr_t*)(vt_test+0x198)=1;
 *(uintptr_t*)(pawn_test+0x650)=(uintptr_t)gameplay_test;*(unsigned*)(gameplay_test+16)=123;*(unsigned*)(game_base+0x9eec140)=123;
 *(uintptr_t*)(pawn_test+0x340)=(uintptr_t)cap_test;*(uintptr_t*)(cap_test+0x10)=456;*(uintptr_t*)(game_base+0xa234940)=456;
 *(double*)(cap_test+0x210)=1;*(double*)(cap_test+0x218)=1;*(double*)(cap_test+0x220)=1;
 *(float*)(cap_test+0x50c)=40;*(float*)(cap_test+0x508)=90;
 get_controller=mock_controller;get_pawn=mock_pawn;
 EXPECT(native_player(npc_test)&&radius==40&&halfheight==90);
 *(float*)(cap_test+0x508)=55;EXPECT(native_player(npc_test)&&halfheight==55);
 *(double*)(cap_test+0x210)=0.5;EXPECT(native_player(npc_test)&&radius==20);
 missing=1;EXPECT(!native_player(npc_test));missing=0;
 *(unsigned*)(items_test+3*24+8)=0x10000000;EXPECT(!native_player(npc_test));*(unsigned*)(items_test+3*24+8)=0;
 *(uintptr_t*)(cap_test+0x10)=999;EXPECT(!native_player(npc_test));*(uintptr_t*)(cap_test+0x10)=456;
 *(double*)(cap_test+0x220)=NAN;EXPECT(!native_player(npc_test));*(double*)(cap_test+0x220)=1;
 *(uintptr_t*)(pawn_test+0x340)=0;*(uintptr_t*)(pawn_test+0x1c0)=(uintptr_t)cap_test;EXPECT(native_player(npc_test));
 *(uintptr_t*)(items_test+2*24)=0;EXPECT(!native_player(npc_test));
 *(uintptr_t*)(items_test+2*24)=(uintptr_t)pawn_test;
 // Fault handling must work with optimized code and real inaccessible mappings.
 unsigned char *page=VirtualAlloc(NULL,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 unsigned char data[32];DWORD old;
 EXPECT(page!=NULL);page[0]=123;EXPECT(safe_copy(data,(uintptr_t)page,32)&&data[0]==123);
 EXPECT(!safe_copy(data,UINTPTR_MAX-4,16));EXPECT(!safe_copy(data,0,1));
 EXPECT(VirtualProtect(page+4096,4096,PAGE_NOACCESS,&old));
 EXPECT(!safe_copy(data,(uintptr_t)page+4090,16));
 EXPECT(VirtualProtect(page,4096,PAGE_READWRITE|PAGE_GUARD,&old));
 EXPECT(!safe_copy(data,(uintptr_t)page,1));
 MEMORY_BASIC_INFORMATION m;EXPECT(VirtualQuery(page,&m,sizeof(m))&&(m.Protect&PAGE_GUARD));
 EXPECT(VirtualProtect(page,4096,PAGE_READWRITE,&old));
 EXPECT(safe_copy(data,(uintptr_t)page,1)&&data[0]==123);
 EXPECT(VirtualFree(page,0,MEM_RELEASE));EXPECT(!safe_copy(data,(uintptr_t)page,1));
 LONG64 faults_before=read_faults,t=stamp();const int loops=20000;
 for(int i=0;i<loops;i++)if(!native_player(npc_test))return 3;
 EXPECT(read_faults==faults_before);
 printf("lookup_benchmark: %.3f us/call (%d calls); no faults on valid lookups\n",
 (stamp()-t)*1e6/perf_frequency.QuadPart/loops,loops);
 LONG64 saved_reads=guarded_reads,saved_success=snapshot_successes,saved_metric=metrics[7].count;
 metrics_enabled=0;
 EXPECT(metric_stamp()==0);
 EXPECT(native_player(npc_test));
 EXPECT(guarded_reads==saved_reads && snapshot_successes==saved_success && metrics[7].count==saved_metric);
 metrics_enabled=1;
 // Exercise the real gate: invalid capsules cannot affect hostile fast exits.
 get_ready=mock_ready;get_relation_identity=mock_relation_identity;get_relation=mock_relation;get_aim=mock_aim;
 *(uintptr_t*)(npc_test+0x650)=(uintptr_t)npc_gameplay_test;
 *(unsigned*)(npc_gameplay_test+16)=456;
 *(uintptr_t*)(vt_test+0xaa0)=game_base+0xa66c56;
 *(float*)(shot_test+8)=1000;
 *(double*)(shot_test+0x20+3*8)=1;
 *(double*)(shot_test+0x20+4*8)=-200;
 test_ready=7;EXPECT(gate_inner(shot_test,npc_test)==7 && relation_calls==0 && aim_calls==0);test_ready=0;
 *(uintptr_t*)(cap_test+0x10)=999;
 LONG64 capsule_count=metrics[3].count;
 for(int rel=0;rel<5;rel++)if(rel!=2 && rel!=3){test_relation=(unsigned char)rel;EXPECT(gate_inner(shot_test,npc_test)==0 && aim_calls==0 && metrics[3].count==capsule_count);}
 test_relation=2;EXPECT(gate_inner(shot_test,npc_test)==0 && aim_calls==0);
 *(uintptr_t*)(cap_test+0x10)=456;
 EXPECT(gate_inner(shot_test,npc_test)==1 && aim_calls==1);
 test_relation=3;EXPECT(gate_inner(shot_test,npc_test)==1 && aim_calls==2);
 aim_y=1000;*(double*)(shot_test+0x20+5*8)=1000;
 EXPECT(gate_inner(shot_test,npc_test)==0 && aim_calls==4);
 aim_y=0;*(double*)(shot_test+0x20+5*8)=0;
 test_relation=0;EXPECT(gate_inner(shot_test,npc_test)==0 && aim_calls==4);
 test_relation=2;EXPECT(gate_inner(shot_test,npc_test)==1 && aim_calls==5);
 int previous_relations=relation_calls;*(uintptr_t*)(vt_test+0xaa0)=0;
 EXPECT(gate_inner(shot_test,npc_test)==0 && relation_calls==previous_relations && aim_calls==5);
 *(uintptr_t*)(vt_test+0xaa0)=game_base+0xa66c56;
 missing=1;EXPECT(gate_inner(shot_test,npc_test)==0 && relation_calls==previous_relations);missing=0;
 // The actual shot may blend a clear current line with the prior transform.
 aim_y=200;*(double*)(shot_test+0x20+5*8)=-200;
 EXPECT(gate_inner(shot_test,npc_test)==1);
 aim_y=1000;*(double*)(shot_test+0x20+5*8)=1000;mode1_override=1;
 int prior_aim_calls=aim_calls;
 EXPECT(gate_inner(shot_test,npc_test)==1 && aim_calls==prior_aim_calls+2);
 mode1_override=0;aim_y=0;*(double*)(shot_test+0x20+5*8)=0;
 aim_y=1000;*(double*)(shot_test+0x20+5*8)=1000;mode2_override=1;shot_test[0x10]=2;
 prior_aim_calls=aim_calls;
 EXPECT(gate_inner(shot_test,npc_test)==1 && aim_calls==prior_aim_calls+3);
 mode2_override=0;shot_test[0x10]=0;aim_y=0;*(double*)(shot_test+0x20+5*8)=0;
 // Identity keys must reach the engine relation routine; pointer arguments fail these checks.
 relationships_requested=1;test_relation=0;
 EXPECT(gate_inner(shot_test,npc_test)==0 && relation_values[0]==1 && withheld_values[0]==0);
 test_relation=2;EXPECT(gate_inner(shot_test,npc_test)==1 && relation_values[2]==1 && withheld_values[2]==1);
 test_relation=3;EXPECT(gate_inner(shot_test,npc_test)==1 && relation_values[3]==1 && withheld_values[3]==1);
 test_relation=255;EXPECT(gate_inner(shot_test,npc_test)==0 && relation_values[255]==1);
 relationships_requested=0;EXPECT(gate_inner(shot_test,npc_test)==0 && relation_values[255]==1);
 VirtualFree((void*)game_base,0,MEM_RELEASE);printf("%d native lookup scenarios passed\n",cases);return 0;
}
#endif
