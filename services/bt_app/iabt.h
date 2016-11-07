/***
 * iabt.h
 */

#ifndef IABT_H
#define IABT_H
#ifdef __cplusplus
extern "C" {
#endif
void IabtInit(void);
void IabtThread(void const *argument);
unsigned char *randaddrgen_get_bt_addr(void);
unsigned char *randaddrgen_get_ble_addr(void);
const char *randaddrgen_get_btd_localname(void);
#ifdef __cplusplus
}
#endif
#endif /* IABT_H */
