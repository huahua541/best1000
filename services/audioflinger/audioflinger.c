#include "cmsis_os.h"
#include "audioflinger.h"
#include "hal_dma.h"
#include "hal_i2s.h"
#include "hal_codec.h"
#include "hal_spdif.h"
#include "hal_btpcm.h"
#include "codec_tlv32aic32.h"
#include "codec_best1000.h"

#include "string.h"

#include "hal_trace.h"
#include "hal_cmu.h"
#include "pmu.h"
#include "analog.h"

/* config params */
#define AF_I2S_INST HAL_I2S_ID_0
#define AF_CODEC_INST HAL_CODEC_ID_0
#define AF_SPDIF_INST HAL_SPDIF_ID_0
#define AF_BTPCM_INST HAL_BTPCM_ID_0

/* internal use */
enum AF_BOOL_T{
    AF_FALSE = 0,
    AF_TRUE = 1
};

enum AF_RESULT_T{
    AF_RES_SUCESS = 0,
    AF_RES_FAILD = 1
};

//pingpong machine
enum AF_PP_T{
    PP_PING = 0,
    PP_PANG = 1
};
#define PP_PINGPANG(v) \
    (v == PP_PING? PP_PANG: PP_PING)

#define AF_STACK_SIZE (2048)
#define AUDIO_BUFFER_COUNT (4)

//status machine
enum AF_STATUS_T{
    AF_STATUS_NULL = 0x00,
    AF_STATUS_OPEN_CLOSE = 0x01,
    AF_STATUS_STREAM_OPEN_CLOSE = 0x02,
    AF_STATUS_STREAM_START_STOP = 0x04,
    AF_STATUS_MASK = 0x07,
};

struct af_stream_ctl_t{
    enum AF_PP_T pp_index;		//pingpong operate
    uint8_t pp_cnt;				//use to count the lost signals
    uint8_t status;				//status machine
    enum AUD_STREAM_USE_DEVICE_T use_device;
};

struct af_stream_cfg_t {
    //used inside
    struct af_stream_ctl_t ctl;

    //dma buf parameters, RAM can be alloced in different way
    uint8_t *dma_buf_ptr;
    uint32_t dma_buf_size;

    //store stream cfg parameters
    struct AF_STREAM_CONFIG_T cfg;

    //dma cfg parameters
    struct HAL_DMA_DESC_T dma_desc[AUDIO_BUFFER_COUNT];
    struct HAL_DMA_CH_CFG_T dma_cfg;

    //callback function
    AF_STREAM_HANDLER_T handler;
};

struct af_stream_fade_out_t{
    bool stop_on_process;
    uint8_t stop_process_cnt;
    osThreadId stop_request_tid;
    uint32_t need_fadeout_len;
    uint32_t need_fadeout_len_processed;
};


struct af_stream_cfg_t af_stream[AUD_STREAM_ID_NUM][AUD_STREAM_NUM];

static struct af_stream_fade_out_t af_stream_fade_out ={
                                                .stop_on_process = false,
                                                .stop_process_cnt = 0,
                                                .stop_request_tid = NULL,
                                                .need_fadeout_len = 0,
                                                .need_fadeout_len_processed = 0,
};

#define AF_TRACE_DEBUG()    //TRACE("%s:%d\n", __func__, __LINE__)


static osThreadId af_thread_tid;

static void af_thread(void const *argument);
osThreadDef(af_thread, osPriorityAboveNormal, AF_STACK_SIZE);

osMutexId audioflinger_mutex_id = NULL;
osMutexDef(audioflinger_mutex);

void af_lock_thread(void)
{
    osMutexWait(audioflinger_mutex_id, osWaitForever);
}

void af_unlock_thread(void)
{
    osMutexRelease(audioflinger_mutex_id);
}

int af_stream_fadeout_start(uint32_t sample)
{
    TRACE("fadein_config sample:%d", sample);
    af_stream_fade_out.need_fadeout_len = sample;
    af_stream_fade_out.need_fadeout_len_processed = sample;
    return 0;
}

