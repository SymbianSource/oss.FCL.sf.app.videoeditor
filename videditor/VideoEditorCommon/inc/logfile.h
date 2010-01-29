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



#ifndef __LOGFILE_H__
#define __LOGFILE_H__

#include <eikapp.h>
#include <e32base.h>
#include <coemain.h>

// Link with the following libraries: EFSRV.LIB HAL.LIB CHARCONV.LIB           

#define LOG_INIT(name) CLogFile *log = CLogFile::NewL(_L(name), ETrue);

#define LOG_INIT2(name) CLogFile *log = CLogFile::NewL(_L(name), EFalse);

#define LOGL(event) \
	log->Log(_L(event));\
	log->LogNewline();

#define LOGNLN(event) \
	log->Log(_L(event));\

#define LOGS(event) \
	log->Log(event);\
	log->LogNewline();

#define LOGN(num) \
	TBuf<64> log_num;\
	log_num.AppendNum(num); \
	log->Log(log_num);\
	log->LogNewline();

#define LOGF(numf) \
	TBuf<16> log_float;\
	log_float.AppendNum(numf, TRealFormat());\
	log->Log(log_float);\
	log->LogNewline();

#define LOGN2(text, num) \
	TBuf<64> log_num;\
	log_num.AppendNum(num);\
	log->Log(_L(text));\
	log->Log(log_num);\
	log->LogNewline();

#define LOGF2(text, numf) \
	TBuf<16> log_float;\
	log_float.AppendNum(numf, TRealFormat());\
	log->Log(_L(text));\
	log->Log(log_float);\
	log->LogNewline();

#define LOG_END \
	delete log;\
	log = 0;



/*! 
  @class CLogFile
  
  @discussion Class to generate a text file containing logging information
  */
