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
* Movie processor interface class.
*
*/



#ifndef __MOVIEPROCESSOR_H__
#define __MOVIEPROCESSOR_H__


#include <e32base.h>
#include <e32std.h>
#include <gdi.h>

#include "VedCommon.h"

/*
 * Forward declarations.
 */

class CVedMovie;
class CVedMovieImp;
class CVedAudioClip;
class TVedVideoFrameInfo;
class TVideoProcessorAudioData;
class CMovieProcessorImpl;

class MVedMovieProcessingObserver;

/**
 * Movie processor.
 */
class CMovieProcessor : public CBase
	{
public:

	/** 
	 * Constructors for instantiating new movie processors.
	 * Should reserve as little resources as possible at this point.
	 */
	static CMovieProcessor* NewL();
	static CMovieProcessor* NewLC();

	/** 
	 * Destructor can be called at any time (i.e., also in the middle of a processing operation).
	 * Should release all allocated resources, including releasing all allocated memory and 
	 * *deleting* all output files that are currently being processed but not yet completed.
	 */
	virtual ~CMovieProcessor();

    /**
	 * Read the header from the specified video clip file and return its properties.
	 * The file should be opened with EFileShareReadersOnly share mode.
	 *
	 * Possible leave codes:
	 *	- <code>KErrNoMemory</code> if memory allocation fails
	 *	- <code>KErrNotFound</code> if there is no file with the specified name
	 *    in the specified directory (but the directory exists)
	 *	- <code>KErrPathNotFound</code> if the specified directory
	 *    does not exist
	 *	- <code>KErrUnknown</code> if the specified file is of unknown format
	 *
	 * @param aFileName             name of the file to read
	 * @aFileHandle                 handle of the file to read
	 * @param aVideoFormat          for returning the video format
	 * @param aVideoType            for returning the video type
	 * @param aResolution           for returning the resolution
	 * @param aAudioType            for returning the audio type
	 * @param aDuration             for returning the duration
	 * @param aVideoFrameCount      for returning the number of video frames
	 * @param aSamplingRate			for returning the sampling rate
	 * @param aChannelMode			for returning the channel mode	 
	 */
	void GetVideoClipPropertiesL(const TDesC& aFileName,
	                             RFile* aFileHandle,
		                         TVedVideoFormat& aFormat,
								 TVedVideoType& aVideoType, 
								 TSize& aResolution,
								 TVedAudioType& aAudioType, 
								 TTimeIntervalMicroSeconds& aDuration,
								 TInt& aVideoFrameCount,
								 TInt& aSamplingRate, 
								 TVedAudioChannelMode& aChannelMode);
	
    /**
	 * Read video frame information from the specified video clip file and fills array of info for 
     * all frames in video.The file should be opened with EFileShareReadersOnly share mode. Video processor 
	 * should not free the video frame info array after it has passed it on as a return value 
	 * of this function. Returned array should be allocated with User::AllocL() and should be 
	 * freed by the caller of this method with User::Free(). 
	 *
	 * Possible leave codes:
	 *	- <code>KErrNoMemory</code> if memory allocation fails
	 *	- <code>KErrNotFound</code> if there is no file with the specified name
	 *    in the specified directory (but the directory exists)
	 *	- <code>KErrPathNotFound</code> if the specified directory
	 *    does not exist
	 *	- <code>KErrUnknown</code> if the specified file is of unknown format
	 *
	 * @param aFileName             name of the file to read
	 * @param aFileHandle            handle of the file to read
	 * @param aVideoFrameInfoArray  Array for video frame parameters
	 */
    void GenerateVideoFrameInfoArrayL(const TDesC& aFileName, RFile* aFileHandle, TVedVideoFrameInfo*& aVideoFrameInfoArray);

    /**
	 * Returns an estimate of the total size of the specified movie.
     * 
	 * @return  size estimate in bytes
	 */
    TInt GetMovieSizeEstimateL(const CVedMovie* aMovie);

	/**
     * Calculate movie size estimate for MMS
     *          
     * @param aMovie			Movie object 
     * @param aTargetSize		Maximum size allowed
     * @param aStartTime		Time of the first frame included in the MMS output
     * @param aEndTime			Time of the last frame included in the MMS output
     * @return Error code
     */
    TInt GetMovieSizeEstimateForMMSL(const CVedMovie* aMovie, TInt aTargetSize, 
		TTimeIntervalMicroSeconds aStartTime, TTimeIntervalMicroSeconds& aEndTime);

	/**
	 * Do all initializations necessary to start generating a thumbnail, e.g. open files, 
	 * allocate memory. The video clip file should be opened with EFileShareReadersOnly 
	 * share mode. The thumbnail should be scaled to the specified resolution and 
	 * converted to the specified display mode. If this method leaves, destructor should be called to free 
	 * allocated resources.
	 * 
	 * Possible leave codes:
	 *	- <code>KErrNoMemory</code> if memory allocation fails
	 *	- <code>KErrNotFound</code> if there is no file with the specified name
	 *    in the specified directory (but the directory exists)
	 *	- <code>KErrPathNotFound</code> if the specified directory
	 *    does not exist
	 *	- <code>KErrUnknown</code> if the specified file is of unknown format
	 *  - <code>KErrNotSupported</code> if the specified combination of parameters
	 *                                  is not supported
	 *
	 * @param aFileName  name of the file to generate thumbnail from
	 * @param aFileHandle handle of the file to generate thumbnail from
   * @param aIndex       Frame index for selecting the thumbnail frame
   *                     -1 means the best thumbnail is retrieved
 	 * @param aResolution  resolution of the desired thumbnail bitmap, or
	 *                     <code>NULL</code> if the thumbnail should be
	 *                     in the original resolution
	 * @param aDisplayMode desired display mode; or <code>ENone</code> if 
	 *                     any display mode is acceptable
	 * @param aEnhance	   apply image enhancement algorithms to improve
	 *                     thumbnail quality; note that this may considerably
	 *                     increase the processing time needed to prepare
	 *                     the thumbnail
	 */
	void StartThumbL(const TDesC& aFileName, RFile* aFileHandle, TInt aIndex, TSize aResolution, 
		TDisplayMode aDisplayMode, TBool aEnhance);	
	
     /**
	 * Starts thumbnail generation. Thumbnail generation is an asynchronous operation,
	 * and its completion is informed to the caller via Active object request completion;
	 * the iStatus member of the caller is passed as a parameter to this method.
	 *
	 * This method may leave if an error occurs in initiating the thumbnail generation.	
	 * If this method leaves, destructor should be called to free allocated resources.	
	 * 
	 * @param aStatus     Reference to caller's iStatus member variable
	 * @param aFactor     Pointer to a TVedTranscodeFactor structure, which is updated by this method
	 *
	 * @return  
	 *          
	 */
	void ProcessThumbL(TRequestStatus &aStatus, TVedTranscodeFactor* aFactor);
	
	/** 
	 * Method for retrieving the completed thumbnail bitmap.
	 * 
	 * Video processor should not free the CFbsBitmap instance after it has passed it on 
	 * as a return value of this function 
	 *	 
	 */
	void FetchThumb(CFbsBitmap*& aThumb);

	/**
	 * Do all initializations necessary to start processing a movie, e.g. open files and
	 * allocate memory. After initialization, processing is started and the observer
     * is notified. The source video and audio files should be opened with 
	 * EFileShareReadersOnly share mode. If this method leaves, destructor should be called 
	 * to free allocated resources.
	 *
	 * Possible leave codes:
	 *  .
	 *
	 * @param aMovie     movie to process
	 * @param aFilename  output file name
	 * @param aFilename  output file handle
	 */
	void StartMovieL(CVedMovieImp* aMovie, const TDesC& aFileName, RFile* aFileHandle,
	                 MVedMovieProcessingObserver *aObserver);
    
    /**
	 * Cancels the processing of a movie. The processor is reseted to an idle state.
	 * The user must call StartMovieL again after this to start processing again.	      
	 *
	 * Possible leave codes:
	 *  .
	 *
	 */
    void CancelProcessingL();
    
    /**
     * Sets the maximum size for the movie
     * 
     * @param aLimit Maximum size in bytes
     */
    void SetMovieSizeLimit(TInt aLimit);


protected:
	CMovieProcessor();

	void ConstructL();

private:
	TInt iThumbProgress;
	TInt iMovieProgress;
    RPointerArray<TVideoProcessorAudioData> iAudioDataArray;
	CMovieProcessorImpl* iMovieProcessor;
	};


#endif // __MEDIAPROCESSOR_H__

