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


#include "vedqualitysettingsapi.h"

const TUint KCameraDisplayID = 2;

// ---------------------------------------------------------------------------
// Constructor of CVideoQualitySelector
// ---------------------------------------------------------------------------
//
CVideoQualitySelector::CVideoQualitySelector()
    {
    }
    
// ---------------------------------------------------------------------------
// Destructor of CVideoQualitySelector
// ---------------------------------------------------------------------------
//
CVideoQualitySelector::~CVideoQualitySelector()
    {
    if( iConfigManager )
        {
        delete iConfigManager;
        iConfigManager = 0;
        }
        
    if( iQualityLevels )
        {
        delete iQualityLevels;
        iQualityLevels = 0;
        }
    }
    
// ---------------------------------------------------------------------------
// ConstructL() of CVideoQualitySelector
// ---------------------------------------------------------------------------
//
void CVideoQualitySelector::ConstructL()
    {
    iConfigManager = CImagingConfigManager::NewL();
    
    iQualityLevels = new (ELeave) CArrayFixFlat<TUint>(16);
    
    iConfigManager->GetVideoQualityLevelsL( *iQualityLevels, KCameraDisplayID );
    }
    
// ---------------------------------------------------------------------------
// NewL() of CVideoQualitySelector
// ---------------------------------------------------------------------------
//
CVideoQualitySelector* CVideoQualitySelector::NewL()
    {
    CVideoQualitySelector* self = CVideoQualitySelector::NewLC();
    CleanupStack::Pop( self );
    return self;
    }

// ---------------------------------------------------------------------------
// NewLC() of CVideoQualitySelector
// ---------------------------------------------------------------------------
//
CVideoQualitySelector* CVideoQualitySelector::NewLC()
    {
    CVideoQualitySelector* self = new( ELeave ) CVideoQualitySelector;
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

// ---------------------------------------------------------------------------
// Get number of defined quality levels.
// ---------------------------------------------------------------------------
//
TInt CVideoQualitySelector::NumberOfQualityLevels()
    {
    return iQualityLevels->Count();
    }

// ---------------------------------------------------------------------------
// Get quality set associated with the given level.
// ---------------------------------------------------------------------------
//
void CVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, TInt aLevel, TBool aAspectRatioWide )
    {
    if ( !aAspectRatioWide )
        {
        GetVideoQualitySetL( aSet, aLevel );
        }
    else
        {
        // NOT READY
        User::Leave( KErrNotSupported );     
        }
    }

// ---------------------------------------------------------------------------
// Get quality set associated with the given level.
// ---------------------------------------------------------------------------
//
void CVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, TInt aLevel )
    {
    TVideoQualitySet currentSet;
    
    switch ( aLevel )
        {
        case EVideoQualityMMS :
            {
            iConfigManager->GetVideoQualitySet( currentSet, CImagingConfigManager::EQualityLow, KCameraDisplayID );
            MapQualitySet( aSet, currentSet );
            }
            break;
            
        case EVideoQualityNormal :
            {
            iConfigManager->GetVideoQualitySet( currentSet, CImagingConfigManager::EQualityNormal, KCameraDisplayID );
            MapQualitySet( aSet, currentSet );
            }
            break;
            
        case EVideoQualityHigh :
            {
            iConfigManager->GetVideoQualitySet( currentSet, CImagingConfigManager::EQualityHigh, KCameraDisplayID );
            MapQualitySet( aSet, currentSet );
            }
            break;
            
        default:
            {
            if ( aLevel < EVideoQualityMin )
                {
                User::Leave( KErrArgument );
                }
            
            // Map our quality level to config manager's quality level   
            TInt configManagerLevel = aLevel * (CImagingConfigManager::EQualityHigh - CImagingConfigManager::EQualityMin);
            configManagerLevel /= (EVideoQualityHigh - EVideoQualityMin);
            
            iConfigManager->GetVideoQualitySet( currentSet, configManagerLevel, KCameraDisplayID );
            MapQualitySet( aSet, currentSet );
            }
            break;
        }
    }

// ---------------------------------------------------------------------------
// Get quality set associated with the given video resolution.
// ---------------------------------------------------------------------------
//
void CVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, const TSize& aVideoResolution )
    {
    TVideoQualitySet currentSet;
    TInt candidate = -1;
    TInt i;
    
    // Go through the qualities until a match is found. If several matches, pick the 1st one
    for ( i = iQualityLevels->Count() - 1; i >= 0; i-- ) // Searches from up to down to find higher quality first
        {
        iConfigManager->GetVideoQualitySet( currentSet, iQualityLevels->At( i ), KCameraDisplayID );
        
        if ( (currentSet.iVideoWidth == aVideoResolution.iWidth) && (currentSet.iVideoHeight == aVideoResolution.iHeight) )
            {
            // We've found a set which matches with the requested size
            
            if ( candidate == -1 )  // Don't set to worse if already found
                {
                candidate = i;
                }
            
            if ( (currentSet.iVideoQualitySetLevel == CImagingConfigManager::EQualityLow) ||
                 (currentSet.iVideoQualitySetLevel == CImagingConfigManager::EQualityNormal) ||
                 (currentSet.iVideoQualitySetLevel == CImagingConfigManager::EQualityHigh) )
                {
                // We've found a set which matches also with preferred qualities
                MapQualitySet( aSet, currentSet );
                return;
                }
            }
        }
    
    if ( candidate >= 0 ) 
        {
        iConfigManager->GetVideoQualitySet( currentSet, iQualityLevels->At( candidate ), KCameraDisplayID );
        MapQualitySet( aSet, currentSet );
        }
    else
        {
        User::Leave( KErrNotSupported );
        }
    }

