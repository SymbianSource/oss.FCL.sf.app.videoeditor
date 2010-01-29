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


// INCLUDE FILES

// System includes
#include <manualvideoeditor.rsg>
#include <stringloader.h>
#include <eikenv.h>

// User includes
#include "TransitionInfo.h"
#include "VideoEditorDebugUtils.h"



// ================= MEMBER FUNCTIONS =======================

/* **********************************************************************
 * CTransitionInfo
 * **********************************************************************/


CTransitionInfo* CTransitionInfo::NewL()
    {
    CTransitionInfo* self = NewLC();
    CleanupStack::Pop( self );
    return self;
    }

CTransitionInfo* CTransitionInfo::NewLC()
    {
    CTransitionInfo* self = new( ELeave )CTransitionInfo;
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

CTransitionInfo::CTransitionInfo()
    {
    }

void CTransitionInfo::ConstructL()
    {
    LOG( KVideoEditorLogFile, "CTransitionInfo::ConstructL: in" );

    HBufC* buf;
    CEikonEnv* eikonEnv = CEikonEnv::Static();

    /* Load start transition effect names. */
    buf = StringLoader::LoadLC( R_VEI_START_TRANSITION_EFFECT_NAME_NONE, eikonEnv );
    User::LeaveIfError( iStartTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_START_TRANSITION_EFFECT_NAME_FADE_FROM_BLACK, eikonEnv );
    User::LeaveIfError( iStartTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_START_TRANSITION_EFFECT_NAME_FADE_FROM_WHITE, eikonEnv );
    User::LeaveIfError( iStartTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );

    /* Load middle transition effect names. */
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_NONE, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_DIP_TO_BLACK, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_DIP_TO_WHITE, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_CROSSFADE, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_WIPE_LEFT, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_WIPE_RIGHT, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_WIPE_TOP, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_MIDDLE_TRANSITION_EFFECT_NAME_WIPE_BOTTOM, eikonEnv );
    User::LeaveIfError( iMiddleTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );


    /* Load end transition effect names. */
    buf = StringLoader::LoadLC( R_VEI_END_TRANSITION_EFFECT_NAME_NONE, eikonEnv );
    User::LeaveIfError( iEndTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_END_TRANSITION_EFFECT_NAME_FADE_TO_BLACK, eikonEnv );
    User::LeaveIfError( iEndTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );
    buf = StringLoader::LoadLC( R_VEI_END_TRANSITION_EFFECT_NAME_FADE_TO_WHITE, eikonEnv );
    User::LeaveIfError( iEndTransitionNameArray.Append( buf ));
    CleanupStack::Pop( buf );

    LOG( KVideoEditorLogFile, "CTransitionInfo::ConstructL: out" );
    }

CTransitionInfo::~CTransitionInfo()
    {
    iStartTransitionNameArray.ResetAndDestroy();
    iMiddleTransitionNameArray.ResetAndDestroy();
    iEndTransitionNameArray.ResetAndDestroy();
    }

HBufC* CTransitionInfo::StartTransitionName( TVedStartTransitionEffect aEffect )
    {
    return iStartTransitionNameArray[aEffect - ( TInt )EVedStartTransitionEffectNone];
    }

HBufC* CTransitionInfo::MiddleTransitionName( TVedMiddleTransitionEffect aEffect )
    {
    return iMiddleTransitionNameArray[aEffect - ( TInt )EVedMiddleTransitionEffectNone];
    }

HBufC* CTransitionInfo::EndTransitionName( TVedEndTransitionEffect aEffect )
    {
    return iEndTransitionNameArray[aEffect - ( TInt )EVedEndTransitionEffectNone];
    }
    
// End of File  
