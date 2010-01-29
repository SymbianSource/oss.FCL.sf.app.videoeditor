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
// User includes
#include <e32math.h> 

#include "SampleArrayHandler.h"
#include "VideoEditorCommon.h"      // Video Editor UID

#include "VideoEditorDebugUtils.h"

// ================= MEMBER FUNCTIONS =======================
CSampleArrayHandler* CSampleArrayHandler::NewL()
    {
    CSampleArrayHandler* self = CSampleArrayHandler::NewLC();
    CleanupStack::Pop( self );
    return self;
    }

CSampleArrayHandler* CSampleArrayHandler::NewLC()
    {
    CSampleArrayHandler* self = new( ELeave )CSampleArrayHandler();
    CleanupStack::PushL( self );
    self->ConstructL();
    return self;
    }

void CSampleArrayHandler::ConstructL()
    {
    }

CSampleArrayHandler::CSampleArrayHandler()
    {
    }


CSampleArrayHandler::~CSampleArrayHandler()
    {
    delete [] iVisualization;
    }

void CSampleArrayHandler::SetVisualizationArray(TInt8* aVisualization, TInt aResolution)
    {
    LOG( KVideoEditorLogFile, "CSampleArrayHandler::SetVisualizationArray, In" );
    iVisualization = aVisualization;
    iVisualizationSize = aResolution;

    // for testing
    /*	for (TInt i = 0; i < iVisualizationSize; i++)
    {
    if (i == 127)	
    iVisualization[i] = i - 127;
    else
    iVisualization[i] = i;				
    }
     */
    TInt8 temp = 0; // help variable used in finding the biggest current sample value
    for ( TInt i = 0; i < iVisualizationSize; i++ )
        {
        if ( iVisualization[i] > temp )
            {
            temp = iVisualization[i];
            }
        //LOGFMT2(KVideoEditorLogFile, "CSampleArrayHandler::SetVisualizationArray, SAMPLE[%d]:%d", i, iVisualization[i]);
        }
    iMaxSample = temp;

    iScaleFactor = 1.0; // TReal
    LOG( KVideoEditorLogFile, "CSampleArrayHandler::SetVisualizationArray, Out" );
    }

void CSampleArrayHandler::ScaleAudioVisualization( const TInt8& aNewMaxValue )
    {
    if ( iVisualization && ( aNewMaxValue != iMaxSampleInCurrentScale ))
        {
        iMaxSampleInCurrentScale = aNewMaxValue;
        if ( !Math::IsZero(( TReal64 )iMaxSampleInCurrentScale ))
            {
            iScaleFactor = ( TReal64 )iMaxSampleInCurrentScale / ( TReal64 )iMaxSample; 

            }

        /*
        //for testing
        if (1 > coeff)
        {				
        for (TInt i = 0; i < iVisualizationSize; i++)
        {
        LOGFMT(KVideoEditorLogFile, "before scaling:%d", iVisualization[i]);				
        iVisualization[i] *= coeff;		
        LOGFMT(KVideoEditorLogFile, "after scaling:%d", iVisualization[i]);
        }					
        }*/
        }
    }

TInt8 CSampleArrayHandler::Sample( const TInt aIndex )const
    {
    // must be scaled down in order to fit into screen			
    if ( iScaleFactor < 1 )
        {
        return (( iVisualization[aIndex]* 1000 )*( iScaleFactor* 1000 )) / 1000000;
                
        }
    else
        {
        return iVisualization[aIndex];
        }
    }

TInt CSampleArrayHandler::Size()const
    {
    return iVisualizationSize;
    }

TInt CSampleArrayHandler::CurrentPoint()const
    {
    return iCurrentIndex;
    }

void CSampleArrayHandler::SetCurrentPoint(const TTimeIntervalMicroSeconds& aCurrentTime)
    {
    iCurrentTime = aCurrentTime;
    iCurrentIndex = ( iCurrentTime.Int64() / 1000 ) / KAudioSampleInterval;
    }

void CSampleArrayHandler::SetCutInPoint(const TTimeIntervalMicroSeconds& aCutInTime)
    {
    iCutInTime = aCutInTime;
    iCutInSampleIndex = ( iCutInTime.Int64() / 1000 ) / KAudioSampleInterval;
    }

void CSampleArrayHandler::SetCutOutPoint(const TTimeIntervalMicroSeconds& aCutOutTime)
    {
    iCutOutTime = aCutOutTime;
    iCutOutSampleIndex = ( iCutOutTime.Int64() / 1000 ) / KAudioSampleInterval;
    }

TBool CSampleArrayHandler::SampleCutted( const TInt aIndex )const
    {
    return ( iCutInSampleIndex < aIndex ) && ( aIndex < iCutOutSampleIndex );
    }

// End of File
