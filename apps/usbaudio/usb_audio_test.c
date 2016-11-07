#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_sysfreq.h"
#include "hal_aud.h"
#include "string.h"
#include "pmu.h"
#include "analog.h"
#include "audioflinger.h"
#include "usb_audio.h"
#include "usb_audio_sync.h"
#include "usb_audio_test.h"
#include "usb_audio_frm_defs.h"
#include "app_key.h"
#include "cmsis.h"
#include "hwtimer_list.h"
#include "safe_queue.h"

#ifdef __HW_FIR_EQ_PROCESS_USBAUDIO__
#include "audio_eq.h"
#include "res_audio_eq.h"
#endif

#if defined(__1MORE_EB100_STYLE__)
#include "cmsis_os.h"
extern int app_audio_mempool_init();
extern uint32_t app_audio_mempool_free_buff_size();
extern int app_audio_mempool_get_buff(uint8_t **buff, uint32_t size);

#define TGT_HW_CHARGER_EN_BY_LOW 1
extern void app_battery_charger_stop(void );
extern void app_battery_monitor_enable(bool on);
#define ALIGNED4                        ALIGNED(4)


static uint8_t ALIGNED4 *test_playback_buff;//[USB_AUDIO_PLAYBACK_BUFF_SIZE];
static uint8_t ALIGNED4 *test_capture_buff;//[USB_AUDIO_CAPTURE_BUFF_SIZE];

static uint8_t ALIGNED4 *test_recv_buff;//[USB_AUDIO_RECV_BUFF_SIZE];
static uint8_t ALIGNED4 *test_send_buff;//[USB_AUDIO_SEND_BUFF_SIZE];

#endif
extern void pmu_shutdown(void);
extern void hal_cmu_sys_reboot(void);

#define DIFF_ERR_THRESH_PLAYBACK        (SAMPLE_FRAME_PLAYBACK / 2)
#define DIFF_ERR_THRESH_CAPTURE         (SAMPLE_FRAME_CAPTURE / 2)

#define DIFF_SYNC_THRESH_PLAYBACK       10 //(SAMPLE_FRAME_PLAYBACK / 4)
#define DIFF_SYNC_THRESH_CAPTURE        10 //(SAMPLE_FRAME_CAPTURE / 4)

#define MAX_VOLUME_VALUE                9
#define VOLUME_UPDATE_INTERVAL          300

//#define VERBOSE_TRACE

enum AUDIO_CMD_T {
    AUDIO_CMD_START_PLAY = 0,
    AUDIO_CMD_STOP_PLAY,
    AUDIO_CMD_START_CAPTURE,
    AUDIO_CMD_STOP_CAPTURE,
    AUDIO_CMD_USB_RESET,
    AUDIO_CMD_USB_DISCONNECT,
    AUDIO_CMD_USB_CONFIG,
    AUDIO_CMD_USB_SLEEP,
    AUDIO_CMD_USB_WAKEUP,
};

enum AUDIO_ITF_STATE_T {
    AUDIO_ITF_STATE_STOPPED = 0,
    AUDIO_ITF_STATE_STARTED,
};

static enum AUD_SAMPRATE_T sample_rate_play;
static enum AUD_SAMPRATE_T POSSIBLY_UNUSED sample_rate_capture;

static enum AUDIO_ITF_STATE_T playback_state = AUDIO_ITF_STATE_STOPPED;
static enum AUDIO_ITF_STATE_T capture_state = AUDIO_ITF_STATE_STOPPED;

static uint8_t rw_conflicted;

static uint8_t *playback_buf;
static uint32_t playback_size;
static uint32_t playback_pos;

static uint8_t *capture_buf;
static uint32_t capture_size;
static uint32_t capture_pos;

static uint8_t *usb_recv_buf;
static uint32_t usb_recv_size;
static uint32_t usb_recv_rpos;
static uint32_t usb_recv_wpos;

static uint8_t *usb_send_buf;
static uint32_t usb_send_size;
static uint32_t usb_send_rpos;
static uint32_t usb_send_wpos;

static uint8_t playback_vol = 0x01;
static const uint8_t capture_vol = 0x06;

#ifdef FADING_IN
static uint8_t cur_vol;
static uint16_t vol_update_frame_cnt;
#endif

#ifndef DELAY_STREAM_OPEN
static uint8_t stream_opened;
#endif

static struct USB_AUDIO_STREAM_INFO_T playback_info;

static struct USB_AUDIO_STREAM_INFO_T capture_info;

//static HWTIMER_ID op_timer;

//static struct SAFE_QUEUE_T cmd_queue;

//#define CMD_LIST_SIZE                   20

//static uint32_t cmd_list[CMD_LIST_SIZE];

#ifdef _DUAL_AUX_MIC_

extern void damic_init(void);
extern void damic_deinit(void);

// second-order IR filter
short soir_filter(int32_t PcmValue)
{
#define MWSPT_NSEC 3

#if 1
    const int NL[MWSPT_NSEC] = { 1,3,1 };
    const float NUM[MWSPT_NSEC][3] = {
      {
       0.003031153698,              0,              0
      },
      {
                    1,              2,              1
      },
      {
                    1,              0,              0
      }
    };
    const int DL[MWSPT_NSEC] = { 1,3,1 };
    const float DEN[MWSPT_NSEC][3] = {
      {
                    1,              0,              0
      },
      {
                    1,   -1.838334084,   0.8504587412
      },
      {
                    1,              0,              0
      }
    };
#else
    const float NUM[MWSPT_NSEC][3] = {
                {0.0002616526908, 0, 0},
                {1, 2, 1},
                {1, 0, 0}
            };
    const float DEN[MWSPT_NSEC][3] = {
                {1, 0, 0},
                {1, -1.953727961, 0.9547745585},
                {1, 0, 0}
            };
#endif
    static float y0 = 0, y1 = 0, y2 = 0, x0 = 0, x1 = 0, x2 = 0;

    x0 = (PcmValue* NUM[0][0]);
    y0 = x0*NUM[1][0] + x1*NUM[1][1] + x2*NUM[1][2] - y1*DEN[1][1] - y2*DEN[1][2];
    y2 = y1;
    y1 = y0;
    x2 = x1;
    x1 = x0;

    if (y0 > 32767.0f) {
        y0 = 32767.0f;
    }

    if (y0 < -32768.0f) {
        y0 = -32768.0f;
    }

    return (short)y0;
}

#endif

static enum AUD_BITS_T sample_size_to_enum(uint32_t size)
{
    if (size == 2) {
        return AUD_BITS_16;
    } else if (size == 4) {
        return AUD_BITS_24;
    } else {
        TRACE( " %s: Invalid sample size: %u", __FUNCTION__, size);
    }

    return 0;
}

static enum AUD_CHANNEL_NUM_T chan_num_to_enum(uint32_t num)
{
    if (num == 2) {
        return AUD_CHANNEL_NUM_2;
    } else if (num == 1) {
        return AUD_CHANNEL_NUM_1;
    } else {
        TRACE(  " %s: Invalid channel num: %u", __FUNCTION__, num);
    }

