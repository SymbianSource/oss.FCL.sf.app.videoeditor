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
* Inverse Discrete Cosine Transform (IDCT) module header.
*
*/


#ifndef _IDCT_H_
#define _IDCT_H_

/*
 * Function prototypes
 */

void init_idct(void);
void idct(int *block);


#endif
// End of File
