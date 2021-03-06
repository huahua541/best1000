/****************************************************************************
 *
 * File:
 *     $Id: iausb_i.h 588 2009-01-20 23:25:48Z gladed $
 *     $Product: iAnywhere Blue SDK v3.x $
 *     $Revision: 588 $
 *
 * Description:
 *     Implements a Blue SDK transport driver that interfaces with
 *     a Blue-SDK-specific USB Windows driver.
 * 
 * Copyright 2006-2007 iAnywhere Solutions, Inc.  ALL RIGHTS RESERVED.
 *
 * Unpublished Confidential Information of iAnywhere Solutions, Inc.  
 * Do Not Disclose.
 *
 * No part of this work may be used or reproduced in any form or by any 
 * means, or stored in a database or retrieval system, without prior written 
 * permission of iAnywhere Solutions, Inc.
 * 
 * Use of this work is governed by a license granted by iAnywhere Solutions,
 * Inc.  This work contains confidential and proprietary information of
 * iAnywhere Solutions, Inc. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

#ifndef _IAUSB_I__H_
#define _IAUSB_I__H_

#include <windows.h>

#include "hcitrans.h"
#include "btiausb2_if.h"
#include "conmgr.h"

/****************************************************************************
 *
 * Constants
 *
 ****************************************************************************/

/*---------------------------------------------------------------------------
 * BT_IAUSB2_SCO16 constant
 *
 *     Used to enable either 16 bit or 8 bit SCO support.  Set to XA_ENABLED
 *     for 16 bit audio.  Set to XA_DISABLED for 8 bit SCO.
 */
#define BT_IAUSB2_SCO16 XA_ENABLED

/*---------------------------------------------------------------------------
 * BT_IAUSB2_NUM_SCORX constant
 *
 *     Determines the number of USB operations to allocate for incoming
 *     SCO data.  While a SCO channel exists the operations are continuously
 *     recycled to retrieve SCO data from the controller.
 *
 *     This value should not exceed (MAXIMUM_WAIT_OBJECTS - 1).
 */
#define BT_IAUSB2_NUM_SCORX (MAXIMUM_WAIT_OBJECTS - 1)

/****************************************************************************
 *
 * Types
 *
 ****************************************************************************/

/*---------------------------------------------------------------------------
 * UsbHciPacket type
 *
 *     Represents a defragmented HciPacket.  A continuous packet buffer is 
 *     required for sending over USB.
 */

typedef struct _UsbHciPacket
{
   ListEntry node;
   
   /*Original HciPacket structure*/
   HciPacket* pstPacket;

   /*Original HciPacket flags*/
   HciBufFlags flags;

   /*Buffer containing the defragmented HciPacket data.*/
   U8* buffer;

   /*Lenght of the data in buffer.*/
   U32 buffLen;
} UsbHciPacket;

/* End of UsbHciPacket */

/*---------------------------------------------------------------------------
 * IaUsbOpType type
 *
 *     Enumeration of available USB operations.
 */
typedef enum _IaUsbOpType
{
   IAUSB_OP_INVALID = 0,
   IAUSB_OP_WRITE_CMD = IOCTL_BTIAUSB2_WRITE_CMD,
   IAUSB_OP_WRITE_ACL = IOCTL_BTIAUSB2_WRITE_ACL,
   IAUSB_OP_WRITE_SCO = IOCTL_BTIAUSB2_WRITE_SCO,
   IAUSB_OP_READ_EVT = IOCTL_BTIAUSB2_READ_EVT,
   IAUSB_OP_READ_ACL = IOCTL_BTIAUSB2_READ_ACL,
   IAUSB_OP_READ_SCO = IOCTL_BTIAUSB2_READ_SCO,
   IAUSB_OP_SET_ALT = IOCTL_BTIAUSB2_SET_ALT
} IaUsbOpType;

/* End of IaUsbOpType */

typedef struct _IaUsbOp IaUsbOp;
typedef struct _IaUsbPipe IaUsbPipe;

typedef void (*IaUsbOpCompleteFunc)(XaStatus OpStatus, IaUsbOp* Op, IaUsbPipe* Pipe);

/*---------------------------------------------------------------------------
 * IaUsbOp type
 *
 *     Structure which defines an asynchronous USB operation.
 */

