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
#include <coemain.h>
#include <akniconutils.h>
#include <manualvideoeditor.mbg>
#include <videoeditoruicomponents.mbg>

// User includes
#include "veiappui.h"
#include "veieditvideocontainer.h"
#include "VeiIconBox.h"
#include "VideoEditorUtils.h"


CVeiIconBox* CVeiIconBox::NewL( const TRect& aRect, const CCoeControl* aParent )
    {
    CVeiIconBox* self = CVeiIconBox::NewLC( aRect, aParent );
    CleanupStack::Pop( self );
    return self;
    }

CVeiIconBox* CVeiIconBox::NewLC( const TRect& aRect, const CCoeControl* aParent )
    {
    CVeiIconBox* self = new( ELeave )CVeiIconBox;
    CleanupStack::PushL( self );
    self->ConstructL( aRect, aParent );
    return self;
    }

void CVeiIconBox::ConstructL( const TRect& aRect, const CCoeControl* aParent )
    {
    SetContainerWindowL( *aParent );

    TFileName mbmPath( VideoEditorUtils::IconFileNameAndPath( KManualVideoEditorIconFileId ));
    TFileName mbmPath2( VideoEditorUtils::IconFileNameAndPath( KVideoEditorUiComponentsIconFileId ));

    AknIconUtils::CreateIconL( iVolumeMute, 
                               iVolumeMuteMask, 
                               mbmPath, 
                               EMbmManualvideoeditorQgn_prop_ve_muted, 
                               EMbmManualvideoeditorQgn_prop_ve_muted_mask );

    AknIconUtils::CreateIconL( iSlowMotion, 
                               iSlowMotionMask, 
                               mbmPath, 
                               EMbmManualvideoeditorQgn_prop_ve_slow, 
                               EMbmManualvideoeditorQgn_prop_ve_slow_mask );

    AknIconUtils::CreateIconL( iBlackAndWhite, 
                               iBlackAndWhiteMask, 
                               mbmPath, 
                               EMbmManualvideoeditorQgn_prop_ve_bw, 
                               EMbmManualvideoeditorQgn_prop_ve_bw_mask );

    AknIconUtils::CreateIconL( iColour, 
                               iColourMask, 
                               mbmPath, 
                               EMbmManualvideoeditorQgn_prop_ve_colour, 
                               EMbmManualvideoeditorQgn_prop_ve_colour_mask );

    AknIconUtils::CreateIconL( iRecAudio, 
                               iRecAudioMask, 
                               mbmPath, 
                               EMbmManualvideoeditorQgn_prop_ve_rec, 
                               EMbmManualvideoeditorQgn_prop_ve_rec_mask );

    AknIconUtils::CreateIconL( iPauseAudio, 
                               iPauseAudioMask, 
                               mbmPath2, 
                               EMbmVideoeditoruicomponentsQgn_prop_ve_pause, 
                               EMbmVideoeditoruicomponentsQgn_prop_ve_pause_mask );

    SetRect( aRect );
    ActivateL();
    }

CVeiIconBox::~CVeiIconBox()
    {
    delete iBlackAndWhite;
    delete iBlackAndWhiteMask;
    delete iColour;
    delete iColourMask;
    delete iSlowMotion;
    delete iSlowMotionMask;
    delete iVolumeMute;
    delete iVolumeMuteMask;
    delete iRecAudio;
    delete iRecAudioMask;
    delete iPauseAudio;
    delete iPauseAudioMask;
    }


void CVeiIconBox::SizeChanged()
    {
    TRect rect = Rect();
    TSize iconSize;
    if ( !iLandscapeScreenOrientation )
        {
        iconSize.SetSize( rect.Width(), rect.Width());
        }
    else
        {
        iconSize.SetSize( rect.Height(), rect.Height());
        }

    AknIconUtils::SetSize( iVolumeMute, iconSize, EAspectRatioNotPreserved );
    AknIconUtils::SetSize( iSlowMotion, iconSize, EAspectRatioNotPreserved );
    AknIconUtils::SetSize( iBlackAndWhite, iconSize, EAspectRatioNotPreserved );
    AknIconUtils::SetSize( iColour, iconSize, EAspectRatioNotPreserved );
    AknIconUtils::SetSize( iRecAudio, iconSize, EAspectRatioNotPreserved );
    AknIconUtils::SetSize( iPauseAudio, iconSize, EAspectRatioNotPreserved );
    }

void CVeiIconBox::SetLandscapeScreenOrientation( TBool aLandscapeScreenOrientation )
    {
    iLandscapeScreenOrientation = aLandscapeScreenOrientation;
    }

void CVeiIconBox::SetVolumeMuteIconVisibility( TBool aVisible )
    {
    iVolumeMuteIconVisible = aVisible;
    DrawDeferred();
    }

void CVeiIconBox::SetSlowMotionIconVisibility( TBool aVisible )
    {
    iSlowMotionIconVisible = aVisible;
    DrawDeferred();
    }

void CVeiIconBox::SetBlackAndWhiteIconVisibility( TBool aVisible )
    {
    iBlackAndWhiteIconVisible = aVisible;
    DrawDeferred();
    }

