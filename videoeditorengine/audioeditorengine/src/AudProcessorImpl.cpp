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



#include "AudProcessorImpl.h"
#include "ProcAMRInFileHandler.h"

#include "ProcMP4InFileHandler.h"
#include "ProcADTSInFileHandler.h"
#include "ProcMP3InFileHandler.h"
#include "ProcAWBInFileHandler.h"
#include "ProcWAVInFileHandler.h"


#include "ProcAMRFrameHandler.h"
#include "ProcAACFrameHandler.h"
#include "ProcMP3FrameHandler.h"
#include "ProcAWBFrameHandler.h"
#include "ProcWAVFrameHandler.h"



#include "AudCommon.h"
#include "ProcTools.h"
#include "AudPanic.h"

// Debug print macro
#if defined _DEBUG 
#include <e32svr.h>
#define PRINT(x) RDebug::Print x;
#else
#define PRINT(x)
#endif

const TInt KTimeEstimateTime = 2000; // 1000 ms

CAudProcessorImpl* CAudProcessorImpl::NewL()
    {


    CAudProcessorImpl* self = new (ELeave) CAudProcessorImpl();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

CAudProcessorImpl* CAudProcessorImpl::NewLC()
    {
    CAudProcessorImpl* self = new (ELeave) CAudProcessorImpl();
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

CAudProcessorImpl::~CAudProcessorImpl() 
    {
    
	PRINT((_L("CAudProcessorImpl::~CAudProcessorImpl() in")));
    TInt a = 0;
    
    for (a = 0; a < iInFiles.Count() ; a++) 
        {
        delete iInFiles[a];
        }
    iInFiles.Reset();
    
	PRINT((_L("CAudProcessorImpl::~CAudProcessorImpl() deleted iInFiles")));
    delete iWAVFrameHandler; 

    iClipsWritten.Reset();

    iProcessingEvents.ResetAndDestroy();

	PRINT((_L("CAudProcessorImpl::~CAudProcessorImpl() out")));
    }
    

void CAudProcessorImpl::ConstructL() 
    {
    
    

    }

CAudProcessorImpl::CAudProcessorImpl() : iInFiles(8), iSong(0), 
                                            iTimeLeft(0),
                                            iCurEvent(0),
                                            iProcessingEvents(8),
                                            iSongDurationMilliSeconds(0),
                                            iSongProcessedMilliSeconds(0)
                                        
    {


    }

void CAudProcessorImpl::ProcessSongL(const CAudSong* aSong, TInt aRawFrameSize, TBool aGetTimeEstimation) 
{
    PRINT((_L("CAudProcessorImpl::ProcessSongL in")));

    iGetTimeEstimation = aGetTimeEstimation;
    
    iSilenceStarted = EFalse;
    
    iSong = aSong;
    
    TInt clips = aSong->iClipArray.Count();
    
    
    // bitdepth always 16
    const TInt KBitDepth = 16;
    
    iWAVFrameHandler = CProcWAVFrameHandler::NewL(KBitDepth);
    
    // create inFileHandlers for each input clips--->
    for (TInt a = 0 ; a < clips ; a++) 
    {
        if (aSong->iClipArray[a]->Info()->Properties().iFileFormat == EAudFormatAMR) 
        {
            PRINT((_L("CAudProcessorImpl::ProcessSongL create AMRNB file handler")));
            CProcInFileHandler* inFileHandler = 
                CProcAMRInFileHandler::NewL(aSong->iClipArray[a]->Info()->FileName(), 
                aSong->iClipArray[a]->Info()->FileHandle(), aSong->iClipArray[a],
                4096, iSong->OutputFileProperties().iSamplingRate,
                iSong->OutputFileProperties().iChannelMode);
            
            iInFiles.Insert(inFileHandler, iInFiles.Count());
            inFileHandler->SetPropertiesL(aSong->iClipArray[a]->Info()->Properties());
            inFileHandler->SetPriority(aSong->iClipArray[a]->Priority());
        
            inFileHandler->SeekCutInFrame();
            
            if (IsDecodingRequired(aSong->OutputFileProperties(), 
                                   aSong->iClipArray[a]->Info()->Properties()))
                {
                inFileHandler->SetDecodingRequired(ETrue);
                }
                
                
                
            
        }            
        else if (aSong->iClipArray[a]->Info()->Properties().iFileFormat == EAudFormatMP4) 
        {
            PRINT((_L("CAudProcessorImpl::ProcessSongL create MP4 file handler")));
            CProcInFileHandler* inFileHandler = 
                CProcMP4InFileHandler::NewL(aSong->iClipArray[a]->Info()->FileName(), 
                aSong->iClipArray[a]->Info()->FileHandle(),
                aSong->iClipArray[a],
                4096, iSong->OutputFileProperties().iSamplingRate,
                iSong->OutputFileProperties().iChannelMode);
            
            iInFiles.Insert(inFileHandler, iInFiles.Count());
            TAudFileProperties prororo;
            inFileHandler->GetPropertiesL(&prororo);
            inFileHandler->SetPriority(aSong->iClipArray[a]->Priority());
            inFileHandler->SeekCutInFrame();
            
            if (IsDecodingRequired(aSong->OutputFileProperties(), 
                                   aSong->iClipArray[a]->Info()->Properties()))
                {
                inFileHandler->SetDecodingRequired(ETrue);
                }
            
            
            
        }
        
        else if (aSong->iClipArray[a]->Info()->Properties().iFileFormat == EAudFormatAAC_ADTS) 
        {
            PRINT((_L("CAudProcessorImpl::ProcessSongL create AAC ADTS file handler")));
            CProcInFileHandler* inFileHandler = 
                CProcADTSInFileHandler::NewL(aSong->iClipArray[a]->Info()->FileName(), 
                aSong->iClipArray[a]->Info()->FileHandle(),
                aSong->iClipArray[a],
                8092, iSong->OutputFileProperties().iSamplingRate,
                iSong->OutputFileProperties().iChannelMode);
            TAudFileProperties pro;
            
            iInFiles.Insert(inFileHandler, iInFiles.Count());
            inFileHandler->GetPropertiesL(&pro);
            inFileHandler->SetPriority(aSong->iClipArray[a]->Priority());
            inFileHandler->SeekCutInFrame();
            
            if (IsDecodingRequired(aSong->OutputFileProperties(), 
                                   aSong->iClipArray[a]->Info()->Properties()))
                {
                inFileHandler->SetDecodingRequired(ETrue);
                }
            
                
            
        }
        else if (aSong->iClipArray[a]->Info()->Properties().iFileFormat == EAudFormatMP3) 
            {
            PRINT((_L("CAudProcessorImpl::ProcessSongL create MP3 file handler")));
            CProcInFileHandler* inFileHandler = 
                CProcMP3InFileHandler::NewL(aSong->iClipArray[a]->Info()->FileName(), 
                aSong->iClipArray[a]->Info()->FileHandle(), 
                aSong->iClipArray[a],
                4096, iSong->OutputFileProperties().iSamplingRate,
                iSong->OutputFileProperties().iChannelMode);
            
            iInFiles.Insert(inFileHandler, iInFiles.Count());
            inFileHandler->SetPropertiesL(aSong->iClipArray[a]->Info()->Properties());
            inFileHandler->SetPriority(aSong->iClipArray[a]->Priority());
            inFileHandler->SeekCutInFrame();
            
            
            if (IsDecodingRequired(aSong->OutputFileProperties(), 
                                   aSong->iClipArray[a]->Info()->Properties()))
                {
                inFileHandler->SetDecodingRequired(ETrue);
                }
            
            }
        else if (aSong->iClipArray[a]->Info()->Properties().iFileFormat == EAudFormatAMRWB) 
            {
            PRINT((_L("CAudProcessorImpl::ProcessSongL create AMRWB file handler")));
            CProcInFileHandler* inFileHandler = 
                CProcAWBInFileHandler::NewL(aSong->iClipArray[a]->Info()->FileName(), 
                aSong->iClipArray[a]->Info()->FileHandle(), 
                aSong->iClipArray[a],
                4096, iSong->OutputFileProperties().iSamplingRate,
                iSong->OutputFileProperties().iChannelMode);
            
            iInFiles.Insert(inFileHandler, iInFiles.Count());
            inFileHandler->SetPropertiesL(aSong->iClipArray[a]->Info()->Properties());
            inFileHandler->SetPriority(aSong->iClipArray[a]->Priority());
            inFileHandler->SeekCutInFrame();
            
            if (IsDecodingRequired(aSong->OutputFileProperties(), 
                                   aSong->iClipArray[a]->Info()->Properties()))
                {
                inFileHandler->SetDecodingRequired(ETrue);
                }
            
            }
        else if (aSong->iClipArray[a]->Info()->Properties().iFileFormat == EAudFormatWAV) 
            {
            PRINT((_L("CAudProcessorImpl::ProcessSongL create WAV file handler")));
            CProcInFileHandler* inFileHandler = 
                CProcWAVInFileHandler::NewL(aSong->iClipArray[a]->Info()->FileName(), 
                aSong->iClipArray[a]->Info()->FileHandle(), 
                aSong->iClipArray[a],
                4096, iSong->OutputFileProperties().iSamplingRate,
                iSong->OutputFileProperties().iChannelMode);
            
            iInFiles.Insert(inFileHandler, iInFiles.Count());
            inFileHandler->SetPropertiesL(aSong->iClipArray[a]->Info()->Properties());
            inFileHandler->SetPriority(aSong->iClipArray[a]->Priority());
            inFileHandler->SeekCutInFrame();
            
            if (IsDecodingRequired(aSong->OutputFileProperties(), 
                                   aSong->iClipArray[a]->Info()->Properties()))
                {
                inFileHandler->SetDecodingRequired(ETrue);
                }
            
            }
        
        }
    
    
    // find the biggest frame size for mixing buffer
    TInt maxFrameSize = 0;
    for (TInt q = 0 ; q < iInFiles.Count() ; q++)
        {
        iInFiles[q]->SetRawAudioFrameSize(aRawFrameSize);
        
        if (iInFiles[q]->GetDecodedFrameSize() > maxFrameSize)
            {
            maxFrameSize = iInFiles[q]->GetDecodedFrameSize();
            }
            
        }
        
        
    
    GetProcessingEventsL();
    iProcessingEvents[0]->GetAllIndexes(iClipsWritten);
    iTimeLeft = iProcessingEvents[1]->iPosition;
    iCurEvent = 0;
    
    
    if (iGetTimeEstimation)
        {
        iTimer.HomeTime();
        
        if (iTimeLeft >= KTimeEstimateTime)
            {
            iTimeEstimateCoefficient = iTimeLeft/KTimeEstimateTime;
            
            iTimeLeft = KTimeEstimateTime;
            }
        else
            {
            iTimeEstimateCoefficient = 1;
            }
        
        }
    
        
    PRINT((_L("CAudProcessorImpl::ProcessSongL out")));
    }

TBool CAudProcessorImpl::ProcessSyncPieceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration, TBool& aRaw) 
{
    
    PRINT((_L("CAudProcessorImpl::ProcessSyncPieceL in")));
    aDuration = 0;
    
    if (iSongProcessedMilliSeconds > iSongDurationMilliSeconds)
        {
        // processing is ready
        StopProcessing();
        PRINT((_L("CAudProcessorImpl::ProcessSyncPieceL stopped processing 1, out")));
        return ETrue;
        
        }
    
    
    while (iTimeLeft <= 0) 
        {
        
        iCurEvent++;
    
        if (iCurEvent >= iProcessingEvents.Count()-1) 
            {

            // do we need silence in the end?
            if (iSongProcessedMilliSeconds < ProcTools::MilliSeconds(iSong->iSongDuration))
                {
                
                if (iGetTimeEstimation)
                    {
                    StopProcessing();
                    PRINT((_L("CAudProcessorImpl::ProcessSyncPieceL stopped processing in time estimate case, out")));
                    return ETrue;        
                    
                    }
                
               
                WriteSilenceL(aFrame, aProgress, aDuration, aRaw);
                return EFalse;
                }
            // processing is ready
            StopProcessing();
            PRINT((_L("CAudProcessorImpl::ProcessSyncPieceL stopped processing 2, out")));
            return ETrue;
            }
        
        iClipsWritten.Reset();
        for (TInt tr = 0 ; tr < iProcessingEvents[iCurEvent]->IndexCount() ; tr++)
            {
            
            TInt clipIndex = iProcessingEvents[iCurEvent]->GetIndex(tr);

            
            if (clipIndex == -1 || !iSong->Clip(clipIndex, KAllTrackIndices)->Muting())
                {
                iClipsWritten.Append(iProcessingEvents[iCurEvent]->GetIndex(tr));
                
                }
            
            }
        iTimeLeft = iProcessingEvents[iCurEvent+1]->iPosition -
                iProcessingEvents[iCurEvent]->iPosition;
                
        if (iGetTimeEstimation)
            {
            
            TTime timeNow;
            timeNow.HomeTime();
            
            // how long has it been from the previous event?
            TTimeIntervalMicroSeconds msFromPrev = timeNow.MicroSecondsFrom(iTimer);
            
            iTimeEstimate += msFromPrev.Int64() * iTimeEstimateCoefficient;
            
            // set iTimer to home time
            iTimer.HomeTime();
            
            if (iTimeLeft > KTimeEstimateTime)
                {
                iTimeEstimateCoefficient = iTimeLeft/KTimeEstimateTime;
                iTimeLeft = KTimeEstimateTime;
                }
            else
                {
                iTimeEstimateCoefficient = 1;
                }
            
            }
                    
        }
    
    
    if (iClipsWritten.Count() == 1) 
        {
        // if silence
        WriteSilenceL(aFrame, aProgress, aDuration, aRaw);
        PRINT((_L("CAudProcessorImpl::ProcessSyncPieceL silence, out")));
        return EFalse;
        
        }
    else if (iTimeLeft > 0) 
        {


        TInt inClipIndex1 = 0;
        TInt inClipIndex2 = 0;
        
        HighestInFilePriority(inClipIndex1, inClipIndex2);
        
        CAudClip* clip1 = iSong->iClipArray[iClipsWritten[inClipIndex1]];
        CAudClip* clip2 = iSong->iClipArray[iClipsWritten[inClipIndex2]];
        
        CProcInFileHandler* inHandler1 = iInFiles[iClipsWritten[inClipIndex1]];
        CProcInFileHandler* inHandler2 = iInFiles[iClipsWritten[inClipIndex2]];
        
        
        TBool writeClip1 = ETrue;
        TBool writeClip2 = ETrue;
        
        if (clip1 == clip2)
            {
            writeClip2 = EFalse;
            }
        if (clip1->Muting())
            {
            writeClip1 = EFalse;
            }
        if (clip2->Muting())
            {
            writeClip2 = EFalse;
            }


        HBufC8* frame1 = 0;
        HBufC8* frame2 = 0;
        TInt siz1 = 0;
        TInt32 tim1 = 0;
        TInt siz2 = 0;
        TInt32 tim2 = 0;
        
        TBool getFrameRet1 = EFalse;
        TBool getFrameRet2 = EFalse;
        // If the clip is muted -> return silence
        
        if (!writeClip1 && !writeClip2)
            {
            
            getFrameRet1 = inHandler1->GetSilentAudioFrameL(frame1, siz1, tim1, aRaw);
            
            }
            
        else if (!writeClip1 && writeClip2)
            {
            
            getFrameRet2 = inHandler2->GetAudioFrameL(frame2, siz2, tim2, aRaw);
                
            }
        else if (writeClip1 && !writeClip2)
            {
            
            getFrameRet1 = inHandler1->GetAudioFrameL(frame1, siz1, tim1, aRaw);
                
            }
        else
            {
            
            TBool decodingRequired1 = inHandler1->DecodingRequired();
            TBool decodingRequired2 = inHandler2->DecodingRequired();
            
            // decoding is needed due to mixing
            inHandler1->SetDecodingRequired(ETrue);
            inHandler2->SetDecodingRequired(ETrue);
            
            getFrameRet1 = inHandler1->GetAudioFrameL(frame1, siz1, tim1, aRaw);
            
            // fix to rel2, put frame1 to cleanup stack for the next operation
            CleanupStack::PushL(frame1);
            getFrameRet2 = inHandler2->GetAudioFrameL(frame2, siz2, tim2, aRaw);
            CleanupStack::Pop(); // frame1, will be put to cleanupstack later->


            inHandler1->SetDecodingRequired(decodingRequired1);
            inHandler2->SetDecodingRequired(decodingRequired2);
            
            }
            
        
        if(!getFrameRet1 && !getFrameRet2)
            {
            // no audio frames left -> write silence
        
            getFrameRet1 = inHandler1->GetSilentAudioFrameL(frame1, siz1, tim1, aRaw);
                            
        }
        
        if (frame1 != 0)
            {
            CleanupStack::PushL(frame1);
            }
        if (frame2 != 0)
            {
            CleanupStack::PushL(frame2);
            }
            

        if (getFrameRet1 && getFrameRet2)
            {
           
            // mix the two frames
            iWAVFrameHandler->MixL(frame1, frame2, aFrame);
            CleanupStack::PopAndDestroy(frame2);
            CleanupStack::PopAndDestroy(frame1);
            aDuration = TUint(tim1*1000);
            
            }
        else if (getFrameRet1 && !getFrameRet2)
            {
            aFrame = HBufC8::NewL(frame1->Length());
            aFrame->Des().Copy(frame1->Des());
            CleanupStack::PopAndDestroy(frame1);
            aDuration = TUint(tim1*1000);
            
            
            }
        else if (!getFrameRet1 && getFrameRet2)
            {
            
            aFrame = HBufC8::NewL(frame2->Length());
            aFrame->Des().Copy(frame2->Des());
            CleanupStack::PopAndDestroy(frame2);
            aDuration = TUint(tim2*1000);
            
            }
        else if (!getFrameRet1 && !getFrameRet2)
            {
            
            // shouldn't get here...
            
            User::Leave(KErrGeneral);
            }

        
        iTimeLeft -= ProcTools::MilliSeconds(aDuration);
        iSongProcessedMilliSeconds += ProcTools::MilliSeconds(aDuration);
        
        
    }
    
    if (iSongDurationMilliSeconds > 0)
        {
        
        // update progress
        aProgress = (iSongProcessedMilliSeconds*100)/iSongDurationMilliSeconds;
            
        }
    PRINT((_L("CAudProcessorImpl::ProcessSyncPieceL out")));
    return EFalse;
    
    }


void CAudProcessorImpl::GetAudFilePropertiesL(const TDesC& aFileName, RFile* aFileHandle,
                                              TAudFileProperties* aProperties) 
    {

    PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL in")));
    
    

    PRINT(_L("CAudProcessorImpl::GetAudFilePropertiesL read header from file"));
    
    TBuf8<10> fileHeader;

    if (!aFileHandle)
    {
        RFs* fs = new (ELeave) RFs;
        CleanupStack::PushL(fs);
        User::LeaveIfError( fs->Connect() );
    
        RFile file;    
        TInt error = file.Open(*fs, aFileName, EFileShareReadersOnly | EFileStream | EFileRead);//EFileShareAny | EFileStream | EFileRead);
        if (error != KErrNone)
            {
            error = file.Open(*fs, aFileName, EFileShareAny | EFileStream | EFileRead);
            }
        if (error == KErrNone)
            {
            error = file.Read(fileHeader);
            }
        file.Close();
        fs->Close();
        CleanupStack::PopAndDestroy(fs);
        User::LeaveIfError(error);
    } 
    else
    {
        TInt pos = 0;
        
        User::LeaveIfError( aFileHandle->Seek(ESeekCurrent, pos) );
        
        TInt zero = 0;        
        User::LeaveIfError( aFileHandle->Seek(ESeekStart, zero) );
        User::LeaveIfError( aFileHandle->Read(fileHeader) );
        
        User::LeaveIfError( aFileHandle->Seek(ESeekStart, pos) );
    }
        
    if (fileHeader.Length() < 10 )  //AMR-WB has 9-byte header, but header-only clips are not accepted. Hence accepting only 10 and more byte clips.
        {
        PRINT(_L("CAudProcessorImpl::GetAudFilePropertiesL the file has less than 9 bytes, it must be invalid"));        
        User::Leave(KErrCorrupt);
        }

    PRINT(_L("CAudProcessorImpl::GetAudFilePropertiesL interpret the header"));
    
    TParse fp;
    TFileName name;
        
    if (!aFileHandle)
        fp.Set(aFileName, NULL, NULL);
    else
    {
        User::LeaveIfError( aFileHandle->FullName(name) );
        fp.Set(name, NULL, NULL);
    }    
    
    if ( fileHeader.Mid(4,4) == _L8("ftyp") )
            {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL 3gp/mp4/m4a based on ftyp")));
        // 3gp/mp4/m4a; extension-based recognition later
        CProcInFileHandler* inFileHandler = CProcMP4InFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);
        
        CleanupStack::PopAndDestroy(inFileHandler);
        }
    else if (fileHeader.Mid(0,6) == _L8("#!AMR\n"))
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL AMR-NB based on #!AMR")));
        // AMR-NB
        CProcInFileHandler* inFileHandler = CProcAMRInFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        CleanupStack::PushL(inFileHandler);

        inFileHandler->GetPropertiesL(aProperties);    
        
        CleanupStack::PopAndDestroy(inFileHandler);
        
        }
    else if (fileHeader.Mid(0,9) == _L8("#!AMR-WB\n")) 
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL awb based on #!AMR-WB")));
        CProcInFileHandler* inFileHandler = CProcAWBInFileHandler::NewL(aFileName, aFileHandle,0, 4096);
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);

        CleanupStack::PopAndDestroy(inFileHandler);
        }
    else if (fp.Ext().CompareF(_L(".aac")) == 0) 
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL aac based on extension")));
        CProcInFileHandler* inFileHandler = CProcADTSInFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);

        CleanupStack::PopAndDestroy(inFileHandler);
        }
    else if (fp.Ext().CompareF(_L(".3gp")) == 0) 
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL 3gp based on extension")));

        CProcInFileHandler* inFileHandler = CProcMP4InFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);
        
        CleanupStack::PopAndDestroy(inFileHandler);
    
        }

    else if (fp.Ext().CompareF(_L(".m4a")) == 0 || 
             fp.Ext().CompareF(_L(".mp4")) == 0) 
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL mp4/m4a based on extension")));
        
        CProcInFileHandler* inFileHandler = 0;

        inFileHandler = CProcMP4InFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);    
        CleanupStack::PopAndDestroy(inFileHandler);
        
        }
    else if (fp.Ext().CompareF(_L(".amr")) == 0)
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL AMR-NB based on extension")));
        // AMR-NB
        CProcInFileHandler* inFileHandler = CProcAMRInFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        CleanupStack::PushL(inFileHandler);

        inFileHandler->GetPropertiesL(aProperties);    
        
        CleanupStack::PopAndDestroy(inFileHandler);
        }
    else if (fp.Ext().CompareF(_L(".awb")) == 0)
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL awb based on extension")));
        CProcInFileHandler* inFileHandler = CProcAWBInFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);

        CleanupStack::PopAndDestroy(inFileHandler);
        }
    else if (fp.Ext().CompareF(_L(".mp3")) == 0) 
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL mp3 based on extension")));
        
        CProcInFileHandler* inFileHandler = CProcMP3InFileHandler::NewL(aFileName, aFileHandle, 0, 4096);
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);
        
        CleanupStack::PopAndDestroy(inFileHandler);
        }
    else if (fp.Ext().CompareF(_L(".wav")) == 0) 
        {
        PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL wav based on extension")));
        
        CProcInFileHandler* inFileHandler = CProcWAVInFileHandler::NewL(aFileName, aFileHandle, 0, 8192);
        CleanupStack::PushL(inFileHandler);
        inFileHandler->GetPropertiesL(aProperties);
        CleanupStack::PopAndDestroy(inFileHandler);
        }
    else
        {
        User::Leave(KErrNotSupported);
        }
    
    PRINT((_L("CAudProcessorImpl::GetAudFilePropertiesL out")));
    }
    
    
