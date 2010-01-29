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
*   File:       ExtProgressContainer.cpp
*   Created:    14-10-2005
*   Author:     
*               
*/

#include "ExtProgressContainer.h"
#include "ExtProgressAnimationControl.h"

#include <eikprogi.h>
#include <AknUtils.h> 
#include <eiklabel.h>
#include <aknsbasicbackgroundcontrolcontext.h> 
#include <aknsdrawutils.h> 
#include <gdi.h>

// Constants
const TInt KProgressBarDefaultFinalValue = 20;
const TInt KProgressBarDefaultHeight = 15;
const TInt KProgressBarDefaultWidth = 240;

// Positions
const TReal KContTX = 0.00;
const TReal KContTY = 0.20;
const TReal KContBX = 1.00;
const TReal KContBY = 1.00;

const TReal KLabelTX = 0.10;
const TReal KLabelTY = 0.10;
const TReal KLabelBX = 0.90;
const TReal KLabelBY = 0.30;

const TReal KAnimTX = 0.10;
const TReal KAnimTY = 0.30;
const TReal KAnimBX = 0.90;
const TReal KAnimBY = 0.80;

const TReal KProgTX = 0.10;
const TReal KProgTY = 0.80;
//const TReal KProgBX = 0.70;
//const TReal KProgBY = 0.90;



//============================================================================= 
CExtProgressContainer * CExtProgressContainer::NewL (const TRect& aRect, CCoeControl* aParent)
{
    CExtProgressContainer* self = new (ELeave) CExtProgressContainer();
    CleanupStack::PushL(self);
    self->ConstructL(aRect, aParent);
    CleanupStack::Pop( self );
    return self;
}

//============================================================================= 
CExtProgressContainer::CExtProgressContainer ()
{
}

//============================================================================= 
CExtProgressContainer::~CExtProgressContainer ()
{
    delete iProgressInfo;
    delete iAnimationControl;
    delete iLabel;
	delete iBgContext;
}

//============================================================================= 
void CExtProgressContainer::ConstructL (const TRect& aRect, CCoeControl* aParent)
{
    SetContainerWindowL(*aParent);
    
    TRect rect;
    AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, rect);   
    TSize size = rect.Size();
    SetRect(TRect
        (static_cast<TInt>(size.iWidth * KContTX + 0.5), 
         static_cast<TInt>(size.iHeight * KContTY + 0.5), 
         static_cast<TInt>(size.iWidth * KContBX + 0.5), 
         static_cast<TInt>(size.iHeight * KContBY + 0.5))); 
 
   
    iLabel = new(ELeave) CEikLabel;
    iLabel->SetContainerWindowL(*this);

    iAnimationControl = CExtProgressAnimationControl::NewL(aRect, aParent);
    iAnimationControl->SetObserver(this);
	iAnimationControl->SetContainerWindowL(*this);	

    // Set Progress Bar property, coding directly
    CEikProgressInfo::SInfo info;
    
    info.iHeight = KProgressBarDefaultHeight;
    info.iWidth = KProgressBarDefaultWidth;
    info.iSplitsInBlock = 0;
    info.iTextType = EEikProgressTextNone;  
    info.iFinalValue = KProgressBarDefaultFinalValue;

    iProgressInfo = new( ELeave ) CEikProgressInfo( info );

    iProgressInfo->ConstructL();
    iProgressInfo->SetContainerWindowL(*this);   
    
    SizeChanged();        
    
    ActivateL();    
}

//============================================================================= 
void CExtProgressContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent aEventType)
{
    if (aEventType == MCoeControlObserver::EEventStateChanged )
    {
        ReportEventL(MCoeControlObserver::EEventStateChanged);    
    }
    
}
    
//============================================================================= 
TInt CExtProgressContainer::CountComponentControls() const
{
    return 3;
}

