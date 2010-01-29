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


#include <hal.h>
#include <charconv.h>
#include "logfile.h"
#include "logfilepanics.h"

_LIT8(KCrLf8, "\r\n");
_LIT(KCrLf, "\r\n");

const TInt KAsciiStart = 0x20;
const TInt KAsciiEnd = 0x7f;
const TInt KHexCharLeft = '<';
const TInt KHexCharRight = '>';

const TInt KNumberOfDecimalPlaces = 3;

CLogFile* CLogFile::NewL(const TDesC& aFileName, TBool aInitialiseLog)
    {
    CLogFile* self = NewLC(aFileName, aInitialiseLog);
    CleanupStack::Pop();
    return(self);
    }


CLogFile* CLogFile::NewLC(const TDesC& aFileName, TBool aInitialiseLog)
    {
    CLogFile* self = new (ELeave) CLogFile();
    CleanupStack::PushL(self);
    self->ConstructL(aFileName, aInitialiseLog);
    return(self);
    }


CLogFile::CLogFile()
    {
    // No implementation required
    }


CLogFile::~CLogFile()
    {
    iLogFile.Flush();
    iLogFile.Close();
    iSession.Close();

    delete iCharacterConverter;
    iCharacterConverter = NULL;
    }


void CLogFile::ConstructL(const TDesC& aFileName, TBool aInitialiseLog)
    {
    TInt period;
    User::LeaveIfError(HAL::Get(HALData::ESystemTickPeriod, period));

    iLogMillisecsPerTick = period / 1000;

    if (iLogMillisecsPerTick == 0)
        {
        iLogMillisecsPerTick = 1;
        }


    User::LeaveIfError(iSession.Connect());

    if (aInitialiseLog)
        {
        User::LeaveIfError(iLogFile.Replace(iSession, aFileName, EFileShareExclusive));
        }
    else
        {
        TInt err = iLogFile.Open(iSession, aFileName, EFileShareExclusive | EFileWrite);

        switch (err)
            {
            case KErrNone: // Opened ok, so seek to end of file
                {
                TInt position = 0;
                User::LeaveIfError(iLogFile.Seek(ESeekEnd, position));
                }
                break;

            case KErrNotFound: // File doesn't exist, so create it
                User::LeaveIfError(iLogFile.Create(iSession, aFileName, EFileShareExclusive | EFileWrite));
                break;

            default: // Unexepected error
                User::Leave(err);
                break;
            }
        }
    
    // Create character converter
    iCharacterConverter = CCnvCharacterSetConverter::NewL();
//    CCnvCharacterSetConverter::TAvailability iConverterAvailability;
    iConverterAvailability = iCharacterConverter->PrepareToConvertToOrFromL(KCharacterSetIdentifierAscii, iSession);
    }


void CLogFile::LogTime()
    {
    StartWrite();
    LogTimeInternal();
    EndWrite();
    }


void CLogFile::Log(const TDesC8& aText)
    {
    StartWrite();
    LogTextInternal(aText);
    EndWrite();
    }


void CLogFile::Log(const TDesC& aText)
    {
    StartWrite();

    for (TInt i = 0; i < aText.Length(); i++)
        {
        if (aText.Mid(i).Find(KCrLf) == 0)
            {
            LogNewline();
            i++;
            }
        else if (iConverterAvailability == CCnvCharacterSetConverter::EAvailable)
            {
            // Convert character from unicode
            TBuf<1> unicodeBuffer;
            TBuf8<10> asciiBuffer;

            unicodeBuffer.Append(aText[i]);
            TInt status = iCharacterConverter->ConvertFromUnicode(asciiBuffer, unicodeBuffer);

            if (status >= 0)
                {
                LogTextInternal(asciiBuffer);
                }
            }
        else // character converter not available
            {
                TBuf8<1> asciiBuffer;
                asciiBuffer.Append(static_cast<TUint8>(aText[i]));
                LogTextInternal(asciiBuffer);
            }
        }

    EndWrite();
    }


void CLogFile::Log(TUint8 aByte)
    {
    StartWrite();
    LogByteInternal(aByte);
    EndWrite();        
    }


void CLogFile::Log(TUint aNumber)
    {
    StartWrite();
    LogIntInternal(aNumber);
    EndWrite();        
    }


