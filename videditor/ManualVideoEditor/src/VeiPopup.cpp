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
#include <avkon.hrh>
#include <manualvideoeditor.rsg>


#include <akntitle.h> 
#include <aknlists.h>
#include <aknquerydialog.h>  
#include <eikmenub.h> 
#include <caknfileselectiondialog.h>
#include <stringloader.h> 
#include <mgfetch.h> 
#include <akncolourselectiongrid.h>

// User includes
#include "VeiEditVideoView.h"
#include "manualvideoeditor.hrh" 
#include "veipopup.h" 
#include "veiaddqueue.h"
#include "veieditvideocontainer.h"
#include "videoeditorcommon.h"
#include "veiappui.h"


CVeiPopup* CVeiPopup::NewL( CVeiEditVideoView& aView )
    {
    CVeiPopup* self = CVeiPopup::NewLC( aView );
    CleanupStack::Pop( self );
    return self;
    }

CVeiPopup* CVeiPopup::NewLC( CVeiEditVideoView& aView )
    {
    CVeiPopup* self = new( ELeave )CVeiPopup( aView );
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

CVeiPopup::CVeiPopup( CVeiEditVideoView& aView ): iView( aView )
{

}


void CVeiPopup::ConstructL()
    {
    LOG( KVideoEditorLogFile, "CVeiPopup::ConstructL: in" );
    LOG( KVideoEditorLogFile, "CVeiPopup::ConstructL: out" );
    }

CVeiPopup::~CVeiPopup()
    {
    LOG( KVideoEditorLogFile, "CVeiPopup::~CVeiPopup" );
    }

TInt CVeiPopup::ExecutePopupListL( TInt aSoftkeysResourceId, 
                                   TInt aPopupTitleResourceId, 
                                   TInt aArrayResourceId, 
                                   TInt aTablesize, 
                                   TBool aDynPopup ) const
    {

    // Create listbox and PUSH it.
    CAknSinglePopupMenuStyleListBox* listBox = new( ELeave )CAknSinglePopupMenuStyleListBox;
    CleanupStack::PushL( listBox );

    // Create popup list and PUSH it.
    CAknPopupList* popupList = CAknPopupList::NewL( listBox, aSoftkeysResourceId );
    CleanupStack::PushL( popupList );

    // Set title for popup from defined resource.
    HBufC* title = CCoeEnv::Static()->AllocReadResourceLC( aPopupTitleResourceId );
    popupList->SetTitleL( *title );
    CleanupStack::PopAndDestroy( title );

    // initialize listbox.
    listBox->ConstructL( popupList, EAknListBoxMenuList );

    // Make listitems. and PUSH it
    CDesCArrayFlat* items = CCoeEnv::Static()->ReadDesCArrayResourceL( aArrayResourceId );
    CleanupStack::PushL( items );

    // Remove given index if at correct range.

    if ( aDynPopup )
        {
        TInt i;
        for ( i = aTablesize - 1; i >= 0; i-- )
            {
            if ( 0 == RemoveArrayIndex[i] )
                {
                items->Delete( i );
                }
            }
        }
    else
        {
        if ( aTablesize >= 0 && aTablesize <= ( items->Count() - 1 ))
            {
            items->Delete( aTablesize );
            }
        }


    // Set listitems.
    CTextListBoxModel* model = listBox->Model();
    model->SetItemTextArray( items );
    model->SetOwnershipType( ELbmOwnsItemArray );

    CleanupStack::Pop( items ); // Pop effect items

    listBox->CreateScrollBarFrameL( ETrue );
    listBox->ScrollBarFrame()->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto );

    TInt popOk = popupList->ExecuteLD();

    TInt returnValue;

    if ( popOk )
        {
        // Return selected item's index.
        returnValue = listBox->CurrentItemIndex();
        }
    else
        {
        returnValue =  - 1;
        }

    // Clenup and destroy.
    CleanupStack::Pop( popupList );
    CleanupStack::PopAndDestroy( listBox );

    return returnValue;
    }


