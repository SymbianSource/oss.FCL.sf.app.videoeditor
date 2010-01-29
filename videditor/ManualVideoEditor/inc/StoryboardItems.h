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


#ifndef STORYBOARDITEMS_H
#define STORYBOARDITEMS_H

#include <e32std.h>

class CFbsBitmap;


/**
 * Storyboard video item
 */
class CStoryboardVideoItem: public CBase
{
public:
    static CStoryboardVideoItem* NewL( const CFbsBitmap& aStartIcon, 
                                       const CFbsBitmap& aStartIconMask,
                                       const TDesC& aFilename,
                                       TBool aIsFile,
                                       const TDesC& aAlbum = KNullDesC );

    static CStoryboardVideoItem* NewLC( const CFbsBitmap& aStartIcon, 
                                        const CFbsBitmap& aStartIconMask,
                                        const TDesC& aFilename,
                                        TBool aIsFile,
                                        const TDesC& aAlbum = KNullDesC );

    virtual ~CStoryboardVideoItem();
    void InsertLastFrameL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask );
    void InsertFirstFrameL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask );
    void InsertTimelineFrameL( const CFbsBitmap& aBitmap, const CFbsBitmap& aMask );

    CStoryboardVideoItem();
private:

    void ConstructL( const CFbsBitmap& aStartIcon, 
                     const CFbsBitmap& aStartIconMask,
                     const TDesC& aFilename,
                     TBool aIsFile,
                     const TDesC& aAlbum );



public:
    CFbsBitmap* iIconBitmap;
    CFbsBitmap* iIconMask;

    CFbsBitmap* iLastFrameBitmap;
    CFbsBitmap* iLastFrameMask;

    CFbsBitmap* iTimelineBitmap;
    CFbsBitmap* iTimelineMask;

    TSize iIconSize;
    HBufC* iFilename;
    HBufC* iAlbumName;
    TBool iIsFile;
    TTime iDateModified;
};

/**
 * Storyboard audio item.
 */
class CStoryboardAudioItem: public CBase
{
public:
    static CStoryboardAudioItem* NewLC( TBool aRecordedAudio, 
                                        const TDesC& aFilename );
    virtual ~CStoryboardAudioItem();

private:
    CStoryboardAudioItem( TBool aRecordedAudio );
    void ConstructL( const TDesC& aFilename );

public:
    TBool iRecordedAudio;
    HBufC* iFilename;
};

#endif 

// End of File
