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


/* Copyright (c) 2003 Nokia. All rights reserved. */

#include "AudClip.h"
#include "AudPanic.h"

#include "AudClipInfo.h"
#include "AudSong.h"
#include "ProcTools.h"

    
CAudClip* CAudClip::NewL(CAudSong* aSong, const TDesC& aFileName,
        TTimeIntervalMicroSeconds aStartTime,                            
        MAudClipInfoObserver& aObserver, TInt aTrackIndex) 
    {
    
    CAudClip* self = new (ELeave) CAudClip(aSong);
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aStartTime, aObserver, aTrackIndex);
    CleanupStack::Pop(self);
    return self;
    
    }
    
CAudClip* CAudClip::NewL(CAudSong* aSong, RFile* aFileHandle,
        TTimeIntervalMicroSeconds aStartTime,                            
        MAudClipInfoObserver& aObserver, TInt aTrackIndex) 
    {
    
    CAudClip* self = new (ELeave) CAudClip(aSong);
    CleanupStack::PushL(self);
    self->ConstructL(aFileHandle, aStartTime, aObserver, aTrackIndex);
    CleanupStack::Pop(self);
    return self;
    
    }


CAudClip::CAudClip(CAudSong* aSong) : iSong(aSong), iMute(EFalse), iNormalize(EFalse), 
iPriority(KAudClipPriorityNormal) 
    {
    
    }

EXPORT_C CAudClipInfo* CAudClip::Info() const 
    {

    return iInfo;
    }

EXPORT_C TBool CAudClip::Normalizing() const 
    {

    return iNormalize;
    }

EXPORT_C TAudDynamicLevelMark CAudClip::DynamicLevelMark(TInt aIndex) const 
    {

    return *iDynamicLevelMarkArray[aIndex];
    }
    
EXPORT_C TInt CAudClip::DynamicLevelMarkCount() const 
    {

    return iDynamicLevelMarkArray.Count();
    }
EXPORT_C TBool CAudClip::Muting() const
    {
    return iMute;
    }

EXPORT_C void CAudClip::SetStartTime(TTimeIntervalMicroSeconds aStartTime) 
    {

    if (iStartTime.Int64() != aStartTime.Int64()-iCutInTime.Int64()) 
        {
        iStartTime = aStartTime.Int64()-iCutInTime.Int64();
        }
        

    TInt clipIndexOnSong = iSong->FindClipIndexOnSong(this);
    TInt newIndex = 0;
    TBool found = EFalse;
    for (newIndex = 0 ; newIndex < iSong->ClipCount(KAllTrackIndices); newIndex++) 
        {
        if (iSong->iClipArray[newIndex]->StartTime() > aStartTime) 
            {
            found = ETrue;
            break;
            }
        }
    if (!found) 
        {
        newIndex = iSong->ClipCount(KAllTrackIndices);
        }

    // the order is the same
    if (newIndex == clipIndexOnSong || newIndex == clipIndexOnSong+1) 
        {
        
        iSong->FireClipTimingsChanged(iSong, this);
        }
    else 
        {


        TInt oldIndexOnTrack = iIndex;
        iSong->UpdateClipArray();
        iSong->UpdateClipIndexes();
        TInt newIndexOnTrack = iIndex;

        if (oldIndexOnTrack != newIndexOnTrack) 
            {
            iSong->FireClipIndicesChanged(iSong, oldIndexOnTrack, newIndexOnTrack, iTrackIndex);
            }
        else 
            {
            iSong->FireClipTimingsChanged(iSong, this);
            }

        }

    }




EXPORT_C TTimeIntervalMicroSeconds CAudClip::StartTime() const 
    {

    return iStartTime.Int64()+iCutInTime.Int64();
    }



TInt32 CAudClip::StartTimeMilliSeconds() const 
    {

    return ProcTools::MilliSeconds(iStartTime);
    
    }

EXPORT_C TTimeIntervalMicroSeconds CAudClip::EndTime() const
    {
    return TTimeIntervalMicroSeconds(iStartTime.Int64()+iCutOutTime.Int64());

    }


EXPORT_C TTimeIntervalMicroSeconds CAudClip::EditedDuration() const
    {
    TTimeIntervalMicroSeconds edDur(iCutOutTime.Int64()-iCutInTime.Int64());
    return edDur;

    }

EXPORT_C TInt CAudClip::Priority() const
    {
    return iPriority;
    
    }


EXPORT_C TInt CAudClip::IndexOnTrack() const 
    {

    return iIndex;
    }



EXPORT_C TInt CAudClip::TrackIndex() const 
    {

    return iTrackIndex;
    }


EXPORT_C TTimeIntervalMicroSeconds CAudClip::CutInTime() const 
    {
    
    return iCutInTime;
    }
    

TInt32 CAudClip::CutInTimeMilliSeconds() const 
    {

    return ProcTools::MilliSeconds(iCutInTime);
    
    }

EXPORT_C void CAudClip::SetCutInTime(TTimeIntervalMicroSeconds aCutInTime) 
    {

    TInt64 oldCutIn = iCutInTime.Int64();
    TInt64 cutDif = aCutInTime.Int64() - oldCutIn;  

    iCutInTime = aCutInTime;
    iStartTime = (TTimeIntervalMicroSeconds)(iStartTime.Int64()-cutDif);
    iSong->FireClipTimingsChanged(iSong, this);
    }
    
EXPORT_C TTimeIntervalMicroSeconds CAudClip::CutOutTime() const 
    {

    return iCutOutTime;
    }

TInt32 CAudClip::CutOutTimeMilliSeconds() const 
    {

    return ProcTools::MilliSeconds(iCutOutTime);
    

    }
    