    return 0;
}

static uint32_t usb_audio_data_playback(uint8_t *buf, uint32_t len)
{
    uint32_t usb_len;
    uint32_t old_rpos, new_rpos, wpos;
    int conflicted;
    uint32_t play_next;
    uint32_t lock;

#if 0
    uint32_t stime;
    uint32_t pos;
    static uint32_t preIrqTime = 0;

    pos = buf + len - playback_buf;
    if (pos >= playback_size) {
        pos = 0;
    }
    stime = hal_sys_timer_get();

    TRACE("%s irqDur:%d Len:%d pos:%d", __FUNCTION__, TICKS_TO_MS(stime - preIrqTime), len, pos);

    preIrqTime = stime;
#endif

    //TRACE(" %s: Invalid buf: %p", __FUNCTION__, buf);

    //----------------------------------------
    // Check conflict
    //----------------------------------------

    usb_len = PLAYBACK_TO_RECV_LEN(len);
    old_rpos = usb_recv_rpos;
    new_rpos = usb_recv_rpos + usb_len;
    wpos = usb_recv_wpos;
    if (new_rpos >= usb_recv_size) {
        new_rpos -= usb_recv_size;
    }
    conflicted = 0;
    if (old_rpos <= wpos) {
        if (new_rpos < old_rpos || wpos < new_rpos) {
            conflicted = 1;
        }
    } else {
        if (wpos < new_rpos && new_rpos < old_rpos) {
            conflicted = 1;
        }
    }

#ifdef VERBOSE_TRACE
    TRACE("playback: rpos=%u usb_len=%u wpos=%u play_pos=%u len=%u", old_rpos, usb_len, wpos, buf + len - playback_buf, len);
#endif

    if (conflicted) {
        uint32_t reset_len = usb_recv_size / 2;
        uint32_t prev_old_rpos = old_rpos;

        // Reset read position
        old_rpos = wpos + reset_len;
        if (old_rpos >= usb_recv_size) {
            old_rpos -= usb_recv_size;
        }
        new_rpos = old_rpos + usb_len;
        if (new_rpos >= usb_recv_size) {
            new_rpos -= usb_recv_size;
        }

        TRACE("playback: Error: rpos=%u goes beyond wpos=%u with usb_len=%u. Reset to %u", prev_old_rpos, wpos, usb_len, old_rpos);
        //hal_cmu_sys_reboot();
	}

    //----------------------------------------
    // USB->CODEC stream format conversion start
    //----------------------------------------

    {
        uint8_t POSSIBLY_UNUSED *cur_buf, *buf_end, *usb_cur_buf, *usb_buf_end;

        cur_buf = buf;
        buf_end = buf + len;
        usb_cur_buf = usb_recv_buf + old_rpos;
        usb_buf_end = usb_recv_buf + usb_recv_size;

#ifdef USB_AUDIO_24BIT

        while (cur_buf + 4 <= buf_end) {
            if (usb_cur_buf + 3 > usb_buf_end) {
                usb_cur_buf = usb_recv_buf;
            }
#ifdef __HW_FIR_EQ_PROCESS_USBAUDIO__
            *(int32_t *)cur_buf = ((usb_cur_buf[0]<<8) | (usb_cur_buf[1] << 16) | (usb_cur_buf[2] << 24)) >> 14;
#else
            *(uint32_t *)cur_buf = (usb_cur_buf[0] | (usb_cur_buf[1] << 8) | (usb_cur_buf[2] << 16)) >> 6;
#endif
            cur_buf += 4;
            usb_cur_buf += 3;
        }

#else // !USB_AUDIO_24BIT

        if (old_rpos + len <= usb_recv_size) {
            memcpy(buf, usb_cur_buf, len);
        } else {
            uint32_t copy_len;

            if (old_rpos >= usb_recv_size) {
                copy_len = 0;
            } else {
                copy_len = usb_recv_size - old_rpos;
                memcpy(buf, usb_cur_buf, copy_len);
            }
            memcpy(buf + copy_len, usb_recv_buf, new_rpos);
        }

#endif
    }

    //----------------------------------------
    // USB->CODEC stream format conversion end
    //----------------------------------------

    play_next = buf + len - playback_buf;
    if (play_next >= playback_size) {
        play_next -= playback_size;
    }

    lock = int_lock();
    usb_recv_rpos = new_rpos;
    playback_pos = play_next;
    if (conflicted) {
        rw_conflicted = conflicted;
    }
    int_unlock(lock);

    //----------------------------------------
    // Audio processing start
    //----------------------------------------

#ifdef __HW_FIR_EQ_PROCESS_USBAUDIO__
    audio_eq_run(buf, len);
#endif

    //----------------------------------------
    // Audio processing end
    //----------------------------------------

    return 0;
}

