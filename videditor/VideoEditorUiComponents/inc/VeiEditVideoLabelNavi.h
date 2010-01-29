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


#ifndef __VEIEDITVIDEOLABELNAVI_H__
#define __VEIEDITVIDEOLABELNAVI_H__

#include <aknview.h>
#include <coecntrl.h>
#include <aknutils.h>
#include <ConeResLoader.h>

// Forward declarations
class CAknLayoutFont;

/**
* CVeiEditVideoLabelNavi. Navilabel with envelope and time.
*
*/

class CVeiEditVideoLabelNavi : public CCoeControl
    {
public:
	/**
	* LabelNavi state.
	*/
	enum TLabelNaviState
		{
        EStateInitializing = 1,
		EStateEditView,
		EStateTrimForMmsView
		};
public: 
	/**
    * Destructor.
    */
	IMPORT_C virtual ~CVeiEditVideoLabelNavi();

	/** 
	* Static factory method.
	*
	* @return  the created CVeiEditVideoLabelNavi object
	*/
	IMPORT_C static CVeiEditVideoLabelNavi* NewL();

	/** 
	* Static factory method. Leaves the created object in the cleanup
	* stack.
	*
	* @return  the created CVeiEditVideoLabelNavi object
	*/
	IMPORT_C static CVeiEditVideoLabelNavi* NewLC();

	/**
	* Set MMS envelope without red line or with it.
	*/
	IMPORT_C void SetMmsAvailableL( TBool aIsAvailable );

	/**
	* 
	*/
	IMPORT_C void SetMemoryAvailableL( TBool aIsAvailable );

	/**
	* Set memory in use Phone/MMC.
	*/
	IMPORT_C void SetMemoryInUseL( TBool aPhoneMemory );

    /**
    *
    */
	IMPORT_C TInt GetMaxMmsSize() const;

public:
	/**
	* Set movie duration.
    * @param aDuration in microseconds
	*/
	void SetDurationLabelL( const TInt64& aDuration );

	/**
	* Set movie size.
	* @param aSize in kB.
	*/
	void SetSizeLabelL( const TUint& aSize );

	/**
	* Set whether editview or trimformms-view 
	* @param aState
	*/
	void SetState( CVeiEditVideoLabelNavi::TLabelNaviState aState );

	/**
	* Set whether editview or trimformms-view 
	* @param aState
	*/
	TBool IsMMSAvailable() const;


protected:
	/**
	 * From CCoeControl. Handle the size change events.
	 */
	void SizeChanged();

	/**
	 * From CCoeControl.  Draw a control.  
	 * @param aRect The region of the control to be redrawn.   
 	 */
	void Draw(const TRect& aRect) const;

	/**
	 * From CCoeControl. Handles a change to the control's resources.  
	 * @param aType A message UID value.
 	 */
	void HandleResourceChange(TInt aType); 

private:
    /**
	* Default constructor.
    */
    void ConstructL();

	/**
	* Constructor.
	*/
	CVeiEditVideoLabelNavi();

	/**
	* Completes construction after session to the messaging serve has been opened.
	*/
	void CompleteConstructL();

	/**
	* Load the icon bitmaps.
	*/
	void LoadBitmapsL();

	/**
	* Delete the icon bitmaps.
	*/
	void DeleteBitmaps();

private: 
	/** Movie duration. */
	TInt64			iStoryboardDuration;

	/** Movie size. */
	TInt			iStoryboardSize;

	/** Layouts for text. */
	TAknLayoutText	iTextLayout[3];

    /** Layouts for icons. */
	TAknLayoutRect  iBitmapLayout[3];
	
	/** MMS available bitmap. */
	CFbsBitmap*		iMmsBitmap;
    /** MMS available bitmap mask. */
    CFbsBitmap*	    iMmsBitmapMask;
	/** MMS not available bitmap. */
	CFbsBitmap*		iNoMmsBitmap;
    /** MMS not available bitmap mask. */
    CFbsBitmap*     iNoMmsBitmapMask;

	/** MMS available flag. */
	TBool			iMmsAvailable;
	/** MMS Max size. */
	TInt			iMmsMaxSize;
	
	/** Current state. */
	TLabelNaviState iState;

	/** Hard disk available bitmap.  */
	CFbsBitmap*	iPhoneMemoryBitmap;
    /** Hard disk available bitmap mask.  */
	CFbsBitmap*	iPhoneMemoryBitmapMask;

	/** Hard disk not available bitmap. */
	CFbsBitmap*	iNoPhoneMemoryBitmap;
    /** Hard disk not available bitmap mask. */
	CFbsBitmap*	iNoPhoneMemoryBitmapMask;

	/** Hard disk available flag. */
	TBool		iPhoneMemoryAvailable;

	/** Hard disk available bitmap.  */
	CFbsBitmap*	iMMCBitmap;
    /** Hard disk available bitmap mask.  */
	CFbsBitmap*	iMMCBitmapMask;

	/** Hard disk not available bitmap. */
	CFbsBitmap*	iNoMMCBitmap;
    /** Hard disk not available bitmap mask. */
	CFbsBitmap*	iNoMMCBitmapMask;


	/** Hard disk available flag. */
	TBool		iMMCAvailable;

	/** Whether phone memory or memory card in use. */
	TBool		iPhoneMemory;

	/** Whether is enough memory  */
	TBool		iMemoryAvailable;
	
	/** Time bitmap. */
	CFbsBitmap* iTimeBitmap;

	/** Time bitmap mask. */
	CFbsBitmap* iTimeBitmapMask;

	RConeResourceLoader 	iResLoader;
	
	CAknLayoutFont* 		iCustomFont;
    };

#endif

