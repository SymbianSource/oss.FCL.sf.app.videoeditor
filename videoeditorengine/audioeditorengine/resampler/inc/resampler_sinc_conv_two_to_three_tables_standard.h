#ifndef __RESAMPLER_SINC_CONV_TWO_TO_THREE_TABLES_STANDARD_H__
#define __RESAMPLER_SINC_CONV_TWO_TO_THREE_TABLES_STANDARD_H__
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



#include "resampler_data_types.h"


#define RESAMPLER_TWO_TO_THREE_ZERO_CROSSINGS_STANDARD 16

extern const int16 RESAMPLER_TWO_TO_THREE_FILTER1_STANDARD[RESAMPLER_TWO_TO_THREE_ZERO_CROSSINGS_STANDARD];
extern const int16 RESAMPLER_TWO_TO_THREE_FILTER2_STANDARD[RESAMPLER_TWO_TO_THREE_ZERO_CROSSINGS_STANDARD];

#endif // __RESAMPLER_SINC_CONV_TWO_TO_THREE_TABLES_STANDARD_H__
