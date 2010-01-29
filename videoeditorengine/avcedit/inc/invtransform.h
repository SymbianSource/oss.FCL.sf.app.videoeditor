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
*
*/


#ifndef _INVTRANSFORM_H_
#define _INVTRANSFORM_H_


#define ITR_DEQUANT_BITS   6
#define ITR_DEQUANT_ROUND  (1<<(ITR_DEQUANT_BITS-1))


void itrIDCTdequant4x4(int src[4][4], int dest[4][4], const int *dequantPtr,
                       int qp_per, int isDc, int dcValue);

void itrIHadaDequant4x4(int src[4][4], int dest[4][4], int deqc);

void itrIDCTdequant2x2(int src[2][2], int dest[2][2], int deqc);


#endif
