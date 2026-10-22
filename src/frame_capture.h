// Optional engine tick intervals. This is not DXGI Present or displayed-frame FPS.
// Capture is installed only at startup when the adjacent INI explicitly enables it.
#define FRAME_CAPACITY 8192
#define FRAME_MASK (FRAME_CAPACITY-1)
typedef struct {
 LONG64 qpc, interval, gate_count, gate_total;
 ULONGLONG uptime;
 LONG64 checks, withheld, faults;
} FrameSample;
static FrameSample frame_ring[FRAME_CAPACITY];
static volatile LONG64 frame_write,frame_read,frame_dropped;
static volatile LONG frame_owner;
static LONG64 frame_previous;
static LONG64 frame_epoch_qpc,frame_epoch_unix_100ns;
static unsigned char *frame_cave;
static int frame_active;
static char frame_csv[MAX_PATH],frame_log[MAX_PATH];
static void frame_status(const char *message){
 FILE *f=fopen(frame_log,"w");if(f){fprintf(f,"%s\n",message);fclose(f);}
}
static void frame_record(LONG64 now){
 LONG tid=(LONG)GetCurrentThreadId(),owner=InterlockedCompareExchange(&frame_owner,tid,0);
 if(owner && owner!=tid){InterlockedIncrement64(&frame_dropped);return;}
 LONG64 previous=frame_previous;frame_previous=now;
 LONG64 w=InterlockedCompareExchange64(&frame_write,0,0);
 LONG64 r=InterlockedCompareExchange64(&frame_read,0,0);
 if(w-r>=FRAME_CAPACITY){InterlockedIncrement64(&frame_dropped);return;}
 FrameSample sample={0};sample.qpc=now;sample.interval=previous?now-previous:0;
 sample.uptime=GetTickCount64();
 sample.gate_count=InterlockedCompareExchange64(&metrics[5].count,0,0);
 sample.gate_total=InterlockedCompareExchange64(&metrics[5].total,0,0);
 sample.checks=InterlockedCompareExchange64(&checks,0,0);
 sample.withheld=InterlockedCompareExchange64(&blocked,0,0);
 sample.faults=InterlockedCompareExchange64(&read_faults,0,0);
 frame_ring[w&FRAME_MASK]=sample;
 InterlockedExchange64(&frame_write,w+1);
}
static void frame_tick_end(uintptr_t cookie){
 // Preserve the original security-cookie check. Only our added work follows it.
 ((void(*)(uintptr_t))(game_base+resolved_rvas[HF_COOKIE]))(cookie);
 frame_record(stamp());
}
static void frame_init(void){
 strcpy(frame_csv,log_path);strcpy(strrchr(frame_csv,'\\')+1,"engine_frames.csv");
 strcpy(frame_log,log_path);strcpy(strrchr(frame_log,'\\')+1,"frame_capture.log");
 if(GetPrivateProfileIntA("Debug","CaptureEngineFrames",0,ini_path)!=1){
  frame_status("DISABLED: CaptureEngineFrames=0 (or INI absent). No frame hook installed.");return;
 }
 // Exact GameEngine::Tick single return epilogue. First 5 bytes are cookie CALL.
 // Next 3 bytes belong to MOVAPS and remain identical; no entry detour conflicts.
 unsigned char expected[8];
 unsigned char *hook=(unsigned char*)(game_base+resolved_rvas[HF_TICK]+1386);
 memcpy(expected,hook,8);
 int32_t delta_original;memcpy(&delta_original,hook+1,4);
 if(hook[0]!=0xe8 || (uintptr_t)(hook+5)+delta_original!=game_base+resolved_rvas[HF_COOKIE]){
  frame_status("REFUSED: resolved tick CALL relationship changed.");return;
 }
 frame_cave=allocate_near((uintptr_t)hook);
 if(!frame_cave){frame_status("REFUSED: no near allocation for capture.");return;}
 // The existing CALL supplies stack alignment/shadow space. Tail-call normal C ABI.
 unsigned char stub[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};
 uintptr_t fn=(uintptr_t)&frame_tick_end;memcpy(stub+2,&fn,8);memcpy(frame_cave,stub,sizeof(stub));
 DWORD old;if(!VirtualProtect(frame_cave,4096,PAGE_EXECUTE_READ,&old))goto failed;
 FlushInstructionCache(GetCurrentProcess(),frame_cave,sizeof(stub));
 FILE *f=fopen(frame_csv,"w");if(!f)goto failed;
 fprintf(f,"sequence,utc_unix_ms,system_uptime_ms,qpc_ticks,engine_interval_ms,gate_calls,gate_total_us,eligible_checks,withheld,guarded_read_faults,dropped_samples,hold_fire_enabled\n");
 fclose(f);
 FILETIME time;GetSystemTimePreciseAsFileTime(&time);frame_epoch_qpc=stamp();
 frame_epoch_unix_100ns=(LONG64)(((ULONGLONG)time.dwHighDateTime<<32)|time.dwLowDateTime)-116444736000000000LL;
 unsigned char patch[8];memcpy(patch,expected,8);
 int32_t delta=(int32_t)((uintptr_t)frame_cave-((uintptr_t)hook+5));memcpy(patch+1,&delta,4);
 if(!patch_safely(hook,patch,expected))goto failed;
 frame_active=1;frame_status("ACTIVE: engine tick end intervals; buffered output every second. Not displayed-frame timings.");return;
failed:
 VirtualFree(frame_cave,0,MEM_RELEASE);frame_cave=NULL;
 frame_status("REFUSED: safe capture installation failed; hold-fire remains active.");
}
static void frame_flush(void){
 if(!frame_active)return;
 LONG64 r=InterlockedCompareExchange64(&frame_read,0,0),w=InterlockedCompareExchange64(&frame_write,0,0);
 if(r==w)return;
 FILE *f=fopen(frame_csv,"a");if(!f)return;
 LONG64 end=r;
 for(;end<w;end++){
  FrameSample s=frame_ring[end&FRAME_MASK];
  double utc=frame_epoch_unix_100ns/10000.0+(s.qpc-frame_epoch_qpc)*1000.0/perf_frequency.QuadPart;
  if(fprintf(f,"%lld,%.3f,%llu,%lld,%.6f,%lld,%.3f,%lld,%lld,%lld,%lld,%d\n",
    end,utc,(unsigned long long)s.uptime,s.qpc,s.interval*1000.0/perf_frequency.QuadPart,
    s.gate_count,s.gate_total*1e6/perf_frequency.QuadPart,s.checks,s.withheld,s.faults,
    InterlockedCompareExchange64(&frame_dropped,0,0),hold_fire_enabled)<0)break;
 }
 // Do not release unread slots to the producer until buffered writes are successful.
 int error=ferror(f);if(fclose(f)!=0)error=1;
 if(!error)InterlockedExchange64(&frame_read,end);
}
#ifdef FRAME_TEST
#define FRAME_EXPECT(x) do{if(!(x)){printf("Frame capture test failed: %d\n",__LINE__);return 1;}cases++;}while(0)
int main(void){
 int cases=0;QueryPerformanceFrequency(&perf_frequency);frame_record(100);frame_record(250);
 FRAME_EXPECT(frame_write==2 && frame_ring[0].interval==0 && frame_ring[1].interval==150);
 frame_record(10000250);FRAME_EXPECT(frame_ring[2].interval==10000000);
 while(frame_write<FRAME_CAPACITY)frame_record(frame_previous+1);
 FRAME_EXPECT(frame_write==FRAME_CAPACITY);
 FrameSample first=frame_ring[0];frame_record(frame_previous+1);
 FRAME_EXPECT(frame_dropped==1 && frame_write==FRAME_CAPACITY && frame_ring[0].qpc==first.qpc);
 InterlockedExchange64(&frame_read,1);frame_record(frame_previous+1);
 FRAME_EXPECT(frame_write==FRAME_CAPACITY+1 && frame_ring[0].qpc==frame_previous);
 // Exercise actual buffered file output and consumer progress on a small sample set.
 InterlockedExchange64(&frame_read,frame_write);frame_record(frame_previous+100);
 char temporary[MAX_PATH];FRAME_EXPECT(GetTempPathA(MAX_PATH,temporary)>0);
 FRAME_EXPECT(GetTempFileNameA(temporary,"hf",0,frame_csv)!=0);
 frame_active=1;frame_flush();FRAME_EXPECT(frame_read==frame_write);
 FILE *f=fopen(frame_csv,"r");char line[1024];
 FRAME_EXPECT(f && fgets(line,sizeof(line),f)!=NULL);if(f)fclose(f);
 FRAME_EXPECT(DeleteFileA(frame_csv));
 printf("%d frame buffer/output scenarios passed\n",cases);return 0;
}
#endif
