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




#include "VedVideoClip.h"
#include "VedMovieImp.h"
#include "VedVideoClipInfoGeneratedImp.h"
#include "VedVideoClipInfoImp.h"
#include "AudClip.h"
#include "AudObservers.h"

CVedVideoClip* CVedVideoClip::NewL(CVedMovieImp* aMovie, const TDesC& aFileName,
                                   TInt aIndex, CAudClip* aAudioClip, 
                                   MVedVideoClipInfoObserver& aObserver)
    {
    CVedVideoClip* self = new (ELeave) CVedVideoClip(aMovie, aAudioClip);
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aIndex, aObserver);
    CleanupStack::Pop(self);
    return self;
    }


CVedVideoClip* CVedVideoClip::NewL(CVedMovieImp* aMovie, CVedVideoClipGenerator& aGenerator, 
                                   TInt aIndex, MVedVideoClipInfoObserver& aObserver, TBool aIsOwnedByVideoClip)
    {
    CVedVideoClip* self = new (ELeave) CVedVideoClip(aMovie, NULL /*CAudClip*/);
    CleanupStack::PushL(self);
    self->ConstructL(aGenerator, aIndex, aObserver, aIsOwnedByVideoClip);
    CleanupStack::Pop(self);
    return self;
    }
    
CVedVideoClip* CVedVideoClip::NewL(CVedMovieImp* aMovie, RFile* aFileHandle,
							       TInt aIndex, CAudClip* aAudioClip,
							       MVedVideoClipInfoObserver& aObserver)
    {
    CVedVideoClip* self = new (ELeave) CVedVideoClip(aMovie, aAudioClip);
    CleanupStack::PushL(self);
    self->ConstructL(aFileHandle, aIndex, aObserver);
    CleanupStack::Pop(self);
    return self;
    }


CVedVideoClip::CVedVideoClip(CVedMovieImp* aMovie, CAudClip* aAudioClip)
        : iMovie(aMovie), iSpeed(KVedNormalSpeed), iColorEffect(EVedColorEffectNone),
          iCutInTime(0), iAudClip(aAudioClip)
    {
    }


void CVedVideoClip::ConstructL(const TDesC& aFileName, TInt aIndex, 
                               MVedVideoClipInfoObserver& aObserver)
    {
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    __ASSERT_ALWAYS(aIndex <= iMovie->VideoClipCount(), 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    iIndex = aIndex;

    if (iMovie->iFs)  // lock file against writing
        {

        TInt error = iLockFile.Open(*iMovie->iFs, aFileName, EFileShareReadersOnly | EFileStream | EFileRead);
        
        if (error != KErrNone)
            {
            User::LeaveIfError(iLockFile.Open(*iMovie->iFs, aFileName, 
                EFileShareAny | EFileStream | EFileRead));
            }
        iLockFileOpened = ETrue;    
        }
        
    if (iAudClip) 
        {
        iInfo = CVedVideoClipInfoImp::NewL(iAudClip->Info(), aFileName, aObserver);
        }
    else
        {
        iInfo = CVedVideoClipInfoImp::NewL(NULL, aFileName, aObserver);
        }
    }


void CVedVideoClip::ConstructL(CVedVideoClipGenerator& aGenerator, TInt aIndex, 
                               MVedVideoClipInfoObserver& aObserver, TBool aIsOwnedByVideoClip)
    {
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    __ASSERT_ALWAYS(aIndex <= iMovie->VideoClipCount(), 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    iIndex = aIndex;

    iInfo = new (ELeave) CVedVideoClipInfoGeneratedImp(aGenerator, aIsOwnedByVideoClip);
    ((CVedVideoClipInfoGeneratedImp*) iInfo)->ConstructL(aObserver);
    }


void CVedVideoClip::ConstructL(RFile* aFileHandle, TInt aIndex, 
                               MVedVideoClipInfoObserver& aObserver)
    {
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    __ASSERT_ALWAYS(aIndex <= iMovie->VideoClipCount(), 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    iIndex = aIndex;

    if (iAudClip) 
        {
        iInfo = CVedVideoClipInfoImp::NewL(iAudClip->Info(), aFileHandle, aObserver);
        }
    else
        {
        iInfo = CVedVideoClipInfoImp::NewL(NULL, aFileHandle, aObserver);
        }
    }


CVedVideoClip::~CVedVideoClip()
    {
    delete iInfo;

    if (iLockFileOpened)
        iLockFile.Close();
    
    }


void CVedVideoClip::SetIndex(TInt aIndex)
    {
    __ASSERT_ALWAYS(aIndex >= 0, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));
    __ASSERT_ALWAYS(aIndex < iMovie->VideoClipCount(), 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalIndex));

    if (aIndex != iIndex)
        {
        TInt oldIndex = iIndex;
        iIndex = aIndex;

        iMovie->iVideoClipArray.Remove(oldIndex);
        TInt err = iMovie->iVideoClipArray.Insert(this, iIndex);
        __ASSERT_DEBUG(err == KErrNone, 
                       TVedPanic::Panic(TVedPanic::EInternal));

        if (oldIndex < iIndex)
            {
            CVedVideoClip* clip = iMovie->iVideoClipArray[oldIndex];
            clip->iIndex = oldIndex;
            iMovie->RecalculateVideoClipTimings(clip);
            }
        else
            {
            iMovie->RecalculateVideoClipTimings(this);
            }

        iMovie->FireVideoClipIndicesChanged(iMovie, oldIndex, iIndex);
        }
    }

    
void CVedVideoClip::SetSpeed(TInt aSpeed)
    {
    __ASSERT_ALWAYS((aSpeed >= KVedMinSpeed) && (aSpeed <= KVedMaxSpeed), 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalSpeed));

    if (aSpeed != KVedNormalSpeed )
        {
        // slow motion, mute the audio track
        if (iAudClip) 
            {
            iAudClip->SetMuting(ETrue);
            iMovie->FireVideoClipAudioSettingsChanged(iMovie, this);
            }
        }
    else
        {
        // normal speed
        if ( !iUserMuted )
            {
            // unmute the audio track unless user has muted the clip
            if (iAudClip) 
                {
                iAudClip->SetMuting(EFalse);
                iMovie->FireVideoClipAudioSettingsChanged(iMovie, this);
                }
            }
        }

    if (aSpeed != iSpeed)
        {
        iSpeed = aSpeed;
        iMovie->RecalculateVideoClipTimings(this);
        iMovie->FireVideoClipTimingsChanged(iMovie, this);
        }
    }

    
