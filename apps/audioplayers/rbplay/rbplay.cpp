/* rbplay source */
/* playback control & rockbox codec porting & codec thread */

#include "mbed.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include "rtos.h"
#include <ctype.h>
#include <unistd.h>
#include "SDFileSystem.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_trace.h"
#include "audioflinger.h"
#include "cqueue.h"
#include "app_audio.h"

#include "eq.h"
#include "pga.h"
#include "metadata.h"
#include "dsp_core.h"
#include "codecs.h"
#include "codeclib.h"
#include "compressor.h"
#include "channel_mode.h"
#include "audiohw.h"
#include "codec_platform.h"
#include "metadata_parsers.h"

#include "hal_overlay.h"
#include "app_overlay.h"
#include "rbpcmbuf.h"
#include "rbplay.h"

/* Internals */

#define _RB_TASK_STACK_SIZE (3072)
#define _RB_TASK_MAIL_MAX (16)

#define _LOG_TAG "[rbplay] "


#ifdef DEBUG
#define _LOG_DBG(str,...) TRACE(_LOG_TAG""str, ##__VA_ARGS__)
#define _LOG_ERR(str,...) TRACE(_LOG_TAG"err:"""str, ##__VA_ARGS__)

#define _LOG_FUNC_LINE() TRACE(_LOG_TAG"%s:%d\n", __FUNCTION__, __LINE__)
#define _LOG_FUNC_IN() TRACE(_LOG_TAG"%s ++++\n", __FUNCTION__)
#define _LOG_FUNC_OUT() TRACE(_LOG_TAG"%s ----\n", __FUNCTION__)
#else
#define _LOG_DBG(str,...)
#define _LOG_ERR(str,...) TRACE(_LOG_TAG"err:"""str, ##__VA_ARGS__)

#define _LOG_FUNC_LINE()
#define _LOG_FUNC_IN()
#define _LOG_FUNC_OUT()
#endif

/* Rb codec Intface Implements */
#ifdef RB_CODEC
int current_fd = 0;
int current_type = 0;
struct mp3entry current_id3;
struct codec_api ci_api;
struct codec_api *ci = &ci_api;
#endif

void init_dsp(void);
void codec_configure(int setting, intptr_t value);

#ifdef RB_CODEC
static void init_ci_base(void)
{
    _LOG_FUNC_LINE();
	memset(ci, 0, sizeof(struct codec_api));
    _LOG_FUNC_LINE();

	ci->dsp = dsp_get_config(CODEC_IDX_AUDIO);
    _LOG_FUNC_LINE();
    
    ci->configure = codec_configure;

    /* kernel/ system */
    ci->sleep = NULL;
    ci->yield = NULL;

    /* strings and memory */
    ci->strcpy = strcpy;
    ci->strlen = strlen;
    ci->strcmp = strcmp;
    ci->strcat = strcat;
    ci->memset = memset;
    ci->memcpy = memcpy;
    ci->memmove = memmove;
    ci->memcmp = memcmp;
    ci->memchr = memchr;
}

static void f_codec_pcmbuf_insert_callback(
        const void *ch1, const void *ch2, int count)
{
    int min;
    uint32_t i = 0;
    struct dsp_buffer src;
    struct dsp_buffer dst;

    _LOG_FUNC_LINE();

    src.remcount  = count;
    src.pin[0]    = (const unsigned char *)ch1;
    src.pin[1]    = (const unsigned char *)ch2;
    src.proc_mask = 0;

    dst.remcount = 0;

    dst.bufcount = MIN(src.remcount, 128); /* Arbitrary min request */

    while (1) {
        if ((dst.p16out = (short *)rb_pcmbuf_request_buffer(&dst.bufcount)) == NULL) {
            _LOG_DBG("wait pcmbuf!\n");
            osDelay(1);
        }
        else
        {
            _LOG_DBG("pcmbuf %d, src.rem %d\n", dst.bufcount, src.remcount);		
            dsp_process(ci->dsp, &src, &dst); 

            if (dst.remcount > 0) {
                rb_pcmbuf_write(dst.remcount);
            }

            if (src.remcount <= 0) {
                return; /* No input remains and DSP purged */
            }
        }
    }
}
void f_audio_codec_update_elapsed(unsigned long elapsed)
{
    _LOG_FUNC_LINE();
#if 0
#ifdef AB_REPEAT_ENABLE
    ab_position_report(elapsed);
#endif
    /* Save in codec's id3 where it is used at next pcm insert */
    id3_get(CODEC_ID3)->elapsed = elapsed;
#endif
}
static size_t f_codec_filebuf_callback(void *ptr, size_t size)
{
    _LOG_FUNC_LINE();
    //TRACE("filebuf ptr 0x%x, size %d [1111111111111111]\n", ptr, size);
    return read(current_fd, ptr, size);
#if 0
    ssize_t copy_n = bufread(ci.audio_hid, size, ptr);

    /* Return the actual amount of data copied to the buffer */
    return copy_n;
#endif
}
static void * f_codec_request_buffer_callback(size_t *realsize, size_t reqsize)
{
    _LOG_FUNC_LINE();
#if 0
    size_t copy_n = reqsize;
    ssize_t ret;
    void *ptr;

    copy_n = MIN((size_t)ret, reqsize);

    *realsize = copy_n;
    return ptr;
#endif
}

