//#include "mbed.h"
#include <stdio.h>
#include <assert.h>

#include "cmsis_os.h"
#include "cmsis.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_chipid.h"
#include "audioflinger.h"
#include "lockcqueue.h"
#include "hal_trace.h"
#include "hal_cmu.h"
#include "analog.h"
#include "app_bt_stream.h"
#include "app_overlay.h"
#include "app_audio.h"
#include "app_utils.h"

#include "resources.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "app_media_player.h"
#endif

extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
#if 1//def __BT_REAL_ONE_BRING_TWO__
#include "sys/mei.h"
#endif
}

#include "rtos.h"
#include "iabt.h"

#include "cqueue.h"
#include "btapp.h"

#ifdef EQ_PROCESS
#include "eq_export.h"
#endif

#include "app_bt_media_manager.h"
#include "app_thread.h"

#include "app_ring_merge.h"

uint8_t btapp_hfp_get_call_state(void);

extern struct BT_DEVICE_T  app_bt_device;





typedef struct
{
    uint8_t media_active[BT_DEVICE_NUM];
    uint8_t media_current_call_state[BT_DEVICE_NUM];
    uint8_t media_curr_sbc;
    uint8_t curr_active_media;
}BT_MEDIA_MANAGER_STRUCT;


BT_MEDIA_MANAGER_STRUCT  bt_meida;



uint8_t bt_media_is_media_active_by_type(uint8_t media_type)
{
    uint8_t i;
    for(i=0;i<BT_DEVICE_NUM;i++)
    {
        if(bt_meida.media_active[i] & media_type)
            return 1;

    }
    return 0;
}

static enum BT_DEVICE_ID_T bt_media_get_active_device_by_type(uint8_t media_type)
{
    uint8_t i;
    for(i=0;i<BT_DEVICE_NUM;i++)
    {
        if(bt_meida.media_active[i] & media_type)
            return (enum BT_DEVICE_ID_T)i;

    }
    return BT_DEVICE_NUM;
}


static uint8_t bt_media_is_media_active_by_device(uint8_t media_type,enum BT_DEVICE_ID_T device_id)
{

    if(bt_meida.media_active[device_id] & media_type)
        return 1;
    return 0;
}


static uint8_t bt_media_get_current_media(void)
{
    return bt_meida.curr_active_media;
}


static void bt_media_set_current_media(uint8_t media_type)
{
    TRACE("set current media = %x",media_type);
    bt_meida.curr_active_media =  media_type;
}

static void bt_media_clear_current_media(void)
{
    TRACE("clear current media = %x");

    bt_meida.curr_active_media = 0;
}


static void bt_media_clear_all_media_type(void)
{
    uint8_t i;
    for(i=0;i<BT_DEVICE_NUM;i++)
    {
        bt_meida.media_active[i] &= (~BT_STREAM_MEDIA);
    }
}


static void bt_media_clear_media_type(uint8_t media_type,enum BT_DEVICE_ID_T device_id)
{
    bt_meida.media_active[device_id] &= (~media_type);
}


static enum BT_DEVICE_ID_T bt_media_get_active_sbc_device(void)
{
    enum BT_DEVICE_ID_T  device = BT_DEVICE_NUM;
    uint8_t i;
    for(i=0;i<BT_DEVICE_NUM;i++)
    {
        if((bt_meida.media_active[i] & BT_STREAM_SBC)  && (i==bt_meida.media_curr_sbc))
            device = (enum BT_DEVICE_ID_T)i;
    }
    return device;
}


