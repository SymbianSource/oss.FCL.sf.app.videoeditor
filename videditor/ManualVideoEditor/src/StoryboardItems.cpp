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
#include <fbs.h>

// User includes
#include "StoryboardItems.h"

// local constants
const TInt KNoThumbnailFrameWidth = 8;


// ================= MEMBER FUNCTIONS =======================

/* **********************************************************************
 * CStoryboardVideoItem
 * **********************************************************************/
CStoryboardVideoItem* CStoryboardVideoItem::NewL( const CFbsBitmap& aStartIcon, 
                                                  const CFbsBitmap& aStartIconMask, 
                                                  const TDesC& aFilename,
                                                  TBool aIsFile,
                                                  const TDesC& aAlbum )
    {
    CStoryboardVideoItem* self = CStoryboardVideoItem::NewLC( aStartIcon,
        aStartIconMask, aFilename, aIsFile, aAlbum );
    CleanupStack::Pop( self );
    return self;
    }

CStoryboardVideoItem* CStoryboardVideoItem::NewLC( const CFbsBitmap& aStartIcon, 
                                                   const CFbsBitmap& aStartIconMask, 
                                                   const TDesC& aFilename, 
                                                   TBool aIsFile,
                                                   const TDesC& aAlbum )
    {
    CStoryboardVideoItem* self = new( ELeave )CStoryboardVideoItem();
    CleanupStack::PushL( self );
    self->ConstructL( aStartIcon, aStartIconMask, aFilename, aIsFile, aAlbum );
    return self;
    }

CStoryboardVideoItem::~CStoryboardVideoItem()
    {
    if ( iFilename )
        {
        delete iFilename;
        }

    if ( iAlbumName )
        {
        delete iAlbumName;
        }

    delete iIconBitmap;
    delete iIconMask;

    delete iLastFrameBitmap;
    delete iLastFrameMask;

    delete iTimelineBitmap;
    delete iTimelineMask;
    }

CStoryboardVideoItem::CStoryboardVideoItem()
    {
    }

void CStoryboardVideoItem::InsertLastFrameL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask )
    {
    delete iLastFrameBitmap;
    iLastFrameBitmap = NULL;
    delete iLastFrameMask;
    iLastFrameMask = NULL;

    CFbsBitmap* icon = new( ELeave )CFbsBitmap;
    icon->Duplicate( aBitmap.Handle());
    CFbsBitmap* mask = new( ELeave )CFbsBitmap;
    mask->Duplicate( aMask.Handle());

    iLastFrameBitmap = icon;
    iLastFrameMask = mask;
    }

void CStoryboardVideoItem::InsertFirstFrameL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask )
    {
    delete iIconBitmap;
    iIconBitmap = NULL;
    delete iIconMask;
    iIconMask = NULL;

    CFbsBitmap* icon = new( ELeave )CFbsBitmap;
    icon->Duplicate( aBitmap.Handle());
    CFbsBitmap* mask = new( ELeave )CFbsBitmap;
    mask->Duplicate( aMask.Handle());

    iIconBitmap = icon;
    iIconMask = mask;
    }

void CStoryboardVideoItem::InsertTimelineFrameL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask )
    {
    delete iTimelineBitmap;
    iTimelineBitmap = NULL;
    delete iTimelineMask;
    iTimelineMask = NULL;

    CFbsBitmap* icon = new( ELeave )CFbsBitmap;
    icon->Duplicate( aBitmap.Handle());
    CFbsBitmap* mask = new( ELeave )CFbsBitmap;
    mask->Duplicate( aMask.Handle());

    iTimelineBitmap = icon;
    iTimelineMask = mask;
    }

void CStoryboardVideoItem::ConstructL( const CFbsBitmap& aStartIcon, 
                                       const CFbsBitmap& aStartIconMask, 
                                       const TDesC& aFilename,
                                       TBool aIsFile,
                                       const TDesC& aAlbum )
    {
    CFbsBitmap* icon = new( ELeave )CFbsBitmap;
    icon->Duplicate( aStartIcon.Handle());
    CFbsBitmap* mask = new( ELeave )CFbsBitmap;
    mask->Duplicate( aStartIconMask.Handle());

    TSize thumbResolution;
	thumbResolution.iWidth = ( aStartIcon.SizeInPixels() ).iWidth-KNoThumbnailFrameWidth;
	thumbResolution.iHeight = ( aStartIcon.SizeInPixels() ).iHeight-KNoThumbnailFrameWidth;

    iIconSize = thumbResolution;
    iIconBitmap = icon;
    iIconMask = mask;

    iFilename = HBufC::NewL( aFilename.Length());
    *iFilename = aFilename;

    iAlbumName = HBufC::NewL( aAlbum.Length());
    *iAlbumName = aAlbum;

    iDateModified.HomeTime();
    iIsFile = aIsFile;
    }

/* **********************************************************************
 * CStoryboardAudioItem
 * **********************************************************************/

CStoryboardAudioItem* CStoryboardAudioItem::NewLC( TBool aRecordedAudio, 
                                                   const TDesC& aFilename )
    {
    CStoryboardAudioItem* self = new( ELeave )CStoryboardAudioItem(
                                     aRecordedAudio );
    CleanupStack::PushL( self );
    self->ConstructL( aFilename );
    return self;
    }


CStoryboardAudioItem::~CStoryboardAudioItem()
    {
    delete iFilename;
    }


CStoryboardAudioItem::CStoryboardAudioItem(TBool aRecordedAudio)
                                            : iRecordedAudio(aRecordedAudio)
    {
    }

void CStoryboardAudioItem::ConstructL( const TDesC& aFilename )
    {
    iFilename = HBufC::NewL( aFilename.Length());
    *iFilename = aFilename;
    }

// End of File
