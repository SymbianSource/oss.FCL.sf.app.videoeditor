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


// Simple RFileLogger based set of log writer macros.
//
// Instructions:
//
// 1. Put the following line into you MMP file
// LIBRARY flogger.lib
//
// 2. Define DEBUG_ON to enable logs in UDEB builds.
//    Alternatively, define DEBUG_ON_ALWAYS to enable logs 
//    always (NOTE: do not use in productions code)
//    - in mmp file: macro DEBUG_ON
//    - in cpp file: #define DEBUG_ON
// 
// 3a. Call LOG_RESET to create the log file, overwriting possibly
//    existing old log file.
//
// 3b. Alternatively, you can skip the step 3a, in which case the 
//    the output is appended to the possibly existing old file.
//
// 4. Use the macros to write to the log file. For example
//    LOG("Started processing")
// 
// 5. To enable the logs, manually create the logging 
//    directory "C:\Logs\VideoEditor" on the device.
//

#ifndef __DEBUGUTILS_H__
#define __DEBUGUTILS_H__

// Two alternative implementations:
// - using RFileLogger (system component)
// - using ClogFile (implemented in this project)
//#define _FLOGGER_IMPLEMENTATION_
#define _CLOGFILE_IMPLEMENTATION_

#ifdef _FLOGGER_IMPLEMENTATION_
#include <flogger.h>
#else
#include "logfile.h"
#include <bautils.h>
#include <f32file.h>
#endif
#include <e32svr.h>

// Default log files
_LIT(KVideoEditorLogFile,"VideoEditor.log");
_LIT(KVideoProviderLogFile,"VideoProvider.log");

// Log directory
_LIT(KLogDir, "VideoEditor");

// Maximum length for a log line. 
const TInt KMaxLogLineLength = 256;

// Overflow handler. Too log lines are truncated. 
class TLogFileDes16OverflowHandler : public TDes16Overflow
	{
	virtual void Overflow(TDes16& /*aDes*/) 
		{
		// do nothing
		}
	};

_LIT(KLogsFolder, "C:\\Logs\\");
_LIT(KBackslash, "\\");


// The log is enabled in debug builds only, unless
// DEBUG_ON_ALWAYS is defined to always enable it.
#ifdef _DEBUG
	#ifdef DEBUG_ON
	#define _ENABLED_
	#endif
#else
	#ifdef DEBUG_ON_ALWAYS
	#define _ENABLED_
	#endif
#endif

#ifdef _ENABLED_

#define DEBUGLOG_ARG(x) x

#ifdef _FLOGGER_IMPLEMENTATION_ 

// Initialize the log file, overwrite existing
// Logs file are always created in C:\Logs. The first argument to CreateLog() 
// is a directory name relative to this C:\Logs directory.
#define LOG_RESET(aLogFile) \
	{\
	RFileLogger logger;\
	logger.Connect();\
	logger.CreateLog(KLogDir,aLogFile,EFileLoggingModeOverwrite);\
	logger.Write(KLogDir,aLogFile,EFileLoggingModeAppend,_L("*** Log file created ***"));\
	logger.CloseLog();\
	logger.Close();\
	}

// Log a simple text, e.g.
//   LOG(_L("logfile.txt"),"Something happens here")
#define LOG(aLogFile,aText) \
	{\
	RFileLogger::Write(KLogDir,aLogFile,EFileLoggingModeAppend,_L(aText));\
	RDebug::Print(_L(aText));\
	}

// Log a Descriptor
#define LOGDES(aLogFile,aDes) \
	{\
	RFileLogger::Write(KLogDir,aLogFile,EFileLoggingModeAppend,aDes);\
	RDebug::Print(aDes);\
	}

// Log a number with string format, e.g. 
//	LOGFMT(KLogFile,"Result=%d",err)
//	LOGFMT(KLogFile, "FileName: %S", &aFileName );
#define LOGFMT(aLogFile,aText,aParam) \
	{\
	RFileLogger::WriteFormat(KLogDir,aLogFile,EFileLoggingModeAppend,_L(aText),aParam);\
	RDebug::Print(_L(aText), aParam);\
	}