//only used in iamain thread ,can't used in other thread or interrupt
void  bt_media_start(uint8_t stream_type,enum BT_DEVICE_ID_T device_id,AUD_ID_ENUM media_id)
{
#ifdef __BT_ONE_BRING_TWO__
    enum BT_DEVICE_ID_T other_device_id = (device_id == BT_DEVICE_ID_1) ? BT_DEVICE_ID_2 : BT_DEVICE_ID_1;
#endif

    bt_meida.media_active[device_id] |= stream_type;


    TRACE("STREAM MANAGE bt_media_start type= %x,device id = %x,media_id = %x",stream_type,device_id,media_id);
#ifdef __BT_ONE_BRING_TWO__
    TRACE("bt_media_start media_active = %x,%x,curr_active_media = %x",
        bt_meida.media_active[0],bt_meida.media_active[1], bt_meida.curr_active_media);
#else
    TRACE("bt_media_start media_active = %x,curr_active_media = %x",
        bt_meida.media_active[0], bt_meida.curr_active_media);

#endif
    switch(stream_type)
    {
        case BT_STREAM_SBC:
             ////because voice is the highest priority and media report will stop soon
            //// so just store the sbc type
            if(bt_meida.media_curr_sbc == BT_DEVICE_NUM)
                bt_meida.media_curr_sbc = device_id;
#ifdef MEDIA_PLAYER_SUPPORT
            if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                return;
            }
#endif
            if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                ////sbc and voice is all on so set sys freq to 104m
               app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_104M);
               return;
            }
#ifdef __BT_ONE_BRING_TWO__
             //if  another device audio is playing,check the active audio device
             //
            else if(bt_media_is_media_active_by_device(BT_STREAM_SBC,other_device_id))
            {
                //if another device is the active stream do nothing
                if(bt_meida.media_curr_sbc == other_device_id)
                {
                    ///2 device is play sbc,so set sys freq to 104m
                    app_audio_sendrequest_param((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_RESTART, 0, APP_SYSFREQ_104M);
                    return;
                }
                ////if curr active media is not sbc,wrong~~
                if(bt_meida.curr_active_media != BT_STREAM_SBC)
                {
                    ASSERT(0,"curr_active_media is wrong!");
                }
                ///stop the old audio sbc and start the new audio sbc
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
            }
#endif
            else{
                //start audio sbc stream
                app_audio_sendrequest( (uint8_t)APP_BT_STREAM_A2DP_SBC,
                        (uint8_t)(APP_BT_SETTING_SETUP),
                        (uint32_t)(app_bt_device.sample_rate[device_id] & A2D_SBC_IE_SAMP_FREQ_MSK));
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);

                bt_media_set_current_media(BT_STREAM_SBC);
            }
            break;
#ifdef MEDIA_PLAYER_SUPPORT
        case BT_STREAM_MEDIA:
            //first,if the voice is active so  mix "dudu" to the stream
            if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {

                if(bt_media_get_current_media() == BT_STREAM_VOICE)
                {
                    //if call is not active so do media report
                    if(btapp_hfp_get_call_state())
                    {
                        //todo ..mix the "dudu"
                        TRACE("BT_STREAM_VOICE-->app_ring_merge_start\n");
                        app_ring_merge_start();
                        //meida is done here
                        bt_media_clear_all_media_type();

                    }
                    else
                    {
                        TRACE("stop sco and do media report\n");

                        app_audio_sendrequest((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                        app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                        bt_media_set_current_media(BT_STREAM_MEDIA);

                    }
                }
                else if(bt_media_get_current_media() == BT_STREAM_MEDIA)
                {
                    app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                    bt_media_set_current_media(BT_STREAM_MEDIA);
                }
                else
                {
                    ///if voice is active but current is not voice something is unkown
#ifdef __BT_ONE_BRING_TWO__
                    TRACE("STREAM MANAGE voice  active media_active = %x,%x,curr_active_media = %x",
                        bt_meida.media_active[0],bt_meida.media_active[1], bt_meida.curr_active_media);
#else
                    TRACE("STREAM MANAGE voice  active media_active = %x,curr_active_media = %x",
                        bt_meida.media_active[0], bt_meida.curr_active_media);

#endif
                }
            }
            ////if sbc active so
            else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                if(bt_media_get_current_media() == BT_STREAM_SBC)
                {
#ifdef __BT_WARNING_TONE_MERGE_INTO_STREAM_SBC__
                    if (media_id == AUD_ID_BT_WARNING)
                    {
                        TRACE("BT_STREAM_SBC-->app_ring_merge_start\n");
                        app_ring_merge_start();
                        //meida is done here
                        bt_media_clear_all_media_type();
                    }
                    else
#endif
                    {
                        app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                        app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                        bt_media_set_current_media(BT_STREAM_MEDIA);
                    }
                }
                else if(bt_media_get_current_media() == BT_STREAM_MEDIA)
                {
                    app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                }
                else
                {
                    ASSERT(0,"media in sbc  current wrong");
                }
            }
            /// just play the media
            else
            {
                app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_OPEN, media_id);
                bt_media_set_current_media(BT_STREAM_MEDIA);
            }
            break;
