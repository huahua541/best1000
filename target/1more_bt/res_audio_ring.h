
const int16_t RES_AUD_RING_SAMPRATE_8000 [] = {
#include "res/ring/SOUND_RING_8000.txt"
};
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
const int16_t RES_AUD_RING_SAMPRATE_44100[] = {
#include "res/ring/SOUND_RING_44100.txt"
};

const int16_t RES_AUD_RING_SAMPRATE_48000 [] = {
#include "res/ring/SOUND_RING_48000.txt"
};
#endif

#if defined(__1MORE_EB100_STYLE__)
const U8 RES_AUD_RING_PWRON_PARING[] = {
#include "res/ring/pwr_on_paring_16k.txt"
};
const U8 RES_AUD_RING_BAT_NORMAL[] = {
#include "res/ring/battery_medium_norm_8k_16k.txt"
};
const U8 RES_AUD_RING_BAT_HIGH[] = {
#include "res/ring/BatteryHigh_16k.txt"
};
const U8 RES_AUD_RING_BAT_LOW[] = {
#include "res/ring/BatteryLow_16k.txt"
};
const U8 RES_AUD_RING_CON[] = {
#include "res/ring/connected_NEW_16k.txt"
};
const U8 RES_AUD_RING_DISCON[] = {
#include "res/ring/disconnected_NEW_16k.txt"
};
const U8 RES_AUD_RING_PARING[] = {
#include "res/ring/Pairing_16k.txt"
};
const U8 RES_AUD_RING_PWROFF[] = {
#include "res/ring/PowerOff_16k.txt"
};
#endif

