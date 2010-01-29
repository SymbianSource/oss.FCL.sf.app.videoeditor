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


#ifndef __VEDQUALITYSETTINGSAPI_INL__
#define __VEDQUALITYSETTINGSAPI_INL__

#include "vedqualitysettingsvariation.h"


inline TInt TVideoQualitySelector::NumberOfQualityLevels()
    {
    return ENumberOfQualitySets;
    }

inline void TVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, TInt aLevel, TBool aAspectRatioWide )
    {
    if ( !aAspectRatioWide )
        {
        GetVideoQualitySetL( aSet, aLevel );
        }
    else
        {
        // NOT READY
        TInt nrOfWideSets = ENumberOfWideQualitySets;
        if ( nrOfWideSets == 0 )
            {
            User::Leave( KErrNotSupported );
            }

        switch ( aLevel )
            {
            case EVideoQualityMMS :
                {
                aSet = KVideoQualitySetsWide[EQualityLow];
                }
                break;
            case EVideoQualityNormal :
                {
                if ( nrOfWideSets > 1 )
                    {
                    aSet = KVideoQualitySetsWide[EQualityMedium];
                    }
                else
                    {
                    aSet = KVideoQualitySetsWide[EQualityLow];
                    }
                }
                break;
            case EVideoQualityHigh :
                {
                if ( nrOfWideSets > 2 )
                    {
                    aSet = KVideoQualitySetsWide[EQualityHigh];
                    }
                else if ( nrOfWideSets > 1 )
                    {
                    aSet = KVideoQualitySetsWide[EQualityMedium];
                    }
                else
                    {
                    aSet = KVideoQualitySetsWide[EQualityLow];
                    }
                }
                break;
            default:
                aSet = KVideoQualitySetsWide[EQualityLow];
            }        
        }
    }


inline void TVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, TInt aLevel )
    {

    switch ( aLevel )
        {
        case EVideoQualityMMS :
            {
            aSet = KVideoQualitySets[EQualityLow];
            }
            break;
        case EVideoQualityNormal :
            {
            aSet = KVideoQualitySets[EQualityMedium];
            }
            break;
        case EVideoQualityHigh :
            {
            aSet = KVideoQualitySets[EQualityHigh];
            }
            break;
        default:
            if ( (aLevel > EVideoQualityHigh) || (aLevel < EVideoQualityMin) )
                {
                User::Leave( KErrArgument );
                }
            TInt moreSetsThanNominal = ENumberOfQualitySets - ENumberOfNominalLevels;
            if ( moreSetsThanNominal )
                {
                // there are also qualities in-between the nominal levels
                if ( aLevel < EVideoQualityMMS )
                    {
                    // even lower than the default MMS; no more than 1 step granularity there (atm)
                    aSet = KVideoQualitySets[EQualityLegacy];
                    }
                else if ( aLevel < EQualityMedium )
                    {
                    // between Low and Medium
                    TInt nrSetsBetweenMediumAndLow = EQualityMedium - EQualityLow - 1;
                    if ( nrSetsBetweenMediumAndLow > 0 )
                        {
                        // no better granularity yet
                        aSet = KVideoQualitySets[EQualityMedium-1];
                        }
                    else
                        {
                        // there are no specified quality in that range, give medium
                        aSet = KVideoQualitySets[EQualityMedium];
                        }
                    }
                else 
                    {
                    // between Medium and High
                    TInt nrSetsBetweenHighAndMedium = EQualityHigh - EQualityMedium - 1;
                    if ( nrSetsBetweenHighAndMedium > 0 )
                        {
                        // no better granularity yet
                        aSet = KVideoQualitySets[EQualityHigh-1];
                        }
                    else
                        {
                        // there are no specified quality in that range, give medium
                        aSet = KVideoQualitySets[EQualityMedium];
                        }
                    }
                }
            else
                {
                // round to the closest nominal level
                aSet = KVideoQualitySets[(aLevel-(EVideoQualityNominalGranularity/2))/EVideoQualityNominalGranularity];
                }
        }
    }

inline void TVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, const TSize& aVideoResolution )
    {
    // go through the qualities until a match is found. If several matches, pick the 1st one
    TInt candidate = -1;
    TInt i;
    for ( i = ENumberOfQualitySets-1; i >= 0; i-- ) // searches from up to down to find higher quality first
        {
        if ( (KVideoQualitySets[i].iVideoWidth == aVideoResolution.iWidth) && (KVideoQualitySets[i].iVideoHeight == aVideoResolution.iHeight) )
            {
            // we've found a set which matches with the requested size
            candidate = i;
            if ( (i == EQualityLow) || (i == EQualityMedium) || (i == EQualityHigh) )
                {
                // we've found a set which matches also with preferred qualities
                break;
                }
            }
        }
    if ( candidate >= 0 ) 
        {
        aSet = KVideoQualitySets[candidate];
        }
    else
        {
        User::Leave( KErrNotSupported );
        }
    }

inline void TVideoQualitySelector::GetVideoQualitySetL( SVideoQualitySet& aSet, const TPtrC8& aVideoCodecMimeType )
    {
    TPtrC8 settingsMimeType;
    
    TInt i;
    for ( i = ENumberOfQualitySets-1; i >= 0; i-- ) // searches from up to down to find higher quality first
        {
        settingsMimeType.Set(TPtrC8(KVideoQualitySets[i].iVideoCodecMimeType));
        if ( settingsMimeType.MatchF( (const TDesC8& )aVideoCodecMimeType ) != KErrNotFound ) 
            {
            // found
            aSet = KVideoQualitySets[i];
            return;
            }
        }
    User::Leave( KErrNotSupported );
    }

#endif