int af_stream_fadeout_stop(void)
{
    af_stream_fade_out.stop_process_cnt = 0;
    af_stream_fade_out.stop_on_process = false;
    return 0;
}
uint32_t af_stream_fadeout(int16_t *buf, uint32_t len, enum AUD_CHANNEL_NUM_T num)
{
    uint32_t i;
    uint32_t j = 0;
    uint32_t start;
    uint32_t end;

    start = af_stream_fade_out.need_fadeout_len_processed;
    end = af_stream_fade_out.need_fadeout_len_processed  > len ? 
           af_stream_fade_out.need_fadeout_len_processed - len :  0;

    if (start <= end){
//        TRACE("skip fadeout");
        memset(buf, 0, len*2);
        return len;
    }
//    TRACE("fadeout l:%d start:%d end:%d", len, start, end);
//    DUMP16("%05d ", buf, 10);    
//    DUMP16("%05d ", buf+len-10, 10);
 

    if (num == AUD_CHANNEL_NUM_1){
        for (i = start; i > end; i--){
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
        }
    }else if (num == AUD_CHANNEL_NUM_2){
        for (i = start; i > end; i-=2){
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
        }
    }else if (num == AUD_CHANNEL_NUM_4){
        for (i = start; i > end; i-=4){
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
        }
    }else if (num == AUD_CHANNEL_NUM_8){
        for (i = start; i > end; i-=8){
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
            *(buf+j) = *(buf+j)*i/af_stream_fade_out.need_fadeout_len;
            j++;
        }
        
    }
    af_stream_fade_out.need_fadeout_len_processed -= j;
    
//    TRACE("out i:%d process:%d x:%d", i, af_stream_fade_out.need_fadeout_len_processed, end+((start-end)/AUD_CHANNEL_NUM_2));
//    DUMP16("%05d ", buf, 10);
//    DUMP16("%05d ", buf+len-10, 10);

    return len;
}

void af_stream_stop_wait_finish()
{
    af_lock_thread();
    af_stream_fade_out.stop_on_process = true;
    af_stream_fade_out.stop_request_tid = osThreadGetId();
    osSignalClear(af_stream_fade_out.stop_request_tid, 0x10);
    af_unlock_thread();
    osSignalWait(0x10, 300);
}

void af_stream_stop_process(struct af_stream_cfg_t *af_cfg, uint8_t *buf, uint32_t len)
{
    af_lock_thread();
    if (af_stream_fade_out.stop_on_process){
//        TRACE("%s num:%d size:%d len:%d cnt:%d", __func__, af_cfg->cfg.channel_num, af_cfg->cfg.data_size, len,  af_stream_fade_out.stop_process_cnt);
        af_stream_fadeout((int16_t *)buf, len/2, af_cfg->cfg.channel_num);
        
//        TRACE("process ret:%d %d %d", *(int16_t *)(buf+len-2-2-2), *(int16_t *)(buf+len-2-2), *(int16_t *)(buf+len-2));
        if (af_stream_fade_out.stop_process_cnt++>3){
            TRACE("stop_process end");
            osSignalSet(af_stream_fade_out.stop_request_tid, 0x10);
        }
    }
    af_unlock_thread();
}

//used by dma irq and af_thread
static inline struct af_stream_cfg_t *af_get_stream_role(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    ASSERT(id<AUD_STREAM_ID_NUM, "[%s] id >= AUD_STREAM_ID_NUM", __func__);
    ASSERT(stream<AUD_STREAM_NUM, "[%s] stream >= AUD_STREAM_NUM", __func__);

    return &af_stream[id][stream];
}

static void af_set_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, enum AF_STATUS_T status)
{
    struct af_stream_cfg_t *role = NULL;

    role = af_get_stream_role(id, stream);

    role->ctl.status |= status;
}

static void af_clear_status(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, enum AF_STATUS_T status)
{
    struct af_stream_cfg_t *role = NULL;

    role = af_get_stream_role(id, stream);

    role->ctl.status &= ~status;
}

//get current stream config parameters
uint32_t af_stream_get_cfg(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, struct AF_STREAM_CONFIG_T **cfg, bool needlock)
{
    struct af_stream_cfg_t *role = NULL;

    if (needlock)
        af_lock_thread();
    role = af_get_stream_role(id, stream);
    *cfg = &(role->cfg);
    if (needlock)
        af_unlock_thread();
    return AF_RES_SUCESS;
}

#if 0
static void af_dump_cfg()
{
    struct af_stream_cfg_t *role = NULL;

    TRACE("[%s] dump cfg.........start");
    //initial parameter
    for(uint8_t id=0; id< AUD_STREAM_ID_NUM; id++)
    {
        for(uint8_t stream=0; stream < AUD_STREAM_NUM; stream++)
        {
            role = af_get_stream_role((enum AUD_STREAM_ID_T)id, (enum AUD_STREAM_T)stream);

            TRACE("id = %d, stream = %d:", id, stream);
            TRACE("ctl.use_device = %d", role->ctl.use_device);
            TRACE("cfg.device = %d", role->cfg.device);
            TRACE("dma_cfg.ch = %d", role->dma_cfg.ch);
        }
    }
    TRACE("[%s] dump cfg.........end");
}
#endif

