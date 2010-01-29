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


/*
*   File:       ExtProgressAnimationControl.cpp
*   Created:    17-10-2005
*   Author:     
*               
*/

#include <gdi.h>
#include <aknbitmapanimation.h>
#include <barsread.h>
#include <bmpancli.h>
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <aknutils.h>
#include <VideoEditorUiComponents.rsg>

#include "ExtProgressAnimationControl.h"
#include "videoeditorcommon.h"

//const TInt KCropFastKeyTimerDelayInMicroseconds = 10000;
//const TInt KDefaultFastKeyTimerIntervalInMicroseconds = 10000;

//	CONSTANTS
//const TReal KIconHeightFrac = 0.25;
//const TInt	KStartOffsetX = 10;

//=============================================================================
CExtProgressAnimationControl * CExtProgressAnimationControl::NewL (
	const TRect &		aRect,
	const CCoeControl *	aParent
	)
{
	CExtProgressAnimationControl * self = new (ELeave) CExtProgressAnimationControl;
	CleanupStack::PushL(self);
	self->ConstructL (aRect, aParent);
	CleanupStack::Pop( self ); 
	return self;
}

//=============================================================================
CExtProgressAnimationControl::~CExtProgressAnimationControl()
{  
	StopAnimation();

	delete iAnimation;
	delete iBgContext;
}

//=============================================================================
void CExtProgressAnimationControl::SetAnimationResourceId(const TInt &aResourceId)
{	
	switch (aResourceId)
		{
		case VideoEditor::EAnimationMerging:
			{			
			iAnimationResourceId = R_VED_MERGING_NOTE_ANIMATION;
			break;
			}
		case VideoEditor::EAnimationChangeAudio:
			{			
			iAnimationResourceId = R_VED_MERGING_AUDIO_NOTE_ANIMATION;
			break;
			}
		case VideoEditor::EAnimationAddText:
			{			
			iAnimationResourceId = R_VED_ADDING_TEXT_NOTE_ANIMATION;
			break;
			}	
		case VideoEditor::EAnimationCut:
			{			
			iAnimationResourceId = R_VED_CUTTING_NOTE_ANIMATION;
			break;
			}				
		default:
			{
			iAnimationResourceId = R_VED_MERGING_NOTE_ANIMATION;
			break;
			}
	}
	
}

//=============================================================================
CExtProgressAnimationControl::CExtProgressAnimationControl() : iBorderWidth(2)
{

}

//=============================================================================
void CExtProgressAnimationControl::ConstructL (
	const TRect &		aRect,
	const CCoeControl *	aParent
	)
{
	SetContainerWindowL( *aParent );
	SetRect(aRect);

	//	Activate control
	ActivateL();     
}

//=============================================================================
void CExtProgressAnimationControl::SizeChanged()
{
	TRect rect = Rect();
	if ( iBgContext )
	{
		iBgContext->SetRect( rect );
	}
}

//=============================================================================
void CExtProgressAnimationControl::Draw (const TRect& aRect) const
{
	CWindowGc& gc = SystemGc();

	// draw skin background
	MAknsSkinInstance* skin = AknsUtils::SkinInstance();
	MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );
	AknsDrawUtils::Background( skin, cc, this, SystemGc(), aRect );
}

//============================================================================= 
TTypeUid::Ptr CExtProgressAnimationControl::MopSupplyObject( TTypeUid aId )
{
	if ( aId.iUid == MAknsControlContext::ETypeId && iBgContext )
	{
		return MAknsControlContext::SupplyMopObject( aId, iBgContext );
	}
	return CCoeControl::MopSupplyObject( aId );
}

//============================================================================= 
void CExtProgressAnimationControl::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
    {
    }

//============================================================================= 
void CExtProgressAnimationControl::StartAnimationL(TInt aFrameIntervalInMilliSeconds )
	{

	if ( iAnimation )
		{
		delete iAnimation;
		iAnimation = 0;
		}	

	iAnimation = CAknBitmapAnimation::NewL();

	TResourceReader reader;
	iCoeEnv->CreateResourceReaderLC( reader, iAnimationResourceId );
	iAnimation->ConstructFromResourceL( reader );
	TRect rect( Rect() );
	rect.Move( iBorderWidth, iBorderWidth );
	rect.Resize( -iBorderWidth*2, -iBorderWidth );
	iAnimation->SetRect( rect );
	iAnimation->SetContainerWindowL( *this );
	iAnimationOn = ETrue;
	iAnimation->StartAnimationL();
	CleanupStack::PopAndDestroy(); //reader
	CBitmapAnimClientData* animClientData = iAnimation->BitmapAnimData();
	iAnimationSpeedInMilliSeconds = animClientData->FrameIntervalInMilliSeconds();

	if ( aFrameIntervalInMilliSeconds > 0 )
		{
		SetFrameIntervalL( aFrameIntervalInMilliSeconds );
		}
	DrawDeferred();
	}

//============================================================================= 
void CExtProgressAnimationControl::StopAnimation()
	{
	if ( iAnimationOn )
		{
		iAnimation->CancelAnimation();
		}
	iAnimationOn = EFalse;
	DrawDeferred();
	}

//============================================================================= 
void CExtProgressAnimationControl::SetFrameIntervalL(TInt aFrameIntervalInMilliSeconds)
	{
	iAnimationSpeedInMilliSeconds+=aFrameIntervalInMilliSeconds;
	iAnimation->SetFrameIntervalL( iAnimationSpeedInMilliSeconds );
	}	

// End of File
