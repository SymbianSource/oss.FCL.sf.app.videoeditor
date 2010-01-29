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
* Navipane control for SVE.
*
*/


#include <aknnavide.h>
#include <Eikspane.h>
#include <stringloader.h> 
#include <AknVolumePopup.h>

#include "veinavipanecontrol.h"
#include "VeiTimeLabelNavi.h"
#include "mveinavipanecontrolobserver.h" 
#include <VedSimpleCutVideo.rsg>


// ======== MEMBER FUNCTIONS ========

// ---------------------------------------------------------------------------
// CVeiNaviPaneControl
// ---------------------------------------------------------------------------
//
CVeiNaviPaneControl::CVeiNaviPaneControl( CEikStatusPane* aStatusPane ) :
	iStatusPane( aStatusPane )
    {
    }


// ---------------------------------------------------------------------------
// ConstructL
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::ConstructL()
    {
    if( iStatusPane )
        {        
        iNaviPane = (CAknNavigationControlContainer*) iStatusPane->ControlL(
            TUid::Uid(EEikStatusPaneUidNavi) );
        iTimeNavi = CreateTimeLabelNaviL();
        iVolumeNavi = iNaviPane->CreateVolumeIndicatorL(
			R_AVKON_NAVI_PANE_VOLUME_INDICATOR );
		iVolumeNavi->SetObserver( this );
        iVolumeHider = CPeriodic::NewL( CActive::EPriorityStandard );        
        }
    }

// ---------------------------------------------------------------------------
// NewL
// ---------------------------------------------------------------------------
//
CVeiNaviPaneControl* CVeiNaviPaneControl::NewL( CEikStatusPane* aStatusPane )
    {
    CVeiNaviPaneControl* self = 
    	new( ELeave ) CVeiNaviPaneControl( aStatusPane );
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }


// ---------------------------------------------------------------------------
// ~CVeiNaviPaneControl
// ---------------------------------------------------------------------------
//
CVeiNaviPaneControl::~CVeiNaviPaneControl()
    {
    iObserver = NULL;
    delete iTimeNavi;
    delete iVolumeNavi;
    delete iVolumeHider;
    }

// ---------------------------------------------------------------------------
// DrawTimeNaviL
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::DrawTimeNaviL( TTime aElapsed, TTime aTotal )
    {
	// check if time is over 59min:59s, then use 00h:00m:00s
	TBool useLong( aTotal.DateTime().Hour() );

	HBufC* dateFormatString = CCoeEnv::Static()->AllocReadResourceLC(
	    useLong ?  R_QTN_TIME_DURAT_LONG_WITH_ZERO : 
	        R_QTN_TIME_DURAT_MIN_SEC_WITH_ZERO ); 
	        
    const TInt bufLength(16);
    TBuf<bufLength> elapsedBuf;			
	aElapsed.FormatL(elapsedBuf, *dateFormatString);
    TBuf<bufLength> totalBuf;
	aTotal.FormatL(totalBuf, *dateFormatString);
	CleanupStack::PopAndDestroy(dateFormatString);
       
	CDesCArrayFlat* strings = new (ELeave) CDesCArrayFlat(2);
	CleanupStack::PushL(strings);
	strings->AppendL(elapsedBuf);
	strings->AppendL(totalBuf);
	HBufC* stringholder = StringLoader::LoadL(R_VEI_NAVI_TIME, *strings);
	CleanupStack::PopAndDestroy(strings);

	
	CleanupStack::PushL(stringholder);	

	GetTimeLabelControl()->SetLabelL(*stringholder);

	CleanupStack::PopAndDestroy(stringholder);


    
    if( iNaviPane->Top() != iVolumeNavi )
        {        
        iNaviPane->PushL( *iTimeNavi );
        }
    
	/* Prevent the screen light dimming. */
	if (aElapsed.DateTime().Second() == 0 || 
	    aElapsed.DateTime().Second() == 15 || 
	    aElapsed.DateTime().Second() == 30 || 
	    aElapsed.DateTime().Second() == 45)
		{
		User::ResetInactivityTime();
		}        
    }

// ---------------------------------------------------------------------------
// SetPauseIconVisibilityL
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::SetPauseIconVisibilityL( TBool aVisible )
    {
	GetTimeLabelControl()->SetPauseIconVisibilityL( aVisible );
	if( iNaviPane->Top() != iVolumeNavi )
        {  
	    iNaviPane->PushL( *iTimeNavi );    
        }
    }

// ---------------------------------------------------------------------------
// SetVolumeIconVisibilityL
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::SetVolumeIconVisibilityL( TBool aVisible )
    {
    GetTimeLabelControl()->SetVolumeIconVisibilityL( aVisible );
	if( iNaviPane->Top() != iVolumeNavi )
        {  
	    iNaviPane->PushL( *iTimeNavi );    
        }
    }