static uint32_t usb_audio_data_capture(uint8_t *buf, uint32_t len)
{
    uint32_t usb_len;
    uint32_t old_wpos, new_wpos, rpos;
    int conflicted;
    uint32_t cap_next;
    uint32_t lock;

#if 0
    uint32_t stime;
    uint32_t pos;
    static uint32_t preIrqTime = 0;

    pos = buf + len - capture_buf;
    if (pos >= capture_size) {
        pos = 0;
    }
    stime = hal_sys_timer_get();

    TRACE("%s irqDur:%d Len:%d pos:%d", __FUNCTION__, TICKS_TO_MS(stime - preIrqTime), len, pos);

    preIrqTime = stime;
#endif
#ifdef VERBOSE_TRACE
    TRACE(  "%s: Invalid buf: %p", __FUNCTION__, buf);
#endif
    //----------------------------------------
    // Check conflict
    //----------------------------------------

    usb_len = CAPTURE_TO_SEND_LEN(len);
    old_wpos = usb_send_wpos;
    new_wpos = usb_send_wpos + usb_len;
    rpos = usb_send_rpos;
    if (new_wpos >= usb_send_size) {
        new_wpos -= usb_send_size;
    }
    conflicted = 0;
    if (old_wpos <= rpos) {
        if (new_wpos < old_wpos || rpos < new_wpos) {
            conflicted = 1;
        }
    } else {
        if (rpos < new_wpos && new_wpos < old_wpos) {
            conflicted = 1;
        }
    }

#ifdef VERBOSE_TRACE
    TRACE("capture: wpos=%u usb_len=%u rpos=%u cap_pos=%u len=%u", old_wpos, usb_len, rpos, buf + len - capture_buf, len);
#endif

    if (conflicted) {
        uint32_t reset_len = usb_send_size / 2;

        TRACE("capture: Error: wpos=%u goes beyond rpos=%u with usb_len=%u", old_wpos, rpos, usb_len);

        // Reset write position
        old_wpos = rpos + reset_len;
        if (old_wpos >= usb_send_size) {
            old_wpos -= usb_send_size;
        }
        new_wpos = old_wpos + usb_len;
        if (new_wpos >= usb_send_size) {
            new_wpos -= usb_send_size;
        }
    }

    //----------------------------------------
    // Audio processing start
    //----------------------------------------
    // TODO ...

    //----------------------------------------
    // Audio processing end
    //----------------------------------------

    //----------------------------------------
    // CODEC->USB stream format conversion start
    //----------------------------------------

    {
        uint8_t POSSIBLY_UNUSED *cur_buf, *buf_end, *usb_cur_buf, *usb_buf_end;

        cur_buf = buf;
        buf_end = buf + len;
        usb_cur_buf = usb_send_buf + old_wpos;
        usb_buf_end = usb_send_buf + usb_send_size;

#ifdef _DUAL_AUX_MIC_
	{
		//static uint32_t usb_cap_buf_pos = 0;

		uint32_t i = 0;
		short* BufSrc = (short*)cur_buf;
		short* BufDst = (short*)usb_cur_buf;

		int32_t PcmValue = 0;
		int32_t BufSrcRVal = 0;

		//TRACE("%s - %d, %d", __func__, BufSrc[0], BufSrc[1]);

		//TRACE("%s - %d", __func__, TICKS_TO_MS(hal_sys_timer_get()));
#if 1
#if (SAMPLE_RATE_CAPTURE == 384000)
        for (i = 0; i < (len>>2); i+=2)
#else
		for (i = 0; i < (len>>2); i++)
#endif
        {
			PcmValue = (int32_t)(BufSrc[i<<1]);
			BufSrcRVal = (int32_t)(BufSrc[(i<<1)+1]);

			if (PcmValue > 32600) {
				PcmValue = BufSrcRVal << 6;
			}
			else if ((PcmValue > (32600/2)) && (PcmValue < 32600)) {
				if (BufSrcRVal > 512) {
					PcmValue = PcmValue*(32600 - PcmValue) + (BufSrcRVal<<6)*(PcmValue - (32600 / 2));
					PcmValue = PcmValue / (32600 / 2);
				}
			}
			else if (PcmValue < -32700) {
				PcmValue = BufSrcRVal << 6;
			}
			else if ((PcmValue > -32700) && (PcmValue < -(32700/2))) {
				if (BufSrcRVal < -512) {
					PcmValue = PcmValue*(-PcmValue - (32700/2)) + (BufSrcRVal<<6)*(32700 + PcmValue);
					PcmValue= PcmValue/(32700/2);
				}
			}
			PcmValue = soir_filter(PcmValue>>3);

#if (SAMPLE_RATE_CAPTURE == 384000)
            if (!(i&0x7)) {
				BufDst[i>>2] = BufDst[(i>>2)+1] = PcmValue;
			}
#else
			if (!(i&0x3))
            {
				BufDst[i>>1] = BufDst[(i>>1)+1] = PcmValue;
			}
#endif

		}

#if (SAMPLE_RATE_CAPTURE == 384000)
		len >>= 1;
#endif

#else
		//short arr[16] = {-600, -200, 200, 600, 600, 200, -200, -600};
		for (i = 0; i < (len>>2); i+=2) {
			BufSrc[i] = BufSrc[i<<1];
			BufSrc[i+1] = BufSrc[(i<<1)+1];
		}
		len >>= 1;

		uint32_t j = 0;
		for (i = 0; i < (len>>2); i++) {

			//PcmValue = (int32_t)(BufSrc[i<<1]);
			//PcmValue = soir_filter(PcmValue>>3);

			if (!(i&3)) {
				//BufDst[i>>1] = arr[(i>>2)&0x7];
				//BufDst[(i>>1)+1] = 0;


				//BufDst[i>>1] = BufDst[(i>>1)+1] = PcmValue;
				//BufDst[i>>1] = BufDst[(i>>1)+1] = BufSrc[i<<1];
				//BufDst[i>>1] = BufDst[(i>>1)+1] = BufSrc[(i<<1)+1];
				BufDst[i>>1] = BufSrc[i<<1];
				BufDst[(i>>1)+1] = BufSrc[(i<<1)+1];
				j += 4;
			}
		}
		//TRACE("%s: %d, %d", __func__, hal_sys_timer_get(), j);
#endif
		//new_wpos = old_wpos + (len>>2);

	}
#elif defined(ANC_USB_TEST) && defined(AUD_PLL_DOUBLE)

        // Assuming codec adc records 192K stereo data, and usb sends 48K stereo data

        while (cur_buf + 16 <= buf_end) {
            uint32_t left, right;

            left = *(uint16_t *)cur_buf + *(uint16_t *)(cur_buf + 4) +
                *(uint16_t *)(cur_buf + 8) + *(uint16_t *)(cur_buf + 12);
            right = *(uint16_t *)(cur_buf + 2) + *(uint16_t *)(cur_buf + 6) +
                *(uint16_t *)(cur_buf + 10) + *(uint16_t *)(cur_buf + 14);

            // Downsampling the adc data
            if (usb_cur_buf + 4 > usb_buf_end) {
                usb_cur_buf = 0;
            }
            *(uint16_t *)usb_cur_buf = left / 4;
            *(uint16_t *)(usb_cur_buf + 2) = right / 4;

            // Move to next data
            usb_cur_buf += 4;
            cur_buf += 16;
        }

#elif defined(ANC_USB_TEST) && defined(USB_AUDIO_MIC_MONO)

        // Assuming codec adc always records stereo data

        while (cur_buf + 4 <= buf_end) {
            // Copy left channel data
            if (usb_cur_buf + 2 > usb_buf_end) {
                usb_cur_buf = 0;
            }
            *(uint16_t *)usb_cur_buf = *(uint16_t *)cur_buf;

            // Move to next data
            usb_cur_buf += 2;
            cur_buf += 4;
        }

#elif !defined(ANC_USB_TEST) && !defined(USB_AUDIO_MIC_MONO)

        // Assuming codec adc always records mono data

        while (cur_buf + 2 <= buf_end) {
            // Duplicate the mono data
            if (usb_cur_buf + 4 > usb_buf_end) {
                usb_cur_buf = 0;
            }
            *(uint16_t *)usb_cur_buf = *(uint16_t *)cur_buf;
            *(uint16_t *)(usb_cur_buf + 2) = *(uint16_t *)cur_buf;

            // Move to next data
            usb_cur_buf += 4;
            cur_buf += 2;
        }

#else // (ANC_USB_TEST && !USB_AUDIO_MIC_MONO) || (!ANC_USB_TEST && USB_AUDIO_MIC_MONO)

        // Codec adc and USB data are in the same format (stereo or mono)

        if (old_wpos + len <= usb_send_size) {
            memcpy(usb_cur_buf, buf, len);
        } else {
            uint32_t copy_len;

            if (old_wpos >= usb_send_size) {
                copy_len = 0;
            } else {
                copy_len = usb_send_size - old_wpos;
                memcpy(usb_cur_buf, buf, copy_len);
            }
            memcpy(usb_send_buf, buf + copy_len, new_wpos);
        }

#endif // (ANC_USB_TEST && !USB_AUDIO_MIC_MONO) || (!ANC_USB_TEST && USB_AUDIO_MIC_MONO)
    }

    //----------------------------------------
    // CODEC->USB stream format conversion end
    //----------------------------------------

    cap_next = buf + len - capture_buf;
    if (cap_next >= capture_size) {
        cap_next -= capture_size;
    }

    lock = int_lock();
    usb_send_wpos = new_wpos;
    capture_pos = cap_next;
    if (conflicted) {
        rw_conflicted = conflicted;
    }
    int_unlock(lock);

    return 0;
}