static void f_codec_advance_buffer_callback(size_t amount)
{
    _LOG_FUNC_LINE();
    _LOG_DBG("adv %d\n", amount);
    ci->curpos += amount;
    lseek(current_fd, (off_t)ci->curpos, SEEK_SET);
#if 0
    if (!codec_advance_buffer_counters(amount))
        return;
#endif
}

static bool f_codec_seek_buffer_callback(size_t newpos)
{
    _LOG_FUNC_LINE();
    _LOG_DBG("seek %d\n", newpos);
    lseek(current_fd, (off_t)newpos, SEEK_SET);
    ci->curpos = newpos;

    return true;
#if 0
    int ret = bufseek(ci.audio_hid, newpos);
    if (ret == 0)
    {
        ci.curpos = newpos;
        return true;
    }

    return false;
#endif
}
static void f_codec_seek_complete_callback(void)
{
    _LOG_FUNC_LINE();
    /* Clear DSP */
    dsp_configure(ci->dsp, DSP_FLUSH, 0);
}
void f_audio_codec_update_offset(size_t offset)
{
    _LOG_FUNC_LINE();
#if 0
    /* Save in codec's id3 where it is used at next pcm insert */
    id3_get(CODEC_ID3)->offset = offset;
#endif
}
static void f_codec_configure_callback(int setting, intptr_t value)
{
    _LOG_FUNC_LINE();
    dsp_configure(ci->dsp, setting, value);
}
static enum codec_command_action f_codec_get_command_callback(intptr_t *param)
{
    //_LOG_FUNC_LINE();
    return CODEC_ACTION_NULL;
}
static bool f_codec_loop_track_callback(void)
{
    _LOG_FUNC_LINE();
#if 0
    return REPEAT_ONE;
#endif
}
static void init_ci_file(void)
{
    _LOG_FUNC_LINE();
    ci->codec_get_buffer = 0;
    ci->pcmbuf_insert    = f_codec_pcmbuf_insert_callback;
    ci->set_elapsed      = f_audio_codec_update_elapsed;  
    ci->read_filebuf     = f_codec_filebuf_callback;
    ci->request_buffer   = f_codec_request_buffer_callback; 
    ci->advance_buffer   = f_codec_advance_buffer_callback; 
    ci->seek_buffer      = f_codec_seek_buffer_callback;  
    ci->seek_complete    = f_codec_seek_complete_callback;
    ci->set_offset       = f_audio_codec_update_offset;   
    ci->configure        = f_codec_configure_callback;    
    ci->get_command      = f_codec_get_command_callback;  
    ci->loop_track       = f_codec_loop_track_callback;   
}
static void init_ci_mem(void)
{
    _LOG_FUNC_LINE();
}
static void init_ci_file_raw(void)
{
    _LOG_FUNC_LINE();
    //ci->codec_get_buffer = codec_get_buffer;

    //ci->set_elapsed = set_elapsed;
    //ci->advance_buffer = advance_buffer;
}
static void init_ci_mem_raw(void)
{
    _LOG_FUNC_LINE();
    //ci->codec_get_buffer = codec_get_buffer;
    //ci->set_elapsed = set_elapsed;
    //ci->advance_buffer = advance_buffer;
}
#endif

int rbcodec_init(void)
{
    _LOG_FUNC_LINE();
    app_overlay_select(APP_OVERLAY_A2DP);
#ifdef RB_CODEC
    _LOG_FUNC_LINE();
    dsp_init();
    _LOG_FUNC_LINE();
    init_ci_base();
    /* rb codec dsp init */
    _LOG_FUNC_LINE();
#endif
    /* eq dsp init */
    _LOG_FUNC_LINE();
    init_dsp();
    _LOG_FUNC_LINE();

    return 0;
}

extern "C" {
    void wav_codec_main(int r);
    void wav_codec_run(void);
    void mpa_codec_main(int r);
    void mpa_codec_run(void);
}

