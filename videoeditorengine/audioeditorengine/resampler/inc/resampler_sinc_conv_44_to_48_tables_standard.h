#ifndef __RESAMPLER_SINC_CONV_44_TO_48_TABLES_STANDARD_H__
#define __RESAMPLER_SINC_CONV_44_TO_48_TABLES_STANDARD_H__
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

#define RESAMPLER_44_TO_48_ZERO_CROSSINGS_STANDARD 12

extern const int16 RESAMPLER_44_TO_48_FILTERS_STANDARD[161 * RESAMPLER_44_TO_48_ZERO_CROSSINGS_STANDARD];

#endif // __RESAMPLER_SINC_CONV_44_TO_48_TABLES_STANDARD_H__