static int usb_audio_open_stream(enum AUD_STREAM_T stream)
{
    int ret;
    struct AF_STREAM_CONFIG_T stream_cfg;

    memset(&stream_cfg, 0, sizeof(stream_cfg));

    if (stream == AUD_STREAM_PLAYBACK) {
        stream_cfg.bits = sample_size_to_enum(SAMPLE_SIZE_PLAYBACK);
        stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_PLAYBACK);
        stream_cfg.sample_rate = sample_rate_play;
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.vol = playback_vol;
        stream_cfg.handler = usb_audio_data_playback;
        stream_cfg.io_path = AUD_OUTPUT_PATH_SPEAKER;
        stream_cfg.data_ptr = playback_buf;
        stream_cfg.data_size = playback_size;

        //TRACE("[%s] sample_rate: %d, bits = %d", __func__, stream_cfg.sample_rate, stream_cfg.bits);

        memset(playback_buf, 0, playback_size);
        ret = af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &stream_cfg);
        TRACE("af_stream_open playback: %d", ret);
    } else {
        stream_cfg.bits = sample_size_to_enum(SAMPLE_SIZE_CAPTURE);
        stream_cfg.channel_num = chan_num_to_enum(CHAN_NUM_CAPTURE);
        stream_cfg.sample_rate = sample_rate_capture;
        stream_cfg.device = AUD_STREAM_USE_INT_CODEC;
        stream_cfg.vol = capture_vol;
        stream_cfg.handler = usb_audio_data_capture;
        stream_cfg.io_path = AUD_INPUT_PATH_MAINMIC;
        stream_cfg.data_ptr = capture_buf;
        stream_cfg.data_size = capture_size;

        //TRACE("[%s] sample_rate: %d, bits = %d", __func__, stream_cfg.sample_rate, stream_cfg.bits);

        memset(capture_buf, 0, capture_size);
        ret = af_stream_open(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE, &stream_cfg);
        TRACE("af_stream_open catpure: %d", ret);
    }

    return ret;
}

static int usb_audio_close_stream(enum AUD_STREAM_T stream)
{
    return af_stream_close(AUD_STREAM_ID_0, stream);
}

static void usb_audio_set_volume(uint8_t vol)
{
    struct AF_STREAM_CONFIG_T *cfg;

    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &cfg, true);
    cfg->vol = vol;
    af_stream_setup(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, cfg);
}

static void usb_audio_enqueue_cmd(enum AUDIO_CMD_T cmd)
{
    //ret = safe_queue_put(&cmd_queue, cmd);
    //ASSERT(ret == 0, "%s: cmd queue overflow", __FUNCTION__);

    //hwtimer_stop(op_timer);
    //hwtimer_start(op_timer, MS_TO_TICKS(1));
	app_uaudio_mailbox_put(1,(uint32_t)cmd);
}

static void usb_audio_playback_start(uint32_t start)
{
    if (start) {
        usb_audio_enqueue_cmd(AUDIO_CMD_START_PLAY);
    } else {
        usb_audio_enqueue_cmd(AUDIO_CMD_STOP_PLAY);
    }
}

static void usb_audio_capture_start(uint32_t start)
{
    if (start) {
        usb_audio_enqueue_cmd(AUDIO_CMD_START_CAPTURE);
    } else {
        usb_audio_enqueue_cmd(AUDIO_CMD_STOP_CAPTURE);
    }
}

static void usb_audio_mute_control(uint32_t mute)
{
    if (mute) {
        analog_aud_coedc_mute();
    } else {
        analog_aud_coedc_nomute();
    }
}

static void usb_audio_vol_control(uint32_t percent)
{
    uint8_t vol_old = playback_vol;
    TRACE( "  %s percent:%d,vol:%d\n",__func__,percent,playback_vol);

    if (percent > 100) {
        playback_vol = MAX_VOLUME_VALUE;
    } else {
        playback_vol = (percent * MAX_VOLUME_VALUE + 50) / 100;
    }
    if (playback_vol <= 1) playback_vol = 1;
#ifdef FADING_IN
    if(cur_vol == vol_old) {
		cur_vol = playback_vol ;
	} else {
		cur_vol = 1;
		return ;
	}
#endif

    if(playback_state == AUDIO_ITF_STATE_STARTED)
	    usb_audio_set_volume(playback_vol);
}

static uint32_t usb_audio_get_vol_percent(void)
{
#ifdef FADING_IN
    // The volume in low-level driver might be an intermediate volume during fading in
    // Just using the current value of playback_vol
#else
    struct AF_STREAM_CONFIG_T *cfg;

    af_stream_get_cfg(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK, &cfg, true);
    playback_vol = cfg->vol;
#endif

    if (playback_vol > MAX_VOLUME_VALUE) {
        return 100;
    } else {
        return (playback_vol * 100 + MAX_VOLUME_VALUE / 2) / MAX_VOLUME_VALUE;
    }
}

