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
#include <bautils.h>
#include "logfile.h"
#include "logfile.pan"

_LIT8(KCrLf8, "\r\n");
_LIT(KCrLf, "\r\n");

static const TInt KAsciiStart = 0x20;
static const TInt KAsciiEnd = 0x7f;
static const TInt KHexCharLeft = '<';
static const TInt KHexCharRight = '>';

//static const TInt KNumberOfDecimalPlaces = 3;

EXPORT_C CLogFile* CLogFile::NewL(const TDesC& aFileName, TBool aInitialiseLog)
	{
    CLogFile* self = NewLC(aFileName, aInitialiseLog);
    CleanupStack::Pop(self);
    return(self);
	}


EXPORT_C CLogFile* CLogFile::NewLC(const TDesC& aFileName, TBool aInitialiseLog)
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


EXPORT_C CLogFile::~CLogFile()
	{
    iLogFile.Flush();
    iLogFile.Close();
    iSession.Close();
	}


void CLogFile::ConstructL(const TDesC& aFileName, TBool aInitialiseLog)
	{

#ifdef ORIGINAL_TIMESTAMP
    TInt period;
	User::LeaveIfError(HAL::Get(HALData::ESystemTickPeriod, period));

    iLogMillisecsPerTick = period / 1000;

    if (iLogMillisecsPerTick == 0)
    	{
        iLogMillisecsPerTick = 1;
    	}
#endif

    User::LeaveIfError(iSession.Connect());

    if (aInitialiseLog)
    	{
        User::LeaveIfError(iLogFile.Replace(iSession, aFileName, EFileShareAny | EFileWrite));
    	}
    else
    	{
        TInt err = iLogFile.Open(iSession, aFileName, EFileShareAny | EFileWrite);

        switch (err)
        	{
            case KErrNone: // Opened ok, so seek to end of file
                {
                TInt position = 0;
                User::LeaveIfError(iLogFile.Seek(ESeekEnd, position));
                }
                break;

            case KErrNotFound: // File doesn't exist, so create it
                User::LeaveIfError(iLogFile.Create(iSession, aFileName, EFileShareAny | EFileWrite));
                break;

            default: // Unexepected error
                User::Leave(err);
                break;
        	}
    	}
	}


EXPORT_C void CLogFile::LogTime()
	{
    StartWrite();
    LogTimeInternal();
    EndWrite();
	}


EXPORT_C void CLogFile::Log(const TDesC8& aText)
	{
    StartWrite();
    LogTextInternal(aText);
    EndWrite();
	}


EXPORT_C void CLogFile::Log(const TDesC& aText)
	{
    StartWrite();

	TRAP_IGNORE( DoLogTextL(aText) );

    EndWrite();
	}