// ---------------------------------------------------------------------------
// ShowVolumeLabelL
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::ShowVolumeLabelL( TInt aVolume )
    {	    
	GetTimeLabelControl()->SetVolumeIconVisibilityL( ETrue );

  	// Remove volume slider from navi control after 2 sec
	iVolumeHider->Cancel();
	const TInt twoSeconds(1900000);
	iVolumeHider->Start(twoSeconds, twoSeconds, 
	    TCallBack( CVeiNaviPaneControl::HideVolumeCallbackL, this) );    

	if (aVolume == 0) 
		{
        // Hide volume icon when volume is set to 0
		GetTimeLabelControl()->SetVolumeIconVisibilityL( EFalse );    
		}
	if(GetVolumeControl()->Value()!= aVolume) 
		{
		//this is in case we change volume with other control than the popup
		GetVolumeControl()->SetValue(aVolume);
		}	
	CAknVolumePopup* popup = static_cast<CAknVolumePopup*> ( GetVolumeControl()->Parent() );
	TRAP_IGNORE( popup->ShowVolumePopupL() );

  	}

// ---------------------------------------------------------------------------
// HandleResourceChange
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::HandleResourceChange( TInt aType )
    {
    if( iTimeNavi && iVolumeNavi )
        {        
        iTimeNavi->DecoratedControl()->HandleResourceChange( aType );
        iVolumeNavi->DecoratedControl()->HandleResourceChange( aType );    
        }
    }


// ---------------------------------------------------------------------------
// HandleResourceChange from MCoeControlObserver
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::HandleControlEventL( 
    CCoeControl* /* aControl */,TCoeEvent /* aEventType */ )
    {
    CAknNavigationDecorator* cntr = iNaviPane->Top( );    
    if ( iVolumeNavi && cntr == iVolumeNavi )
        {
        if( iObserver )
            {
            iObserver->SetVolumeLevelL( GetVolumeControl()->Value() );
            }
        }
 
    }

// ---------------------------------------------------------------------------
// HandleNaviEventL from MTimeLabelNaviObserver
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::HandleNaviEventL()
    {
    // Open Volume slider
	// First push then set value	
	iNaviPane->PushL(*iVolumeNavi); 
	TInt currenValue( GetVolumeControl()->Value() );	    
	ShowVolumeLabelL( currenValue );
	
    }
    
// ---------------------------------------------------------------------------
// CreateTimeLabelNaviL
// ---------------------------------------------------------------------------
//
CAknNavigationDecorator* CVeiNaviPaneControl::CreateTimeLabelNaviL()
	{
	ASSERT( iNaviPane );
	
	CVeiTimeLabelNavi* timelabelnavi = CVeiTimeLabelNavi::NewLC();
	timelabelnavi->SetNaviObserver( this );
	CAknNavigationDecorator* decoratedFolder = CAknNavigationDecorator::NewL(
	    iNaviPane, timelabelnavi, CAknNavigationDecorator::ENotSpecified);
    CleanupStack::Pop(timelabelnavi);
	
    CleanupStack::PushL(decoratedFolder);
	decoratedFolder->SetContainerWindowL(*iNaviPane);
	CleanupStack::Pop(decoratedFolder);
	decoratedFolder->MakeScrollButtonVisible(EFalse);
	
	return decoratedFolder;
	}

// ---------------------------------------------------------------------------
// GetTimeLabelControl
// ---------------------------------------------------------------------------
//
CVeiTimeLabelNavi* CVeiNaviPaneControl::GetTimeLabelControl()
    {
    ASSERT( iTimeNavi );
    return static_cast<CVeiTimeLabelNavi*> ( iTimeNavi->DecoratedControl() );
    }

// ---------------------------------------------------------------------------
// GetVolumeControl
// ---------------------------------------------------------------------------
//
CAknVolumeControl* CVeiNaviPaneControl::GetVolumeControl()
    {
    ASSERT( iVolumeNavi );
    return static_cast<CAknVolumeControl*> ( iVolumeNavi->DecoratedControl() );
    }

// ---------------------------------------------------------------------------
// HideVolumeCallbackL
// ---------------------------------------------------------------------------
//    
TInt CVeiNaviPaneControl::HideVolumeCallbackL(TAny* aPtr)
	{
	CVeiNaviPaneControl* view = (CVeiNaviPaneControl*)aPtr;
    view->HideVolume();
	return 0;
	}

// ---------------------------------------------------------------------------
// HideVolume
// ---------------------------------------------------------------------------
//
void CVeiNaviPaneControl::HideVolume()
    {
    ASSERT( iVolumeNavi );
   	iNaviPane->Pop( iVolumeNavi );
    }
    
