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




#include "ProcVisProcessor.h"
#include "audconstants.h"

#include "ProcMP4InFileHandler.h"
#include "ProcADTSInFileHandler.h"
#include "ProcMP3InFileHandler.h"
#include "ProcAWBInFileHandler.h"
#include "ProcWAVInFileHandler.h"
#include "ProcAMRInFileHandler.h"


#include "ProcAMRFrameHandler.h"
#include "ProcAACFrameHandler.h"
#include "ProcMP3FrameHandler.h"
#include "ProcAWBFrameHandler.h"
#include "ProcWAVFrameHandler.h"

CProcVisProcessor* CProcVisProcessor::NewL()
    {


    CProcVisProcessor* self = new (ELeave) CProcVisProcessor();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CProcVisProcessor* CProcVisProcessor::NewLC()
    {
    CProcVisProcessor* self = new (ELeave) CProcVisProcessor();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }


CProcVisProcessor::~CProcVisProcessor()
    {

    if (iVisualization != 0)
        {
        delete[] iVisualization;
        }
    if (iInFile != 0)
        {
        delete iInFile;
        }
    if (iFrameHandler != 0)
        {
        delete iFrameHandler;
        }

    }

void CProcVisProcessor::VisualizeClipL(const CAudClipInfo* aClipInfo, TInt aSize)
    {

        // initialize...
    iClipInfo = aClipInfo;
    iVisualizationSize = aSize;
    iVisualization = new (ELeave) TInt8[iVisualizationSize];
    

    if (aClipInfo->Properties().iFileFormat == EAudFormatAMR) 
        {
        CProcInFileHandler* inFileHandler = CProcAMRInFileHandler::NewL(aClipInfo->FileName(), 
                                                                        aClipInfo->FileHandle(), 
                                                                        0, 2048);
        iInFile = inFileHandler;
        iFrameHandler = CProcAMRFrameHandler::NewL();
        TAudFileProperties properties;
        inFileHandler->GetPropertiesL(&properties);
        inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);
        }
    else if (aClipInfo->Properties().iFileFormat == EAudFormatAMRWB) 
        {
        CProcInFileHandler* inFileHandler = CProcAWBInFileHandler::NewL(aClipInfo->FileName(), 
                                                                        aClipInfo->FileHandle(), 
                                                                        0, 2048);
        iInFile = inFileHandler;
        iFrameHandler = CProcAWBFrameHandler::NewL();
        TAudFileProperties properties;
        inFileHandler->GetPropertiesL(&properties);
        inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);
        }
    else if (aClipInfo->Properties().iFileFormat == EAudFormatMP3) 
        {
        CProcInFileHandler* inFileHandler = CProcMP3InFileHandler::NewL(aClipInfo->FileName(), 
                                                                        aClipInfo->FileHandle(), 
                                                                        0, 2048);
        iInFile = inFileHandler;
        iFrameHandler = CProcMP3FrameHandler::NewL();
        TAudFileProperties properties;
        inFileHandler->GetPropertiesL(&properties);
        inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);
        }
    else if (aClipInfo->Properties().iFileFormat == EAudFormatWAV) 
        {
        CProcInFileHandler* inFileHandler = CProcWAVInFileHandler::NewL(aClipInfo->FileName(), 
                                                                        aClipInfo->FileHandle(), 
                                                                        0, 8192);
        iInFile = inFileHandler;
        TAudFileProperties properties;
        inFileHandler->GetPropertiesL(&properties);
        inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);
        iFrameHandler = CProcWAVFrameHandler::NewL(properties.iNumberOfBitsPerSample);
        }
    

    else if (aClipInfo->Properties().iFileFormat == EAudFormatMP4 &&                                                           
            aClipInfo->Properties().iAudioType == EAudAAC_MPEG4)
        {

        CProcInFileHandler* inFileHandler = CProcMP4InFileHandler::NewL(aClipInfo->FileName(), 
                                                                        aClipInfo->FileHandle(), 
                                                                        0, 8192);

        iInFile = inFileHandler;
        TAudFileProperties properties;
        
        inFileHandler->GetPropertiesL(&properties);
        
        inFileHandler->SetPropertiesL(properties);
        //inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);

        TAACFrameHandlerInfo frameInfo;
        CProcMP4InFileHandler* MP4inFileHandler = static_cast<CProcMP4InFileHandler*>(inFileHandler);
        MP4inFileHandler->GetInfoForFrameHandler(frameInfo);
        
        iFrameHandler = CProcAACFrameHandler::NewL(frameInfo);    

        }
    else if (aClipInfo->Properties().iFileFormat == EAudFormatMP4 &&
            (aClipInfo->Properties().iAudioType == EAudAMR))
        {

        CProcInFileHandler* inFileHandler = CProcMP4InFileHandler::NewL(aClipInfo->FileName(), 
                                                                        aClipInfo->FileHandle(), 
                                                                        0, 8092);

        iInFile = inFileHandler;
        TAudFileProperties properties;
        
        inFileHandler->GetPropertiesL(&properties);
        
        inFileHandler->SetPropertiesL(properties);
        //inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);

        iFrameHandler = CProcAMRFrameHandler::NewL();    

        }
    else if (aClipInfo->Properties().iFileFormat == EAudFormatMP4 &&
            (aClipInfo->Properties().iAudioType == EAudAMRWB))
        {

        CProcInFileHandler* inFileHandler = CProcMP4InFileHandler::NewL(aClipInfo->FileName(), 
                                                                        aClipInfo->FileHandle(), 
                                                                        0, 8092);

        iInFile = inFileHandler;
        TAudFileProperties properties;
        
        inFileHandler->GetPropertiesL(&properties);
        
        inFileHandler->SetPropertiesL(properties);
        //inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);

        iFrameHandler = CProcAWBFrameHandler::NewL();    

        }
    else if (aClipInfo->Properties().iFileFormat == EAudFormatAAC_ADTS &&
            aClipInfo->Properties().iAudioType == EAudAAC_MPEG4
            )
        {

        CProcInFileHandler* inFileHandler = CProcADTSInFileHandler::NewL(aClipInfo->FileName(), 
                                                                         aClipInfo->FileHandle(), 
                                                                        0, 8092);



        iInFile = inFileHandler;
        TAudFileProperties properties;
        
        inFileHandler->GetPropertiesL(&properties);
        
        inFileHandler->SetPropertiesL(properties);
        iFrameAmount = properties.iFrameCount;
        inFileHandler->SeekAudioFrame(0);

        TAACFrameHandlerInfo frameInfo;
        CProcADTSInFileHandler* ADTSinFileHandler = static_cast<CProcADTSInFileHandler*>(inFileHandler);
        ADTSinFileHandler->GetInfoForFrameHandler(frameInfo);
        
        iFrameHandler = CProcAACFrameHandler::NewL(frameInfo);    

        }


    }