static void af_thread(void const *argument)
{
    osEvent evt;
    uint32_t signals = 0;
    struct af_stream_cfg_t *role = NULL;

    while(1)
    {
        //wait any signal
        evt = osSignalWait(0x0, osWaitForever);
        signals = evt.value.signals;
//        TRACE("[%s] status = %x, signals = %d", __func__, evt.status, evt.value.signals);
#if 1
        if(evt.status == osEventSignal)
        {
            for(uint8_t i=0; i<AUD_STREAM_ID_NUM * AUD_STREAM_NUM; i++)
            {
                if(signals & (0x01 << i))
                {
                    role = af_get_stream_role((enum AUD_STREAM_ID_T)(i>>1), (enum AUD_STREAM_T)(i & 0x01));
//                    role = &af_stream[i>>1][i & 0x01];

                    af_lock_thread();
                    if (role->handler)
                    {
            			role->ctl.pp_cnt = 0;
                        role->handler(role->dma_buf_ptr + PP_PINGPANG(role->ctl.pp_index) * (role->dma_buf_size / 2), role->dma_buf_size / 2);

            			//measure task and irq speed
            			if(role->ctl.pp_cnt > 1)
            			{
            			    //if this TRACE happened, thread is slow and irq is fast
            				//so you can use larger dma buff to solve this problem
            				//af_stream_open: cfg->data
            				TRACE("[%s] WARNING: role[%d] lost %d signals", __func__, signals-1, role->ctl.pp_cnt-1);
            			}
                    }
                    af_unlock_thread();
                    af_stream_stop_process(role, role->dma_buf_ptr + PP_PINGPANG(role->ctl.pp_index) * (role->dma_buf_size / 2), role->dma_buf_size / 2);
                }
            }
        }
        else
        {
            TRACE("[%s] ERROR: evt.status = %d", __func__, evt.status);
            continue;
        }

#else
        //get role from signal value
        if(evt.status == osEventSignal)
        {
            if(evt.value.signals == 0x01)
            {
                role = &af_stream[AUD_STREAM_ID_0][AUD_STREAM_PLAYBACK];
            }
            else if(evt.value.signals == 0x02)
            {
                role = &af_stream[AUD_STREAM_ID_0][AUD_STREAM_CAPTURE];
            }
            else if(evt.value.signals == 0x04)
            {
                role = &af_stream[AUD_STREAM_ID_1][AUD_STREAM_PLAYBACK];
            }
            else if(evt.value.signals == 0x08)
            {
                role = &af_stream[AUD_STREAM_ID_1][AUD_STREAM_CAPTURE];
            }
            else
            {
                TRACE("[%s] ERROR: evt.value.signals = %d", __func__, evt.value.signals);
                continue;
            }
        }
        else
        {
            TRACE("[%s] ERROR: evt.status = %d", __func__, evt.status);
            continue;
        }
        af_lock_thread();
        if (role->handler)
        {
			role->ctl.pp_cnt = 0;
            role->handler(role->dma_buf_ptr + PP_PINGPANG(role->ctl.pp_index) * (role->dma_buf_size / 2), role->dma_buf_size / 2);

			//measure task and irq speed
			if(role->ctl.pp_cnt > 1)
			{
			    //if this TRACE happened, thread is slow and irq is fast
				//so you can use larger dma buff to solve this problem
				//af_stream_open: cfg->data
				TRACE("[%s] WARNING: role[%d] lost %d signals", __func__, evt.value.signals-1, role->ctl.pp_cnt-1);
			}
        }
        af_unlock_thread();
#endif
    }
}

static void af_dma_irq_handler(uint8_t ch, uint32_t remain_dst_tsize, uint32_t error, struct HAL_DMA_DESC_T *lli)
{
    struct af_stream_cfg_t *role = NULL;

    //initial parameter
    for(uint8_t id=0; id< AUD_STREAM_ID_NUM; id++)
    {
        for(uint8_t stream=0; stream < AUD_STREAM_NUM; stream++)
        {
            role = af_get_stream_role((enum AUD_STREAM_ID_T)id, (enum AUD_STREAM_T)stream);
//            role = &af_stream[id][stream];

            if(role->dma_cfg.ch == ch)
            {
                role->ctl.pp_index = PP_PINGPANG(role->ctl.pp_index);
                role->ctl.pp_cnt++;
//                TRACE("[%s] id = %d, stream = %d, ch = %d", __func__, id, stream, ch);
//                TRACE("[%s] PLAYBACK pp_cnt = %d", af_stream[AUD_STREAM_PLAYBACK].ctl.pp_cnt);
                osSignalSet(af_thread_tid, 0x01 << (id * 2 + stream));
                return;
            }
        }
    }

    //invalid dma irq
    ASSERT(0, "[%s] ERROR: channel id = %d", __func__, ch);
}

