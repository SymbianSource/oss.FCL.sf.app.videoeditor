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


#include "resampler_sinc_conv_one_to_three_tables_standard.h"

const int16 RESAMPLER_ONE_TO_THREE_FILTER1_STANDARD[RESAMPLER_ONE_TO_THREE_ZERO_CROSSINGS_STANDARD] = 
{ 27074, -6676, 3701, -2471, 1782, -1332, 1013, -774, 589, -444, 329, -238, 167, -113, 71, -41 };

const int16 RESAMPLER_ONE_TO_THREE_FILTER2_STANDARD[RESAMPLER_ONE_TO_THREE_ZERO_CROSSINGS_STANDARD] = 
{ 13500, -5297, 3194, -2203, 1612, -1215, 926, -707, 537, -403, 296, -213, 148, -97, 60, -33 };
