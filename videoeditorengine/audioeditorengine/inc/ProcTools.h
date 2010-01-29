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



#ifndef __PROCTOOLS_H__
#define __PROCTOOLS_H__

#include <e32base.h>
#include <f32file.h>
#include "AudCommon.h"
#include "ProcConstants.h"
#include "Mp3API.h"

class ProcTools 
    {

public:

    /**
    * Decimal number to binary number
    */
    static TBool Dec2Bin(TUint8 aDec, TBuf8<8>& aBinary);
    /**
    * This version allocates memory (aBin), remember to release
    */
    static TBool Dec2BinL(TUint32 aDec, HBufC8*& aBin);

    /**
    * Binary number to decimal number
    */
    static TBool Bin2Dec(const TDesC8& aBin, TUint& aDec);
    
    /**
    * Decimal number in descriptor to unsigned integer
    */
    static TBool Des2Dec(TDesC8& aDes, TUint& aDec);
    
    /**
    * Decimal number in descriptor to binary descriptor
    * allocates memory, the caller must release
    */
    static TBool Des2BinL(const TDesC8& aDes, HBufC8*& aBin);
    
    /**
    * Gets milliseconds(TInt) from microseconds (TTimeIntervalMicroSeconds)
    * @param    aMicroSeconds    microseconds
    *
    * @return                     milliseconds
    */
    
    static TInt MilliSeconds(TTimeIntervalMicroSeconds aMicroSeconds);
    
    /**
    * Gets microseconds (TTimeIntervalMicroSeconds) from milliseconds (TInt)
    * @param    aMilliSeconds    milliseconds
    *
    * @return                     microiseconds
    */
    
    static TTimeIntervalMicroSeconds MicroSeconds(TInt aMilliSeconds);
    
    /**
    * casts TInt64 to TInt (overflow possible, the user is resonsible!!)
    *
    * @param    aInt64    TInt64
    *
    * @return            Tint
    */
    
    static TInt GetTInt(TInt64 aTInt64);
    
    
    /**
    * Retrieves a decimal number from a frame
    *
    * @param aFrame        frame
    * @param aBitFirst    index of the first bit of the needed balue
    * @param aLength    number of bits in the needed value
    *
    * @return            retrieved integer, if -1 then operation was unsuccessful
    */
    static TInt GetValueFromFrame(const HBufC8* aFrame, TInt aBitFirst, TInt aLength);
    
    /**
    * Retrieves a decimal number from a frame, whose bits are shuffled
    *
    * @param aFrame            frame
    * @param aBitRate        bitrate
    * @param aBitPositions    bit position in a deshuffled frame
    * @param aLength        number of bits read
    *
    * @return                retrieved integer
    *
    */
    static TInt GetValueFromShuffledAWBFrameL(const HBufC8* aFrame, TInt aBitRate, TInt aBitPosition, TInt aLength);

    static TBool SetValueToShuffledAWBFrame(TUint8 aNewValue, HBufC8* aFrame, 
                                            TInt aBitRate, TInt aBitPosition, TInt aLength);

    /**
    * Retrieves a decimal number from a frame, whose bits are shuffled
    *
    * @param aFrame            frame
    * @param aBitPositions    shuffling table
    * @param TInt aSize        length of <code>aBitPositions</code>
    *
    * @return                retrieved integer, if -1 then operation was unsuccessful
    *
    */
    static TInt GetValueFromShuffledFrame(const HBufC8* aFrame, const TUint8 aBitPositions[], TInt aSize);


    /**
    * Writes a decimal number to a frame, whose bits are shuffled
    *
    * @param aFrame            frame
    * @param aNewValue        value to be written
    * @param aBitPositions    bit indexes in order
    * @param TInt aSize        length of <code>aBitPositions</code>
    *
    * @return                ETrue if successful
    *
    */
    static TBool SetValueToShuffledFrame(HBufC8* aFrame, TUint8 aNewValue, const TUint8 aBitPositions[], TInt aSize);

    /**
    * Appends integers to a file separated by linefeeds
    * Mostly for debugging purposes
    *    
    * @param aArray        array of integers to be written
    * @param aFilename    file name, created if doesn't exist, append otherwise
    *
    * @return            ETrue if successful
    */
    static TBool WriteValuesToFileL(const RArray<TInt>& aArray, const TDesC& aFilename);

    /**
    * Finds a closest match in a gain table
    * used for scalar quantized gain tables
    *
    * @param aNewGain        search key
    * @param aGainTable        gain table
    * @param aTableSize        gain table length
    *
    * @return                index of the closest match
    */
    static TUint8 FindNewIndexSQ(TInt aNewGain, const TInt aGainTable[], TInt aTableSize); 
    
    /**
    * Finds a closest match in a gain table
    * used for vector quantized gain tables
    * pitch: Q14. FC gain Q12
    *
    * @param aNewGain        search key
    * @param aGainTable        gain table
    * @param aTableSize        gain table length
    *
    * @return                index of the closest match
    */
    static TUint8 FindNewIndexVQ(TInt aNewGain, TInt aOldPitch, const TInt aGainTable[], TInt aTableSize); 

    /**
    * Finds a closest match in a gain table
    * used for vector quantized gain tables
    * pitch: Q14. FC gain Q11!!
    *
    * @param aNewGain        search key
    * @param aGainTable        gain table
    * @param aTableSize        gain table length
    *
    * @return                index of the closest match
    */
    static TUint8 FindNewIndexVQ2(TInt aNewGain, TInt aOldPitch, const TInt aGainTable[], TInt aTableSize); 


    /**
    * Finds a closest match in a gain table for 4.75 kBit/s
    *
    * @param aNewGain0    new gain of subframe 0 or 2
    * @param aOldPitch0 new pitch of subframe 0 or 2
    * @param aNewGain1    new gain of subframe 1 or 3
    * @param aNewGain1    new pitch of subframe 1 or 3
    *
    */
    static TUint8 FindNewIndex475VQ(TInt aNewGain0, TInt aOldPitch0, TInt aNewGain1, TInt aOldPitch1);

    static TInt FindIndex(TInt aKey, const TInt aBitPositions[], TInt aTableLength);

    static TBool GenerateADTSHeaderL(TBuf8<7>& aHeader, TInt aFrameLength, TAudFileProperties aProperties);

    static TInt GetNextAMRFrameLength(const HBufC8* aFrame, TInt aPosNow);

    };