struct _IaUsbOp
{
   ListEntry node;

   /*Asyncronous operation ID.*/
   U8 id;

   /*USB operation to perform.*/
   IaUsbOpType type;

   /*Tracking member for async operations.*/
   OVERLAPPED overlap;

   union
   {
      struct
      {
         /*HciPacket for TX operations.  Used
           for IAUSB_OP_WRITE_CMD, IAUSB_OP_WRITE_ACL, 
           and IAUSB_OP_WRITE_SCO operations.
         */
         UsbHciPacket* usbHciPkt;
      } tx;

      struct 
      {
         /*Buffer handle for RX operations.  Used
           for IAUSB_OP_READ_EVT and IAUSB_OP_READ_ACL
           operations.
         */
         RxBuffHandle hBuffer;

         /*Length of the buffer in hBuffer*/
         U32 len;
      } rx;

      /*New alternate setting to apply.  Used for
        IAUSB_OP_SET_ALT operations.
      */
      U8 alt;

      struct
      {
         /*Buffer used for reading SCO data.  Used for
           IAUSB_OP_READ_SCO operations.
         */
         U8 buffer[49];
      } scorx;
   } p;

   /*
   Operation completion function.
   Called after USB driver finishes a 
   transaction.
   */
   IaUsbOpCompleteFunc complete;
};

/* End of IaUsbOp */

/*---------------------------------------------------------------------------
 * IaUsbPipe type
 *
 *     Structure which represents a USB endpoint pipe.
 *
 *     Active pipes:
 *        1. HCI Commands
 *        2. RX ACL
 *        3. TX ACL
 *        4. HCI Events
 *        5. SCO RX (Active only when a SCO channel exists)
 *        6. SCO TX (Active only when a SCO channel exists)
 */

struct _IaUsbPipe
{
   /*Thread which processes the pipe.  Sends
   Op's to device in hFile.*/
   HANDLE hThread;
   volatile LONG bThreadStop;

   /*Handle to send op's to.*/
   HANDLE hFile;

   /*Event to signal a new addition to waitingOps.*/
   HANDLE hNewOpEvent;

   /*Operations waiting to be given to USB device.*/
   ListEntry waitingOps;

   /*Operations given to USB device.*/
   U32 maxPending;
   ListEntry pendingOps;

   /*Operations that have completed but not given to the stack.*/
   ListEntry completedOps;

   /*TRUE if initialized*/
   BOOL bInitialized;

   /*
   Boolean.  If set to >0 stops the flow of OP's.
   OP's can still be queue'd but will not be sent
   to the device.
   */
   volatile LONG bDisabled;

   /*
   Pipes current alternate setting.
   */
   U8 alt;

   /*RX buffer used for staging RX SCO data*/
   RxBuffHandle hRxScoBuffer;
   /*Number of bytes left in order to complete hRxScoBuffer.*/
   U16 rxScoRemaining;
   /*Number of bytes copied to hRxScoBuffer*/
   U16 rxScoCopied;
};

/* End of IaUsbPipe */

/*---------------------------------------------------------------------------
 * IaUsbTranCtx type
 *
 *     Transport context structure.  A separate context must exist for 
 *     each transport instance.
 */
typedef struct _IaUsbTranCtx
{
   /*Pipe used for servicing HCI Command operations*/
   IaUsbPipe pipeCmd;

   /*Pipe used for servicing HCI Event operations*/
   IaUsbPipe pipeEvt;

   /*Pipe used for servicing ACL TX operations*/
   IaUsbPipe pipeAclTx;

   /*Pipe used for servicing ACL RX operations*/
   IaUsbPipe pipeAclRx;

   /*Pipe used for servicing SCO TX operations*/
   IaUsbPipe pipeScoTx;

   /*Pipe used for servicing SCO RX operations*/
   IaUsbPipe pipeScoRx;

   /*File handle to USB driver.*/
   HANDLE hDevice;

   /*Global flag for tracking transport errors.*/
   BOOL bError;
} IaUsbTranCtx;

/* End of IaUsbTranCtx */

/* Public function prototypes */
void IAUSB_SendData(HciPacket *packet);

void IAUSB_BufferAvailable(HciBufFlags buffType);

#endif /*_IAUSB_I__H_*/