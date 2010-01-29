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
* Time label navi observer.
*
*/


#ifndef M_TIMELABELNAVIOBSERVER_H
#define M_TIMELABELNAVIOBSERVER_H

/**  ?description */
//const ?type ?constant_var = ?constant;

/**
 *  Time label navi observer API
 *
 *  @lib VideoEditorUiComponents.lib
 *  @since S60 v5.0
 */
class MTimeLabelNaviObserver
    {

public:
    /**
     * HandleNaviEventL
     *
     * Callend when navipane is clicked.
     * @since S60 v5.0
     */
    virtual void HandleNaviEventL() = 0;

    };


#endif // M_TIMELABELNAVIOBSERVER_H
