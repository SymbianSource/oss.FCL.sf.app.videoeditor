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

  
#ifndef _SIMPLEVIDEOEDITOR_H_
#define _SIMPLEVIDEOEDITOR_H_

//<IBUDSW>
#include <e32base.h>
#include "SimpleVideoEditorExitObserver.h"

// FORWARD DECLARATIONS
class CSimpleVideoEditorImpl;

// CLASS DECLARATION
NONSHARABLE_CLASS(CSimpleVideoEditor) :  public CBase
	{
	public:	// Constructor and destructor

		IMPORT_C static CSimpleVideoEditor* NewL( MSimpleVideoEditorExitObserver& aExitObserver );
		IMPORT_C ~CSimpleVideoEditor();

	public: // New functions

		/** 
		*	Merges the source video clip with another video or image.
		*
		*   @param aSourceFileName - The input video clip
		*   @return -
		*/
		IMPORT_C void Merge( const TDesC& aSourceFileName );

		/** 
		*	Adds sound track to the source video clip.
		*	Removes the original sound track.
		*
		*   @param aSourceFileName - The input video clip
		*   @return -
		*/
		IMPORT_C void ChangeAudio( const TDesC& aSourceFileName );

		/** 
		*	Adds text to the source video clip.
		*
		*   @param aSourceFileName - The input video clip
		*   @return -
		*/
		IMPORT_C void AddText( const TDesC& aSourceFileName );		

		/** 
		*	Cancels ongoing processing.
		*
		*   @param -
		*   @return -
		*/
		IMPORT_C void Cancel();	

	private: // Construct

		CSimpleVideoEditor();
		void ConstructL( MSimpleVideoEditorExitObserver& aExitObserver );

	private: // Data

		CSimpleVideoEditorImpl*	iImpl;
	};

//</IBUDSW>
#endif

// End of file
