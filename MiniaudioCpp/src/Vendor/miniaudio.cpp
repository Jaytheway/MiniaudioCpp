#define MINIAUDIO_IMPLEMENTATION

// TODO: allow user to specify definitions for miniaudio

//#define MA_LOG_LEVEL 3
#define MA_USE_QUAD_MICROSOFT_CHANNEL_MAP

#define STB_VORBIS_HEADER_ONLY
#include "miniaudio/extras/stb_vorbis.c"

#define DR_WAV_IMPLEMENTATION
#include "dr_libs/dr_wav.h"

#include "miniaudio/miniaudio.h"