//============================================================================= 
CCoeControl* CExtProgressContainer::ComponentControl(TInt aIndex) const
{
    CCoeControl* ret = NULL;
    switch (aIndex)
    {
        case 0:
        {
            ret = iLabel;
            break;
        }        
        case 1:
        {
            ret = iAnimationControl;
            break;
        }
        case 2:
        {
            ret = iProgressInfo;
            break;
        }
        default:
        {
            break;
        }
        
    }
    
    return ret;
}

//============================================================================= 
CEikProgressInfo* CExtProgressContainer::GetProgressInfoL()
{
    return iProgressInfo;
}

//============================================================================= 		  
CExtProgressAnimationControl* CExtProgressContainer::GetAnimationControlL()
{
    return iAnimationControl;   
}

//============================================================================= 		  
void CExtProgressContainer::SetTextL(const TDesC &aText)
{
    iLabel->SetTextL(aText);   
    DrawNow();
}
								  

//============================================================================= 
void CExtProgressContainer::Draw(const TRect& aRect) const
{
	CWindowGc& gc = SystemGc();

	MAknsSkinInstance* skin = AknsUtils::SkinInstance();	
	MAknsControlContext* cc = AknsDrawUtils::ControlContext( this );
	
	AknsDrawUtils::Background( skin, cc, this, gc, aRect );	
//	AknsDrawUtils::Background( skin, cc, iLabel, gc, aRect );
//	AknsDrawUtils::Background( skin, cc, iProgressInfo, gc, aRect );	
//	AknsDrawUtils::Background( skin, cc, iAnimationControl, gc, aRect );	
	    
	// Just draw a rectangle round the edge of the control.
	// CWindowGc& gc=SystemGc();
	//gc.Clear(aRect);
	//gc.SetClippingRect(aRect);
	//gc.DrawRect(Rect());
}


//============================================================================= 
void CExtProgressContainer::SizeChanged()
{
    TRect rect( Rect() ); 
	if ( iBgContext )
	{
		iBgContext->SetRect( rect );
	}

    TSize size= rect.Size();

    if (iLabel)
    {
        iLabel->SetRect(TRect
            (static_cast<TInt>(size.iWidth * KLabelTX + 0.5), 
             static_cast<TInt>(size.iHeight * KLabelTY + 0.5), 
             static_cast<TInt>(size.iWidth * KLabelBX + 0.5), 
             static_cast<TInt>(size.iHeight * KLabelBY + 0.5 )));      
    }

    if (iAnimationControl )
    {
        iAnimationControl->SetRect(TRect
            (static_cast<TInt>(size.iWidth * KAnimTX + 0.5), 
             static_cast<TInt>(size.iHeight * KAnimTY + 0.5), 
             static_cast<TInt>(size.iWidth * KAnimBX + 0.5), 
             static_cast<TInt>(size.iHeight * KAnimBY + 0.5)));         
    }
    
    if (iProgressInfo)
    {
        TSize minSize = iProgressInfo->MinimumSize();

        iProgressInfo->SetExtent(TPoint
            (static_cast<TInt>(size.iWidth * KProgTX + 0.5), 
             static_cast<TInt>(size.iHeight * KProgTY + 0.5)),
             minSize);

        /*
        iProgressInfo->SetPosition(TPoint
            (size.iWidth * KProgTX, 
             size.iHeight * KProgTY));
        
        iProgressInfo->SetExtent(TRect
            (size.iWidth * KProgTX, 
             size.iHeight * KProgTY, 
             size.iWidth * KProgBX, 
             size.iHeight * KProgBY));    
 		  */
    }
        
}

//============================================================================= 
TTypeUid::Ptr CExtProgressContainer::MopSupplyObject( TTypeUid aId )
{
	if ( aId.iUid == MAknsControlContext::ETypeId && iBgContext )
	{
		return MAknsControlContext::SupplyMopObject( aId, iBgContext );
	}
	return CCoeControl::MopSupplyObject( aId );
}

//=============================================================================
TSize CExtProgressContainer::MinimumSize()
{  
    return Rect().Size();   
}
	
//=============================================================================
void CExtProgressContainer::Test()
{


}

// End of File
