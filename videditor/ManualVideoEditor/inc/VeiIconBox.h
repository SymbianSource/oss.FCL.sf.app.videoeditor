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




#ifndef VEIICONBOX_H
#define VEIICONBOX_H

#include <coecntrl.h>
#include <aknutils.h>

#include <aknview.h>
// FORWARD DECLARATIONS
class CFbsBitmap;


/**
 * VeiDummyCutBar control class.
 */
class CVeiIconBox: public CCoeControl /*, public MVideoPlayerUtilityObserver*/
{
public:
    /**
     * Destructor.
     */
    virtual ~CVeiIconBox();

    /** 
     * Static factory method.
     * 
     * @return  the created VeiDummyCutBar object
     */
    static CVeiIconBox* NewL( const TRect& aRect, const CCoeControl* aParent );

    /** 
     * Static factory method. Leaves the created object in the cleanup
     * stack.
     *
     * @return  the created CVeiCutAudioBar object
     */
    static CVeiIconBox* NewLC( const TRect& aRect, const CCoeControl* aParent );

public:
    void SetVolumeMuteIconVisibility( TBool aVisible );
    void SetSlowMotionIconVisibility( TBool aVisible );
    void SetBlackAndWhiteIconVisibility( TBool aVisible );
    void SetColourIconVisibility( TBool aVisible );
    void SetRecAudioIconVisibility( TBool aVisible );
    void SetPauseAudioIconVisibility( TBool aVisible );


    /**
     * Screen mode change 
     *
     */
    void SetLandscapeScreenOrientation( TBool aLandscapeScreenOrientation );

private:
    /**
     * Default constructor.
     *
     */
    void ConstructL( const TRect& aRect, const CCoeControl* aParent );

    /**
     * From CCoeControl,Draw.
     *
     * @param aRect  rectangle to draw
     */
    void Draw( const TRect& aRect )const;
    void SizeChanged();


private:
    // data

    CFbsBitmap* iVolumeMute;
    CFbsBitmap* iVolumeMuteMask;
    CFbsBitmap* iSlowMotion;
    CFbsBitmap* iSlowMotionMask;
    CFbsBitmap* iBlackAndWhite;
    CFbsBitmap* iBlackAndWhiteMask;
    CFbsBitmap* iColour;
    CFbsBitmap* iColourMask;
    CFbsBitmap* iRecAudio;
    CFbsBitmap* iRecAudioMask;
    CFbsBitmap* iPauseAudio;
    CFbsBitmap* iPauseAudioMask;

    TBool iVolumeMuteIconVisible;
    TBool iSlowMotionIconVisible;
    TBool iBlackAndWhiteIconVisible;
    TBool iColourIconVisible;
    TBool iRecAudioIconVisibile;
    TBool iPauseAudioIconVisibile;
    TBool iLandscapeScreenOrientation;


};
#endif 

// End of File