static void usb_audio_data_recv_handler(const uint8_t *data, uint32_t size, int error)
{
    uint32_t old_wpos, new_wpos, rpos;
    uint32_t old_play_pos;
    const uint8_t *play_addr;
    uint32_t lock;
    int conflicted;
    uint32_t recv_samp, play_samp;

#ifdef FADING_IN
	if (cur_vol < playback_vol) {
	    if (vol_update_frame_cnt >= VOLUME_UPDATE_INTERVAL)
	    {
	        vol_update_frame_cnt = 0;
            cur_vol++;
            usb_audio_set_volume(cur_vol);
	    } else {
	        vol_update_frame_cnt++;
	    }
	}
#endif

    if (usb_recv_buf <= data && data <= usb_recv_buf + usb_recv_size) {
        if (data != usb_recv_buf + usb_recv_wpos) {
            TRACE("recv: WARNING: Invalid wpos=0x%x data=%p recv_buf=%p. IRQ missing?", usb_recv_wpos, data, usb_recv_buf);
            usb_recv_wpos = data - usb_recv_buf;
        }
    }

    old_wpos = usb_recv_wpos;
    new_wpos = old_wpos + size;
    if (new_wpos >= usb_recv_size) {
        new_wpos -= usb_recv_size;
    }
    conflicted = 0;

    lock = int_lock();
    if (rw_conflicted) {
        rw_conflicted = 0;
        rpos = 0; // Avoid compiler warnings
        conflicted = 1;
    } else {
        rpos = usb_recv_rpos;
        old_play_pos = playback_pos;
        play_addr = (uint8_t *)af_stream_get_cur_dma_addr(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
    }
    int_unlock(lock);

    if (conflicted == 0) {
        if (old_wpos <= rpos) {
            if (new_wpos < old_wpos || rpos < new_wpos) {
                conflicted = 1;
            }
        } else {
            if (rpos < new_wpos && new_wpos < old_wpos) {
                conflicted = 1;
            }
        }
        if (conflicted) {
            uint32_t reset_len = usb_recv_size / 2;
            uint32_t saved_old_wpos = old_wpos;

            // Reset read position
            old_wpos = rpos + reset_len;
            if (old_wpos >= usb_recv_size) {
                old_wpos -= usb_recv_size;
            }
            new_wpos = old_wpos + size;
            if (new_wpos >= usb_recv_size) {
                new_wpos -= usb_recv_size;
            }

            usb_audio_stop_recv();
            usb_audio_start_recv(usb_recv_buf, new_wpos, usb_recv_size);

            TRACE("recv: Error: wpos=%u goes beyond rpos=%u with len=%u. Reset to %u", saved_old_wpos, rpos, size, old_wpos);
        }
    }

    usb_recv_wpos = new_wpos;

#ifdef VERBOSE_TRACE
    TRACE("recv: wpos=%u size=%u rpos=%u", old_wpos, size, rpos);
#endif

    if (conflicted) {
        usb_audio_sync_reset();
		//hal_cmu_sys_reboot();
        return;
    }

    if (usb_recv_buf <= data && data <= usb_recv_buf + usb_recv_size) {
        recv_samp = BYTE_TO_SAMP_RECV(old_wpos);
    } else {
        recv_samp = -1;
    }

    if (play_addr >= playback_buf && play_addr <= playback_buf + playback_size) {
        uint32_t play_pos;

        play_pos = play_addr - playback_buf;
        if (play_pos == playback_size) {
            play_pos = 0;
        }
        if (old_play_pos <= play_pos) {
            play_pos -= old_play_pos;
        } else {
            play_pos = play_pos + playback_size - old_play_pos;
        }
        play_pos = PLAYBACK_TO_RECV_LEN(play_pos) + rpos;
        if (play_pos >= usb_recv_size) {
            play_pos -= usb_recv_size;
        }
        play_samp = BYTE_TO_SAMP_RECV(play_pos);
    } else {
        play_samp = -1;
    }

    if (recv_samp != -1 && play_samp != -1) {
        usb_audio_sync(play_samp, recv_samp, &playback_info);
    } else {
        //TRACE("recv_hdlr: recv_samp=0x%08x play_samp=0x%08x", recv_samp, play_samp);
    }
}

static void usb_audio_data_send_handler(const uint8_t *data, uint32_t size, int error)
{
    uint32_t old_rpos, new_rpos, wpos;
    uint32_t old_cap_pos;
    const uint8_t *cap_addr;
    uint32_t lock;
    int conflicted;
    uint32_t send_samp, cap_samp;

    if (usb_send_buf <= data && data <= usb_send_buf + usb_send_size) {
        if (data != usb_send_buf + usb_send_rpos) {
            TRACE("send: WARNING: Invalid rpos=0x%x data=%p send_buf=%p. IRQ missing?", usb_send_rpos, data, usb_send_buf);
            usb_send_rpos = data - usb_send_buf;
        }
    }

    old_rpos = usb_send_rpos;
    new_rpos = old_rpos + size;
    if (new_rpos >= usb_send_size) {
        new_rpos -= usb_send_size;
    }
    conflicted = 0;

    lock = int_lock();
    if (rw_conflicted) {
        rw_conflicted = 0;
        wpos = 0; // Avoid compiler warnings
        conflicted = 1;
    } else {
        wpos = usb_send_wpos;
        old_cap_pos = capture_pos;
        cap_addr = (uint8_t *)af_stream_get_cur_dma_addr(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
    }
    int_unlock(lock);

    if (conflicted == 0) {
        if (old_rpos <= wpos) {
            if (new_rpos < old_rpos || wpos < new_rpos) {
                conflicted = 1;
            }
        } else {
            if (wpos < new_rpos && new_rpos < old_rpos) {
                conflicted = 1;
            }
        }
        if (conflicted) {
            uint32_t reset_len = usb_send_size / 2;
            uint32_t saved_old_rpos = old_rpos;

            // Reset read position
            old_rpos = wpos + reset_len;
            if (old_rpos >= usb_send_size) {
                old_rpos -= usb_send_size;
            }
            new_rpos = old_rpos + size;
            if (new_rpos >= usb_send_size) {
                new_rpos -= usb_send_size;
            }

            usb_audio_stop_send();
            usb_audio_start_send(usb_send_buf, new_rpos, usb_send_size);

            TRACE("send: Error: wpos=%u goes beyond wpos=%u with len=%u. Reset to %u", saved_old_rpos, wpos, size, old_rpos);
        }
    }

    usb_send_rpos = new_rpos;

#ifdef VERBOSE_TRACE
    TRACE("send: rpos=%u size=%u wpos=%u", old_rpos, size, wpos);
#endif

    if (conflicted) {
        usb_audio_sync_reset();
        return;
    }

    // Playback takes precedence
    if (playback_state == AUDIO_ITF_STATE_STARTED) {
        return;
    }

    if (usb_send_buf <= data && data <= usb_send_buf + usb_send_size) {
        send_samp = BYTE_TO_SAMP_SEND(old_rpos);
    } else {
        send_samp = -1;
    }

    if (cap_addr >= capture_buf && cap_addr <= capture_buf + capture_size) {
        uint32_t cap_pos;

        cap_pos = cap_addr - capture_buf;
        if (cap_pos == capture_size) {
            cap_pos = 0;
        }
        if (old_cap_pos <= cap_pos) {
            cap_pos -= old_cap_pos;
        } else {
            cap_pos = cap_pos + capture_size - old_cap_pos;
        }
        cap_pos = CAPTURE_TO_SEND_LEN(cap_pos) + wpos;
        if (cap_pos >= usb_send_size) {
            cap_pos -= usb_send_size;
        }
        cap_samp = BYTE_TO_SAMP_SEND(cap_pos);
    } else {
        cap_samp = -1;
    }

    if (send_samp != -1 && cap_samp != -1) {
        usb_audio_sync(cap_samp, send_samp, &capture_info);
    } else {
        //TRACE("send_hdlr: send_samp=0x%08x cap_samp=0x%08x", send_samp, cap_samp);
    }
}

static void usb_audio_reset_stream_state(void)
{
    playback_state = AUDIO_ITF_STATE_STOPPED;
    capture_state = AUDIO_ITF_STATE_STOPPED;
}

static void usb_audio_state_control(enum USB_AUDIO_STATE_EVENT_T event)
{
    TRACE("%s: %d", __FUNCTION__, event);

    if (event == USB_AUDIO_STATE_RESET) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_RESET);
    } else if (event == USB_AUDIO_STATE_DISCONNECT) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_DISCONNECT);
    } else if (event == USB_AUDIO_STATE_CONFIG) {
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_CONFIG);
		app_battery_charger_stop();
    } else if (event == USB_AUDIO_STATE_SLEEP) {
		app_battery_monitor_enable(true);
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_SLEEP);
    } else { // WAKEUP
		app_battery_monitor_enable(false);
        usb_audio_enqueue_cmd(AUDIO_CMD_USB_WAKEUP);
    }


}