// ---------------------------------------------------------------------------
// Get quality set associated with the given video codec MIME-type.
// ---------------------------------------------------------------------------
//
void CVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, const TPtrC8& aVideoCodecMimeType )
    {
    TVideoQualitySet currentSet;
    TPtrC8 settingsMimeType;
    TInt i;
    
    for ( i = iQualityLevels->Count() - 1; i >= 0; i-- ) // searches from up to down to find higher quality first
        {
        iConfigManager->GetVideoQualitySet( currentSet, iQualityLevels->At( i ), KCameraDisplayID );
        
        settingsMimeType.Set(TPtrC8(currentSet.iVideoCodecMimeType));
        if ( settingsMimeType.MatchF( (const TDesC8& )aVideoCodecMimeType ) != KErrNotFound ) 
            {
            // Found a match
            MapQualitySet( aSet, currentSet );
            return;
            }
        }
        
    User::Leave( KErrNotSupported );
    }
    
// ---------------------------------------------------------------------------
// Copies the values from the source set to the target set.
// ---------------------------------------------------------------------------
//
void CVideoQualitySelector::MapQualitySet( SVideoQualitySet& aTargetSet, TVideoQualitySet& aSourceSet )
    {
    TInt i = 0;
    
    // Video aspect ratio
    // We don't have wide screen quality sets yet so this is always true
    aTargetSet.iVideoAspectRatioNormal = ETrue;
    
    // Video file format mime type    
    for ( i = 0; (i < KQSMaxShortStringLength - 1) && (i < KMaxStringLength); i++ )
        {
        aTargetSet.iVideoFileMimeType[i] = aSourceSet.iVideoFileMimeType[i];
        }
    aTargetSet.iVideoFileMimeType[i] = '\0';    // Add null termination
    
    // Video codec mime type    
    for ( i = 0; (i < KQSMaxLongStringLength - 1) && (i < KMaxStringLength); i++ )
        {
        aTargetSet.iVideoCodecMimeType[i] = aSourceSet.iVideoCodecMimeType[i];
        }
    aTargetSet.iVideoCodecMimeType[i] = '\0';   // Add null termination
    
    // Video picture width in pixels (luminance)
    aTargetSet.iVideoWidth = aSourceSet.iVideoWidth;
    
    // Video picture height in pixels (luminance)
    aTargetSet.iVideoHeight = aSourceSet.iVideoHeight;
    
    // Video framerate in fps
    aTargetSet.iVideoFrameRate = aSourceSet.iVideoFrameRate;
    
    // Video bitrate in bps
    aTargetSet.iVideoBitRate = aSourceSet.iVideoBitRate;
    
    // Random access point rate, in pictures per second
    aTargetSet.iRandomAccessRate = aSourceSet.iRandomAccessRate;
    
    // Audio codec FourCC
    if ( aSourceSet.iAudioFourCCType == TFourCC(' ', 'A', 'M', 'R') )
    {
        // AMR
        aTargetSet.iAudioFourCCType[0] = ' ';
        aTargetSet.iAudioFourCCType[1] = 'A';
        aTargetSet.iAudioFourCCType[2] = 'M';
        aTargetSet.iAudioFourCCType[3] = 'R';
        aTargetSet.iAudioFourCCType[4] = '\0';
    } 
    
    else if (aSourceSet.iAudioFourCCType == TFourCC(' ', 'A', 'A', 'C') )
    {
        // AAC
        aTargetSet.iAudioFourCCType[0] = ' ';
        aTargetSet.iAudioFourCCType[1] = 'A';
        aTargetSet.iAudioFourCCType[2] = 'A';
        aTargetSet.iAudioFourCCType[3] = 'C';
        aTargetSet.iAudioFourCCType[4] = '\0';
    } 
    else 
        aTargetSet.iAudioFourCCType[0] = '\0';

    // Audio bitrate in bps
    aTargetSet.iAudioBitRate = aSourceSet.iAudioBitRate;
    
    // Audio sampling rate in Hz
    aTargetSet.iAudioSamplingRate = aSourceSet.iAudioSamplingRate;
    
    // Number of audio channels
    aTargetSet.iAudioChannels = aSourceSet.iAudioChannels;
    }