void CVedVideoClip::SetColorEffect(TVedColorEffect aColorEffect)
    {
    __ASSERT_ALWAYS((aColorEffect >= EVedColorEffectNone) 
                    && (aColorEffect < EVedColorEffectLast), 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalColorEffect));

    if (aColorEffect != iColorEffect)
        {
        iColorEffect = aColorEffect;
        iMovie->FireVideoClipColorEffectChanged(iMovie, this);
        }
    }

TRgb CVedVideoClip::ColorTone() const
    {
    return iColorTone;
    }
    
void CVedVideoClip::SetColorTone(TRgb aColorTone)
    {
    if (aColorTone != iColorTone)
        {
        // any checks needed?
        iColorTone = aColorTone;
        // reuses the observer callback from color effect, since essentially it is about the same thing
        iMovie->FireVideoClipColorEffectChanged(iMovie, this);
        }
    }

void CVedVideoClip::SetMuted(TBool aMuted)
    {
    if (!iAudClip) 
        {
        return;
        }
    iUserMuted = aMuted; // store the mute-value (true/false) to be able to differentiate user and automatic mute
    iAudClip->SetMuting(aMuted);
    iMovie->FireVideoClipAudioSettingsChanged(iMovie, this);
    }

TInt CVedVideoClip::InsertDynamicLevelMarkL(const TVedDynamicLevelMark& aMark)
    {
    __ASSERT_ALWAYS(aMark.iTime.Int64() >= 0,
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMark));
    __ASSERT_ALWAYS(aMark.iTime.Int64() <= CutOutTime().Int64(),
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMark));
                    
    if ( iAudClip )
        {
        return iAudClip->InsertDynamicLevelMarkL(TAudDynamicLevelMark(aMark.iTime, aMark.iLevel));
        }
    else
        {
        User::Leave(KErrNotFound);
        return -1;//to make compiler happy
        }
    }
    