/*
*
*    One processing event represents a section in the output clip
*    It includes the following information:
*        - when the section starts (iPosition)
*        - is the new section a result of a new clip (iCutIn == ETrue)
*            or a result of an ending clip (iCutIn == EFalse)
*        - what clips are supposed to be mixed from this processing event (iAllIndexes)
*        - what is the clip starting or ending that is causing this processing event (iChangedIndex)
*
*
*/

class CProcessingEvent : public CBase
    {

public:

        
    static CProcessingEvent* NewL();
    ~CProcessingEvent();
    /*
    * Insert a new clip to this processing event
    */
    void InsertIndex(TInt aIndex);

    /*
    * Gets the clip index in CAudSong based on the clip index in processing event
    */  
    TInt GetIndex(TInt aProcessingEventIndex);
    
    /*
    * Gets all clip indexes in this processing event
    */
    TBool GetAllIndexes(RArray<TInt>& aAllIndexes);
    
    /* 
    * Index count
    */
    TInt IndexCount();
    
    /*
    * Find processing event index based on clip index
    */
    TInt FindIndex(TInt aClipIndex);

    /*
    * Remove processing event
    */
    void RemoveIndex(TInt aProcessingEventIndex);

public:

    //global position in milliseconds
    TInt32 iPosition; 
    //true = cutIn, false = cutOut
    TBool iCutIn; 
    
    // there can be only one different clip in iAllIndexes in consecutive
    // processing events:
    // iChangedClipIndex is that index
    TInt iChangedClipIndex;
    
    // compare starting times (used by a common ordering function)
    static TInt Compare(const CProcessingEvent& c1, const CProcessingEvent& c2);
        
private:

    void ConstructL();
    CProcessingEvent();
    
    // indexes of all the clips that should be mixed
    // after this processing events
    RArray<TInt> iAllIndexes; // -1 = silence
    };



class AudioEngineUtilTools 
    {
public:

    /**
    * Displays messages on a box
    */
    static void AudioEngineMessageBox(const TDesC& aMessage);

    };

#endif