uint32_t af_open(void)
{
    AF_TRACE_DEBUG();
    struct af_stream_cfg_t *role = NULL;

    if (audioflinger_mutex_id == NULL)
    {
        audioflinger_mutex_id = osMutexCreate((osMutex(audioflinger_mutex)));
    }

    af_lock_thread();

    //initial parameters
    for(uint8_t id=0; id< AUD_STREAM_ID_NUM; id++)
    {
        for(uint8_t stream=0; stream < AUD_STREAM_NUM; stream++)
        {
            role = af_get_stream_role((enum AUD_STREAM_ID_T)id, (enum AUD_STREAM_T)stream);

            if(role->ctl.status == AF_STATUS_NULL)
            {
                role->dma_buf_ptr = NULL;
                role->dma_buf_size = 0;
                role->ctl.pp_index = PP_PING;
                role->ctl.status = AF_STATUS_OPEN_CLOSE;
                role->ctl.use_device = AUD_STREAM_USE_DEVICE_NULL;
                role->dma_cfg.ch = 0xff;
            }
            else
            {
                ASSERT(0, "[%s] ERROR: id = %d, stream = %d", __func__, id, stream);
            }
        }
    }

#ifndef _AUDIO_NO_THREAD_
    af_thread_tid = osThreadCreate(osThread(af_thread), NULL);
    osSignalSet(af_thread_tid, 0x0);

#endif

    af_unlock_thread();

    return AF_RES_SUCESS;
}