TBool CVedVideoClip::RemoveDynamicLevelMark(TInt aIndex)
    {
    __ASSERT_ALWAYS(aIndex >= 0,
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
    __ASSERT_ALWAYS(aIndex < DynamicLevelMarkCount(),
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
    if ( iAudClip )
        {
        return iAudClip->RemoveDynamicLevelMark(aIndex);
        }
    else
        {
        return EFalse;
        }
    }
    
TVedDynamicLevelMark CVedVideoClip::DynamicLevelMark(TInt aIndex) const
    {
    __ASSERT_ALWAYS(aIndex >= 0,
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
    __ASSERT_ALWAYS(aIndex < DynamicLevelMarkCount(),
                    TVedPanic::Panic(TVedPanic::EIllegalDynamicLevelMarkIndex));
                    
    if ( iAudClip )
        {
        TAudDynamicLevelMark mark = iAudClip->DynamicLevelMark(aIndex);    
        return TVedDynamicLevelMark(mark.iTime, mark.iLevel);
        }
    else
        {
        return TVedDynamicLevelMark(TTimeIntervalMicroSeconds(0), 0);
        }
    }
    
TInt CVedVideoClip::DynamicLevelMarkCount() const
    {
    if ( iAudClip )
        {
        return iAudClip->DynamicLevelMarkCount();
        }
    else
        {
        return 0;
        }
    }

void CVedVideoClip::SetVolumeGain(TInt aVolumeGain)
    {
    if ( iAudClip )
        {
        iAudClip->SetVolumeGain(aVolumeGain);
        }
    }
    
TInt CVedVideoClip::GetVolumeGain()
    {
    if ( iAudClip )
        {
        return iAudClip->GetVolumeGain();
        }
    else
        {
        return 0;
        }
    }


void CVedVideoClip::SetCutInTime(TTimeIntervalMicroSeconds aCutInTime)
    {
    __ASSERT_ALWAYS(aCutInTime >= TTimeIntervalMicroSeconds(0), 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalCutInTime));
    __ASSERT_ALWAYS(aCutInTime <= iCutOutTime, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalCutInTime));
    __ASSERT_ALWAYS(iInfo->Class() == EVedVideoClipClassFile, 
                    TVedPanic::Panic(TVedPanic::EVideoClipNoFileAssociated));

    if (aCutInTime != iCutInTime)
        {
        iCutInTime = aCutInTime;
        iMovie->RecalculateVideoClipTimings(this);
        iMovie->FireVideoClipTimingsChanged(iMovie, this);
        }   
    }


void CVedVideoClip::SetCutOutTime(TTimeIntervalMicroSeconds aCutOutTime)
    {
    __ASSERT_ALWAYS(aCutOutTime >= iCutInTime, 
                    TVedPanic::Panic(TVedPanic::EVideoClipIllegalCutOutTime));
    __ASSERT_ALWAYS(iInfo->Class() == EVedVideoClipClassFile, 
                    TVedPanic::Panic(TVedPanic::EVideoClipNoFileAssociated));

    if ( aCutOutTime > iInfo->Duration() )
        {
        aCutOutTime = iInfo->Duration();
        }

    if (aCutOutTime != iCutOutTime)
        {
        iCutOutTime = aCutOutTime;
        iMovie->RecalculateVideoClipTimings(this);
        iMovie->FireVideoClipTimingsChanged(iMovie, this);
        }
    }

void CVedVideoClip::UpdateAudioClip()
    {
    if (!iAudClip) 
        {
        return;
        }
    iAudClip->SetStartTime(iStartTime);
    iAudClip->SetCutInTime(iCutInTime);
    iAudClip->SetCutOutTime(iCutOutTime);
    }



CVedVideoClipInfo* CVedVideoClip::Info()
    {
    return iInfo;
    }

 
TBool CVedVideoClip::EditedHasAudio() const
    {
    TInt speed = Speed();
    TBool isMuted = IsMuted();
    TBool hasAudio = iInfo->HasAudio();
    TBool ret = hasAudio && (speed == KVedNormalSpeed);
    ret = ret && !isMuted;
    return ret;
    }

    
CVedMovieImp* CVedVideoClip::Movie()
    {
    return iMovie;
    }


TInt CVedVideoClip::Index() const
    {
    return iIndex;
    }

 
TInt CVedVideoClip::Speed() const
    {
    return iSpeed;
    }


TVedColorEffect CVedVideoClip::ColorEffect() const
    {
    return iColorEffect;
    }


TBool CVedVideoClip::IsMuteable() const
    {
    return (iInfo->HasAudio() && (Speed() == KVedNormalSpeed));
    }


TBool CVedVideoClip::Muting() const
    {
    if (!iAudClip) 
        {
        return ETrue;
        }
    return iAudClip->Muting();
    }

TBool CVedVideoClip::IsMuted() const
    {
    if (!iAudClip) 
        {
        return ETrue;
        }
    if ( iUserMuted )
        {   
        return ETrue;
        }
    else
        {
        // automatic mute due to slow motion is not informed to client as muted 
        // since it can cause some conflicts
        return EFalse;
        }
    }

void CVedVideoClip::SetNormalizing(TBool aNormalizing)
    {
    if (iAudClip) 
        {
        iAudClip->SetNormalizing(aNormalizing);     
        }
    }
    
TBool CVedVideoClip::Normalizing() const
    {
    return (iAudClip && iAudClip->Normalizing());
    }
    
TTimeIntervalMicroSeconds CVedVideoClip::CutInTime() const
    {
    if (iInfo->Class() == EVedVideoClipClassFile) 
        {
        return iCutInTime;
        }
    else
        {
        return TTimeIntervalMicroSeconds(0);
        }
    }


TTimeIntervalMicroSeconds CVedVideoClip::CutOutTime() const
    {
    if (iInfo->Class() == EVedVideoClipClassFile) 
        {
        return iCutOutTime;
        }
    else
        {
        return iInfo->Duration();
        }
    }

 
TTimeIntervalMicroSeconds CVedVideoClip::StartTime() const
    {
    return iStartTime;
    }


TTimeIntervalMicroSeconds CVedVideoClip::EndTime() const
    {
    return TTimeIntervalMicroSeconds(iStartTime.Int64() + EditedDuration().Int64());
    }


TTimeIntervalMicroSeconds CVedVideoClip::EditedDuration() const
    {
    TInt64 editedDuration = CutOutTime().Int64() - CutInTime().Int64();
    return TTimeIntervalMicroSeconds((TInt64(1000) * editedDuration) / TInt64(Speed()));
    }