static void usb_audio_acquire_freq(void)
{
#ifdef ULTRA_LOW_POWER
    hal_sysfreq_req(HAL_SYSFREQ_USER_RESVALID_2, HAL_CMU_FREQ_52M);
#else
#ifdef _DUAL_AUX_MIC_
#ifdef AUD_PLL_DOUBLE
    hal_sysfreq_req(HAL_SYSFREQ_USER_RESVALID_2, HAL_CMU_FREQ_104M);
#else
    hal_sysfreq_req(HAL_SYSFREQ_USER_RESVALID_2, HAL_CMU_FREQ_208M);
#endif
#else
    hal_sysfreq_req(HAL_SYSFREQ_USER_RESVALID_2, HAL_CMU_FREQ_78M);
#endif
#endif
}

static void usb_audio_release_freq(void)
{
    hal_sysfreq_req(HAL_SYSFREQ_USER_RESVALID_2, HAL_CMU_FREQ_32K);
}

void usb_audio_tester(bool on)
{
    static const struct USB_AUDIO_CFG_T usb_audio_cfg = {
        .recv_start_callback = usb_audio_playback_start,
        .send_start_callback = usb_audio_capture_start,
        .mute_callback = usb_audio_mute_control,
        .set_volume = usb_audio_vol_control,
        .get_volume = usb_audio_get_vol_percent,
        .state_callback = usb_audio_state_control,
        .recv_callback = usb_audio_data_recv_handler,
        .send_callback = usb_audio_data_send_handler,
    };
    static bool isRun = false;
    int ret;

    if (isRun==on)
        return;
    else
        isRun=on;

    TRACE("%s: on=%d", __FUNCTION__, on);

    usb_audio_reset_stream_state();

    if (on) {
        usb_audio_acquire_freq();

        TRACE("CODEC playback sample: rate=%u size=%u chan=%u", SAMPLE_RATE_PLAYBACK, SAMPLE_SIZE_PLAYBACK, CHAN_NUM_PLAYBACK);
        TRACE("CODEC capture sample: rate=%u size=%u chan=%u", SAMPLE_RATE_CAPTURE, SAMPLE_SIZE_CAPTURE, CHAN_NUM_CAPTURE);
        TRACE("USB recv sample: rate=%u size=%u", SAMPLE_RATE_RECV, SAMPLE_SIZE_RECV);
        TRACE("USB send sample: rate=%u size=%u", SAMPLE_RATE_SEND, SAMPLE_SIZE_SEND);

        sample_rate_play = SAMPLE_RATE_PLAYBACK;
        sample_rate_capture = SAMPLE_RATE_CAPTURE;

#ifndef DELAY_STREAM_OPEN
        usb_audio_open_stream(AUD_STREAM_PLAYBACK);
        usb_audio_open_stream(AUD_STREAM_CAPTURE);
        stream_opened = 1;
#endif

        pmu_usb_config(1);
        analog_usbdevice_init();
        ret = usb_audio_open(&usb_audio_cfg);
        TRACE("usb_audio_open RET: %d", ret);
    } else {
        usb_audio_close();
        pmu_usb_config(0);

#ifndef DELAY_STREAM_OPEN
        usb_audio_close_stream(AUD_STREAM_CAPTURE);
        usb_audio_close_stream(AUD_STREAM_PLAYBACK);
        stream_opened = 0;
#endif

        usb_audio_release_freq();
    }

    if( stream_opened == 1) {
		app_battery_monitor_enable(false);
	} else {
		app_battery_monitor_enable(true);
	}
}


static void usb_audio_cmd_start_play(void)
{
    int ret;

    if (playback_state == AUDIO_ITF_STATE_STOPPED) {
        playback_pos = 0;
        usb_recv_rpos = 0;
        usb_recv_wpos = usb_recv_size / 2;

        memset(playback_buf, 0, playback_size);
        memset(usb_recv_buf, 0, usb_recv_size / 2);

#ifdef DELAY_STREAM_OPEN
        usb_audio_open_stream(AUD_STREAM_PLAYBACK);
#endif

#ifdef FADING_IN
        cur_vol = 1; //notice 1 mean vol zero!
        vol_update_frame_cnt = 0;
        usb_audio_set_volume(cur_vol);
#endif

        ret = af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
        TRACE(" af_stream_start playback ret: %d,playback_size %d", ret,playback_size);

#ifdef __HW_FIR_EQ_PROCESS_USBAUDIO__
        ret = audio_eq_open(AUDIO_EQ_SETTING_0, 0, NULL);
        TRACE("audio_eq_open: %d", ret);
#endif

        ret = usb_audio_start_recv(usb_recv_buf, usb_recv_wpos, usb_recv_size);
        TRACE( "usb_audio_start_recv : %d", ret);

        usb_audio_sync_reset();

        playback_state = AUDIO_ITF_STATE_STARTED;
    }
}

static void usb_audio_cmd_stop_play(void)
{
    if (playback_state == AUDIO_ITF_STATE_STARTED) {
        usb_audio_stop_recv();

#ifdef __HW_FIR_EQ_PROCESS_USBAUDIO__
        audio_eq_close();
#endif

        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#ifdef DELAY_STREAM_OPEN
        usb_audio_close_stream(AUD_STREAM_PLAYBACK);
#endif

        usb_audio_sync_reset();

        playback_state = AUDIO_ITF_STATE_STOPPED;
    }
}

