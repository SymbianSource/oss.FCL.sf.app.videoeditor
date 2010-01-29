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


#ifndef __VEICUTVIDEOCONTAINER_INL__
#define __VEICUTVIDEOCONTAINER_INL__

public:
		/**
		 * Returns total length of the video clip.
		 *
		 * @return  total length
		 */
		inline TTimeIntervalMicroSeconds TotalLength() { return iDuration; };

		/**
		 * Returns the player state.
		 *
		 * @return  player state
		 */
		inline TCutVideoState State() { return iState; };
		/**
		 * Returns current volume level.
		 *
		 * @return	volume level
		 */
		inline TInt Volume() const { return iInternalVolume; };

		/**
		 * Returns minimum volume level.
		 *
		 * @return	min volume level
		 */
		inline TInt MinVolume() const { return KMinCutVideoVolumeLevel; };

		/**
		 * Returns maximum volume level.
		 *
		 * @return	max volume level
		 */
		inline TInt MaxVolume() const { return KMaxVolumeLevel; };
		/**
		 * Change volume level. Changes current volume level to given level.
		 *
		 * @param aVolumeLevel		new volume level 
		 */
		inline void SetVolume( TInt aVolumeLevel ) { iInternalVolume = aVolumeLevel; };
	
#endif  // __VEICUTVIDEOCONTAINER