//Support memory<-->peripheral
// Note:Do not support peripheral <--> peripheral
uint32_t af_stream_open(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, struct AF_STREAM_CONFIG_T *cfg)
{
    AF_TRACE_DEBUG();
    struct af_stream_cfg_t *role = NULL;
    enum AUD_STREAM_USE_DEVICE_T device;

    struct HAL_DMA_CH_CFG_T *dma_cfg = NULL;
    struct HAL_DMA_DESC_T *dma_desc = NULL;

    role = af_get_stream_role(id, stream);
    TRACE("[%s] id = %d, stream = %d", __func__, id, stream);

    //check af is open
    if(role->ctl.status != AF_STATUS_OPEN_CLOSE)
    {
        TRACE("[%s] ERROR: status = %d",__func__, role->ctl.status);
        return AF_RES_FAILD;
    }

	ASSERT(cfg->data_ptr != NULL, "[%s] ERROR: data_ptr == NULL!!!", __func__);
	ASSERT(cfg->data_size !=0, "[%s] ERROR: data_size == 0!!!", __func__);

    AF_TRACE_DEBUG();

    af_lock_thread();

    device = cfg->device;
    role->ctl.use_device = device;

    dma_desc = &role->dma_desc[0];

    dma_cfg = &role->dma_cfg;
    memset(dma_cfg, 0, sizeof(*dma_cfg));
    dma_cfg->ch = hal_audma_get_chan(HAL_DMA_HIGH_PRIO);
    dma_cfg->dst_bsize = HAL_DMA_BSIZE_4;
    dma_cfg->src_bsize = HAL_DMA_BSIZE_4;
    if (device == AUD_STREAM_USE_INT_CODEC && cfg->bits == AUD_BITS_24
            && stream == AUD_STREAM_PLAYBACK)
    {
        dma_cfg->dst_width = HAL_DMA_WIDTH_WORD;
        dma_cfg->src_width = HAL_DMA_WIDTH_WORD;
        dma_cfg->src_tsize = cfg->data_size / AUDIO_BUFFER_COUNT / 4; /* cause of word width */
    }
    else if (device == AUD_STREAM_USE_DPD_RX && stream == AUD_STREAM_CAPTURE)
    {
        dma_cfg->dst_width = HAL_DMA_WIDTH_WORD;
        dma_cfg->src_width = HAL_DMA_WIDTH_WORD;
        dma_cfg->src_tsize = cfg->data_size / AUDIO_BUFFER_COUNT / 4; /* cause of word width */
    }    
    else
    {
        dma_cfg->dst_width = HAL_DMA_WIDTH_HALFWORD;
        dma_cfg->src_width = HAL_DMA_WIDTH_HALFWORD;
        dma_cfg->src_tsize = cfg->data_size / AUDIO_BUFFER_COUNT / 2; /* cause of half word width */
    }
    dma_cfg->try_burst = 1;
    dma_cfg->handler = af_dma_irq_handler;

    if(stream ==  AUD_STREAM_PLAYBACK)
    {
        AF_TRACE_DEBUG();
        dma_cfg->src_periph = (enum HAL_DMA_PERIPH_T)0;
        dma_cfg->type = HAL_DMA_FLOW_M2P_DMA;

        //open device and stream
        if(device == AUD_STREAM_USE_EXT_CODEC)
        {
            AF_TRACE_DEBUG();
            tlv32aic32_open();
            tlv32aic32_stream_open(stream);

            dma_cfg->dst_periph = HAL_AUDMA_I2S_TX;
        }
		else if(device == AUD_STREAM_USE_I2S)
		{
			AF_TRACE_DEBUG();
            hal_i2s_open(AF_I2S_INST);
            dma_cfg->dst_periph = HAL_AUDMA_I2S_TX;
		}
        else if(device == AUD_STREAM_USE_INT_CODEC)
        {
            //打开设备初始化配置及DAC
            AF_TRACE_DEBUG();
            codec_best1000_open();
            codec_best1000_stream_open(stream);

            dma_cfg->dst_periph = HAL_AUDMA_CODEC_TX;
        }
        else if(device == AUD_STREAM_USE_INT_SPDIF)
        {
            AF_TRACE_DEBUG();
            hal_spdif_open(AF_SPDIF_INST);
            dma_cfg->dst_periph = HAL_AUDMA_SPDIF_TX;
        }
        else if(device == AUD_STREAM_USE_BT_PCM)
        {
            AF_TRACE_DEBUG();
            hal_btpcm_open(AF_BTPCM_INST);
            dma_cfg->dst_periph = HAL_AUDMA_BTPCM_TX;
        }
		else
		{
			ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
		}
    }
    else
    {
        AF_TRACE_DEBUG();
        dma_cfg->dst_periph = (enum HAL_DMA_PERIPH_T)0;
        dma_cfg->type = HAL_DMA_FLOW_P2M_DMA;

        //open device and stream
        if(device == AUD_STREAM_USE_EXT_CODEC)
        {
            AF_TRACE_DEBUG();
            tlv32aic32_open();
            tlv32aic32_stream_open(stream);

            dma_cfg->src_periph = HAL_AUDMA_I2S_RX;
        }
		else if(device == AUD_STREAM_USE_I2S)
		{
            AF_TRACE_DEBUG();
            hal_i2s_open(AF_I2S_INST);

            dma_cfg->src_periph = HAL_AUDMA_I2S_RX;
		}
        else if(device == AUD_STREAM_USE_INT_CODEC)
        {
            //打开设备初始化配置及ADC
            AF_TRACE_DEBUG();
            codec_best1000_open();
            codec_best1000_stream_open(stream);

            dma_cfg->src_periph = HAL_AUDMA_CODEC_RX;
        }
        else if(device == AUD_STREAM_USE_INT_SPDIF)
        {
            AF_TRACE_DEBUG();
            hal_spdif_open(AF_SPDIF_INST);

            dma_cfg->src_periph = HAL_AUDMA_SPDIF_RX;
        }
        else if(device == AUD_STREAM_USE_BT_PCM)
        {
            AF_TRACE_DEBUG();
            hal_btpcm_open(AF_BTPCM_INST);

            dma_cfg->src_periph = HAL_AUDMA_BTPCM_RX;
        }
        else if(device == AUD_STREAM_USE_DPD_RX)
        {
            AF_TRACE_DEBUG();
            dma_cfg->src_periph = HAL_AUDMA_DPD_RX;
        }  
		else
		{
			ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
		}
    }

    AF_TRACE_DEBUG();
    role->dma_buf_ptr = cfg->data_ptr;
    role->dma_buf_size = cfg->data_size;

    //分四次搬运，每两次触发一次中断，符合pingpong机制
    if (stream == AUD_STREAM_PLAYBACK) {
        AF_TRACE_DEBUG();
        dma_cfg->src = (uint32_t)(role->dma_buf_ptr);
        hal_audma_init_desc(&dma_desc[0], dma_cfg, &dma_desc[1], 0);
        dma_cfg->src = (uint32_t)(role->dma_buf_ptr + role->dma_buf_size/4);
        hal_audma_init_desc(&dma_desc[1], dma_cfg, &dma_desc[2], 1);
        dma_cfg->src = (uint32_t)(role->dma_buf_ptr + role->dma_buf_size/2);
        hal_audma_init_desc(&dma_desc[2], dma_cfg, &dma_desc[3], 0);
        dma_cfg->src = (uint32_t)(role->dma_buf_ptr + (role->dma_buf_size/4)*3);
        hal_audma_init_desc(&dma_desc[3], dma_cfg, &dma_desc[0], 1);
    }
    else {
        AF_TRACE_DEBUG();
        dma_cfg->dst = (uint32_t)(role->dma_buf_ptr);
        hal_audma_init_desc(&dma_desc[0], dma_cfg, &dma_desc[1], 0);
        dma_cfg->dst = (uint32_t)(role->dma_buf_ptr + role->dma_buf_size/4);
        hal_audma_init_desc(&dma_desc[1], dma_cfg, &dma_desc[2], 1);
        dma_cfg->dst = (uint32_t)(role->dma_buf_ptr + role->dma_buf_size/2);
        hal_audma_init_desc(&dma_desc[2], dma_cfg, &dma_desc[3], 0);
        dma_cfg->dst = (uint32_t)(role->dma_buf_ptr + (role->dma_buf_size/4)*3);
        hal_audma_init_desc(&dma_desc[3], dma_cfg, &dma_desc[0], 1);
    }

    AF_TRACE_DEBUG();
    role->handler = cfg->handler;

    af_set_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);

    af_stream_setup(id, stream, cfg);

    af_unlock_thread();

    return AF_RES_SUCESS;
}

