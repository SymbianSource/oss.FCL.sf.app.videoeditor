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


#ifndef __VEDQUALITYSETTINGSVARIATION_H__
#define __VEDQUALITYSETTINGSVARIATION_H__


#include "vedqualitysettingsapi.h"

#if 0

/*
 *  This is a sample configuration
 */

/*
 *  Enumerations for video levels.
 *  Each configuration MUST have the following enumerations defined. They can be overlapping, but they must exist. Otherwise the compiling of
 *  of video applications will fail.
 */
enum TVideoInternalQualityLevels
    {
    EQualityLegacy = 0,     // If a set exist that is lower than the default MMS, e.g. subQCIF
    EQualityLow = 0,        // Low / MMS quality
    EQualityMedium = 1,     // Medium/normal quality
    EQualityHigh = 2,       // Highest quality
    ENumberOfQualitySets = 3
    };

/*
 *  Definition of video quality sets.
 *  Each configuration MUST have the following variable with size ENumberOfQualitySets
 *  For explanation of the members, please see vedqualitysettingsapi.h
 */
static const SVideoQualitySet KVideoQualitySets[ENumberOfQualitySets] =
    {
        //a sample set for MMS == EQualityLow
        { ETrue, "video/3gpp", "video/H263-2000", 176, 144, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },
            // other sets for other levels
    };


#endif









/*
 *  Actual variations of video settings for different configurations
 */
 
#ifdef NCP_COMMON_TEFLON_FAMILY
// Quality sets for 

enum TVideoInternalQualityLevels
    {
    EQualityLegacy = 0,
    EQualityLow = 1,
    EQualityMedium = 2,
    EQualityHigh = 6,
    ENumberOfQualitySets = 7,
    ENumberOfWideQualitySets = 0
    };

static const SVideoQualitySet KVideoQualitySets[ENumberOfQualitySets] =
    {
        // Legacy subQCIF support
        { ETrue, "video/3gpp", "video/H263-2000", 128, 96, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },
        // MMS
        { ETrue, "video/3gpp", "video/H263-2000", 176, 144, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },
        // Normal
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=2", 320, 240, 15.0, 512000, 0.5, " AAC", 48000, 16000, 1 },
        // Normal+
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=3", 320, 240, 30.0, 1024000, 0.5, " AAC", 48000, 16000, 1 },
        // CIF settings in case of CIF input is dominating, to avoid transcoding to QVGA
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=2", 352, 288, 15.0, 512000, 0.5, " AAC", 48000, 16000, 1 },
        // HighMinus; level 4a
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 480, 15.0, 2048000, 1.0, " AAC", 96000, 48000, 1 },
        // High; level 4a
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 480, 30.0, 3072000, 1.0, " AAC", 96000, 48000, 1 }
    };

static const SVideoQualitySet KVideoQualitySetsWide[1] = {{ EFalse, "", "", 0, 0, 0.0, 0, 0, "", 0, 0, 0 }};

#else
// Quality sets e.g. for , WINSCW

#ifndef SPP_VSW_VIDEOEDITORENGINE_AVC_EDITING
enum TVideoInternalQualityLevels
    {
    EQualityLegacy = 0,
    EQualityLow = 1,
    EQualityMedium = 2,
    EQualityHigh = 3,
    ENumberOfQualitySets = 5,
    ENumberOfWideQualitySets = 4
    };

static const SVideoQualitySet KVideoQualitySets[ENumberOfQualitySets] =
    {
        // Legacy subQCIF support
        { ETrue, "video/3gpp", "video/H263-2000", 128, 96, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },
        // MMS
        { ETrue, "video/3gpp", "video/H263-2000", 176, 144, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },
        // Normal; H.263 level 45
        { ETrue, "video/3gpp", "video/H263-2000; profile=0; level=45", 176, 144, 15.0, 124000, 0.2, " AMR", 12200, 8000, 1 },
        // High
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=2", 320, 240, 15.0, 512000, 0.5, " AAC", 72000, 48000, 1 },
        // High+
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=2", 352, 288, 15.0, 512000, 0.5, " AAC", 72000, 48000, 1 },
    };
    