EXPORT_C void CAudClip::SetCutOutTime(TTimeIntervalMicroSeconds aCutOutTime) 
    {

    iCutOutTime = aCutOutTime;
    iSong->FireClipTimingsChanged(iSong, this);
    }




EXPORT_C TBool CAudClip::SetPriority(TInt aPriority)

    {
    if (aPriority < 0) return EFalse;

    iPriority = aPriority;
    return ETrue;

    }


EXPORT_C TInt CAudClip::InsertDynamicLevelMarkL(const TAudDynamicLevelMark& aMark) 
    {

    if (aMark.iTime < TTimeIntervalMicroSeconds(0)) 
        {
        TAudPanic::Panic(TAudPanic::EIllegalDynamicLevelMark);
        }

    TAudDynamicLevelMark* newMark = new (ELeave) TAudDynamicLevelMark(aMark);
    
    TBool added = EFalse;
    TInt err = KErrNone;

    // insert marks so that they are always sorted by time
    TInt index = 0;
    for (index = 0 ; index < iDynamicLevelMarkArray.Count() ; index++) 
        {
        if (iDynamicLevelMarkArray[index]->iTime == newMark->iTime) 
            {
            // replace the level
            TAudDynamicLevelMark* oldMark = iDynamicLevelMarkArray[index];
            iDynamicLevelMarkArray.Remove(index);
            delete oldMark;
            err = iDynamicLevelMarkArray.Insert(newMark, index);
            added = ETrue;
            break;
            }
        else if (iDynamicLevelMarkArray[index]->iTime > newMark->iTime) 
            {
            err = iDynamicLevelMarkArray.Insert(newMark, index);
            added = ETrue;
            break;
            }
        }
    if (!added) 
        {
        index = iDynamicLevelMarkArray.Count();
        err = iDynamicLevelMarkArray.Insert(newMark, index);
        }
    if (err != KErrNone) 
        {
        User::Leave(err);
        }

    iSong->FireDynamicLevelMarkInserted(*this, *newMark, index);
    return index;


    }
    
EXPORT_C TBool CAudClip::RemoveDynamicLevelMark(TInt aIndex) 
    {

    if (aIndex > iDynamicLevelMarkArray.Count() || aIndex < 0) 
        {
        TAudPanic::Panic(TAudPanic::EAudioClipIllegalIndex);
        }
    
    TAudDynamicLevelMark* mark = iDynamicLevelMarkArray[aIndex];
    
    iDynamicLevelMarkArray.Remove(aIndex);
    delete mark;
    
    iSong->FireDynamicLevelMarkRemoved(*this, aIndex);
    return ETrue;
    }
    
EXPORT_C void CAudClip::SetMuting(TBool aMuted)
    {
    
    iMute = aMuted;
    
    }

EXPORT_C void CAudClip::SetNormalizing(TBool aNormalizing) 
    {
    
    iNormalize = aNormalizing;    

    }

EXPORT_C void CAudClip::Reset(TBool aNotify) 
    {

    TInt a = 0;

    for (a = 0 ; a < iDynamicLevelMarkArray.Count() ; a++) 
        {
        delete iDynamicLevelMarkArray[a];
        }
    iDynamicLevelMarkArray.Reset();
    iCutInTime = 0;
    
    if (iInfo != 0)
        {
        iCutOutTime = iInfo->Properties().iDuration;
        }

    if (aNotify) 
        {
        iSong->FireClipReseted(*this);
        }

    }


EXPORT_C void CAudClip::SetVolumeGain(TInt aVolumeGain)
    {
    iVolumeGain = aVolumeGain;
    }

EXPORT_C TInt CAudClip::GetVolumeGain()
    {
    return iVolumeGain;
    }

/*
EXPORT_C TBool CAudClip::operator<(const CAudClip &c2) const {

    return (this->iStartTime < c2.StartTime());

}


EXPORT_C TBool CAudClip::operator>(const CAudClip &c2) const {

    return (this->iStartTime > c2.StartTime());

}

EXPORT_C TBool CAudClip::operator==(const CAudClip &c2) const {

    return (this->iStartTime == c2.StartTime());

}
*/
TInt CAudClip::Compare(const CAudClip& c1, const CAudClip& c2) 
    {

    if (c1.iStartTime > c2.iStartTime) 
        {
        return 1;
        }
    else if (c1.iStartTime < c2.iStartTime) 
        {
        return -1;
        }
    else 
        {
        return 0;
        }
    }



void CAudClip::ConstructL(const TDesC& aFileName,
                          TTimeIntervalMicroSeconds aStartTime,
                          MAudClipInfoObserver& aObserver, TInt aTrackIndex) 
    {
    
    __ASSERT_ALWAYS(aStartTime >= TInt64(0), 
        TAudPanic::Panic(TAudPanic::EAudioClipIllegalStartTime));
    iStartTime = aStartTime;
    iTrackIndex = aTrackIndex;
    
    iInfo = CAudClipInfo::NewL(aFileName, aObserver);
    
    }
    
void CAudClip::ConstructL(RFile* aFileHandle,
                          TTimeIntervalMicroSeconds aStartTime,
                          MAudClipInfoObserver& aObserver, TInt aTrackIndex) 
    {
    
    __ASSERT_ALWAYS(aStartTime >= TInt64(0), 
        TAudPanic::Panic(TAudPanic::EAudioClipIllegalStartTime));
    iStartTime = aStartTime;
    iTrackIndex = aTrackIndex;
    
    iInfo = CAudClipInfo::NewL(aFileHandle, aObserver);
    
    }


CAudClip::~CAudClip() 
    {

    Reset(EFalse);
    if (iInfo != 0)
        {
        delete iInfo;
        iInfo = 0;
        }
    
    }