void CVeiPopup::ShowEndTransitionPopupListL()
    {
    TInt removeIndex;
    // Which effect to remove from listbox.
    switch ( iView.Movie()->EndTransitionEffect())
        {
        case EVedEndTransitionEffectFadeToBlack:
            removeIndex = 0;
            break;
        case EVedEndTransitionEffectFadeToWhite:
            removeIndex = 1;
            break;
        case EVedEndTransitionEffectNone:
            removeIndex = 2;
            break;
        default:
            removeIndex =  - 1;
            break;
        }

    TInt selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, 
                                            R_VEI_TRANSTION_POPUP_TITLE, 
                                            R_VEI_END_TRANSITION_LIST_BOX_ARRAY, 
                                            removeIndex, 
                                            EFalse );

    switch ( selectedIndex )
        {
        case 0:
                {
                if ( removeIndex == 0 )
                    {
                    iView.Movie()->SetEndTransitionEffect( EVedEndTransitionEffectFadeToWhite );
                    }
                else
                    {
                    iView.Movie()->SetEndTransitionEffect( EVedEndTransitionEffectFadeToBlack );
                    }
                break;
                }
        case 1:
                {
                if ( removeIndex <= 1 )
                    {
                    iView.Movie()->SetEndTransitionEffect( EVedEndTransitionEffectNone );
                    }
                else
                    {
                    iView.Movie()->SetEndTransitionEffect( EVedEndTransitionEffectFadeToWhite );
                    }
                break;
                }
        default:
            break;
        }

    }

void CVeiPopup::ShowMiddleTransitionPopupListL()
    {
    TInt currentindex = iView.Container()->CurrentIndex() - 1;
    TInt removeIndex;
    switch ( iView.Movie()->MiddleTransitionEffect( currentindex ))
        {
        case EVedMiddleTransitionEffectWipeTopToBottom:
            removeIndex = 0;
            break;
        case EVedMiddleTransitionEffectWipeBottomToTop:
            removeIndex = 1;
            break;
        case EVedMiddleTransitionEffectWipeLeftToRight:
            removeIndex = 2;
            break;
        case EVedMiddleTransitionEffectWipeRightToLeft:
            removeIndex = 3;
            break;
        case EVedMiddleTransitionEffectCrossfade:
            removeIndex = 4;
            break;
        case EVedMiddleTransitionEffectDipToBlack:
            removeIndex = 5;
            break;
        case EVedMiddleTransitionEffectDipToWhite:
            removeIndex = 6;
            break;
        case EVedMiddleTransitionEffectNone:
            removeIndex = 7;
            break;
        default:
            removeIndex =  - 1;
            break;
        }

    TInt selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, 
                                            R_VEI_TRANSTION_POPUP_TITLE, 
                                            R_VEI_MIDDLE_TRANSITION_LIST_BOX_ARRAY, 
                                            removeIndex, 
                                            EFalse );

    switch ( selectedIndex )
        {
        case 0:
                {
                if ( removeIndex == 0 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeBottomToTop, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeTopToBottom, currentindex );
                    }
                break;
                }
        case 1:
                {
                if ( removeIndex <= 1 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeLeftToRight, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeBottomToTop, currentindex );
                    }
                break;
                }
        case 2:
                {
                if ( removeIndex <= 2 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeRightToLeft, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeLeftToRight, currentindex );
                    }
                break;
                }
        case 3:
                {
                if ( removeIndex <= 3 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectCrossfade, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeRightToLeft, currentindex );
                    }
                break;
                }
        case 4:
                {
                if ( removeIndex <= 4 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectDipToBlack, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectCrossfade, currentindex );
                    }
                break;
                }
        case 5:
                {
                if ( removeIndex <= 5 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectDipToWhite, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectDipToBlack, currentindex );
                    }
                break;
                }
        case 6:
                {
                if ( removeIndex <= 6 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectNone, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectDipToWhite, currentindex );
                    }
                break;
                }
        case 7:
                {
                if ( removeIndex <= 7 )
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectWipeLeftToRight, currentindex );
                    }
                else
                    {
                    iView.Movie()->SetMiddleTransitionEffect( EVedMiddleTransitionEffectNone, currentindex );
                    iView.Movie()->SetEndTransitionEffect( EVedEndTransitionEffectFadeToWhite );
                    }
                break;
                }
        default:
            break;
        }

    }

