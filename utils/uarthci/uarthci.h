/***
 * uart hci
 * YuLongWang @2015
*/

#ifndef UARTHCI_H
#define UARTHCI_H

#if defined(__cplusplus)
extern "C" {
#endif

void IAHCI_BufferAvai(void *packet);
void IAHCI_Open(void);
void IAHCI_Poll(void);
void IAHCI_LockBuffer(void);
void IAHCI_UNLockBuffer(void);
void IAHCI_SendData(void *packet);
void IAHCI_SCO_Data_Start(void);
void IAHCI_SCO_Data_Stop(void);
void uartrxtx(void const *argument);

#if defined(__cplusplus)
}
#endif

#endif /* UARTHCI_H */