int rbcodec_init_codec(int type) 
{
    switch (type)
    {
#if 0
        case AFMT_FLAC:
            flac_codec_main(CODEC_LOAD);
            break;
        case AFMT_APE:
            ape_codec_main(CODEC_LOAD);
            break;
#endif
        case AFMT_PCM_WAV:
            _LOG_FUNC_LINE();
            //wav_codec_main(CODEC_LOAD);
            break;
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            _LOG_FUNC_LINE();
            mpa_codec_main(CODEC_LOAD);
            break;
#if 0
        case AFMT_MP4_AAC:
        case AFMT_MP4_AAC_HE:
            aac_codec_main(CODEC_LOAD);
            break;
        case AFMT_RM_AAC:
            raac_codec_main(CODEC_LOAD);
            break;
        case AFMT_WMA:
            wma_codec_main(CODEC_LOAD);
            break;
        case AFMT_SBC:
            sbc_codec_main(CODEC_LOAD);
            break;
        case AFMT_RAW_AAC:
            rawaac_codec_main(CODEC_LOAD);
            break;
#endif
        default:
            _LOG_ERR("unkown codec type init\n");
            break;
    }

    current_type = type;

    return 0;
}

int rbcodec_run_codec(void) 
{
    switch (current_type)
    {
#if 0
        case AFMT_FLAC:
            flac_codec_run();
            break;
        case AFMT_APE:
            ape_codec_run();
            break;
#endif
        case AFMT_PCM_WAV:
            _LOG_FUNC_LINE();
            //wav_codec_run();		
            break;
        case AFMT_MPA_L1:
        case AFMT_MPA_L2:
        case AFMT_MPA_L3:
            _LOG_FUNC_LINE();
            mpa_codec_run();
            break;
#if 0
        case AFMT_MP4_AAC:
        case AFMT_MP4_AAC_HE:
            aac_codec_run();
            break;
        case AFMT_RM_AAC:
            raac_codec_run();
            break;
        case AFMT_WMA:
            wma_codec_run();
            break;
        case AFMT_SBC:
            sbc_codec_run();
            break;
        case AFMT_RAW_AAC:
            rawaac_codec_run();
            break;
#endif
        default:
            _LOG_ERR("unkown codec type run\n");
            break;
    }
    return 0;
}

typedef struct _RB_MAIL_MSG_T {
    RB_CTRL_CMD_T cmd;
    void *param;
} RB_MAIL_MSG_T;

static osThreadId _rb_tid;
static void _rb_thread(void const *argument);
osThreadDef(_rb_thread, osPriorityRealtime, _RB_TASK_STACK_SIZE);

osMailQDef (_rb_mailbox, _RB_TASK_MAIL_MAX, RB_MAIL_MSG_T);
static osMailQId _rb_mailbox = NULL;

static int _rb_mailbox_init(void)
{
    _LOG_FUNC_IN();
    _rb_mailbox = osMailCreate(osMailQ(_rb_mailbox), NULL);
    if (_rb_mailbox == NULL)  {
        _LOG_ERR("Failed to Create _rb_mailbox\n");
        return -1;
    }
    _LOG_FUNC_OUT();
    return 0;
}

static int _rb_mailbox_put(RB_MAIL_MSG_T* msg_src)
{
    osStatus status;

    RB_MAIL_MSG_T *msg_p = NULL;

    msg_p = (RB_MAIL_MSG_T*)osMailAlloc(_rb_mailbox, 0);
    ASSERT(msg_p, "osMailAlloc error");
    msg_p->cmd = msg_src->cmd;
    msg_p->param = msg_src->param;

    status = osMailPut(_rb_mailbox, msg_p);

    return (int)status;
}

int _rb_mailbox_free(RB_MAIL_MSG_T* msg_p)
{
    osStatus status;

    status = osMailFree(_rb_mailbox, msg_p);

    return (int)status;
}

int _rb_mailbox_get(RB_MAIL_MSG_T** msg_p)
{
    osEvent evt;
    evt = osMailGet(_rb_mailbox, osWaitForever);
    if (evt.status == osEventMail) {
        *msg_p = (RB_MAIL_MSG_T *)evt.value.p;
        return 0;
    }
    return -1;
}