static void usb_audio_cmd_start_capture(void)
{
    int ret;

    if (capture_state == AUDIO_ITF_STATE_STOPPED) {
        capture_pos = 0;
        usb_send_rpos = usb_send_size / 2;
        usb_send_wpos = 0;

        memset(usb_send_buf + usb_send_size / 2, 0, usb_send_size / 2);

#ifdef DELAY_STREAM_OPEN
        usb_audio_open_stream(AUD_STREAM_CAPTURE);
#endif
        ret = af_stream_start(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
        TRACE(  "af_stream_start catpure ret: %d", ret);

#ifdef _DUAL_AUX_MIC_
        damic_init();
        //*((uint32_t*)(0x4000a048)) |= (1<<24);
#endif

        ret = usb_audio_start_send(usb_send_buf, usb_send_rpos, usb_send_size);
        TRACE(  "usb_audio_start_send ret: %d", ret);

        if (playback_state != AUDIO_ITF_STATE_STARTED) {
            usb_audio_sync_reset();
        }

        capture_state = AUDIO_ITF_STATE_STARTED;
    }
}

static void usb_audio_cmd_stop_capture(void)
{
    if (capture_state == AUDIO_ITF_STATE_STARTED) {
        usb_audio_stop_send();

#ifdef _DUAL_AUX_MIC_
        damic_deinit();
#endif

        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#ifdef DELAY_STREAM_OPEN
        usb_audio_close_stream(AUD_STREAM_CAPTURE);
#endif

        if (playback_state != AUDIO_ITF_STATE_STARTED) {
            usb_audio_sync_reset();
        }

        capture_state = AUDIO_ITF_STATE_STOPPED;
    }
}

static void usb_audio_cmd_usb_state(enum AUDIO_CMD_T cmd)
{
    if (capture_state == AUDIO_ITF_STATE_STARTED) {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_CAPTURE);
#ifdef DELAY_STREAM_OPEN
        usb_audio_close_stream(AUD_STREAM_CAPTURE);
#endif
        capture_state = AUDIO_ITF_STATE_STOPPED;
    }

    if (playback_state == AUDIO_ITF_STATE_STARTED) {
        af_stream_stop(AUD_STREAM_ID_0, AUD_STREAM_PLAYBACK);
#ifdef DELAY_STREAM_OPEN
        usb_audio_close_stream(AUD_STREAM_PLAYBACK);
#endif
        playback_state = AUDIO_ITF_STATE_STOPPED;
    }

    if (cmd == AUDIO_CMD_USB_RESET )  {
        // Nothing to do
    } else if(cmd == AUDIO_CMD_USB_DISCONNECT){

        pmu_shutdown();
       // TRACE("reset\n");
       // hal_cmu_sys_reboot();

	} else if (cmd == AUDIO_CMD_USB_CONFIG) {
        usb_audio_acquire_freq();
#ifndef DELAY_STREAM_OPEN
        if (stream_opened == 0) {
            usb_audio_open_stream(AUD_STREAM_PLAYBACK);
            usb_audio_open_stream(AUD_STREAM_CAPTURE);
            stream_opened = 1;
        }
#endif
    } else if (cmd == AUDIO_CMD_USB_SLEEP) {
	   pmu_shutdown();
#ifndef DELAY_STREAM_OPEN
        usb_audio_close_stream(AUD_STREAM_CAPTURE);
        usb_audio_close_stream(AUD_STREAM_PLAYBACK);
        stream_opened = 0;
#endif
        usb_audio_release_freq();

    } else { // WAKEUP
        usb_audio_acquire_freq();
#ifndef DELAY_STREAM_OPEN
        usb_audio_open_stream(AUD_STREAM_PLAYBACK);
        usb_audio_open_stream(AUD_STREAM_CAPTURE);
        stream_opened = 1;
#endif
    }
}

static void usb_audio_cmd_handler(void *param)
{
#if 0
    uint32_t cmd;
    while (safe_queue_get(&cmd_queue, &cmd) == 0) {
        TRACE("%s: cmd=%d", __FUNCTION__, cmd);

        switch (cmd) {
        case AUDIO_CMD_START_PLAY:
            usb_audio_cmd_start_play();
            break;

        case AUDIO_CMD_STOP_PLAY:
            usb_audio_cmd_stop_play();
            break;

        case AUDIO_CMD_START_CAPTURE:
            usb_audio_cmd_start_capture();
            break;

        case AUDIO_CMD_STOP_CAPTURE:
            usb_audio_cmd_stop_capture();
            break;

        case AUDIO_CMD_USB_RESET:
        case AUDIO_CMD_USB_DISCONNECT:
        case AUDIO_CMD_USB_CONFIG:
        case AUDIO_CMD_USB_SLEEP:
        case AUDIO_CMD_USB_WAKEUP:
            usb_audio_cmd_usb_state(cmd);
            break;

        default:
            ASSERT("%s: Invalid cmd %d", __FUNCTION__, cmd);
            break;
        }
    }
#endif
}

void usb_audio_tester_init(uint8_t *play_buf, uint32_t play_size,
                           uint8_t *cap_buf, uint32_t cap_size,
                           uint8_t *recv_buf, uint32_t recv_size,
                           uint8_t *send_buf, uint32_t send_size)
{
    TRACE( " %s: play frames %u should < recv frames %u", __FUNCTION__,
        play_size / SAMPLE_FRAME_PLAYBACK, recv_size / SAMPLE_FRAME_RECV);
    TRACE(  "%s: cap frames %u should < send frames %u", __FUNCTION__,
        cap_size / SAMPLE_FRAME_CAPTURE, send_size / SAMPLE_FRAME_SEND);

    playback_buf = play_buf;
    playback_size = play_size;
    capture_buf = cap_buf;
    capture_size = cap_size;
    usb_recv_buf = recv_buf;
    usb_recv_size = recv_size;
    usb_send_buf = send_buf;
    usb_send_size = send_size;

    playback_pos = 0;
    usb_recv_rpos = 0;
    usb_recv_wpos = recv_size / 2;
    capture_pos = 0;
    usb_send_rpos = send_size / 2;
    usb_send_wpos = 0;

    playback_info.capture = 0;
    playback_info.buffer_size = BYTE_TO_SAMP_RECV(recv_size);
    playback_info.err_thresh = DIFF_ERR_THRESH_PLAYBACK;
    playback_info.sync_thresh = DIFF_SYNC_THRESH_PLAYBACK;

    capture_info.capture = 1;
    capture_info.buffer_size = BYTE_TO_SAMP_SEND(send_size);
    capture_info.err_thresh = DIFF_ERR_THRESH_CAPTURE;
    capture_info.sync_thresh = DIFF_SYNC_THRESH_CAPTURE;

   // op_timer = hwtimer_alloc(usb_audio_cmd_handler, NULL);
   // ASSERT(op_timer, "%s: Failed to alloc hwtimer", __FUNCTION__);

   // safe_queue_init(&cmd_queue, cmd_list, CMD_LIST_SIZE);
}

void usb_audio_tester_term(void)
{
    playback_buf = NULL;
    playback_size = 0;
    capture_buf = NULL;
    capture_size = 0;
    usb_recv_buf = NULL;
    usb_recv_size = 0;
    usb_send_buf = NULL;
    usb_send_size = 0;

    playback_pos = 0;
    usb_recv_rpos = 0;
    usb_recv_wpos = 0;

    capture_pos = 0;
    usb_send_rpos = 0;
    usb_send_wpos = 0;

  //  hwtimer_stop(op_timer);
  //  hwtimer_free(op_timer);
  //  op_timer = NULL;
}

