#include "cmsis_os.h"
#include "list.h"
#include "string.h"
#include "app_thread.h"
#include "app_key.h"
#include "hal_trace.h"

#define APP_KEY_TRACE(s,...)
//TRACE(s, ##__VA_ARGS__)

#define KEY_EVENT_CNT_LIMIT (3)

typedef struct {
  list_t *key_list;
}APP_KEY_CONFIG;

APP_KEY_CONFIG app_key_conifg = {
    .key_list = NULL
};

typedef struct {
  list_t *key_list;
}APP_KEY_STATUS_LIST;
APP_KEY_STATUS_LIST app_keys_status = {
    .key_list = NULL
};

//APP_KEY_TIMER,the combo keys process
//any key down happen start
//any key key up   happen stop
//check two keys down status, counter increase
//check if two keys down last the threshold time, then do the process function. stop the times

typedef struct {
	enum APP_KEY_CODE_T key_code1;
	enum APP_KEY_CODE_T key_code2;
	uint8_t threshold_1ms_cnt;
} doublekey_type;

static const doublekey_type dkey_clr_nv = {APP_KEY_CODE_FN1,
									APP_KEY_CODE_FN2,
									6};
static uint8_t doublekey_1ms_cnt = 0;
static void app_key_timer_handler(void const *param);
extern void app_nv_clear_bt_scan_mode(void);
enum APP_KEY_EVENT_T app_bt_key_read(enum APP_KEY_CODE_T code);

osTimerDef (APP_KEY_TIMER, app_key_timer_handler);                      // define timers
static osTimerId  g_app_timer;

uint8_t app_doublekey_count(void) {
	return doublekey_1ms_cnt;
}
static void app_key_timer_handler(void const *param)
{
	enum APP_KEY_EVENT_T k1 = app_bt_key_read(dkey_clr_nv.key_code1);
	enum APP_KEY_EVENT_T k2 = app_bt_key_read(dkey_clr_nv.key_code2);
	APP_KEY_TRACE("%s k1 %d, k2 %d, doublekey_1ms_cnt:%d\n",__func__,k1,k2,doublekey_1ms_cnt);

       if((k1==APP_KEY_EVENT_DOWN)&&(k2==APP_KEY_EVENT_DOWN)) {
		   doublekey_1ms_cnt++;
	   } else {
	       osTimerStop(g_app_timer);
		   doublekey_1ms_cnt = 0;
	   }

	   if(doublekey_1ms_cnt >= dkey_clr_nv.threshold_1ms_cnt) {
	   	   osTimerStop(g_app_timer);
		   doublekey_1ms_cnt = 0;
		   app_nv_clear_bt_scan_mode();
	   }
}

static void app_double_key_funcs(APP_KEY_STATUS* key_status)
{
    APP_KEY_TRACE("%s code:%d event:%d,doublekey_1ms_cnt:%d\n",__func__,key_status.code, key_status.event,doublekey_1ms_cnt);
    //check, start,stop
    if(key_status->event == APP_KEY_EVENT_DOWN) {
        if(key_status->code == dkey_clr_nv.key_code1 ||
			key_status->code == dkey_clr_nv.key_code2) {
			osTimerStop(g_app_timer);
            osTimerStart(g_app_timer,1000);
		}
	}else if(key_status->event == APP_KEY_EVENT_UP) {
		osTimerStop(g_app_timer);
		doublekey_1ms_cnt = 0;
	}
}
osPoolDef (app_key_handle_mempool, 20, APP_KEY_HANDLE);
osPoolId   app_key_handle_mempool = NULL;
static uint8_t key_event_cnt = 0;

osPoolDef (app_keys_status_mempool, 10, APP_KEY_STATUS);
osPoolId   app_keys_status_mempool = NULL;

static int key_event_process(uint8_t key_code, uint8_t key_event)
{
    uint32_t app_keyevt;
    APP_MESSAGE_BLOCK msg;

    if (key_event_cnt>KEY_EVENT_CNT_LIMIT){
        return 0;
    }else{
		key_event_cnt++;
	}

    msg.mod_id = APP_MODUAL_KEY;
    APP_KEY_SET_MESSAGE(app_keyevt, key_code, key_event);
    msg.msg_body.message_id = app_keyevt;
    msg.msg_body.message_ptr = (uint32_t)NULL;
    app_mailbox_put(&msg);
    
    return 0;
}

static void app_key_handle_free(void *key_handle)
{
    osPoolFree (app_key_handle_mempool, key_handle);
}