//volume, path, sample rate, channel num ...
uint32_t af_stream_setup(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream, struct AF_STREAM_CONFIG_T *cfg)
{
    AF_TRACE_DEBUG();
    struct af_stream_cfg_t *role = NULL;

    enum AUD_STREAM_USE_DEVICE_T device;

    role = af_get_stream_role(id, stream);
    device = role->ctl.use_device;

    //check stream is open
    if(!(role->ctl.status & AF_STATUS_STREAM_OPEN_CLOSE))
    {
        TRACE("[%s] ERROR: status = %d",__func__, role->ctl.status);
        return AF_RES_FAILD;
    }

    af_lock_thread();
    role->cfg = *cfg;

    AF_TRACE_DEBUG();
    if(device == AUD_STREAM_USE_EXT_CODEC)
    {
        AF_TRACE_DEBUG();

		struct tlv32aic32_config_t tlv32aic32_cfg;
		tlv32aic32_cfg.bits = cfg->bits;
		tlv32aic32_cfg.channel_num = cfg->channel_num;
		tlv32aic32_cfg.sample_rate = cfg->sample_rate;
		tlv32aic32_cfg.use_dma = AF_TRUE;
		tlv32aic32_stream_setup(stream, &tlv32aic32_cfg);
    }
	else if(device == AUD_STREAM_USE_I2S)
	{
		AF_TRACE_DEBUG();

		struct HAL_I2S_CONFIG_T i2s_cfg;
		i2s_cfg.bits = cfg->bits;
		i2s_cfg.channel_num = cfg->channel_num;
		i2s_cfg.sample_rate = cfg->sample_rate;
		i2s_cfg.use_dma = AF_TRUE;

		hal_i2s_setup_stream(AF_I2S_INST, stream, &i2s_cfg);
	}
    else if(device == AUD_STREAM_USE_INT_CODEC)
    {
        AF_TRACE_DEBUG();
        struct HAL_CODEC_CONFIG_T codec_cfg;
        codec_cfg.bits = cfg->bits;
        codec_cfg.sample_rate = cfg->sample_rate;
        codec_cfg.channel_num = cfg->channel_num;
        codec_cfg.use_dma = AF_TRUE;
        codec_cfg.vol = cfg->vol;
        codec_cfg.io_path = cfg->io_path;

        AF_TRACE_DEBUG();
        codec_best1000_stream_setup(stream, &codec_cfg);
    }
    else if(device == AUD_STREAM_USE_INT_SPDIF)
    {
        AF_TRACE_DEBUG();
        struct HAL_SPDIF_CONFIG_T spdif_cfg;
        spdif_cfg.use_dma = AF_TRUE;
        spdif_cfg.bits = cfg->bits;
        spdif_cfg.channel_num = cfg->channel_num;
        spdif_cfg.sample_rate = cfg->sample_rate;
        hal_spdif_setup_stream(AF_SPDIF_INST, stream, &spdif_cfg);
    }
    else if(device == AUD_STREAM_USE_BT_PCM)
    {
        AF_TRACE_DEBUG();
        struct HAL_BTPCM_CONFIG_T btpcm_cfg;
        btpcm_cfg.use_dma = AF_TRUE;
        btpcm_cfg.bits = cfg->bits;
        btpcm_cfg.channel_num = cfg->channel_num;
        btpcm_cfg.sample_rate = cfg->sample_rate;
        hal_btpcm_setup_stream(AF_BTPCM_INST, stream, &btpcm_cfg);
    }
    else if(device == AUD_STREAM_USE_DPD_RX)
    {
        AF_TRACE_DEBUG();
    }
	else
	{
		ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
	}

    AF_TRACE_DEBUG();
    af_unlock_thread();

    return AF_RES_SUCESS;
}

