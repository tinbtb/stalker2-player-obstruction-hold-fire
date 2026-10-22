#pragma once
#include <windows.h>
#include <stdint.h>
enum { HF_SHOT, HF_READY, HF_IDENTITY, HF_RELATION, HF_AIM, HF_CONTROLLER, HF_PAWN, HF_FOCUS, HF_WEAK, HF_FOCUS_ACCESSOR, HF_PAWN_WITNESS, HF_CAPSULE_CLASS, HF_CAPSULE_CTOR, HF_CAPSULE_DIMS, HF_CAPSULE_POINTER, HF_SCENE_SCALE, HF_AIM_TRANSFORM, HF_TICK, HF_COOKIE, HF_PROFILE_COUNT };
enum { HF_OBJECT_CHUNKS, HF_OBJECT_COUNT, HF_PLAYER_ID, HF_CAPSULE_CLASS_PTR, HF_GLOBAL_COUNT };

typedef void (*HfLog)(const char *, ...);
/* Reads mapped PE memory only. Never calls game code or installs hooks. */
BOOL hf_resolve(HMODULE module, uint32_t rvas[HF_PROFILE_COUNT], uintptr_t globals[HF_GLOBAL_COUNT], HfLog log);

typedef struct { uint32_t offset, size; } HfMask;
typedef struct {
    const char *name;
    uint32_t size;
    unsigned char anchor[32], anchor_mask[32], digest[32];
    const HfMask *masks;
    uint32_t mask_count, leaf;
} HfProfile;
typedef struct { uint32_t from, displacement, next, to; } HfReference;