void CVeiIconBox::SetColourIconVisibility( TBool aVisible )
    {
    iColourIconVisible = aVisible;
    DrawDeferred();
    }

void CVeiIconBox::SetRecAudioIconVisibility( TBool aVisible )
    {
    iRecAudioIconVisibile = aVisible;
    DrawDeferred();
    }

void CVeiIconBox::SetPauseAudioIconVisibility( TBool aVisible )
    {
    iPauseAudioIconVisibile = aVisible;
    DrawDeferred();
    }



void CVeiIconBox::Draw( const TRect& aRect )const
    {
    CWindowGc& gc = SystemGc();

    TRect rect = aRect;

    TPoint slowMotionPos;
    TPoint BWPos;

    /* Icon area frame */

    if ( !iLandscapeScreenOrientation )
    // Portrait
        {
        TInt symboldistance = STATIC_CAST( TInt, rect.Size().iHeight* 0.01428571 );
        /** volume, rec and pause icons are on same position.*/
        if ( iVolumeMuteIconVisible )
            {
            TPoint audioMutePos( rect.iTl );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( audioMutePos, iVolumeMute, clipRect, iVolumeMuteMask, EFalse );
            }
        else
            {
            if ( iRecAudioIconVisibile )
                {
                TPoint audioRecPos( rect.iTl );
                TRect clipRect( TPoint( 0, 0 ), iRecAudio->SizeInPixels().AsPoint());
                gc.BitBltMasked( audioRecPos, iRecAudio, clipRect, iRecAudioMask, EFalse );
                }
            else if ( iPauseAudioIconVisibile )
                {
                TPoint audioPausePos( rect.iTl );
                TRect clipRect( TPoint( 0, 0 ), iPauseAudio->SizeInPixels().AsPoint());
                gc.BitBltMasked( audioPausePos, iPauseAudio, clipRect, iPauseAudioMask, EFalse );
                }
            }

        if ( iSlowMotionIconVisible )
            {
            slowMotionPos.SetXY( rect.iTl.iX, rect.iTl.iY + iVolumeMute->SizeInPixels().iHeight + symboldistance );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( slowMotionPos, iSlowMotion, clipRect, iSlowMotionMask, EFalse );
            }


        if ( iBlackAndWhiteIconVisible )
            {
            BWPos.SetXY( rect.iTl.iX, rect.iTl.iY + iVolumeMute->SizeInPixels().iHeight + iSlowMotion->SizeInPixels().iHeight + symboldistance* 2 );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( BWPos, iBlackAndWhite, clipRect, iBlackAndWhiteMask, EFalse );
            }

        if ( iColourIconVisible )
            {
            BWPos.SetXY( rect.iTl.iX, rect.iTl.iY + iVolumeMute->SizeInPixels().iHeight + iSlowMotion->SizeInPixels().iHeight + symboldistance* 2 );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( BWPos, iColour, clipRect, iColourMask, EFalse );
            }


        }
    else
    // Landscape
        {
        TInt symboldistance = STATIC_CAST( TInt, rect.Size().iWidth* 0.04347826 );
        if ( iVolumeMuteIconVisible )
            {
            TPoint audioMutePos( rect.iTl );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( audioMutePos, iVolumeMute, clipRect, iVolumeMuteMask, EFalse );
            }
        else
            {
            if ( iRecAudioIconVisibile )
                {
                TPoint audioRecPos( rect.iTl );
                TRect clipRect( TPoint( 0, 0 ), iRecAudio->SizeInPixels().AsPoint());
                gc.BitBltMasked( audioRecPos, iRecAudio, clipRect, iRecAudioMask, EFalse );
                }
            else if ( iPauseAudioIconVisibile )
                {
                TPoint audioPausePos( rect.iTl );
                TRect clipRect( TPoint( 0, 0 ), iPauseAudio->SizeInPixels().AsPoint());
                gc.BitBltMasked( audioPausePos, iPauseAudio, clipRect, iPauseAudioMask, EFalse );
                }
            }

        if ( iSlowMotionIconVisible )
            {
            slowMotionPos.SetXY( rect.iTl.iX + iVolumeMute->SizeInPixels().iWidth + symboldistance, rect.iTl.iY );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( slowMotionPos, iSlowMotion, clipRect, iSlowMotionMask, EFalse );
            }

        if ( iBlackAndWhiteIconVisible )
            {
            BWPos.SetXY( rect.iTl.iX + iVolumeMute->SizeInPixels().iWidth + iSlowMotion->SizeInPixels().iWidth + symboldistance * 2, rect.iTl.iY );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( BWPos, iBlackAndWhite, clipRect, iBlackAndWhiteMask, EFalse );
            }

        if ( iColourIconVisible )
            {
            BWPos.SetXY( rect.iTl.iX + iVolumeMute->SizeInPixels().iWidth + iSlowMotion->SizeInPixels().iWidth + symboldistance * 2, rect.iTl.iY );
            TRect clipRect( TPoint( 0, 0 ), iVolumeMute->SizeInPixels().AsPoint());
            gc.BitBltMasked( BWPos, iColour, clipRect, iColourMask, EFalse );
            }
        }
    }

// End of File