void CLogFile::LogBytes(const TDesC8& aBuffer, TBool fastWrite)
    {
    StartWrite();

    if(fastWrite)
      Write(aBuffer);
    else
    {
      for (TInt i = 0; i < aBuffer.Length(); i++)
      {
        LogByteInternal(aBuffer[i], ETrue);
      }
    }

    EndWrite();
    }


void CLogFile::LogTimeInternal()
    {
    TBuf8<50> text;
    TInt timeInMillisecs = User::TickCount() * iLogMillisecsPerTick;
    TInt secs = timeInMillisecs / 1000;
    TInt millisecs = timeInMillisecs % 1000;
    text.Num(secs);
    text.Append('.');
    Write(text);
    text.Num(millisecs);

    while (text.Length() < KNumberOfDecimalPlaces)
        {
        text.Insert(0, _L8("0"));
        }

    text.Append('-');
    Write(text);
    }


void CLogFile::LogTextInternal(const TDesC8& aText)
    {
    TPtrC8 tail(aText.Ptr(), aText.Length());

    TInt newLinePosition = tail.Find(KCrLf8);
    while (newLinePosition != KErrNotFound)
        {
        if (newLinePosition > 0)
            {
            Write(tail.Left(newLinePosition));
            tail.Set(aText.Ptr() + newLinePosition, tail.Length() - newLinePosition);
            }
        LogNewline();
        tail.Set(aText.Ptr() + KCrLf8.iTypeLength, tail.Length() - KCrLf8.iTypeLength);

        newLinePosition = tail.Find(KCrLf8);
        }

    //    No more newlines left so print remainder
    Write(tail);

    }


void CLogFile::LogByteInternal(TUint8 aByte, TBool acsiiMode)
    {
    if ((aByte >= KAsciiStart) && (aByte < KAsciiEnd) || acsiiMode)
        {
        // Display as ASCII char
        TBuf8<1> str;
        str.Append(aByte);
        Write(str);
        }
    else
        {
        // Display as hex number
        TBuf8<4> str;
        str.Append(KHexCharLeft);
        str.AppendNum(static_cast<TUint>(aByte), EHex);
        str.Append(KHexCharRight);
        Write(str);
        }
    }


void CLogFile::LogIntInternal(TUint aNumber)
    {
    // Display as ASCII char
    TBuf8<20> str;
    str.Append(KHexCharLeft);
    str.AppendNum(aNumber/*, EHex*/);
    str.Append(KHexCharRight);
    Write(str);
    }


void CLogFile::LogNewline()
    {
    Write(KCrLf8);

    if (iAutoTimestamp)
        {
        LogTimeInternal();
        }
    }


void CLogFile::StartWrite()
    {
    ASSERT(iCheckNestDepth == 0);
    iCheckNestDepth++;

    if (iAutoNewline)
        {
        LogNewline();
        }
    }


void CLogFile::EndWrite()
    {
    if (iAutoFlush)
        {
        iLogFile.Flush();
        }

    iCheckNestDepth--;
    ASSERT(iCheckNestDepth == 0);
    }

void CLogFile::Write(const TDesC8& aText)
    {

    if (iLogFile.Write(aText) != KErrNone)
        {
        //  As the framework may be trapping User::Panic we need to
        //  produce the panic at a lower level.
        RThread().Panic(KLogFilePanic, TLogFileWriteFailed);
        }
    }

void CLogFile::SetAutoFlush(TBool aOn)
    {
    iAutoFlush = aOn;
    }


void CLogFile::SetAutoTimeStamp(TBool aOn)
    {
    iAutoTimestamp = aOn;
    }


void CLogFile::SetAutoNewline(TBool aOn)
    {
    iAutoNewline = aOn;
    }


void CLogFile::StaticLogL(const TDesC& aFileName, const TDesC8& aText)
    {
    CLogFile* logFile = NewLC(aFileName, EFalse);
    logFile->Log(aText);
    CleanupStack::Pop(logFile);
    delete logFile;
    }


void CLogFile::StaticLogL(const TDesC& aFileName, const TDesC& aText)
    {
    CLogFile* logFile = NewLC(aFileName, EFalse);
    logFile->Log(aText);
    CleanupStack::Pop(logFile);
    delete logFile;
    }



