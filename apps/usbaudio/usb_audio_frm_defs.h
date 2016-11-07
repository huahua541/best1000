#ifndef __USB_AUDIO_FRM_DEFS_H__
#define __USB_AUDIO_FRM_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef USB_AUDIO_96K
#define SAMPLE_RATE_PLAYBACK            96000
#ifdef AUD_PLL_DOUBLE
#ifdef _DUAL_AUX_MIC_
#define SAMPLE_RATE_CAPTURE             384000
#else
#define SAMPLE_RATE_CAPTURE             192000
#endif
#elif defined(_DUAL_AUX_MIC_)
#define SAMPLE_RATE_CAPTURE             384000
#else
#define SAMPLE_RATE_CAPTURE             48000
#endif
#elif defined(USB_AUDIO_44_1K)
#define SAMPLE_RATE_PLAYBACK            44100
#define SAMPLE_RATE_CAPTURE             44100
#else // 48K
#define SAMPLE_RATE_PLAYBACK            48000
#define SAMPLE_RATE_CAPTURE             48000
#endif

#define SAMPLE_RATE_RECV                SAMPLE_RATE_PLAYBACK
#if defined(USB_AUDIO_96K) && defined(AUD_PLL_DOUBLE)
#define SAMPLE_RATE_SEND                48000
#elif defined(_DUAL_AUX_MIC_)
#define SAMPLE_RATE_SEND                48000
#else
#define SAMPLE_RATE_SEND                SAMPLE_RATE_CAPTURE
#endif

#ifdef USB_AUDIO_24BIT
#define SAMPLE_SIZE_PLAYBACK            4
#define SAMPLE_SIZE_RECV                3
#else
#define SAMPLE_SIZE_PLAYBACK            2
#define SAMPLE_SIZE_RECV                2
#endif
#define SAMPLE_SIZE_CAPTURE             2
#define SAMPLE_SIZE_SEND                2

#define CHAN_NUM_PLAYBACK               2
#ifdef ANC_USB_TEST
#define CHAN_NUM_CAPTURE                2
#else
#define CHAN_NUM_CAPTURE                1
#endif

#define BYTE_TO_SAMP_PLAYBACK(n)        ((n) / SAMPLE_SIZE_PLAYBACK / CHAN_NUM_PLAYBACK)
#define SAMP_TO_BYTE_PLAYBACK(n)        ((n) * SAMPLE_SIZE_PLAYBACK * CHAN_NUM_PLAYBACK)

#define BYTE_TO_SAMP_CAPTURE(n)         ((n) / SAMPLE_SIZE_CAPTURE / CHAN_NUM_CAPTURE)
#define SAMP_TO_BYTE_CAPTURE(n)         ((n) * SAMPLE_SIZE_CAPTURE * CHAN_NUM_CAPTURE)

#define BYTE_TO_SAMP_RECV(n)            ((n) / SAMPLE_SIZE_RECV / CHAN_NUM_PLAYBACK)
#define SAMP_TO_BYTE_RECV(n)            ((n) * SAMPLE_SIZE_RECV * CHAN_NUM_PLAYBACK)

#define BYTE_TO_SAMP_SEND(n)            ((n) / SAMPLE_SIZE_SEND / CHAN_NUM_CAPTURE)
#define SAMP_TO_BYTE_SEND(n)            ((n) * SAMPLE_SIZE_SEND * CHAN_NUM_CAPTURE)

#define SAMPLE_FRAME_PLAYBACK           ((SAMPLE_RATE_PLAYBACK + (1000 - 1)) / 1000)
#define SAMPLE_FRAME_CAPTURE            ((SAMPLE_RATE_CAPTURE + (1000 - 1)) / 1000)

#define SAMPLE_FRAME_RECV               ((SAMPLE_RATE_RECV + (1000 - 1)) / 1000)
#define SAMPLE_FRAME_SEND               ((SAMPLE_RATE_SEND + (1000 - 1)) / 1000)

#define FRAME_SIZE_PLAYBACK             SAMP_TO_BYTE_PLAYBACK(SAMPLE_FRAME_PLAYBACK)
#define FRAME_SIZE_CAPTURE              SAMP_TO_BYTE_CAPTURE(SAMPLE_FRAME_CAPTURE)

#define FRAME_SIZE_RECV                 SAMP_TO_BYTE_RECV(SAMPLE_FRAME_RECV)
#define FRAME_SIZE_SEND                 SAMP_TO_BYTE_SEND(SAMPLE_FRAME_SEND)

#define PLAYBACK_TO_RECV_LEN(n)         (SAMP_TO_BYTE_RECV(BYTE_TO_SAMP_PLAYBACK(n)) * \
    (SAMPLE_RATE_RECV / 100) / (SAMPLE_RATE_PLAYBACK / 100))
#define CAPTURE_TO_SEND_LEN(n)          (SAMP_TO_BYTE_SEND(BYTE_TO_SAMP_CAPTURE(n)) * \
    (SAMPLE_RATE_SEND / 100) / (SAMPLE_RATE_CAPTURE / 100))

#ifdef __cplusplus
}
#endif

#endif