uint32_t af_stream_start(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    AF_TRACE_DEBUG();
    struct af_stream_cfg_t *role = NULL;
    enum AUD_STREAM_USE_DEVICE_T device;

    role = af_get_stream_role(id, stream);
    device = role->ctl.use_device;

    //check stream is open and not start.
    if(role->ctl.status != (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE))
    {
        TRACE("[%s] ERROR: status = %d",__func__, role->ctl.status);
        return AF_RES_FAILD;
    }

    af_lock_thread();

    hal_audma_sg_start(&role->dma_desc[0], &role->dma_cfg);

    AF_TRACE_DEBUG();
    if(device == AUD_STREAM_USE_EXT_CODEC)
    {
        AF_TRACE_DEBUG();
        tlv32aic32_stream_start(stream);
    }
	else if(device == AUD_STREAM_USE_I2S)
	{
		hal_i2s_start_stream(AF_I2S_INST, stream);
	}
    else if(device == AUD_STREAM_USE_INT_CODEC)
    {
        AF_TRACE_DEBUG();
        codec_best1000_stream_start(stream);
    }
    else if(device == AUD_STREAM_USE_INT_SPDIF)
    {
        AF_TRACE_DEBUG();
        hal_spdif_start_stream(AF_SPDIF_INST, stream);
    }
    else if(device == AUD_STREAM_USE_BT_PCM)
    {
        AF_TRACE_DEBUG();
        hal_btpcm_start_stream(AF_BTPCM_INST, stream);
    }
    else if(device == AUD_STREAM_USE_DPD_RX)
    {
        AF_TRACE_DEBUG();
        hal_btpcm_start_stream(AF_BTPCM_INST, stream);
    }
    else
    {
        ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
    }

    AF_TRACE_DEBUG();
    af_set_status(id, stream, AF_STATUS_STREAM_START_STOP);

    af_unlock_thread();

    return AF_RES_SUCESS;
}

uint32_t af_stream_stop(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    AF_TRACE_DEBUG();
    struct af_stream_cfg_t *role = NULL;
    enum AUD_STREAM_USE_DEVICE_T device;

    role = af_get_stream_role(id, stream);
    device = role->ctl.use_device;

    //check stream is start and not stop
    if(role->ctl.status != (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE | AF_STATUS_STREAM_START_STOP))
    {
        TRACE("[%s] ERROR: status = %d",__func__, role->ctl.status);
        return AF_RES_FAILD;
    }

    if (id == AUD_STREAM_ID_0 && stream == AUD_STREAM_PLAYBACK){
        af_stream_fadeout_start(512);
        af_stream_stop_wait_finish();        
	}

    af_lock_thread();

    hal_audma_stop(role->dma_cfg.ch);
    
    if (id == AUD_STREAM_ID_0 && stream == AUD_STREAM_PLAYBACK){
        af_stream_fadeout_stop();
    }
    
    if(device == AUD_STREAM_USE_EXT_CODEC)
    {
        AF_TRACE_DEBUG();
        tlv32aic32_stream_stop(stream);
    }
	else if(device == AUD_STREAM_USE_I2S)
	{
        AF_TRACE_DEBUG();
        hal_i2s_stop_stream(AF_I2S_INST, stream);
	}
    else if(device == AUD_STREAM_USE_INT_CODEC)
    {
        AF_TRACE_DEBUG();
        codec_best1000_stream_stop(stream);
    }
    else if(device == AUD_STREAM_USE_INT_SPDIF)
    {
        AF_TRACE_DEBUG();
        hal_spdif_stop_stream(AF_SPDIF_INST, stream);
    }
    else if(device == AUD_STREAM_USE_BT_PCM)
    {
        AF_TRACE_DEBUG();
        hal_btpcm_stop_stream(AF_BTPCM_INST, stream);
    }
    else
    {
        ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
    }

#ifndef _AUDIO_NO_THREAD_
    //deal with thread
#endif


    AF_TRACE_DEBUG();
    af_clear_status(id, stream, AF_STATUS_STREAM_START_STOP);

    af_unlock_thread();

    return AF_RES_SUCESS;
}