void CVeiPopup::ShowStartTransitionPopupListL()
    {
    TInt removeIndex;
    switch ( iView.Movie()->StartTransitionEffect())
        {
        case EVedStartTransitionEffectFadeFromBlack:
            removeIndex = 0;
            break;
        case EVedStartTransitionEffectFadeFromWhite:
            removeIndex = 1;
            break;
        case EVedStartTransitionEffectNone:
            removeIndex = 2;
            break;
        default:
            removeIndex =  - 1;
            break;
        }

    TInt selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, 
                                            R_VEI_TRANSTION_POPUP_TITLE, 
                                            R_VEI_START_TRANSITION_LIST_BOX_ARRAY, 
                                            removeIndex, 
                                            EFalse );

    switch ( selectedIndex )
        {
        case 0:
                {
                if ( removeIndex == 0 )
                    {
                    iView.Movie()->SetStartTransitionEffect( EVedStartTransitionEffectFadeFromWhite );
                    }
                else
                    {
                    iView.Movie()->SetStartTransitionEffect( EVedStartTransitionEffectFadeFromBlack );
                    }
                break;
                }
        case 1:
                {
                if ( removeIndex <= 1 )
                    {
                    iView.Movie()->SetStartTransitionEffect( EVedStartTransitionEffectNone );
                    }
                else
                    {
                    iView.Movie()->SetStartTransitionEffect( EVedStartTransitionEffectFadeFromWhite );
                    }
                break;
                }
        default:
            break;
        }

    }

void CVeiPopup::ShowInsertAudioPopupList()
    {
    TInt selectedIndex; // Selected item's index in popup list.
    TInt removeIndex; // Index to remove from list.

    removeIndex =  - 1; // -1 = Nothing to remove from list array.

    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, 
                                       R_VEI_POPUP_INSERT_AUDIO_TITLE, 
                                       R_VEI_INSERT_AUDIO_LIST_BOX_ARRAY, 
                                       removeIndex, 
                                       EFalse );

    switch ( selectedIndex )
        {
        /**
         * Sound clip
         */
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertAudio );
            break;
            /**
             * New sound clip
             */
        case 1:
            iView.InsertNewAudio();
            break;
        default:
            break;
        }

    }

/*
Show popup list when cursor is on empty video track.
Video, text, image etc..
 */

void CVeiPopup::ShowInsertStuffPopupList()
    {
    TInt selectedIndex; // Selected item's index in popup list.
    TInt removeIndex; // Index to remove from list.

    removeIndex =  - 1; // -1 = Nothing to remove from list array.

    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, 
                                       R_VEI_POPUP_INSERT_STUFF_TITLE, 
                                       R_VEI_INSERT_STUFF_LIST_BOX_ARRAY, 
                                       removeIndex, 
                                       EFalse );

    switch ( selectedIndex )
        {
        /**
         * Video clip
         */
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertVideo );
            break;
            /**
            INSERT IMAGE
             */
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertImage );
            break;
            /**
            INSERT Text
             */
        case 2:
            ShowInsertTextPopupList();
            break;
        default:
            break;
        }

    }

/*
Show popup list when cursor is on empty video track.
Video, text, image etc..
 */


void CVeiPopup::ShowInsertTextPopupList()
    {
    TInt selectedIndex; // Selected item's index in popup list.
    TInt removeIndex; // Index to remove from list.

    removeIndex =  - 1; // -1 = Nothing to remove from list array.

    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, 
                                       R_VEI_POPUP_INSERT_STUFF_TITLE, 
                                       R_VEI_INSERT_TEXT_LIST_BOX_ARRAY, 
                                       removeIndex, 
                                       EFalse );

    switch ( selectedIndex )
        {
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertTextTitle );
            break;
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertTextTitleFading );
            break;
        case 2:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertTextSubTitle );
            break;
        case 3:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertTextSubTitleFading );
            break;
        case 4:
            iView.HandleCommandL( EVeiCmdEditVideoViewInsertTextCredits );
            break;
        default:
            break;
        }
    }

