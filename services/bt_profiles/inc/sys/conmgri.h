/****************************************************************************
 *
 * File:
 *     $Id: conmgri.h 588 2009-01-20 23:25:48Z gladed $
 *     $Product: iAnywhere Blue SDK v3.x $
 *     $Revision: 588 $
 *
 * Description:
 *     Private definitions for the Connection Manager.  
 *
 * Created:
 *     August 5, 2005
 *
 * Copyright 2005 Extended Systems, Inc.
 * Portions copyright 2005-2008 iAnywhere Solutions, Inc.
 * All rights reserved. All unpublished rights reserved.
 *
 * Unpublished Confidential Information of iAnywhere Solutions, Inc.  
 * Do Not Disclose.
 *
 * No part of this work may be used or reproduced in any form or by any 
 * means, or stored in a database or retrieval system, without prior written 
 * permission of iAnywhere Solutions, Inc.
 *
 * Use of this work is governed by a license granted by iAnywhere Solutions,
 * Inc. This work contains confidential and proprietary information of
 * iAnywhere Solutions, Inc. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

#ifndef __CONMGRI_H_
#define __CONMGRI_H_

#include "conmgr.h"

/****************************************************************************
 *
 * Data Structures
 *
 ****************************************************************************/

/* The BtCmgrContext structure contains the global context of the 
 * connection manager. 
 */
typedef struct _BtCmgrContext {
    BtHandler            globHandler;
    ListEntry            handlerList;
    U8                   linkCount;
    BOOL                 roleSwitch;

#if NUM_SCO_CONNS > 0
    BtHandler            scoHandler;
    BOOL                 scoRequest;
    CmgrAudioParms       scoParms;
    CmgrAudioParms       scoDefaultParms;
    BtScoTxParms         scoCustomParms;
    EvmTimer             scoRetryTimer;
#endif
    BtRemoteDevice      *cancelRemDev;
} BtCmgrContext;

#endif /* __CONMGRI_H_ */