static const SVideoQualitySet KVideoQualitySetsWide[ENumberOfWideQualitySets] =
    {
        // Legacy 
        { ETrue, "video/3gpp", "video/H263-2000", 128, 96, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },        
        // Low
        { EFalse, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 352, 10.0, 256000, 1.0, " AAC", 72000, 48000, 1 },        
        // Medium
        { EFalse, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 352, 15.0, 1024000, 1.0, " AAC", 72000, 48000, 1 },        
        // High
        { EFalse, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 352, 15.0, 2048000, 1.0, " AAC", 72000, 48000, 1 },
    };    

    
#else // AVC support

enum TVideoInternalQualityLevels
    {
    EQualityLegacy = 0,
    EQualityLow = 1,
    EQualityMedium = 3,
    EQualityHigh = 7,
    ENumberOfQualitySets = 9,
    ENumberOfWideQualitySets = 8
    };

static const SVideoQualitySet KVideoQualitySets[ENumberOfQualitySets] =
    {
    
        // Legacy subQCIF support
        { ETrue, "video/3gpp", "video/H263-2000", 128, 96, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },
        // MMS
        { ETrue, "video/3gpp", "video/H263-2000", 176, 144, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },
        // MMS+1
        { ETrue, "video/3gpp", "video/H264; profile-level-id=42900B", 176, 144, 15.0, 124000, 0.2, " AMR", 12200, 8000, 1 },
        // Medium
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=2", 320, 240, 15.0, 512000, 0.5, " AAC", 72000, 48000, 1 },
        // Medium+
        { ETrue, "video/mp4", "video/H264; profile-level-id=42800C", 320, 240, 15.0, 384000, 0.5, " AAC", 72000, 48000, 1 },
        // Medium++
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=2", 352, 288, 15.0, 512000, 0.5, " AAC", 72000, 48000, 1 },
        // Medium+++
        { ETrue, "video/mp4", "video/H264; profile-level-id=42800C", 352, 288, 15.0, 384000, 0.5, " AAC", 72000, 48000, 1 },
        // High
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 480, 15.0, 2048000, 1.0, " AAC", 72000, 48000, 1 },
        // High+
        { ETrue, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 480, 30.0, 3072000, 1.0, " AAC", 72000, 48000, 1 },
        
    };
    
static const SVideoQualitySet KVideoQualitySetsWide[ENumberOfWideQualitySets] =
    {        
        
        // Legacy 
        { ETrue, "video/3gpp", "video/H263-2000", 128, 96, 15.0, 60000, 0.2, " AMR", 12200, 8000, 1 },        
        // Low
        { EFalse, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 352, 10.0, 256000, 1.0, " AAC", 72000, 48000, 1 },
        // dummy       
        { EFalse, "", "", 0, 0, 0.0, 0, 0, "", 0, 0, 0 },        
        // Medium
        { EFalse, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 352, 15.0, 1024000, 1.0, " AAC", 72000, 48000, 1 },
        // dummy
        { EFalse, "", "", 0, 0, 0.0, 0, 0, "", 0, 0, 0 },
        // dummy
        { EFalse, "", "", 0, 0, 0.0, 0, 0, "", 0, 0, 0 },
        // dummy
        { EFalse, "", "", 0, 0, 0.0, 0, 0, "", 0, 0, 0 },                
        // High
        { EFalse, "video/mp4", "video/mp4v-es; profile-level-id=4", 640, 352, 25.0, 2048000, 1.0, " AAC", 72000, 48000, 1 },                
        
    };    

#endif


//static const SVideoQualitySet KVideoQualitySetsWide[1] = {{ EFalse, "", "", 0, 0, 0.0, 0, 0, "", 0, 0, 0 }};

#endif

#endif
