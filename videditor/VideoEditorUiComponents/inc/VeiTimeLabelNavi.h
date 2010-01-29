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

#ifndef TIMELABELNAVI_H
#define TIMELABELNAVI_H

// INCLUDES
#include <coecntrl.h>
#include <aknutils.h>


// FORWARD DECLARATIONS
class CFbsBitmap;
class MTimeLabelNaviObserver;
#ifdef RD_TACTILE_FEEDBACK 
class MTouchFeedback; 
#endif /* RD_TACTILE_FEEDBACK  */

// CLASS DECLARATION

/**
 * CTimeLabelNavi
 */
class CVeiTimeLabelNavi : public CCoeControl
{
	public: // Constructors and destructor
		/**
		 *  Destructor      
		 */
		IMPORT_C virtual ~CVeiTimeLabelNavi();

		/**
		 *  Constructors.
 		 */
		IMPORT_C static CVeiTimeLabelNavi* NewL();
		IMPORT_C static CVeiTimeLabelNavi* NewLC();


	public: // New functions
		/**
         * Changes navipane label.
         * @param aLabel label text
         */
		IMPORT_C void SetLabelL(const TDesC& aLabel);
		
		/**
         * Sets left navipane arrow visibility
         * @param aVisible Whether to show or not.
         * @return -
         */
		IMPORT_C void SetLeftArrowVisibilityL(TBool aVisible);

		/**
         * Sets right navipane arrow visibility
         * @param aVisible Whether to show or not.
         * @return -
         */
		IMPORT_C void SetRightArrowVisibilityL(TBool aVisible);

		/**
         * Sets volume icon visibility
         * @param aVisible Whether to show or not.
         * @return -
         */
		IMPORT_C void SetVolumeIconVisibilityL(TBool aVisible);
		
		/**
		* Sets pause icon visibility
		* @param aVisible whether to show or not.
		* @return -
		*/
		IMPORT_C void SetPauseIconVisibilityL(TBool aVisible);

		/**
		* SetNaviObserver
		* @param aObserver Observer.
		*/		
		void SetNaviObserver(MTimeLabelNaviObserver* aObserver)
		    {
		    iObserver = aObserver;
		    };

	protected: // Functions from base classes   

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
		
		/**
		* From CCoeControl. Handles a control's pointer events.  
		* @param aPointerEvent  pointer event.
	 	*/		
		void HandlePointerEventL(const TPointerEvent& aPointerEvent);

	private: // Constructors and destructor

		/**
		 * Constructor.
		 */
		CVeiTimeLabelNavi();

		/**
 		 * EPOC 2nd phase constructor.
		 */
		void ConstructL();

		/**
 		 * Load the icon bitmaps.
		 */
		void LoadBitmapsL();

		/**
 		 * Delete the icon bitmaps.
		 */
		void DeleteBitmaps();
	
	private:    // Data
	    /// Own: Volume bitmap
	    CFbsBitmap* iVolumeBitmap;
	    CFbsBitmap* iVolumeBitmapMask;
	    
	    /// Own: Arrow bitmap
		CFbsBitmap* iArrowBitmap;
		CFbsBitmap* iArrowBitmapMask;

		/// Own: Muted bitmap
		CFbsBitmap* iMutedBitmap;
		CFbsBitmap* iMutedBitmapMask;
		
		/// Own: Paused bitmap
		CFbsBitmap* iPausedBitmap;
		CFbsBitmap* iPausedBitmapMask;
		
		TBool iArrowVisible;
		TBool iVolumeIconVisible;
		TBool iPauseIconVisible;

		TBuf<32> iLabel;

		/// Rectangle where label is drawn
		TAknLayoutText iTextLayout;
		/// Layout array for volume/muted, array and paused items 
		TAknLayoutRect iBitmapLayout[3];
		
		/// Ref: to observer
		MTimeLabelNaviObserver* iObserver;

    	// Feedback for screen touch:
#ifdef RD_TACTILE_FEEDBACK 
		MTouchFeedback* iTouchFeedBack;
#endif /* RD_TACTILE_FEEDBACK  */ 

};

#endif // VEITIMELABELNAVI_H

// End of file