uint32_t af_stream_close(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    AF_TRACE_DEBUG();
    struct af_stream_cfg_t *role = NULL;
    enum AUD_STREAM_USE_DEVICE_T device;

    role = af_get_stream_role(id, stream);
    device = role->ctl.use_device;

    //check stream is stop and not close.
    if(role->ctl.status != (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE))
    {
        TRACE("[%s] ERROR: status = %d",__func__, role->ctl.status);
        return AF_RES_FAILD;
    }

    AF_TRACE_DEBUG();
    af_lock_thread();

    memset(role->dma_buf_ptr, 0, role->dma_buf_size);
    hal_audma_free_chan(role->dma_cfg.ch);

    //	TODO: more parameter should be set!!!
//    memset(role, 0xff, sizeof(struct af_stream_cfg_t));
    role->handler = NULL;
    role->ctl.pp_index = PP_PING;
    role->ctl.use_device = AUD_STREAM_USE_DEVICE_NULL;
    role->dma_buf_ptr = NULL;
    role->dma_buf_size = 0;

    role->dma_cfg.ch = 0xff;

    if(device == AUD_STREAM_USE_EXT_CODEC)
    {
        AF_TRACE_DEBUG();
        tlv32aic32_stream_close(stream);
    }
	else if(device == AUD_STREAM_USE_I2S)
	{
		AF_TRACE_DEBUG();
        hal_i2s_close(AF_I2S_INST);
	}
    else if(device == AUD_STREAM_USE_INT_CODEC)
    {
        AF_TRACE_DEBUG();
		codec_best1000_stream_close(stream);
		codec_best1000_close();
    }
    else if(device == AUD_STREAM_USE_INT_SPDIF)
    {
        AF_TRACE_DEBUG();
        hal_spdif_close(AF_SPDIF_INST);
    }
    else if(device == AUD_STREAM_USE_BT_PCM)
    {
        AF_TRACE_DEBUG();
        hal_btpcm_close(AF_BTPCM_INST);
    }
    else if(device == AUD_STREAM_USE_DPD_RX)
    {
        AF_TRACE_DEBUG();
    }    
    else
    {
        ASSERT(0, "[%s] ERROR: device %d is not defined!", __func__, device);
    }

#ifndef _AUDIO_NO_THREAD_
    //deal with thread
#endif

    AF_TRACE_DEBUG();
    af_clear_status(id, stream, AF_STATUS_STREAM_OPEN_CLOSE);

    af_unlock_thread();

    return AF_RES_SUCESS;
}

uint32_t af_close(void)
{
    AF_TRACE_DEBUG();
    struct af_stream_cfg_t *role = NULL;

    af_lock_thread();

    for(uint8_t id=0; id< AUD_STREAM_ID_NUM; id++)
    {
        for(uint8_t stream=0; stream < AUD_STREAM_NUM; stream++)
        {
            role = af_get_stream_role((enum AUD_STREAM_ID_T)id, (enum AUD_STREAM_T)stream);

            if(role->ctl.status == AF_STATUS_OPEN_CLOSE)
            {
                role->ctl.status = AF_STATUS_NULL;
            }
            else
            {
                ASSERT(0, "[%s] ERROR: id = %d, stream = %d", __func__, id, stream);
            }

        }
    }

    af_unlock_thread();

    return AF_RES_SUCESS;
}

uint32_t af_stream_get_cur_dma_addr(enum AUD_STREAM_ID_T id, enum AUD_STREAM_T stream)
{
    struct af_stream_cfg_t *role = NULL;

    role = af_get_stream_role(id, stream);

    //check stream is start and not stop
    if(role->ctl.status == (AF_STATUS_OPEN_CLOSE | AF_STATUS_STREAM_OPEN_CLOSE | AF_STATUS_STREAM_START_STOP))
    {
        if (role->dma_cfg.ch != HAL_DMA_CHAN_NONE) {
            if (stream == AUD_STREAM_PLAYBACK) {
                return hal_audma_get_cur_src_addr(role->dma_cfg.ch);
            } else {
                return hal_audma_get_cur_dst_addr(role->dma_cfg.ch);
            }
        }
    }

    return 0;
}
void af_set_priority(int priority)
{
    osThreadSetPriority(af_thread_tid, priority);
}

int af_get_priority(void)
{
    return (int)osThreadGetPriority(af_thread_tid);
}
