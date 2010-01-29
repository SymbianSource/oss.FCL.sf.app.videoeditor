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
*   File:       ExtProgressDialog.cpp
*   Created:    14-10-2005
*   Author:     
*               
*/

#include "ExtProgressDialog.h"
#include "ExtProgressContainer.h"
#include "ExtProgressAnimationControl.h"

#include <avkon.hrh>
#include <aknborders.h> 
#include <AknUtils.h> 
#include <eikapp.h>
#include <eikprogi.h>
#include <eiklabel.h>
#include <data_caging_path_literals.hrh>

// CONSTANTS
_LIT(KResourceFile, "VideoEditorUiComponents.rsc");


//=============================================================================
EXPORT_C CExtProgressDialog::CExtProgressDialog(CExtProgressDialog** aSelfPtr)
: iSelfPtr(aSelfPtr), 
  iResLoader(*CEikonEnv::Static()) 
{
}

//=============================================================================
EXPORT_C CExtProgressDialog::~CExtProgressDialog()
{
    
    delete iContainer;
    iResLoader.Close();   
    
    // Nullify self pointer
    *iSelfPtr = NULL;        
}

//=============================================================================
EXPORT_C void CExtProgressDialog::PrepareLC(TInt aResourceId)
{
	TFileName resourceFile;
    Dll::FileName(resourceFile);
    TParse p;
    p.Set(KResourceFile, &KDC_RESOURCE_FILES_DIR, &resourceFile);
    resourceFile = p.FullName();
	iResLoader.OpenL( resourceFile );

	CEikDialog::PrepareLC(aResourceId);

	TRect rect;
	iContainer = CExtProgressContainer::NewL(rect, this);

	iContainer->SetControlContext(this);
	iContainer->SetObserver(this);

}

//============================================================================= 
void CExtProgressDialog::SetSizeAndPosition( const TSize& /*aSize*/ )
{
    TRect rect;
    AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, rect);

    TSize size = iContainer->Rect().Size();
    SetSize(size);
    SetPosition(TPoint(0, rect.iBr.iY - size.iHeight));
}

//=============================================================================
TBool CExtProgressDialog::OkToExitL( TInt aButtonId )
{
    if (iCallback)
    {
        iCallback->DialogDismissedL(aButtonId);
    }
    
    return ETrue;
}

//=============================================================================    
void CExtProgressDialog::PreLayoutDynInitL() 
{ 

}
    
//=============================================================================    
TKeyResponse CExtProgressDialog::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
    TKeyResponse res = EKeyWasConsumed;
            
    if (aType == EEventKey)
    {
        switch (aKeyEvent.iCode)
        {
            case 0x31:
            {
                iContainer->GetProgressInfoL()->IncrementAndDraw(1);
                
                break;
            }
            case 0x32:
            {
                DrawNow();
                
                break;
            }
            case 0x33:
            {
                iContainer->GetAnimationControlL()->StartAnimationL();
                
                break;
            }
            case 0x34:
            {
                iContainer->DrawNow();
                
                break;
            }
           case 0x37:
            {
                iContainer->Test();
                
                break;
            }             
           default:
            {
                res = CEikDialog::OfferKeyEventL(aKeyEvent, aType);
            }
        }
    }
    return res;
}

//=============================================================================    
EXPORT_C void CExtProgressDialog::SetCallback(MExtProgressDialogCallback* aCallback)
{
    iCallback = aCallback;
}

//============================================================================= 
EXPORT_C CEikProgressInfo* CExtProgressDialog::GetProgressInfoL()
{
    return iContainer->GetProgressInfoL();
}
 
//============================================================================= 
EXPORT_C void CExtProgressDialog::StartAnimationL()
{
    return iContainer->GetAnimationControlL()->StartAnimationL();
   
}

//============================================================================= 
EXPORT_C void CExtProgressDialog::SetTextL(const TDesC &aText)
{
    iContainer->SetTextL(aText);
}
//============================================================================= 

EXPORT_C void CExtProgressDialog::SetAnimationResourceIdL(const TInt &aResourceId)
{
    iContainer->GetAnimationControlL()->SetAnimationResourceId(aResourceId);
}
//============================================================================= 

void CExtProgressDialog::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent aEventType)
{
    if (aEventType == MCoeControlObserver::EEventStateChanged )
    {
        DrawNow();    
    }
}

//============================================================================= 
TInt CExtProgressDialog::CountComponentControls() const
{
    return 1;
}

//============================================================================= 
CCoeControl* CExtProgressDialog::ComponentControl(TInt aIndex) const
{
    CCoeControl* ret = NULL;
    switch (aIndex)
    {
        case 0:
        {
            ret = iContainer;
            break;
        }
        default:
        {
            break;
        }
        
    }
    
    return ret;
}

// End of File