// With 2 parameters
#define LOGFMT2(aLogFile,aText,aParam1,aParam2) \
	{\
	RFileLogger::WriteFormat(KLogDir,aLogFile,EFileLoggingModeAppend,_L(aText),aParam1,aParam2);\
	RDebug::Print(_L(aText), aParam1,aParam2);\
	}

// With 3 parameters
#define LOGFMT3(aLogFile,aText,aParam1,aParam2,aParam3) \
	{\
	RFileLogger::WriteFormat(KLogDir,aLogFile,EFileLoggingModeAppend,_L(aText),aParam1,aParam2,aParam3);\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3);\
	}

// With 4 parameters
#define LOGFMT4(aLogFile,aText,aParam1,aParam2,aParam3,aParam4) \
	{\
	RFileLogger::WriteFormat(KLogDir,aLogFile,EFileLoggingModeAppend,_L(aText),aParam1,aParam2,aParam3,aParam4);\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3,aParam4);\
	}

// With 6 parameters
#define LOGFMT6(aLogFile,aText,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6) \
	{\
	RFileLogger::WriteFormat(KLogDir,aLogFile,EFileLoggingModeAppend,_L(aText),aParam1,aParam2,aParam3,aParam4,aParam5,aParam6);\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3,aParam4,aParam5,aParam6);\
	}

// With 12 parameters
#define LOGFMT12(aLogFile,aText,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6,aParam7,aParam8,aParam9,aParam10,aParam11,aParam12) \
	{\
	RFileLogger::WriteFormat(KLogDir,aLogFile,EFileLoggingModeAppend,_L(aText),aParam1,aParam2,aParam3,aParam4);\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3,aParam4,,aParam5,aParam6,aParam7,aParam8,aParam9,aParam10,aParam11,aParam12);\
	}

// Log hex dump
#define LOGHEXDUMP(aLogFile,aHeader,aMargin,aPtr,aLen) \
	RFileLogger::HexDump(KLogDir,aLogFile, EFileLoggingModeAppend, _S(aHeader), _S(aMargin), aPtr, aLen);

// Log the memory allocated on the current thread's default heap
#define LOG_HEAP_USAGE(aLogFile) \
	{\
	TInt allocSize;\
	User::Heap().AllocSize(allocSize);\
	_LIT(KText,"* Memory allocated on the thread's default heap: %d *");\
	RFileLogger::WriteFormat(KLogDir,aLogFile,EFileLoggingModeAppend,KText,allocSize);\
	RDebug::Print(KText,allocSize);\
	}

#endif // _FLOGGER_IMPLEMENTATION_

#ifdef _CLOGFILE_IMPLEMENTATION_

#define LOG_RESET(aLogFile) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		RFile file;\
		file.Replace(fs, fileNameAndPath, EFileShareAny|EFileWrite);\
		file.Close();\
		CLogFile::StaticLog(fileNameAndPath,_L("*** Log file created ***\n"));\
		}\
	fs.Close();\
	}

#define LOG(aLogFile,aText) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		CLogFile::StaticLog(fileNameAndPath,_L(aText));\
		}\
	fs.Close();\
	RDebug::Print(_L(aText));\
	}

#define LOGDES(aLogFile,aDes) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		CLogFile::StaticLog(fileNameAndPath,aDes);\
		}\
	fs.Close();\
	RDebug::Print(aDes);\
	}

#define LOGFMT(aLogFile,aText,aParam) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		_LIT(KText,aText);\
		TLogFileDes16OverflowHandler ofh;\
		TBuf<KMaxLogLineLength> buf;\
		buf.AppendFormat(KText,&ofh,aParam);\
		CLogFile::StaticLog(fileNameAndPath,buf);\
		}\
	fs.Close();\
	RDebug::Print(_L(aText), aParam);\
	}

// With 2 parameters
#define LOGFMT2(aLogFile,aText,aParam1,aParam2) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		_LIT(KText,aText);\
		TLogFileDes16OverflowHandler ofh;\
		TBuf<KMaxLogLineLength> buf;\
		buf.AppendFormat(KText,&ofh,aParam1,aParam2);\
		CLogFile::StaticLog(fileNameAndPath,buf);\
		}\
	fs.Close();\
	RDebug::Print(_L(aText), aParam1,aParam2);\
	}

