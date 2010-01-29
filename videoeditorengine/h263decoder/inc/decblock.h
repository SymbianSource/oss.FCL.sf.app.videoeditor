/*
* Copyright (c) 2010 Ixonos Plc.
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - Initial contribution
*
* Contributors:
* Ixonos Plc
*
* Description:  
* Header for block layer decoding module.
*
*/


#ifndef _DECBLOCK_H_
#define _DECBLOCK_H_

#include "h263dext.h"

/*
 * Defines
 */

// unify error codes
#define DBL_OK H263D_OK
#define DBL_ERR H263D_ERROR


/*
 * Function prototypes
 */

int dblFree(void);

int dblLoad(void);

void dblIdctAndDequant(int *block, int quant, int skip);

void dblIdct(int *block);

#endif
// End of File
