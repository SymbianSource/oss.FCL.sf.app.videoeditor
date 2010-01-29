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


#include "resampler_sinc_conv_two_to_three_tables_standard.h"


const int16 RESAMPLER_TWO_TO_THREE_FILTER1_STANDARD[RESAMPLER_TWO_TO_THREE_ZERO_CROSSINGS_STANDARD] = 
{ 27073, -6673, 3695, -2463, 1771, -1321, 1001, -761, 576, -431, 318, -228, 158, -105, 65, -37 };

const int16 RESAMPLER_TWO_TO_THREE_FILTER2_STANDARD[RESAMPLER_TWO_TO_THREE_ZERO_CROSSINGS_STANDARD] = 
{ 13498, -5293, 3187, -2194, 1602, -1203, 913, -694, 524, -390, 285, -203, 139, -91, 55, -29 };

