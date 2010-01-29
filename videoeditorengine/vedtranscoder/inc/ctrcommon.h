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
* Common elements / data types.
*
*/


#ifndef CTRCOMMON_H
#define CTRCOMMON_H

// INCLUDES
#include <e32std.h>


/**
*  Video coding options
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class TTRVideoCodingOptions
    {
    public:
        // Segment interval in picture. In H.263 baseline this means number of non-empty GOB headers 
        // (1=every GOB has a header), in H.264 and MPEG-4 the segement size in bytes. 
        // Default is 0 == no segments inside picture
        // Coding standard & used profile etc. limit the value.
        TInt iSyncIntervalInPicture;
        
        // Time between random access points (I-Frame)
        TUint iMinRandomAccessPeriodInSeconds;

        // Relevant to MPEG4 only. Specifies whether data partitioning is in use. 
        // When equal to ETrue, data partitioning is in use.
        TBool iDataPartitioning;

        // Relevant to MPEG4 only. Specifies whether reversible variable length coding is in use. 
        // When equal to ETrue, reversible variable length coding is in use. 
        // Valid only if iDataPartitioned is equal to ETrue.
        TBool iReversibleVLC;

        // Relevant to MPEG4 only. Header extension code. 
        TUint iHeaderExtension;
    };


/**
*  Video format
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class TTRVideoFormat
    {
    public:
        // Video picture size
        TSize iSize;

        // Video data type
        TInt iDataType;
    };



/**
*  Video picture
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class TTRVideoPicture
    {
    public:
        // Picture data
        TPtr8* iRawData;

        // Picture size in pixels
        TSize iDataSize;

        // Picture timestamp
        TTimeIntervalMicroSeconds iTimestamp;

        // Queue element
        TDblQueLink iLink; 

        // Misc user info
        TAny* iUser;
    };


/**
*  Display options
*  @lib TRANSCODER.LIB
*  @since 3.1
*/
class TTRDisplayOptions
    {
    public:
        // The video output rectangle on screen
        TRect   iVideoRect;

        // Initial clipping region to use
        TRegion iClipRegion;
    };


/**
*  TTREventItem
*  @lib TRANSCODER.LIB
*/
class CTREventItem : public CBase
    {
    public:
        // Timestamp from which to start iAction
        TTimeIntervalMicroSeconds iTimestamp;
        
        // EnableEncoder setting status
        TBool iEnableEncoderStatus; 
        
        // Enable PS setting status
        TBool iEnablePictureSinkStatus;
        
        // RandomAccess client's setting
        TBool iRandomAccessStatus;
        
        // Enable / Disable encoder client setting
        TBool iEnableEncoderClientSetting;
        
        // Enable / Disable picture sink client's setting
        TBool iEnablePictureSinkClientSetting;
        
        // Queue link
        TDblQueLink iLink;
        
    public:
        // Reset item's setting
        inline void Reset()
            {
            // Reset setting statuses & ts
            iTimestamp = -1;
            iEnableEncoderStatus = EFalse;
            iEnablePictureSinkStatus = EFalse;
            iRandomAccessStatus = EFalse;
            };
    };

#endif
