/****************************************************************************
 *
 * File:
 *     $Id: avbrowse.h 1329 2009-04-14 15:12:06Z dliechty $
 *     $Product: iAnywhere AV Profiles SDK v2.x $
 *     $Revision: 1329 $
 *
 * Description: This file contains the header file for code that simulates
 *     browsing responses for AVRCP 1.4.
 *
 * Created:     Feb 9, 2009
 *
 * Copyright 2004-2005 Extended Systems, Inc.
 * Portions copyright 2005-2009 iAnywhere Solutions, Inc. All rights
 * reserved. All unpublished rights reserved.
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

#ifndef __AVBROWSE_H_
#define __AVBROWSE_H_

#include "avrcp.h"

#if AVRCP_BROWSING_TARGET == XA_ENABLED

/*---------------------------------------------------------------------------
 * BRWS_HandleBrowseCommand()
 * 
 *     Handle a browse command.
 * 
 * Parameters:
 *
 *      chnl - A connected AVRCP channel.
 *
 *      Parms - AVRCP callback parameters received in a AVRCP
 *          event.
 */
void BRWS_HandleBrowseCommand(AvrcpChannel             *chnl, 
                              const AvrcpCallbackParms *Parms);

#endif /* AVRCP_BROWSING_TARGET == XA_ENABLED */

#endif /* __AVBROWSE_H_ */

