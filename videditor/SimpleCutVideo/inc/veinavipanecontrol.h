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
* Navipane control for SVE.
*
*/

#ifndef C_CVEINAVIPANECONTROL_H
#define C_CVEINAVIPANECONTROL_H

#include <e32base.h>
#include <coecobs.h>        // MCoeControlObserver
#include "mtimelabelnaviobserver.h"

class CEikStatusPane;
class CAknNavigationDecorator;
class CAknNavigationControlContainer;
class CVeiTimeLabelNavi;
class CAknVolumeControl;
class CPeriodic;
class MVeiNaviPaneControlObserver;

/**
 *  Navipane control.
 *
 *  @code
 *   ?good_class_usage_example(s)
 *  @endcode
 *
 *  @lib internal (VedSimpleCutVideo.exe)
 *  @since S60 v5.0
 */
class CVeiNaviPaneControl : public CBase, MCoeControlObserver,
    MTimeLabelNaviObserver
    {

public:

    /**
     * Two-phased constructor.
     * @param aStatusPane aPointer to app status pane
     */
    static CVeiNaviPaneControl* NewL( CEikStatusPane* aStatusPane );
     
    /**
    * Destructor.
    */
    ~CVeiNaviPaneControl();

    /**
     * DrawTimeNaviL
     *
     * @since S60 v5.0
     * @param aElapsed Elapsed time
     * @param aTotal Total time
     */      
    void DrawTimeNaviL( TTime aElapsed, TTime aTotal );

    /**
     * SetObserver
     *
     * @since S60 v5.0
     * @param aObserver Navi pane control observer. 
     *      See "mveinavipanecontrolobserver.h"
     */     
    void SetObserver( MVeiNaviPaneControlObserver* aObserver )
        {
        ASSERT( aObserver );
        iObserver = aObserver;
        };
    
    /**
     * SetPauseIconVisibilityL
     *
     * @since S60 v5.0
     * @param aVisible ETrue = visible
     */    
    void SetPauseIconVisibilityL( TBool aVisible );
    
    /**
     * SetVolumeIconVisibilityL
     *
     * @since S60 v5.0
     * @param aVisible ETrue = visible
     */    
    void SetVolumeIconVisibilityL( TBool aVisible );    

    /**
     * ShowVolumeLabelL
     *
     * @since S60 v5.0
     * @param aVolume Volume level
     */
    void ShowVolumeLabelL( TInt aVolume );
    
    /**
     * HandleResourceChange
     *
     * @since S60 v5.0
     * @param aType Type of the resource change.
     */
    void HandleResourceChange( TInt aType );
    
// from base class MCoeControlObserver
    void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
    
// from base class MTimeLabelNaviObserver    
    void HandleNaviEventL();
    
private:

    CVeiNaviPaneControl( CEikStatusPane* aStatusPane );
    void ConstructL();

// Implementation

    CAknNavigationDecorator* CreateTimeLabelNaviL();
    CVeiTimeLabelNavi* GetTimeLabelControl();
    CAknVolumeControl* GetVolumeControl();
    static TInt HideVolumeCallbackL(TAny* aPtr);
    void HideVolume();    
    
private: // data

    /**
     * Ref to StatusPane.
     * Not own.
     */
	CEikStatusPane* iStatusPane;
	 
    /**
     * Ref to NaviPane.
     * Not own.
     */
	CAknNavigationControlContainer* iNaviPane;	
    
    /**
     * Time Navi item.
     * Own.
     */
	CAknNavigationDecorator* iTimeNavi; 
	
	/**
     * Volume navi decorator.
     * Own.
     */
	CAknNavigationDecorator* iVolumeNavi;
	
	/**
     * Volume navi decorator.
     * Own.
     */
	CPeriodic* iVolumeHider;

	/**
     * Ref to observer.
     * Not own.
     */	
	MVeiNaviPaneControlObserver* iObserver;		
    };


#endif // C_CVEINAVIPANECONTROL_H
