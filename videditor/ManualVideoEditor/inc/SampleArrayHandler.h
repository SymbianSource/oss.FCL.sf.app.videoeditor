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


#ifndef VEISAMPLE_ARRAY_HANDLER_H
#define VEISAMPLE_ARRAY_HANDLER_H

#include <e32base.h>
/**
 * CSampleArrayHandler container control class.
 *  
 * Container for CVeiCutAudioView.
 */
class CSampleArrayHandler: public CBase
{
public:
    /**
     * Creates a CStoryboardContainer object, which will draw itself to aRect.
     *
     * @param aRect Frame rectangle for container.
     * @param aMovie  movie being edited
     *
     * @return a pointer to the created instance of CStoryboardContainer
     */
    static CSampleArrayHandler* NewL();

    /**  
     * Creates a CStoryboardContainer object, which will draw itself to aRect.
     * Leaves the created object in the cleanup stack.
     *
     * @param aRect Frame rectangle for container.
     * @param aMovie  movie being edited
     *
     * @return a pointer to the created instance of CStoryboardContainer
     */
    static CSampleArrayHandler* NewLC();

    /**
     * Default constructor.
     *
     * @param aRect  Frame rectangle for container.
     * @param aView  pointer to the view.
     */
    void ConstructL();

    /**
     * Destructor.
     */
    virtual ~CSampleArrayHandler();

public:

    void SetVisualizationArray( TInt8* aVisualization, TInt aResolution );

    TInt8 Sample( const TInt aIndex )const;
    void ScaleAudioVisualization( const TInt8& aHeight );
    TInt Size()const;
    TInt CurrentPoint()const;
    void SetCurrentPoint( const TTimeIntervalMicroSeconds& aTime );
    void SetCutInPoint( const TTimeIntervalMicroSeconds& aCutInTime );
    void SetCutOutPoint( const TTimeIntervalMicroSeconds& aCutOutTime );
    TBool SampleCutted( const TInt aIndex )const;

private:

    /**
     * Constructor.
     *
     * @param -
     */
    CSampleArrayHandler();

private:
    //data

    TInt8* iVisualization;
    TInt iVisualizationSize;
    TInt8 iMaxSample;
    TInt8 iMaxSampleInCurrentScale;
    TReal iScaleFactor;

    TTimeIntervalMicroSeconds iCurrentTime;
    TTimeIntervalMicroSeconds iCutInTime;
    TTimeIntervalMicroSeconds iCutOutTime;

    TInt iCurrentIndex;
    TInt iCutInSampleIndex;
    TInt iCutOutSampleIndex;

    TInt iMarkOutCounter;
    TInt iMarkInCounter;

    TInt iPreviousScreenMode;
    TInt iCurrentScreenMode;

    TTimeIntervalMicroSeconds iMarkedInTime;
    TTimeIntervalMicroSeconds iMarkedOutTime;
};
#endif 

// End of File