#endif
        case BT_STREAM_VOICE:
#ifdef MEDIA_PLAYER_SUPPORT
            if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                //if call is active ,so disable media report
                if(btapp_hfp_get_call_state())
                {
                    //if meida is open ,close media clear all media type
                    if(bt_media_get_current_media() == BT_STREAM_MEDIA)
                    {
                        TRACE("call active so start sco and stop media report\n");
#ifdef __AUDIO_QUEUE_SUPPORT__
                        app_audio_list_clear();
#endif
                        app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);

                    }
                    bt_media_clear_all_media_type();
                    app_audio_sendrequest((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    bt_media_set_current_media(BT_STREAM_VOICE);

                }
                else
                {
                    ////call is not active so media report continue
                }
            }
            else
#endif
            if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                ///if sbc is open  stop sbc
                 if(bt_media_get_current_media() == BT_STREAM_SBC)
                {
                    app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                }
                 ////start voice stream
                app_audio_sendrequest_param((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0, APP_SYSFREQ_104M);
                bt_media_set_current_media(BT_STREAM_VOICE);
            }
            else
            {
                //voice is open already so do nothing
                if(bt_media_get_current_media() == BT_STREAM_VOICE)
                {
                    //do nohting
                }
                else
                {
                    app_audio_sendrequest((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    bt_media_set_current_media(BT_STREAM_VOICE);
                }
            }

            break;
        default:
            ASSERT(0,"bt_media_open ERROR TYPE");
            break;

    }
}


/*
    bt_media_stop function is called to stop media by app or media play callback
    sbc is just stop by a2dp stream suspend or close
    voice is just stop by hfp audio disconnect
    media is stop by media player finished call back

*/
void bt_media_stop(uint8_t stream_type,enum BT_DEVICE_ID_T device_id)
{
    TRACE("STREAM MANAGE bt_media_stop type= %x,device id = %x",stream_type,device_id);
#ifdef __BT_ONE_BRING_TWO__
    TRACE("bt_media_stop media_active = %x,%x,curr_active_media = %x",
        bt_meida.media_active[0],bt_meida.media_active[1], bt_meida.curr_active_media);
#else
    TRACE("bt_media_stop media_active = %x,curr_active_media = %x",
        bt_meida.media_active[0], bt_meida.curr_active_media);

#endif

    switch(stream_type)
    {
        case BT_STREAM_SBC:
            TRACE("SBC STOPPING");
            ////if current media is sbc ,stop the sbc streaming
            bt_media_clear_media_type(stream_type,device_id);
            //if current stream is the stop one ,so stop it
            if(bt_media_get_current_media() == BT_STREAM_SBC && bt_meida.media_curr_sbc  == device_id)
            {
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                bt_media_clear_current_media();
                TRACE("SBC STOPED!");

            }
            if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_device_by_type(BT_STREAM_SBC);
                if(sbc_id < BT_DEVICE_NUM)
                {
                    bt_meida.media_curr_sbc =sbc_id;
                }
            }
            else
            {
                bt_meida.media_curr_sbc = BT_DEVICE_NUM;
            }
            if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                //ASSERT(bt_media_get_current_media() == BT_STREAM_VOICE);
            }
            else if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                //do nothing
            }
            else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_device_by_type(BT_STREAM_SBC);
                if(sbc_id < BT_DEVICE_NUM)
                {
                    app_audio_sendrequest( (uint8_t)APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)(APP_BT_SETTING_SETUP),
                            (uint32_t)(app_bt_device.sample_rate[sbc_id] & A2D_SBC_IE_SAMP_FREQ_MSK));
                    app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    bt_media_set_current_media(BT_STREAM_SBC);
                }
            }

            break;
