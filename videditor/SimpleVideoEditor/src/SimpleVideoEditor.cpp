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
* Class implementation.
*
*/


// INCLUDES
#include "VideoEditorCommon.h"
#include "SimpleVideoEditor.h"
#include "SimpleVideoEditorImpl.h"
#include "VideoEditorDebugUtils.h"


// MEMBER FUNCTIONS

//=============================================================================
EXPORT_C CSimpleVideoEditor* CSimpleVideoEditor::NewL( MSimpleVideoEditorExitObserver& aExitObserver )
	{
	CSimpleVideoEditor* self = new (ELeave)	CSimpleVideoEditor();
	CleanupStack::PushL (self);
	self->ConstructL( aExitObserver );
	CleanupStack::Pop (self);
	return self;
	}

//=============================================================================
EXPORT_C CSimpleVideoEditor::~CSimpleVideoEditor()
	{
	delete iImpl;
	}

//=============================================================================
EXPORT_C void CSimpleVideoEditor::Merge( const TDesC& aSourceFileName )
	{
	iImpl->StartMerge(aSourceFileName);
	} 

//=============================================================================
EXPORT_C void CSimpleVideoEditor::ChangeAudio( const TDesC& aSourceFileName )
	{
	iImpl->StartChangeAudio(aSourceFileName);
	}

//=============================================================================
EXPORT_C void CSimpleVideoEditor::AddText( const TDesC& aSourceFileName )
	{
	iImpl->StartAddText(aSourceFileName);
	}

//=============================================================================
EXPORT_C void CSimpleVideoEditor::Cancel()
	{
	iImpl->CancelMovieProcessing();
	}

//=============================================================================
void CSimpleVideoEditor::ConstructL(MSimpleVideoEditorExitObserver& aExitObserver)
	{
	iImpl = CSimpleVideoEditorImpl::NewL(aExitObserver);
	}

//=============================================================================
CSimpleVideoEditor::CSimpleVideoEditor() 
	{
	}


	
// End of file