// With 3 parameters
#define LOGFMT3(aLogFile,aText,aParam1,aParam2,aParam3) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		_LIT(KText,aText);\
		TLogFileDes16OverflowHandler ofh;\
		TBuf<KMaxLogLineLength> buf;\
		buf.AppendFormat(KText,&ofh,aParam1,aParam2,aParam3);\
		CLogFile::StaticLog(fileNameAndPath,buf);\
		}\
	fs.Close();\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3);\
	}

// With 4 parameters
#define LOGFMT4(aLogFile,aText,aParam1,aParam2,aParam3,aParam4) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		_LIT(KText,aText);\
		TLogFileDes16OverflowHandler ofh;\
		TBuf<KMaxLogLineLength> buf;\
		buf.AppendFormat(KText,&ofh,aParam1,aParam2,aParam3,aParam4);\
		CLogFile::StaticLog(fileNameAndPath,buf);\
		}\
	fs.Close();\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3,aParam4);\
	}

// With 6 parameters
#define LOGFMT6(aLogFile,aText,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		_LIT(KText,aText);\
		TLogFileDes16OverflowHandler ofh;\
		TBuf<KMaxLogLineLength> buf;\
		buf.AppendFormat(KText,&ofh,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6);\
		CLogFile::StaticLog(fileNameAndPath,buf);\
		}\
	fs.Close();\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3,aParam4,aParam5,aParam6);\
	}

// With 12 parameters
#define LOGFMT12(aLogFile,aText,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6,aParam7,aParam8,aParam9,aParam10,aParam11,aParam12) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		_LIT(KText,aText);\
		TLogFileDes16OverflowHandler ofh;\
		TBuf<KMaxLogLineLength> buf;\
		buf.AppendFormat(KText,&ofh,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6,aParam7,aParam8,aParam9,aParam10,aParam11,aParam12);\
		CLogFile::StaticLog(fileNameAndPath,buf);\
		}\
	fs.Close();\
	RDebug::Print(_L(aText), aParam1,aParam2,aParam3,aParam4,aParam5,aParam6,aParam7,aParam8,aParam9,aParam10,aParam11,aParam12);\
	}

// Not implemented
#define LOGHEXDUMP(aLogFile,aHeader,aMargin,aPtr,aLen)

// Log the memory allocated on the current thread's default heap
#define LOG_HEAP_USAGE(aLogFile) \
	{\
	TFileName path(KLogsFolder);\
	path.Append(KLogDir);\
	path.Append(KBackslash);\
	TFileName fileNameAndPath(path);\
	fileNameAndPath.Append(aLogFile);\
	RFs fs;\
	fs.Connect();\
	TInt allocSize;\
	User::Heap().AllocSize(allocSize);\
	_LIT(KText,"* Memory allocated on the thread's default heap: %d *");\
	if(BaflUtils::FolderExists(fs,path))\
		{\
		TLogFileDes16OverflowHandler ofh;\
		TBuf<KMaxLogLineLength> buf;\
		buf.AppendFormat(KText,&ofh,allocSize);\
		CLogFile::StaticLog(fileNameAndPath,buf);\
		}\
	fs.Close();\
	RDebug::Print(KText, allocSize);\
	}

#endif // _CLOGFILE_IMPLEMENTATION_

#else // _ENABLED_

#define DEBUGLOG_ARG(x)

#define LOG_RESET(aLogFile)
#define LOG(aLogFile,aText)
#define LOGDES(aLogFile,aDes)
#define LOGFMT(aLogFile,aText,aParam)
#define LOGFMT2(aLogFile,aText,aParam1,aParam2)
#define LOGFMT3(aLogFile,aText,aParam1,aParam2,aParam3)
#define LOGFMT4(aLogFile,aText,aParam1,aParam2,aParam3,aParam4)
#define LOGFMT6(aLogFile,aText,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6)
#define LOGFMT12(aLogFile,aText,aParam1,aParam2,aParam3,aParam4,aParam5,aParam6,aParam7,aParam8,aParam9,aParam10,aParam11,aParam12)
#define LOGHEXDUMP(aLogFile,aHeader,aMargin,aPtr,aLen)
#define LOG_HEAP_USAGE(aLogFile)

#endif // _ENABLED_

#endif // __DEBUGUTILS_H__
