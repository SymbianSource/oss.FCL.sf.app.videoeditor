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
* Observer API.
*
*/


#ifndef MVEINAVIPANECONTROLOBSERVER_H
#define MVEINAVIPANECONTROLOBSERVER_H

/**
 *  Observer api for CVeiNaviPaneControl
 *
 *  @lib internal
 *  @since S60 v5.0
 */
class MVeiNaviPaneControlObserver
    {

public:
    /**
     * SetVolumeLevelL
     *
     * Called when volume level is changed
     * @since S60 v5.0
     * @param aVolume Volume level
     */
    virtual void SetVolumeLevelL( TInt aVolume ) = 0;
    };


#endif // MVEINAVIPANECONTROLOBSERVER_H