TInt64 CAudProcessorImpl::GetFinalTimeEstimate() const
    {
    return iTimeEstimate;
    }

void CAudProcessorImpl::GetProcessingEventsL() 
    {

    TInt clips = iSong->iClipArray.Count();

    CProcessingEvent* newEvent = 0;
    
    TInt a = 0;

    // create a new processing event from each clips cutIn and cutOut
    for (a = 0 ; a < clips ; a++) 
        {
        
        
        // cut in time:
        newEvent = CProcessingEvent::NewL();
        newEvent->iChangedClipIndex = a;
        newEvent->iCutIn = ETrue;
        TInt32 startTime = iSong->iClipArray[a]->StartTimeMilliSeconds();
        TInt32 cutInTime= iSong->iClipArray[a]->CutInTimeMilliSeconds(); 
        newEvent->iPosition = startTime+cutInTime;
        iProcessingEvents.Insert(newEvent, 0);

        // cut out time
        newEvent = CProcessingEvent::NewL();
        newEvent->iChangedClipIndex = a;
        newEvent->iCutIn = EFalse;
        TInt32 cutOutTime= iSong->iClipArray[a]->CutOutTimeMilliSeconds(); 
        newEvent->iPosition = startTime+cutOutTime;
        iProcessingEvents.Insert(newEvent, 0);

        }

    
    

    // order processing events
    TLinearOrder<CProcessingEvent> order(CProcessingEvent::Compare);
    iProcessingEvents.Sort(order);

    // add a new processing events in the beginning to represent silence
    // (there is a possibility that the first clip doesn't start from 0 ms)
    newEvent = CProcessingEvent::NewL();
    newEvent->iChangedClipIndex = -1;
    newEvent->InsertIndex(-1);
    newEvent->iCutIn = ETrue;
    newEvent->iPosition = 0;
    iProcessingEvents.Insert(newEvent, 0);

    // the next for-loop adds indexes of those clips that are supposed to be mixed to
    // each processing event

    for (TInt r = 1; r < iProcessingEvents.Count() ; r++) 
        {
        
        
        for (TInt i = 0 ; i < iProcessingEvents[r-1]->IndexCount() ; i++)
            {
            iProcessingEvents[r]->InsertIndex(iProcessingEvents[r-1]->GetIndex(i));
            }

        
        if (iProcessingEvents[r]->iCutIn)
            {
            iProcessingEvents[r]->InsertIndex(iProcessingEvents[r]->iChangedClipIndex);
            }
        else
            {

            TInt oldIndexInArray = iProcessingEvents[r]->FindIndex(iProcessingEvents[r]->iChangedClipIndex);

            if (oldIndexInArray >= 0 && oldIndexInArray < iProcessingEvents[r]->IndexCount())
                {
                iProcessingEvents[r]->RemoveIndex(oldIndexInArray);
                }

            }

        }
                
    //iSongDurationMilliSeconds = (iSong->iSongDuration.Int64()/1000);
    iSongDurationMilliSeconds = ProcTools::MilliSeconds(iSong->iSongDuration);
    
    //iProcessingEvents[iProcessingEvents.Count()-1]->iPosition;


    }




