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


#ifndef __VEDQUALITYSETTINGSAPI_H__
#define __VEDQUALITYSETTINGSAPI_H__

#include <e32base.h>
#include "imagingconfigmanager.h"



const TUint KQSMaxShortStringLength = 16;
const TUint KQSMaxLongStringLength = 256;



/*
 * Video quality set structure
 */
struct SVideoQualitySet
    {
    public:
        // Video aspect ratio: normal means one of the standard resolutions like QCIF, QVGA, CIF, VGA, if something else, like 16:9
        TBool iVideoAspectRatioNormal;
        // Video file format mime type, e.g. "video/3gpp"
        TText8 iVideoFileMimeType[KQSMaxShortStringLength];
        // Video codec mime type, e.g. "video/mp4v-es"
        TText8 iVideoCodecMimeType[KQSMaxLongStringLength];
        // Video picture width in pixels (luminance)
        TInt iVideoWidth;
        // Video picture height in pixels (luminance)
        TInt iVideoHeight;
        // Video framerate in fps
        TReal iVideoFrameRate;
        // Video bitrate in bps
        TInt iVideoBitRate;
        // Random access point rate, in pictures per second. For example, to request a random access point every ten seconds, set the value to 0.1
        TReal iRandomAccessRate;
        // Audio codec FourCC, e.g. " AMR"
        TText8 iAudioFourCCType[KQSMaxShortStringLength];
        // Audio bitrate in bps
        TInt iAudioBitRate;
        // Audio sampling rate in Hz
        TInt iAudioSamplingRate;
        // Number of audio channels; in practice mono(1) vs stereo(2)
        TInt iAudioChannels;
    };


/*
 * API class for getting a quality set for a given level or given resolution
 */
class CVideoQualitySelector : public CBase
    {
    public:
        /*
         * Enumeration for nominal video quality levels. Client must use these enumerations to get quality sets.
         * However, if NumberOfQualityLevels() below indicates there are more than ENumberOfNominalLevels, client
         * can use values in-between the nominal values to get set for some other quality level. 
         * It is however up to the implementation of the API to map such intermediate values to actual levels
         */
        enum TVideoQualityLevels
            {
            // client can try asking values in between these but the class may round the values depending on the availability
            EVideoQualityMin = 0,       // if there are several MMS compatible settings, EVideoQualityMMS is not the lowest. However level cannot go below this value
            EVideoQualityMMS = 10,      // use this when MMS compatible settings are needed
            EVideoQualityNormal = 20,   // use this when normal quality settings are needed, i.e. typically higher quality than MMS, but possibly not the highest still
            EVideoQualityHigh = 30,     // use this when highest possible quality settings are needed
            EVideoQualityNominalGranularity = 10,
            ENumberOfNominalLevels = 3
            };

    public:
        /*
         * Create new CVideoQualitySelector object
         */
        static CVideoQualitySelector* NewL();

        static CVideoQualitySelector* NewLC();
        
        /*
         * Destructor
         */
        ~CVideoQualitySelector();   

        /*
         *  Get number of defined quality levels. This is always at least ENumberOfNominalLevels but can be higher
         */
        TInt NumberOfQualityLevels();
        /*
         *  Get quality set associated with the given level. The level should be between EVideoQualityHigh and EVideoQualityMin.
         *  One the defined nominal levels should be used especially if NumberOfQualityLevels() == ENumberOfNominalLevels.
         *  If there is no set associated with given intermediate level, then set from a nearest level is returned.
         */
        void GetVideoQualitySetL( SVideoQualitySet& aSet, TInt aLevel );
        /*
         *  Get quality set associated with the given level. Otherwise the same as above but if aAspectRatioWide == ETrue, 
         *  tries to find 16:9 aspect ratio settings, otherwise returns a set from the normal aspect ratio as above.
         */
        void GetVideoQualitySetL( SVideoQualitySet& aSet, TInt aLevel, TBool aAspectRatioWide );
        /*
         *  Get quality set associated with the given video resolution. Leaves if there is no defined set for the resolution.
         *  E.g. if product configuration prefers QVGA over CIF, but client asks for CIF, this function leaves. The CIF settings
         *  can then be requested using the MIME-type
         */
        void GetVideoQualitySetL( SVideoQualitySet& aSet, const TSize& aVideoResolution );
        /*
         *  Get quality set associated with the given video codec MIME-type. Leaves if there is no defined set for the MIME-type.
         */
        void GetVideoQualitySetL( SVideoQualitySet& aSet, const TPtrC8& aVideoCodecMimeType );
        
    private:
        /*
         * Default constructor
         */
        CVideoQualitySelector();

        /*
         * Actual constructor
         */
        void ConstructL();
        
        /*
         * Copies the values from the source set to the target set
         */
        void MapQualitySet( SVideoQualitySet& aTargetSet, TVideoQualitySet& aSourceSet );
        
    private: // Data
    
        CImagingConfigManager* iConfigManager;
        
        CArrayFixFlat<TUint>* iQualityLevels;

    };

#endif