void usb_audio_tester_key(enum HAL_KEY_CODE_T code, enum HAL_KEY_EVENT_T event)
{
    int state;
    enum USB_AUDIO_HID_EVENT_T uevt;

    if (event == HAL_KEY_EVENT_DOWN || event == HAL_KEY_EVENT_UP) {
        if (code == HAL_KEY_CODE_FN1) {
            TRACE("[%s] USB_AUDIO_HID_PLAY", __func__);
            uevt = USB_AUDIO_HID_PLAY;
        } else if (code == HAL_KEY_CODE_FN2) {
            TRACE("[%s] USB_AUDIO_HID_VOL_UP", __func__);
            uevt = USB_AUDIO_HID_VOL_UP;
        } else if (code == HAL_KEY_CODE_FN3) {
            TRACE("[%s] USB_AUDIO_HID_VOL_DOWN", __func__);
            uevt = USB_AUDIO_HID_VOL_DOWN;
        } else {
            return;
        }

        if (event == HAL_KEY_EVENT_DOWN) {
            state = 1;
        } else {
            state = 0;
        }

        usb_audio_hid_set_event(uevt, state);
    }
}


void app_uaudio_open(uint8_t *ply_buf,uint8_t *cap_buf,uint8_t *recv_buf,uint8_t *send_buf)
{
    TRACE("%s \n",__func__);

	usb_audio_tester_init(ply_buf, USB_AUDIO_PLAYBACK_BUFF_SIZE,
			cap_buf, USB_AUDIO_CAPTURE_BUFF_SIZE,
			recv_buf,USB_AUDIO_RECV_BUFF_SIZE,
			send_buf,USB_AUDIO_SEND_BUFF_SIZE);

	usb_audio_tester(true);
}


typedef struct {
	uint32_t msg_id;
	uint32_t   msg_arg;
} APP_UAUDIO_MESSAGE_BLOCK;

static void app_uaudio_thread(void const *argument);
osThreadDef(app_uaudio_thread, osPriorityHigh, 4096);
osMailQDef (app_uaudio_mailbox, 10, APP_UAUDIO_MESSAGE_BLOCK);
static osMailQId app_uaudio_mailbox = NULL;

static int app_uaudio_mailbox_init(void)
{
	app_uaudio_mailbox = osMailCreate(osMailQ(app_uaudio_mailbox), NULL);
	if (app_uaudio_mailbox == NULL)  {
        TRACE(" Failed to Create app_uaudio_mailbox\n");
		return -1;
	}
	return 0;
}

int app_uaudio_mailbox_put(uint32_t id,uint32_t arg)
{
	osStatus status;

	APP_UAUDIO_MESSAGE_BLOCK *msg_p = NULL;

	msg_p = (APP_UAUDIO_MESSAGE_BLOCK*)osMailAlloc(app_uaudio_mailbox, 0);
	if(!msg_p) {
		TRACE(  " uaudio osMailAlloc fail");
		return -1;
	}
	msg_p->msg_id = id;
	msg_p->msg_arg = arg;

	status = osMailPut(app_uaudio_mailbox, msg_p);
	if (osOK != status) {
		TRACE(" %s send fail :%d",__func__,(int)status);
	}
	return (int)status;
}

int app_uaudio_mailbox_free(APP_UAUDIO_MESSAGE_BLOCK* msg_p)
{
	osStatus status;

	status = osMailFree(app_uaudio_mailbox, msg_p);
	if (osOK != status) {
        TRACE(" %s fail:%d",__func__,(int)status);
	}

	return (int)status;
}

int app_uaudio_mailbox_get(APP_UAUDIO_MESSAGE_BLOCK** msg_p)
{
	osEvent evt;
	evt = osMailGet(app_uaudio_mailbox, osWaitForever);
	if (evt.status == osEventMail) {
		*msg_p = (APP_UAUDIO_MESSAGE_BLOCK *)evt.value.p;
		return 0;
	}
	return -1;
}

static void app_uaudio_thread(void const *argument)
{
	APP_UAUDIO_MESSAGE_BLOCK *msg_p = NULL;

	while(1){

		if(NULL != msg_p) {
			app_uaudio_mailbox_free(msg_p);
			msg_p = NULL;
		}

		app_uaudio_mailbox_get(&msg_p);
//process key evt
//process battery monitor
//process shutdown,close audio
        TRACE("  %s  msg id:%d , cmd :%d",__func__,msg_p->msg_id,msg_p->msg_arg);
        switch(msg_p->msg_id) {
            case 0://keys , the arg save key status
			{
				enum HAL_KEY_CODE_T code;
				enum HAL_KEY_EVENT_T event;
				APP_KEY_STATUS *st = (APP_KEY_STATUS*)(msg_p->msg_arg);
				code = (enum HAL_KEY_CODE_T )st->code;
				event = (enum HAL_KEY_EVENT_T)st->event;
				if(event == HAL_KEY_EVENT_CLICK)
				event = HAL_KEY_EVENT_UP;
				TRACE("%s code:%d,evt:%d \n",__func__,code,event);
				usb_audio_tester_key(code,event);
			}
			break;
			case 1://usb status
			{
				uint32_t cmd = msg_p->msg_arg;
				switch (cmd) {
				case AUDIO_CMD_START_PLAY:
					usb_audio_cmd_start_play();
					break;

				case AUDIO_CMD_STOP_PLAY:
					usb_audio_cmd_stop_play();
					break;

				case AUDIO_CMD_START_CAPTURE:
					usb_audio_cmd_start_capture();
					break;

				case AUDIO_CMD_STOP_CAPTURE:
					usb_audio_cmd_stop_capture();
					break;

				case AUDIO_CMD_USB_RESET:
				case AUDIO_CMD_USB_DISCONNECT:
				case AUDIO_CMD_USB_CONFIG:
				case AUDIO_CMD_USB_SLEEP:
				case AUDIO_CMD_USB_WAKEUP:
					usb_audio_cmd_usb_state(cmd);
					break;

				default:
					TRACE("%s: Invalid cmd %d", __FUNCTION__, cmd);
					break;
				}
			}
			break;

			default:
				break;
		}
	}
}

int app_uaudio_init(void)
{
	if (app_uaudio_mailbox_init())
		return -1;

	if (osThreadCreate(osThread(app_uaudio_thread), NULL) == NULL)  {
        TRACE(" Failed to Create app_uaudio_thread\n");
		return 0;
	}

	app_audio_mempool_init();
	if(app_audio_mempool_free_buff_size() < (USB_AUDIO_PLAYBACK_BUFF_SIZE+USB_AUDIO_CAPTURE_BUFF_SIZE+USB_AUDIO_RECV_BUFF_SIZE+USB_AUDIO_SEND_BUFF_SIZE)) {
		TRACE("BUFFER IS TOO BIG , goto rst!!\n");
		return 0;
	}

	app_audio_mempool_get_buff(&test_playback_buff,USB_AUDIO_PLAYBACK_BUFF_SIZE) ;
	app_audio_mempool_get_buff(&test_capture_buff,USB_AUDIO_CAPTURE_BUFF_SIZE) ;
	app_audio_mempool_get_buff(&test_recv_buff,USB_AUDIO_RECV_BUFF_SIZE) ;
	app_audio_mempool_get_buff(&test_send_buff,USB_AUDIO_SEND_BUFF_SIZE) ;

	TRACE("!!!!!USB_AUDIO_MODE!!!!!\n");
	app_uaudio_open(test_playback_buff,test_capture_buff,test_recv_buff,test_send_buff);

	return 0;
}


