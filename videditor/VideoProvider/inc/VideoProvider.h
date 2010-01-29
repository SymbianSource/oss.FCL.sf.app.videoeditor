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
* Declares Base service API for all providers to implement in order to
* offer services to Application Interworking Framework.
*
*/

  
#ifndef _VIDEOPROVIDER_H
#define _VIDEOPROVIDER_H

//<IBUDSW>

#include <AiwServiceIfMenu.h>
#include <apparc.h>
#include <ConeResLoader.h> 
#include <AknServerApp.h> 
#include "SimpleVideoEditor.h"


// FORWARD DECLARATIONS
class MAiwNotifyCallback;
class CAknOpenFileService;
class TDataType;


class CVideoProvider :	public CAiwServiceIfMenu, 
						public MAknServerAppExitObserver,
						public MSimpleVideoEditorExitObserver
	{
	public:	
		/** New factory method
		*
		*	
		*	@param -
		*	@return - pointer to a new instance of CEditorProvider
		*/ 
		static CVideoProvider* NewL();

		/** Destructor
		*	
		*	@param -
		*	@return - 
		*/ 
		~CVideoProvider();

	public:		
		/** InitialiseL
		*
		*   @see CAiwServiceIfBase
		*/ 
		virtual void InitialiseL (
			MAiwNotifyCallback &			aFrameworkCallback,
			const RCriteriaArray &			aInterest
			);

		/** HandleServiceCmdL
		*
		*   @see CAiwServiceIfBase
		*/ 
		virtual void HandleServiceCmdL (
		    const TInt &                    aCmdId,
			const CAiwGenericParamList &    aInParamList,
			CAiwGenericParamList &          aOutParamList,
			TUint                           aCmdOptions = 0,
			const MAiwNotifyCallback *      aCallback = NULL
			);

		/** InitializeMenuPaneL
		*
		*   @see CAiwServiceIfMenu
		*/ 
		virtual void InitializeMenuPaneL (
		    CAiwMenuPane &                  aMenuPane,
		    TInt                            aIndex,
		    TInt                            aCascadeId,
		    const CAiwGenericParamList &    aInParamList
		    );

		/** HandleMenuCmdL
		*
		*   @see CAiwServiceIfMenu
		*/ 
		virtual void HandleMenuCmdL (
		    TInt                            aMenuCmdId,
		    const CAiwGenericParamList &	aInParamList,
		    CAiwGenericParamList &          aOutParamList,
		    TUint                           aCmdOptions = 0,
		    const MAiwNotifyCallback *      aCallback = NULL
		    );

		/** HandleServerAppExit
		*
		*   @see MAknServerAppExitObserver
		*/ 
		virtual void HandleServerAppExit (TInt aReason);

		/** HandleSimpleVideoEditorExit
		*
		*   @see MSimpleVideoEditorExitObserver
		*/ 
		virtual void HandleSimpleVideoEditorExit (TInt aReason, const TDesC& aResultFileName);

	private:
		CVideoProvider();

	private:
		/** 
		*   @param aMenuCmdId
		*   @param aFileName
		*   @param CAiwGenericParamList
		*   @return -
		*/
	    void LaunchEditorL( 
	    	TInt aMenuCmdId, 
			const TDesC & 					aFileName,
		    const CAiwGenericParamList &	aInParamList
		    );

		/** HandleCmdsL
		*
		*   Handle menu and service commands
		*
		*   @see HandleMenuCmdL
		*   @see HandleServiceCmdL
		*/ 
		void HandleCmdsL (
		    TInt                            aMenuCmdId,
		    const CAiwGenericParamList &	aInParamList,
		    CAiwGenericParamList &          aOutParamList,
		    TUint                           aCmdOptions,
		    const MAiwNotifyCallback *      aCallback
		    );

		TBool IsSupportedVideoFile (const TDesC& aDataType) const;
		TBool IsSupportedAudioFile (const TDesC& aDataType) const;
		TBool IsSupportedImageFile (const TDesC& aDataType) const;
		
		void FinalizeL (const TDesC& aFileName);
		void CloseFsSession();

	private: // Data

		RFs 						iSharableFS;
		TFileName					iResourceFile;
		RConeResourceLoader 		iResLoader;
		TBool						iResFileIsLoaded;
		CAknOpenFileService *		iOpenFileService;
		const MAiwNotifyCallback*	iAiwNotifyCallback;
		#if defined(INCLUDE_SIMPLE_VIDEO_EDITOR)
		CSimpleVideoEditor* 		iSimpleVideoEditor;
		#endif
		CAiwGenericParamList*		iInParamList;
		CAiwGenericParamList*		iOutParamList;

		// Time stamp of the original file. If there are multiple files,
		// the most recent.
		TTime						iOriginalTimeStamp;
		
		/// Media Gallery Albums support. 
		/// List of albums where the source file(s) belong(s) to.
		RArray<TInt>				iSourceMGAlbumIdList;
	};

//</IBUDSW>
#endif

// End of file