void CVeiPopup::ShowEffectSelectionPopupListL()
    {
    TInt currentIndex = iView.Container()->CurrentIndex();
    TInt removeIndex;

    // Which effect to remove from listbox.
    switch ( iView.Movie()->VideoClipColorEffect( currentIndex ))
        {
        case EVedColorEffectBlackAndWhite:
            removeIndex = 0; // blackandwhite removed from list
            break;
            /*case EVedColorEffectToning: // some other color must be able to be chosen still
            removeIndex = 1;
            break;					
             */
        case EVedColorEffectNone:
            removeIndex = 2;
            break;
        default:
            removeIndex =  - 1; // Remove nothing from array.
            break;
        }


    /*
    array indexes:
    qtn_vei_list_query_select_effect_black_white;
    qtn_vei_list_query_select_effect_colour;
    qtn_vei_list_query_select_effect_no_effect;	

     */

    // Execute popup list with proper parameters.
    TInt selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, 
                                            R_VEI_POPUP_SELECT_EFFECT_TITLE, 
                                            R_VEI_EFFECT_LIST_BOX_ARRAY, // order: bw, col, no effect
                                            removeIndex, 
                                            EFalse );
    switch ( selectedIndex )
        {
        case 0:
            // blackandwhite
                {
                if ( removeIndex == 0 )
                // current effect blackandwhite -> colour chosen
                    {
                    TRgb color;
                    if ( !ShowColorSelectorL( color ))
                        {
                        break;
                        }

                    iView.Movie()->VideoClipSetColorTone( currentIndex, color );
                    iView.Movie()->VideoClipSetColorEffect( currentIndex, EVedColorEffectToning );
                    }
                else
                    {
                    iView.Movie()->VideoClipSetColorEffect( currentIndex, EVedColorEffectBlackAndWhite );
                    }
                break;
                }
        case 1:
            // colour
                {
                if ( removeIndex == 0 )
                // // current effect blackandwhite -> no effect chosen
                    {
                    iView.Movie()->VideoClipSetColorEffect( currentIndex, EVedColorEffectNone );
                    }
                else
                    {
                    // current event none
                    TRgb color;

                    if ( !ShowColorSelectorL( color ))
                        {
                        break;
                        }

                    //				TInt R_ct = color.Red();
                    //				TInt G_ct = color.Green();
                    //				TInt B_ct = color.Blue();	

                    iView.Movie()->VideoClipSetColorTone( currentIndex, color );
                    iView.Movie()->VideoClipSetColorEffect( currentIndex, EVedColorEffectToning );

                    //				TRgb toning = iView.Movie()->VideoClipColorTone(currentIndex);
                    //				R_ct = toning.Red();
                    //				G_ct = toning.Green();
                    //				B_ct = toning.Blue();
                    }
                break;
                }
        case 2:
            // no effect
                {
                iView.Movie()->VideoClipSetColorEffect( currentIndex, EVedColorEffectNone );
                break;
                }
        default:
            break;
        }

    }

void CVeiPopup::ShowEditTextPopupList()
    {
    TInt selectedIndex =  - 1; // Selected item's index in popup list.
    TInt listSelection =  - 1; // -1 = Nothing to remove from list array.
    TInt removeIndex =  - 1; // -1 = Nothing to remove from list array.

    TBool oneclip = EFalse;

    if ( iView.Movie()->VideoClipCount() <= 1 )
        {
        removeIndex = 0;
        oneclip = ETrue;
        }

    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, R_VEI_POPUP_EDIT_TEXT_TITLE, R_VEI_EDIT_TEXT_LIST_BOX_ARRAY, removeIndex, EFalse );

    listSelection = selectedIndex;

    if ( selectedIndex >= 0 )
        {
        if ( oneclip )
            {
            listSelection++;
            }
        }

    switch ( listSelection )
        {
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextMove );
            break;
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextRemove );
            break;
        case 2:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextChangeDuration );
            break;
        case 3:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextChangeText );
            break;
        case 4:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextSetTextColor );
            break;
        case 5:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextSetBackGround );
            break;
        case 6:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextAddColorEffect );
            break;
        case 7:
            ShowEditTextStylePopUpList();
            break;
        case 8:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextDuplicate );
            break;
        default:
            break;
        }

    }

void CVeiPopup::ShowEditTextStylePopUpList()
    {

    TInt selectedIndex; // Selected item's index in popup list.
    TInt removeIndex; // Index to remove from list.

    removeIndex =  - 1; // -1 = Nothing to remove from list array.

    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, R_VEI_POPUP_EDIT_TEXT_TITLE, R_VEI_INSERT_TEXT_LIST_BOX_ARRAY, removeIndex, EFalse );

    switch ( selectedIndex )
        {
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleTitle );
            break;
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleTitleFading );
            break;
        case 2:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleSubTitle );
            break;
        case 3:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleSubTitleFading );
            break;
        case 4:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleCredit );
            break;
        default:
            break;
        }

    }

