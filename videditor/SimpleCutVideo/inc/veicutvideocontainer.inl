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

	
#endif  // __VEICUTVIDEOCONTAINER
