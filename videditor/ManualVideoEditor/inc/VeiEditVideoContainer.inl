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


#ifndef __VEIEDITVIDEOCONTAINER_INL__
#define __VEIEDITVIDEOCONTAINER_INL__

public:
  		inline TSelectionMode SelectionMode() { return iSelectionMode; };

		inline void SetVideoCursorPosition( TUint aPosition ) { iVideoCursorPos = aPosition; };

		inline TCursorLocation CursorLocation() { return iCursorLocation; };

		inline TTimeIntervalMicroSeconds RecordedAudioStartTime() const { return iRecordedAudioStartTime; };

		inline TTimeIntervalMicroSeconds RecordedAudioDuration() const { return iRecordedAudioDuration; };

		inline void SetRecordedAudioStartTime( TTimeIntervalMicroSeconds aStartTime )
			{
			iRecordedAudioStartTime = aStartTime;
			}

		inline TPreviewState PreviewState() { return iPreviewState; };

		inline void SetRecordedAudio( TBool aRecordedAudio )
			{
			iRecordedAudio = aRecordedAudio;
			}

		/**
		* Return value of slow motion in container.
		*
		* @return TInt
		*/
		inline TInt SlowMotionValue() { return iSlowMotionValue; };

#endif //__VEIEDITVIDEOCONTAINER_INL__