TBool CAudProcessorImpl::StopProcessing()
    {

    if (iGetTimeEstimation)
        {
        
        TTime timeNow;
        timeNow.HomeTime();
        
        // how long has it been from the previous event?
        TTimeIntervalMicroSeconds msFromPrev = timeNow.MicroSecondsFrom(iTimer);
        
        iTimeEstimate += msFromPrev.Int64() * iTimeEstimateCoefficient;
        }


    iProcessingEvents.ResetAndDestroy();
    iInFiles.ResetAndDestroy();

    return ETrue;
    }

TBool CAudProcessorImpl::WriteSilenceL(HBufC8*& aFrame, TInt& aProgress,
                                       TTimeIntervalMicroSeconds& aDuration, TBool& aRaw)                                
    {
    HBufC8* silentFrame = 0;
    TInt silSize = 0;
    TInt32 silDur = 0;

    if (iSong->iProperties->iAudioType == EAudAAC_MPEG4 && 
        iSong->iProperties->iChannelMode == EAudSingleChannel)
        {
        
        aDuration = ((1024*1000)/(iSong->iProperties->iSamplingRate))*1000;
        
        silDur = ProcTools::MilliSeconds(aDuration);
        
        aFrame = HBufC8::NewL(KSilentMonoAACFrameLenght);
        aFrame->Des().Append(KSilentMonoAACFrame, KSilentMonoAACFrameLenght);
        
        aRaw = EFalse;

        }
    else if (iSong->iProperties->iAudioType == EAudAAC_MPEG4 && 
             iSong->iProperties->iChannelMode == EAudStereo)
        {

        aDuration = ((1024*1000)/(iSong->iProperties->iSamplingRate))*1000;
        
        silDur = ProcTools::MilliSeconds(aDuration);
        
        aFrame = HBufC8::NewL(KSilentStereoAACFrameLenght);    
        aFrame->Des().Append(KSilentStereoAACFrame, KSilentStereoAACFrameLenght);
        
        aRaw = EFalse;

        }
    else
        {
        iInFiles[0]->GetSilentAudioFrameL(silentFrame, silSize, silDur, aRaw);
    
        aFrame= silentFrame;
        aDuration = TUint(silDur*1000);
        
        }
    
    iSongProcessedMilliSeconds += silDur;
    iTimeLeft -= silDur;
    
    if (iSongDurationMilliSeconds > 0)
        {
        aProgress = (iSongProcessedMilliSeconds*100)/iSongDurationMilliSeconds;    
        }
        
    return ETrue;
    }