#ifdef MEDIA_PLAYER_SUPPORT
        case BT_STREAM_MEDIA:

                bt_media_clear_all_media_type();
               // bt_media_set_current_media(0);

            if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                //also have media report so do nothing
            }
            else if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {
                if(bt_media_get_current_media() == BT_STREAM_VOICE)
                {
                    //do nothing
                }
                else if(bt_media_get_current_media() == BT_STREAM_MEDIA)
                {
                    ///media report is end ,so goto voice
                  //  app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                    app_audio_sendrequest((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    bt_media_set_current_media(BT_STREAM_VOICE);

                }

            }
            else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                    ///if another device is also in sbc mode
                    enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();
                    //app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                    app_audio_sendrequest( (uint8_t)APP_BT_STREAM_A2DP_SBC,
                            (uint8_t)(APP_BT_SETTING_SETUP),
                            (uint32_t)(app_bt_device.sample_rate[sbc_id] & A2D_SBC_IE_SAMP_FREQ_MSK));
                    app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                    bt_media_set_current_media(BT_STREAM_SBC);
            }
            else
            {
                //have no meida task,so goto idle
                //app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                bt_media_set_current_media(0);
            }
            break;
#endif
        case BT_STREAM_VOICE:
            bt_media_clear_media_type(stream_type,device_id);
#ifdef MEDIA_PLAYER_SUPPORT
            if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
            {
                if(bt_media_get_current_media() == BT_STREAM_MEDIA)
                {
                    //do nothing
                }
                else
                {
                    ASSERT(0,"if voice and media is all on,media should be the current media");
                }

            }
            else
#endif
            if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
            {


                //another device is in sco mode,so change to the device
//#ifdef __BT_REAL_ONE_BRING_TWO__
                if(hal_get_chip_metal_id() >= HAL_CHIP_METAL_ID_2)
                {
                    enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_device_by_type(BT_STREAM_VOICE);
                    Me_switch_sco(app_bt_device.hf_channel[sbc_id].cmgrHandler.scoConnect->scoHciHandle);
                }
//#endif
                bt_media_set_current_media(BT_STREAM_VOICE);
            }

            else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
            {
                ///if another device is also in sbc mode
                enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                app_audio_sendrequest( (uint8_t)APP_BT_STREAM_A2DP_SBC,
                        (uint8_t)(APP_BT_SETTING_SETUP),
                        (uint32_t)(app_bt_device.sample_rate[sbc_id] & A2D_SBC_IE_SAMP_FREQ_MSK));
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                bt_media_set_current_media(BT_STREAM_SBC);
            }
            else
            {
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_CLOSE, 0);
                bt_media_set_current_media(0);
            }
            break;
        default:
            ASSERT(0,"bt_media_close ERROR TYPE");
            break;
    }
#ifdef __BT_ONE_BRING_TWO__
    TRACE("bt_media_stop end media_active = %x,%x,curr_active_media = %x",
        bt_meida.media_active[0],bt_meida.media_active[1], bt_meida.curr_active_media);
#else
    TRACE("bt_media_stop end media_active = %x,curr_active_media = %x",
        bt_meida.media_active[0], bt_meida.curr_active_media);

#endif
}


void app_media_stop_media(uint8_t stream_type,enum BT_DEVICE_ID_T device_id)
{
#ifdef MEDIA_PLAYER_SUPPORT
    if(bt_media_is_media_active_by_type(BT_STREAM_MEDIA))
    {
#ifdef __AUDIO_QUEUE_SUPPORT__
            ////should have no sbc
            app_audio_list_clear();
#endif
        if(bt_media_get_current_media() == BT_STREAM_MEDIA)
        {

            TRACE("bt_media_switch_to_voice stop the media");
            app_audio_sendrequest((uint8_t)APP_PLAY_BACK_AUDIO, (uint8_t)APP_BT_SETTING_CLOSE, 0);
            bt_media_set_current_media(0);

        }
        bt_media_clear_all_media_type();
        if(bt_media_is_media_active_by_type(BT_STREAM_VOICE))
        {
            app_audio_sendrequest((uint8_t)APP_BT_STREAM_HFP_PCM, (uint8_t)APP_BT_SETTING_OPEN, 0);
            bt_media_set_current_media(BT_STREAM_VOICE);
        }
        else if(bt_media_is_media_active_by_type(BT_STREAM_SBC))
        {
                enum BT_DEVICE_ID_T sbc_id  = bt_media_get_active_sbc_device();

                app_audio_sendrequest( (uint8_t)APP_BT_STREAM_A2DP_SBC,
                        (uint8_t)(APP_BT_SETTING_SETUP),
                        (uint32_t)(app_bt_device.sample_rate[sbc_id] & A2D_SBC_IE_SAMP_FREQ_MSK));
                app_audio_sendrequest((uint8_t)APP_BT_STREAM_A2DP_SBC, (uint8_t)APP_BT_SETTING_OPEN, 0);
                bt_media_set_current_media(BT_STREAM_SBC);
        }
    }
#endif
}