void CVeiPopup::ShowEditVideoPopupList()
    {

    TInt originalIndex =  - 1;
    // Selected item's index in original list (read from resource file)	
    TInt dynamicIndex =  - 1;
    // Selected item's index in dynamic list (where some items in original list may be removed)

    // by default, all items are included in list (all are 1s)			
    for ( TInt i = 0; i < KAmountOfMenuItems; i++ )
        {
        RemoveArrayIndex[i] = 1;
        }

    // next some items are possibly removed from the list (by marking their index with '0's)
    // the reference order MUST be the same what it is in the original list in .rss

    // menu item "Cut" removed
    TTimeIntervalMicroSeconds duration = iView.Movie()->VideoClipInfo( iView.Container()->CurrentIndex())->Duration();
    if ( duration.Int64() < KMinCutVideoLength )
        {
        RemoveArrayIndex[0] = 0;
        }
    // menu item "Move" removed
    if ( iView.Movie()->VideoClipCount() <= 1 )
        {
        RemoveArrayIndex[1] = 0;
        }

    // menu items "Mute" and "Unmute" removed		
    if ( EFalse == iView.Movie()->VideoClipIsMuteable( iView.Container()->CurrentIndex()))
        {
        RemoveArrayIndex[4] = 0;
        RemoveArrayIndex[5] = 0;
        }
    else
        {
        // menu item "Mute" removed
        if ( iView.Movie()->VideoClipIsMuted( iView.Container()->CurrentIndex()) )
            {
            RemoveArrayIndex[4] = 0;
            }
        // menu item "Unmute" removed	
        else
            {
            RemoveArrayIndex[5] = 0;
            }
        }

    if ( !( iView.Movie()->VideoClipInfo( iView.Container()->CurrentIndex()))->HasAudio())
        {
        RemoveArrayIndex[6] = 0; //EVeiCmdEditVideoAdjustVolume);
        }


    dynamicIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, R_VEI_POPUP_EDIT_VIDEO_TITLE, R_VEI_EDIT_VIDEO_LIST_BOX_ARRAY, KAmountOfMenuItems, ETrue );

    if ( dynamicIndex < 0 )
        {
        return ;
        }

    // next find out what was the selected item in the original list
    // i.e. count 1's until dynamicIndex reached
    TInt cnt =  - 1;
    for ( TInt i = 0; i < KAmountOfMenuItems; i++ )
        {
        if ( 1 == RemoveArrayIndex[i] )
            {
            cnt++;
            if ( dynamicIndex == cnt )
                {
                originalIndex = i;
                break;
                }
            }
        }

    switch ( originalIndex )
        {
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoCutting );
            break;
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoMove );
            break;
        case 2:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoColorEffect );
            break;
        case 3:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoSlowMotion );
            break;
        case 4:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoMute );
            break;
        case 5:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoUnmute );
            break;
        case 6:
            iView.HandleCommandL( EVeiCmdEditVideoAdjustVolume );
            break;
        case 7:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoRemove );
            break;
        case 8:
            iView.HandleCommandL( EVeiCmdEditVideoDuplicate );
            break;
        default:
            break;
        }
    }

void CVeiPopup::ShowEditImagePopupList()
    {
    TInt selectedIndex =  - 1; // Selected item's index in popup list.
    TInt listSelection =  - 1; // -1 = Nothing to remove from list array.
    TInt removeIndex =  - 1; // -1 = Nothing to remove from list array.

    TBool oneclip = EFalse;

    if ( iView.Movie()->VideoClipCount() <= 1 )
        {
        removeIndex = 0;
        oneclip = ETrue;
        }


    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, R_VEI_POPUP_EDIT_IMAGE_TITLE, R_VEI_EDIT_IMAGE_LIST_BOX_ARRAY, removeIndex, EFalse );

    listSelection = selectedIndex;

    if ( selectedIndex >= 0 )
        {
        if ( oneclip )
            {
            listSelection++;
            }
        }

    switch ( listSelection )
        {
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditImageMove );
            break;
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditImageRemove );
            break;
        case 2:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditImageChangeDuration );
            break;
        case 3:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditImageBackGround );
            break;
        case 4:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditImageAddColorEffect );
            break;
        case 5:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditImageDuplicate );
            break;
        default:
            break;
        }

    }