static void _rb_thread(void const *argument)
{
    while(1){
        RB_MAIL_MSG_T *msg_p = NULL;
        _LOG_FUNC_LINE();
        if (!_rb_mailbox_get(&msg_p)){
            _LOG_FUNC_LINE();
            _LOG_DBG("msg_p->cmd %d\n", msg_p->cmd);
            switch(msg_p->cmd) {
                case RB_CTRL_CMD_CODEC_INIT:
                    _LOG_FUNC_LINE();
                    rbcodec_init_codec((int)msg_p->param);
                    break;
                case RB_CTRL_CMD_CODEC_RUN:
                    _LOG_FUNC_LINE();
                    rbcodec_run_codec();
                    break;
                case RB_CTRL_CMD_CODEC_STOP:
                    break;
                case RB_CTRL_CMD_CODEC_DEINIT:
                    break;
                default:
                    _LOG_ERR("unkown rb cmd\n");
                    break;
            }
            _rb_mailbox_free(msg_p);
        }
    }
}

void rb_play_codec_init(int type)
{
    _LOG_FUNC_IN();
    RB_MAIL_MSG_T msg;

    msg.cmd = RB_CTRL_CMD_CODEC_INIT;
    msg.param = (void *)type;

    _rb_mailbox_put(&msg);
    _LOG_FUNC_OUT();
}
void rb_play_codec_run(void)
{
    _LOG_FUNC_IN();
    RB_MAIL_MSG_T msg;

    msg.cmd = RB_CTRL_CMD_CODEC_RUN;
    msg.param = 0;

    _rb_mailbox_put(&msg);
    _LOG_FUNC_OUT();
}
void rb_play_codec_pause(void)
{
    _LOG_FUNC_IN();
    RB_MAIL_MSG_T msg;

    msg.cmd = RB_CTRL_CMD_CODEC_PAUSE;
    msg.param = 0;

    _rb_mailbox_put(&msg);
    _LOG_FUNC_OUT();
}
void rb_play_codec_stop(void)
{
    _LOG_FUNC_IN();
    RB_MAIL_MSG_T msg;

    msg.cmd = RB_CTRL_CMD_CODEC_STOP;
    msg.param = 0;

    _rb_mailbox_put(&msg);
    _LOG_FUNC_OUT();
}
void rb_play_codec_deinit(void)
{
    _LOG_FUNC_IN();
    _LOG_FUNC_OUT();
}

/* APIs */
void rb_play_init(RB_ST_Callback cb)
{
    _LOG_FUNC_IN();

    _rb_mailbox_init();

    _LOG_FUNC_LINE();

    _rb_tid = osThreadCreate(osThread(_rb_thread), NULL);
    if (_rb_tid == NULL)  {
        _LOG_ERR("Failed to Create rb_thread \n");
        return;
    }

    _LOG_FUNC_LINE();

    rbcodec_init();

    _LOG_FUNC_LINE();

    rb_pcmbuf_init();

    _LOG_FUNC_OUT();
}
void rb_play_prepare_file(const char* file_path)
{
    int fd = 0;
    _LOG_FUNC_IN();

    /* open file */
    fd = open((const char *)file_path, O_RDWR);
    if (fd == 0) {
        _LOG_ERR("open file %s failed\n", file_path);
        return;
    }
    current_fd = fd;

    init_ci_file();

    /* get id3 */
    memset(&current_id3, 0, sizeof(struct mp3entry));
    if (!get_metadata(&current_id3, current_fd, file_path)) {
        _LOG_ERR("get_metadata failed\n");
        return;
    }

    /* init ci info */
    ci->filesize = filesize(current_fd);
    ci->id3 = &current_id3;
    ci->curpos = 0;

    dsp_configure(ci->dsp, DSP_RESET, 0);
    dsp_configure(ci->dsp, DSP_FLUSH, 0);

    rb_play_codec_init(ci->id3->codectype);

    _LOG_FUNC_OUT();
}
void rb_play_prepare_mem(const char* mem_address)
{
    _LOG_FUNC_IN();
    _LOG_FUNC_OUT();
}
void rb_play_prepare_file_raw(const char* file_path, unsigned int len, int type)
{
    _LOG_FUNC_IN();
    _LOG_FUNC_OUT();
}
void rb_play_prepare_mem_raw(const char *mem_address, unsigned int len, int type)
{
    _LOG_FUNC_IN();
    _LOG_FUNC_OUT();
}
void rb_play_start(void)
{
    _LOG_FUNC_IN();
    rb_play_codec_run();
    _LOG_FUNC_OUT();
}
void rb_play_pause(void)
{
    _LOG_FUNC_IN();
    _LOG_FUNC_OUT();
}
void rb_play_stop(void)
{
    _LOG_FUNC_IN();
    rb_play_codec_stop();
    _LOG_FUNC_OUT();
}
void rb_play_deinit(void)
{
    _LOG_FUNC_IN();
    _LOG_FUNC_OUT();
}
