/* rbplay header */
/* playback control & rockbox codec porting & codec thread */
#ifndef RBPLAY_HEADER
#define RBPLAY_HEADER

/* ---- APIs ---- */
typedef enum _RB_CTRL_CMD_T {
    RB_CTRL_CMD_NONE = 0,
    RB_CTRL_CMD_CODEC_INIT,
    RB_CTRL_CMD_CODEC_RUN,
    RB_CTRL_CMD_CODEC_PAUSE,
    RB_CTRL_CMD_CODEC_STOP,
    RB_CTRL_CMD_CODEC_DEINIT,

    RB_CTRL_CMD_CODEC_SEEK, /* 5 */

    RB_CTRL_CMD_CODEC_MAX,
} RB_CTRL_CMD_T;

typedef enum _RB_ST_EVT_T {
    RB_ST_EVT_NONE = 0,
    RB_ST_EVT_CODEC_ERR,

    RB_ST_EVT_MAX,
} RB_ST_EVT_T;

typedef void (*RB_ST_Callback)(RB_ST_EVT_T evt, void *param);

void rb_play_init(RB_ST_Callback cb);
void rb_play_prepare_file(const char* file_path);
void rb_play_prepare_mem(const char* mem_address);
void rb_play_prepare_file_raw(const char* file_path, unsigned int len, int type);
void rb_play_prepare_mem_raw(const char *mem_address, unsigned int len, int type);
void rb_play_start(void);
void rb_play_pause(void);
void rb_play_stop(void);
void rb_play_deinit(void);

#endif /* RBPLAY_HEADER */