static void app_key_status_free(void *key_status)
{
    osPoolFree (app_keys_status_mempool, key_status);
}

static APP_KEY_HANDLE *app_key_handle_find(const APP_KEY_STATUS *key_status)
{
    APP_KEY_HANDLE *key_handle = NULL;
    list_node_t *node = NULL;
    
    for (node = list_begin(app_key_conifg.key_list); node != list_end(app_key_conifg.key_list); node = list_next(node)) {
        key_handle = (APP_KEY_HANDLE *)list_node(node);
        if ((key_handle->key_status.code == key_status->code)&&(key_handle->key_status.event == key_status->event))
            return key_handle;
    }

    return NULL;
}

static int app_key_handle_process(APP_MESSAGE_BODY *msg_body)
{
    APP_KEY_STATUS *key_status = NULL;
    APP_KEY_HANDLE *key_handle = NULL;

	do {
		key_status = (APP_KEY_STATUS *)osPoolCAlloc (app_keys_status_mempool);
		if(key_status != NULL){
			list_append(app_keys_status.key_list, key_status);
		} else {
            TRACE("%s will remove old keys status\n");
			list_remove(app_keys_status.key_list,list_front(app_keys_status.key_list));
		}
	} while(key_status == NULL);
    APP_KEY_GET_CODE(msg_body->message_id, key_status->code);
    APP_KEY_GET_EVENT(msg_body->message_id, key_status->event);

    APP_KEY_TRACE("%s code:%d event:%d",__func__,key_status->code, key_status->event);

   key_event_cnt--;

    key_handle = app_key_handle_find(key_status);

    if (key_handle != NULL)
        ((APP_KEY_HANDLE_CB_T)key_handle->function)(key_status,key_handle->param);

	app_double_key_funcs(key_status);

    return 0;
}

int app_key_handle_registration(const APP_KEY_HANDLE *key_handle)
{
    APP_KEY_HANDLE *dest_key_handle = NULL;
    APP_KEY_TRACE("%s",__func__);
    dest_key_handle = app_key_handle_find(&(key_handle->key_status));
    
    APP_KEY_TRACE("%s dest handle:0x%x",__func__,dest_key_handle);
    if (dest_key_handle == NULL){
        dest_key_handle = (APP_KEY_HANDLE *)osPoolCAlloc (app_key_handle_mempool);
        APP_KEY_TRACE("%s malloc:0x%x",__func__,dest_key_handle);
        list_append(app_key_conifg.key_list, dest_key_handle);
    }
    if (dest_key_handle == NULL)
        return -1;
    APP_KEY_TRACE("%s set handle:0x%x code:%d event:%d function:%x",__func__,dest_key_handle, key_handle->key_status.code, key_handle->key_status.event, key_handle->function);
    dest_key_handle->key_status.code = key_handle->key_status.code;
    dest_key_handle->key_status.event = key_handle->key_status.event;
    dest_key_handle->string = key_handle->string;
    dest_key_handle->function = key_handle->function;
    dest_key_handle->param = key_handle->param;;

    return 0;
}

void app_key_handle_clear(void)
{
    list_clear(app_key_conifg.key_list);
}

int app_key_open(int checkPwrKey)
{   
    APP_KEY_TRACE("%s %x",__func__, app_key_conifg.key_list);
	doublekey_1ms_cnt = 0;

    if (app_key_conifg.key_list == NULL)
        app_key_conifg.key_list = list_new(app_key_handle_free);
    
    if (app_key_handle_mempool == NULL)
        app_key_handle_mempool = osPoolCreate(osPool(app_key_handle_mempool));

    if (app_keys_status.key_list == NULL)
        app_keys_status.key_list = list_new(app_key_status_free);
    if (app_keys_status_mempool == NULL)
        app_keys_status_mempool = osPoolCreate(osPool(app_keys_status_mempool));
    
    app_set_threadhandle(APP_MODUAL_KEY, app_key_handle_process);

	g_app_timer = osTimerCreate(osTimer(APP_KEY_TIMER),osTimerPeriodic,NULL);

    return hal_key_open(checkPwrKey, key_event_process);
}

int app_key_close(void)
{   
    hal_key_close();
    if (app_key_conifg.key_list != NULL)
        list_free(app_key_conifg.key_list);
    if (app_keys_status.key_list != NULL)
        list_free(app_keys_status.key_list);
    app_set_threadhandle(APP_MODUAL_KEY, NULL);
    return 0;
}


