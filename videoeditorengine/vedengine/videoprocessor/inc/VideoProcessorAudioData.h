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


/* Copyright (c) 2003, Nokia. All rights reserved. */

#ifndef __VIDEOPROCESSORAUDIODATA_H__
#define __VIDEOPROCESSORAUDIODATA_H__


#include <e32base.h>


enum TVideoProcessorAudioType
	{
	EVideoProcessorAudioTypeSilence,
	EVideoProcessorAudioTypeOriginal,
	EVideoProcessorAudioTypeExternal
	};


class TVideoProcessorAudioData
	{
public:
	TTimeIntervalMicroSeconds iStartTime;
	TVideoProcessorAudioType iType;
	CVedAudioClip* iClip;	
	};


#endif // __VIDEOPROCESSORAUDIODATA_H__