void CLogFile::DoLogTextL(const TDesC& aText)
	{
    // Create character converter
    CCnvCharacterSetConverter* characterConverter = CCnvCharacterSetConverter::NewLC();
    CCnvCharacterSetConverter::TAvailability converterAvailability;
    converterAvailability = characterConverter->PrepareToConvertToOrFromL(KCharacterSetIdentifierAscii, iSession);

    for (TInt i = 0; i < aText.Length(); i++)
    	{
        if (aText.Mid(i).Find(KCrLf) == 0)
        	{
            LogNewline();
            i++;
        	}
        else if (converterAvailability == CCnvCharacterSetConverter::EAvailable)
        	{
            // Convert character from unicode
            TBuf<1> unicodeBuffer;
            TBuf8<10> asciiBuffer;

            unicodeBuffer.Append(aText[i]);
            TInt status = characterConverter->ConvertFromUnicode(asciiBuffer, unicodeBuffer);

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

    CleanupStack::PopAndDestroy(characterConverter);
	}

EXPORT_C void CLogFile::Log(TUint8 aByte)
	{
    StartWrite();
    LogByteInternal(aByte);
    EndWrite();        
	}


EXPORT_C void CLogFile::Log(TUint aNumber)
	{
    StartWrite();
    LogIntInternal(aNumber);
    EndWrite();        
	}


EXPORT_C void CLogFile::LogBytes(const TDesC8& aBuffer)
	{
    StartWrite();

    for (TInt i = 0; i < aBuffer.Length(); i++)
    	{
        LogByteInternal(aBuffer[i]);
    	}

    EndWrite();
	}


void CLogFile::LogTimeInternal()
	{
    TBuf8<50> text;
    
#ifdef ORIGINAL_TIMESTAMP

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

#else

    TTime time;
    time.HomeTime();
    TBuf<31> dateString;
    _LIT(KDateString4,"%-B%:0%J%:1%T%:2%S%.%*C4%:3%+B ");
	TRAPD(err, time.FormatL(dateString,KDateString4) );
	if (KErrNone == err)
		{
		text.Append(dateString);
		}
	else
		{
		text.Append( _L("### date string format error: ") );
		text.AppendNum(err);
		}
	Write(text);

#endif // ORIGINAL_TIMESTAMP
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

	//	No more newlines left so print remainder
	Write(tail);

	}


void CLogFile::LogByteInternal(TUint8 aByte)
	{
    if ((aByte >= KAsciiStart) && (aByte < KAsciiEnd))
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
        str.AppendNum((TUint)aByte, EHex);
        str.Append(KHexCharRight);
		Write(str);
    	}
	}


void CLogFile::LogIntInternal(TUint aNumber)
	{
    // Display as ASCII char
    TBuf8<20> str;
    str.Append(KHexCharLeft);
    str.AppendNum(aNumber, EHex);
    str.Append(KHexCharRight);
	Write(str);
	}


EXPORT_C void CLogFile::LogNewline()
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
        RThread().Panic(KLogFilePanic, ELogFileWriteFailed);
        }
    }

EXPORT_C void CLogFile::SetAutoFlush(TBool aOn)
	{
    iAutoFlush = aOn;
	}


EXPORT_C void CLogFile::SetAutoTimeStamp(TBool aOn)
	{
    iAutoTimestamp = aOn;
	}


EXPORT_C void CLogFile::SetAutoNewline(TBool aOn)
	{
    iAutoNewline = aOn;
	}


EXPORT_C void CLogFile::StaticLog(const TDesC& aFileName, const TDesC8& aText)
	{
	// This needs to be inside a TRAP statement. Calling StaticLogL 
	// from certain places, for example AppUi destructors, 
	// would result in E32USER-CBase 66 panic.
	TRAP_IGNORE( CLogFile::StaticLogL(aFileName,aText) );
	}


EXPORT_C void CLogFile::StaticLog(const TDesC& aFileName, const TDesC& aText)
	{
	// This needs to be inside a TRAP statement. Calling StaticLogL 
	// from certain places, for example AppUi destructors, 
	// would result in E32USER-CBase 66 panic.
	TRAP_IGNORE( CLogFile::StaticLogL(aFileName,aText) );
	}


EXPORT_C void CLogFile::StaticLogL(const TDesC& aFileName, const TDesC8& aText)
	{
	CLogFile* logFile = NewLC(aFileName, EFalse);
	logFile->SetAutoNewline(ETrue);
	logFile->SetAutoTimeStamp(ETrue);
	logFile->Log(aText);
	CleanupStack::PopAndDestroy(logFile);
	}


EXPORT_C void CLogFile::StaticLogL(const TDesC& aFileName, const TDesC& aText)
	{
	CLogFile* logFile = NewLC(aFileName, EFalse);
	logFile->SetAutoNewline(ETrue);
	logFile->SetAutoTimeStamp(ETrue);
	logFile->Log(aText);
	CleanupStack::PopAndDestroy(logFile);
	}

void CLogFile::GetFileName(TDes& aFileName) const
	{
	iLogFile.FullName(aFileName);
	}


// End of File