NONSHARABLE_CLASS( CLogFile ) : public CBase
    {
public:
/*!
  @function NewL
   
  @discussion Create a CLogFile object
  @param aFileName the name of the file to create
  @param aInitialiseLog if true, and the log file already exists, previous
  contents will be deleted. If false, append to any existing contents
  @result a pointer to the created instance of CLogFile
  */
    IMPORT_C static CLogFile* NewL(const TDesC& aFileName, TBool aInitialiseLog);

/*!
  @function NewLC
   
  @discussion Create a CLogFile object
  @param aFileName the name of the file to create
  @param aInitialiseLog if true, and the log file already exists, previous
  contents will be deleted. If false, append to any existing contents
  @result a pointer to the created instance of CLogFile
  */
    IMPORT_C static CLogFile* NewLC(const TDesC& aFileName, TBool aInitialiseLog);

/*!
  @function ~CLogFile
  
  @discussion Destroy the object and release all memory objects
  */
    IMPORT_C ~CLogFile();

/*!
  @function Log
  
  @discussion Append the byte to the log file (if not a printable char, it will be logged as ascii-hex)
  @param aByte the byte to log
  */
    IMPORT_C void Log(TUint8 aByte);

/*!
  @function Log
  
  @discussion Append the integer to the log file (logged as ascii-hex)
  @param aNumber the integer to log
  */
    IMPORT_C void Log(TUint aNumber);

/*!
  @function Log
  
  @discussion Append text to the log file
  @param aText the text to log
  */
    IMPORT_C void Log(const TDesC8& aText);

/*!
  @function Log
  
  @discussion Append text to the log file
  @param aText the text to log
  */
    IMPORT_C void Log(const TDesC& aText);

/*!
  @function LogTime
  
  @discussion Append a timestamp to the log file.
  Timestamps are in seconds with three decimal places (but resolution is limited to system timer tick period)
  */
    IMPORT_C void LogTime();

/*!
  @function LogBytes

  @discussion Append the bytes to the log file (non-printable bytes will be logged as ascii-hex)
  @param aBuffer the bytes to log
  */
    IMPORT_C void LogBytes(const TDesC8& aBuffer);

/*!
  @function LogNewline

  @discussion Start a newline in the log file
  */
    IMPORT_C void LogNewline();

/*!
  @function SetAutoFlush

  @discussion Turn AutoFlush on or off. AutoFlush will automatically flush the log file after each write
  @param aOn if true turns AutoFlush on
  */
    IMPORT_C void SetAutoFlush(TBool aOn);

/*!
  @function SetAutoTimeStamp

  @discussion Turn AutoTimeStamp on or off. AutoTimeStamp will add a timestamp to the start of each new line in the log
  @param aOn if true turn AutoTimeStamp on
  */
    IMPORT_C void SetAutoTimeStamp(TBool aOn);

/*!
  @function SetAutoNewline

  @discussion Turn AutoNewline on or off. AutoNewline starts a new line after each log operation
  @param aOn if true turn AutoNewline on
  */
    IMPORT_C void SetAutoNewline(TBool aOn);

/*!
  @function StaticLogL

  @discussion Static option to append text to the log file (leaving version)
  @param aFileName the file to append to
  @param aText the text to append
  */
    IMPORT_C static void StaticLogL(const TDesC& aFileName, const TDesC8& aText);

/*!
  @function StaticLogL

  @discussion Static option to append text to the log file (leaving version)
  @param aFileName the file to append to
  @param aText the text to append
  */
    IMPORT_C static void StaticLogL(const TDesC& aFileName, const TDesC& aText);

/*!
  @function StaticLog

  @discussion Static option to append text to the log file (non-leaving version)
  @param aFileName the file to append to
  @param aText the text to append
  */
    IMPORT_C static void StaticLog(const TDesC& aFileName, const TDesC8& aText);

/*!
  @function StaticLog

  @discussion Static option to append text to the log file (non-leaving version)
  @param aFileName the file to append to
  @param aText the text to append
  */
    IMPORT_C static void StaticLog(const TDesC& aFileName, const TDesC& aText);

/*!
  @function FileName

  @discussion Get the file name and path of the file we are writing to
  @param aFileName
  */
    void GetFileName( TDes& aFileName ) const;

private:
/*!
  @function CLogFile
  
  @discussion Perform the first phase of two phase construction 
  */
    CLogFile();
/*!
  @function ConstructL
  
  @discussion  Perform the second phase construction of a CLogFile object
  @param aFileName the file to open
  @param aInitialiseLog if true, and the log file already exists, previous
  contents will be deleted. If false, append to any existing contents
  */
    void ConstructL(const TDesC& aFileName, TBool aInitialiseLog);
/*!
  @function LogTimeInternal

  @discussion Internal function to log time
  */
    void LogTimeInternal();
/*!
  @function LogTextInternal

  @discussion Internal function to log text
  @param aText the text to log
  */
    void LogTextInternal(const TDesC8& aText);
/*!
  @function LogByteInternal

  @discussion internal function to log a byte
  @param aByte the byte to log
  */
    void LogByteInternal(TUint8 aByte);
/*!
  @function LogIntInternal

  @discussion Internal function to log an integer
  @param aNumber the integer to log
  */
    void LogIntInternal(TUint aNumber);
/*!
  @function StartWrite

  @discussion Perform any initial operation before the main log operation
  */
    void StartWrite();

/*!
  @function EndWrite

  @discussion Perform any tidying up operations after the main log operation
  */
    void EndWrite();

/*!
  @function Write

  @discussion Do the actual writing, and associated error checking
  @param aText the text to write
  */
    void Write(const TDesC8& aText);

/*!
  @function DoLogTextL

  @discussion Leaving functions from void Log(const TDesC& aText)
  */
    void DoLogTextL(const TDesC& aText);

private:
/*!
  @var iLogFile handle to the log file
  */
    RFile       iLogFile;

/*!
  @var iSession file server session
  */
    RFs         iSession;

/*!
  @var iLogMillisecsPerTick number of millisecs per system timer tick
  */
    TInt        iLogMillisecsPerTick;

/*!
  @var iAutoFlush flag - AutoFlush on
  */
    TBool       iAutoFlush;

/*!
  @var iAutoTimestamp flag - AutoTimeStamp on
  */
    TBool       iAutoTimestamp;

/*!
  @var iAutoNewline flag - AutoNewline on
  */
    TBool       iAutoNewline;

/*!
  @var iCheckNestDepth internal to check StartWrite and EndWrite have been called correctly
  */
    TInt        iCheckNestDepth;

    };

#endif // __LOGFILE_H__
