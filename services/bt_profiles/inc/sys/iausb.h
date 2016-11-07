/***************************************************************************
 *
 * File:
 *     $Id: iausb.h 588 2009-01-20 23:25:48Z gladed $
 *     $Product: iAnywhere Blue SDK v3.x $
 *     $Revision: 588 $
 *
 * Description:
 *     This file contains definitions and structures specific 
 *     to the IA HCI USB hardware driver.  
 *
 * Copyright 2005-2008 iAnywhere Solutions, Inc.
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
 * Inc.  This work contains confidential and proprietary information of
 * iAnywhere Solutions, Inc. which is protected by copyright, trade secret,
 * trademark and other intellectual property rights.
 *
 ****************************************************************************/

#ifndef __IA_USB_H
#define __IA_USB_H

#include "hcitrans.h"

/* Function prototypes */
BtStatus IAUSB_Init(TranCallback cb);
BtStatus IAUSB_Shutdown(void);

#endif /* __IA_USB_H */
