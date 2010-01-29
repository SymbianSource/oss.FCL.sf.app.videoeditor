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


#include "resampler_sinc_conv_three_to_one_tables_standard.h"


const int16 RESAMPLER_THREE_TO_ONE_FILTERS_STANDARD[ RESAMPLER_THREE_TO_ONE_COEFF_COUNT_STANDARD ] =
{
     9829,  8430,  4942,  1066, -1512, -2041,  -991,   441,  1173,   874,
        0,  -691,  -730,  -214,   369,   572,   306,  -147,  -415,  -323,
        0,   272,   293,    87,  -153,  -238,  -128,    62,   173,   135,
        0,  -112,  -119,   -35,    61,    93,    49,   -23,   -64,   -49,
        0,    39,    40,    11,   -19,   -28,   -14,     6,    17,    12,
        0,    -8,    -8,    -2,     3,     4,     2
};          