void bt_media_switch_to_voice(uint8_t stream_type,enum BT_DEVICE_ID_T device_id)
{

    TRACE("bt_media_switch_to_voice stream_type= %x,device_id=%x ",stream_type,device_id);
#ifdef __BT_ONE_BRING_TWO__
    TRACE("bt_media_switch_to_voice media_active = %x,%x,curr_active_media = %x",
        bt_meida.media_active[0],bt_meida.media_active[1], bt_meida.curr_active_media);
#else
    TRACE("bt_media_switch_to_voice media_active = %x,curr_active_media = %x",
        bt_meida.media_active[0], bt_meida.curr_active_media);

#endif


    ///already in voice ,so return
    if(bt_media_get_current_media() == BT_STREAM_VOICE)
        return;
    app_media_stop_media(stream_type,device_id);


}


static bool app_audio_manager_init = false;


int app_audio_manager_sendrequest(uint8_t massage_id,uint8_t stream_type, uint8_t device_id, uint8_t aud_id)
{
    uint32_t audevt;
    uint32_t msg0;
    APP_MESSAGE_BLOCK msg;

    if(app_audio_manager_init == false)
        return -1;

    msg.mod_id = APP_MODUAL_AUDIO_MANAGE;
    APP_AUDIO_MANAGER_SET_MESSAGE(audevt, massage_id, stream_type);
    APP_AUDIO_MANAGER_SET_MESSAGE0(msg0,device_id,aud_id);
    msg.msg_body.message_id = audevt;
    msg.msg_body.message_ptr = msg0;
    msg.msg_body.message_Param0 = msg0;
    app_mailbox_put(&msg);

    return 0;
}

static int app_audio_manager_handle_process(APP_MESSAGE_BODY *msg_body)
{
    int nRet = -1;

    APP_AUDIO_MANAGER_MSG_STRUCT aud_manager_msg;

    if(app_audio_manager_init == false)
        return -1;

    APP_AUDIO_MANAGER_GET_ID(msg_body->message_id, aud_manager_msg.id);
    APP_AUDIO_MANAGER_GET_STREAM_TYPE(msg_body->message_id, aud_manager_msg.stream_type);
    APP_AUDIO_MANAGER_GET_DEVICE_ID(msg_body->message_Param0, aud_manager_msg.device_id);
    APP_AUDIO_MANAGER_GET_AUD_ID(msg_body->message_Param0, aud_manager_msg.aud_id);

    switch (aud_manager_msg.id ) {
        case APP_BT_STREAM_MANAGER_START:
        bt_media_start(aud_manager_msg.stream_type,(enum BT_DEVICE_ID_T) aud_manager_msg.device_id,(AUD_ID_ENUM)aud_manager_msg.aud_id);
            break;
        case APP_BT_STREAM_MANAGER_STOP:
            bt_media_stop(aud_manager_msg.stream_type, (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
            break;
        case APP_BT_STREAM_MANAGER_SWITCHTO_SCO:
            bt_media_switch_to_voice(aud_manager_msg.stream_type, (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
            break;
        case APP_BT_STREAM_MANAGER_STOP_MEDIA:
            app_media_stop_media(aud_manager_msg.stream_type, (enum BT_DEVICE_ID_T)aud_manager_msg.device_id);
            break;

        default:
            break;
    }

    return nRet;
}



void app_audio_manager_open(void)
{
    bt_meida.media_curr_sbc = BT_DEVICE_NUM;
    bt_meida.curr_active_media = 0;
    app_set_threadhandle(APP_MODUAL_AUDIO_MANAGE, app_audio_manager_handle_process);
    app_audio_manager_init = true;
}

void app_audio_manager_close(void)
{
    app_set_threadhandle(APP_MODUAL_AUDIO_MANAGE, NULL);
    app_audio_manager_init = false;
}

