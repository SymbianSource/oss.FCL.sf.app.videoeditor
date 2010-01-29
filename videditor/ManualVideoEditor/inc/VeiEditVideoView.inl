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


#ifndef __VEIEDITVIDEOVIEW_INL__
#define __VEIEDITVIDEOVIEW_INL__

public:
		/**
		* Set temporary filename to aFilename
		*
		* @param aFilename
		*/
		inline void TempFilename( HBufC& aFilename ) const { aFilename = *iTempFile; };

		inline void SetSendKey( TBool aState ) { iSendKey = aState; };

		inline void SetConfirmExit()
			{
			iUpdateTemp = ETrue;
			iMovieSavedFlag = EFalse;
			};

		inline TEditorState EditorState() { return iEditorState; };

		/**
		 * No description.
		 *
		 * @return No description.
		 */
		inline TWaitMode WaitMode() { return iWaitMode; };

		inline void SetWaitMode( TWaitMode aMode ) { iWaitMode = aMode; }

		inline CVeiPopup* Popup() { return iPopup; }

		inline CVedMovie* Movie() { return iMovie; }

		inline CVeiEditVideoContainer* Container() { return iContainer; }

#endif //__VEIEDITVIDEOVIEW_INL__