TBool CAudProcessorImpl::HighestInFilePriority(TInt& aFirst, TInt& aSecond)
    {
    
    aFirst = 1;
    aSecond = 1;
    
    TInt highest = 1;
    
    TInt secondHighest = 1;
    
 
    for (TInt a = 1 ; a < iClipsWritten.Count() ; a++)
        {
        if (iInFiles[iClipsWritten[a]]->Priority() >= highest)
            {
            
            // highest priority
            aFirst = a;
            highest = iInFiles[iClipsWritten[a]]->Priority();
            
            }
        else if (iInFiles[iClipsWritten[a]]->Priority() >= secondHighest)
            {
            aSecond = a;
            secondHighest = iInFiles[iClipsWritten[a]]->Priority();
            }
 
        }
        
 
    return ETrue;
    }
    
TBool CAudProcessorImpl::IsDecodingRequired(const TAudFileProperties& prop1, 
                                            const TAudFileProperties& prop2)
    {
    
    TBool decodingNeeded = EFalse;
    TAudFileProperties tmpProp1 = prop1;
    TAudFileProperties tmpProp2 = prop2;
    
 
    if (tmpProp1.iAudioType != 
        tmpProp2.iAudioType ||
        tmpProp1.iSamplingRate != 
        tmpProp2.iSamplingRate ||
        tmpProp1.iChannelMode != 
        tmpProp2.iChannelMode)
        {
        decodingNeeded = ETrue;
        }
        
    return decodingNeeded;
    
    
    }

    