TBool CProcVisProcessor::VisualizeClipPieceL(TInt &aProgress)
    {

    if (iInFile == 0)
        return ETrue;
    
    if (iVisualizationPos >= iVisualizationSize) 
    {
        if (iInFile != 0)
        {
            delete iInFile;
            iInFile = 0;
        }
        
        if (iFrameHandler != 0)
        {
            delete iFrameHandler;
            iFrameHandler = 0;
        }
        return ETrue;
    }
    
    HBufC8* point = 0;
    TInt siz;
    TInt32 tim = 0;
    
    RArray<TInt> gains;
    
    TAudType audType = iClipInfo->Properties().iAudioType;
    
    
    TInt maxGain = 0;
    TInt gainAmount = 0;
    TInt divider = 0;
    for (TInt b = 0 ; b < 2 ; b++)
    {
        gains.Reset();
        
        
        TBool decReq = iInFile->DecodingRequired();
        iInFile->SetDecodingRequired(EFalse);
        TBool tmp = EFalse;
        
        if (!iInFile->GetAudioFrameL(point, siz, tim, tmp)) 
        {
            iInFile->SetDecodingRequired(decReq);
            
            delete iInFile;
            iInFile = 0;
            delete iFrameHandler;
            iFrameHandler = 0;
            return ETrue;
            
        }
        iInFile->SetDecodingRequired(decReq);
        CleanupStack::PushL(point);
        
        iFramesProcessed++;
        if (siz > 0) 
        {
            
            
            iFrameHandler->GetGainL(point, gains, maxGain);
            
            if (point != NULL) 
            {
                CleanupStack::PopAndDestroy(point);
            }
            
            
            if (audType == EAudAMR || audType == EAudAMRWB)
            {
                
                for (TInt c = 0 ; c < gains.Count() ; c++)
                {
                    if (gains[c] > gainAmount)
                    {
                        gainAmount = gains[c];
                    }
                    
                    //gainAmount+=gains[c];
                }
            }
            else if (audType == EAudAAC_MPEG4)
            {
                for (TInt c = 0 ; c < gains.Count() ; c++)
                {
                    if (gains[c] > gainAmount)
                    {
                        gainAmount = gains[c];
                    }
                    
                    //gainAmount+=gains[c];
                }
            }        
            else if (audType == EAudMP3)
            {
                for (TInt c = 0 ; c < gains.Count() ; c++)
                {
                    
                    gainAmount+=gains[c];
                    divider++;
                }
                //gainAmount /= gains.Count();
            }
            else if (audType == EAudWAV)
                {
                for (TInt c = 0 ; c < gains.Count() ; c++)
                    {
                    
                    gainAmount+=gains[c];
                    divider++;
                    }
                //gainAmount /= gains.Count();
                }
            
        }
        
        else 
        {
            CleanupStack::PopAndDestroy(point);
            // silent frame
            gains.Append(0);
            gains.Append(0);
            maxGain = 1;
        }
    }
    
    
    if (audType == EAudMP3 && divider > 0) gainAmount/= divider;
    if (audType == EAudWAV && divider > 0) gainAmount/= divider;
    
    
    
    iVisualizationProcessed = (iFramesProcessed*KAedMaxVisualizationResolution)/iFrameAmount;
    
    
    while (iVisualizationWritten <= iVisualizationProcessed)
    {
        if (iVisualizationPos >= iVisualizationSize)
        {
            break;
        }
        
        if (audType == EAudAAC_MPEG4)
        {
            
            TReal ga(gainAmount-100);
            TReal exponent= ga/36;
            TReal visResult(0);
            Math::Pow(visResult, 10, exponent);
            
            TInt16 visR = 0;
            TInt err = Math::Int(visR, visResult);
            
            if (visR > 127) visR = 127;
            
            if (err == KErrNone)
            {
                iVisualization[iVisualizationPos] = static_cast<TInt8>(visR);
            }
            else
            {
                iVisualization[iVisualizationPos] = 0;
                
            }
            
        }
        else 
        {
            TInt visValue = (127*gainAmount)/maxGain;
            
            if (visValue < 0)
                {
                visValue = 0;
                }
            else if (visValue > KMaxTInt8)
                {
                visValue = KMaxTInt8;
                }
        
            iVisualization[iVisualizationPos] = static_cast<TInt8>(visValue);
        }
        iVisualizationPos++;
        iVisualizationWritten = (KAedMaxVisualizationResolution*iVisualizationPos)/iVisualizationSize;
    }
    
    
    gains.Reset();
    aProgress = iVisualizationWritten/(KAedMaxVisualizationResolution/100); // convert resolution to percentages
    return EFalse;


    }

void CProcVisProcessor::GetFinalVisualizationL(TInt8*& aVisualization, TInt& aSize)
    {

    if (iVisualization == 0)
        {
        User::Leave(KErrNotReady);
        }

    aVisualization = new (ELeave) TInt8[iVisualizationSize];
    
    for (TInt a = 0 ; a < iVisualizationSize ; a++)
        {
        aVisualization[a] = iVisualization[a];
        }
    aSize = iVisualizationSize;

    delete[] iVisualization;
    iVisualization = 0;

    }


void CProcVisProcessor::ConstructL()
    {

    }

CProcVisProcessor::CProcVisProcessor()
    {

    }
    