TInt CVeiPopup::ShowTitleScreenBackgroundSelectionPopupL( TBool& aImageSelected )const
    {
    TInt selectedItem(  - 1 );
    CAknListQueryDialog* query = new( ELeave )CAknListQueryDialog( &selectedItem );
    query->PrepareLC( R_VEI_TITLESCREEN_BACKGROUND_LIST_QUERY );
    if ( !query->RunLD())
        {
        return KErrCancel;
        }

    if ( selectedItem == 0 )
        {
        aImageSelected = ETrue;
        }
    else
        {
        aImageSelected = EFalse;
        }

    return KErrNone;

    }

void CVeiPopup::ShowTitleScreenStyleSelectionPopupL()
    {
    TInt selectedIndex; // Selected item's index in popup list.
    TInt removeIndex; // Index to remove from list.

    removeIndex =  - 1; // -1 = Nothing to remove from list array.

    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, R_VEI_POPUP_SELECT_TEXT_STYLE_TITLE, R_VEI_INSERT_TEXT_LIST_BOX_ARRAY, removeIndex, EFalse );

    switch ( selectedIndex )
        {
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleTitle );
            break;
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleTitleFading );
            break;
        case 2:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleSubTitle );
            break;
        case 3:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleSubTitleFading );
            break;
        case 4:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditTextStyleCredit );
            break;
        default:
            break;
        }

    }

void CVeiPopup::ShowEditAudioPopupList()
    {
    TInt selectedIndex; // Selected item's index in popup list.
    TInt removeIndex; // Index to remove from list.

    removeIndex =  - 1; // -1 = Nothing to remove from list array.

    selectedIndex = ExecutePopupListL( R_AVKON_SOFTKEYS_SELECT_CANCEL, R_VEI_POPUP_EDIT_AUDIO_TITLE, R_VEI_EDIT_AUDIO_LIST_BOX_ARRAY, removeIndex, EFalse );

    switch ( selectedIndex )
        {
        case 0:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoCutting );
            break;
        case 1:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoMove );
            break;
        case 2:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditAudioSetDuration );
            break;
        case 3:
            iView.HandleCommandL( EVeiCmdEditVideoAdjustVolume );
            break;
        case 4:
            iView.HandleCommandL( EVeiCmdEditVideoViewEditVideoRemove );
            break;
        case 5:
            iView.HandleCommandL( EVeiCmdEditVideoDuplicate );
            break;
        default:
            break;
        }

    }

TBool CVeiPopup::ShowColorSelectorL( TRgb& aColor )const
    {
    TBool noneChosen = EFalse;
    /* None color option: True/False */


    TBool noneExist = EFalse;

    CArrayFixFlat < TRgb > * colors = new( ELeave )CArrayFixFlat < TRgb > ( 16 );
    CleanupStack::PushL( colors );

    colors->AppendL( TRgb( 0xffffff ));
    colors->AppendL( TRgb( 0xcccccc ));
    colors->AppendL( TRgb( 0x4d4d4d ));
    colors->AppendL( TRgb( 0x000000 ));
    colors->AppendL( TRgb( 0x00ffff ));
    colors->AppendL( TRgb( 0x44d8ff ));
    colors->AppendL( TRgb( 0x0268ff ));
    colors->AppendL( TRgb( 0x001ef1 ));
    colors->AppendL( TRgb( 0x00ffb9 ));
    colors->AppendL( TRgb( 0x00c873 ));
    colors->AppendL( TRgb( 0x026c3e ));
    colors->AppendL( TRgb( 0x0017c8 ));
    colors->AppendL( TRgb( 0xe7be7a ));
    colors->AppendL( TRgb( 0xff9b00 ));
    colors->AppendL( TRgb( 0xb36718 ));
    colors->AppendL( TRgb( 0x803e00 ));


    CAknColourSelectionGrid* d = CAknColourSelectionGrid::NewL( colors, noneExist, noneChosen, aColor );
    TBool selected = d->ExecuteLD();
    CleanupStack::PopAndDestroy( colors );

    //return !noneChosen;
    return selected;
    }

// End of File
