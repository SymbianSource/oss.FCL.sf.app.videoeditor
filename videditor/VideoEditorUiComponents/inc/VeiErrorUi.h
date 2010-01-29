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


#ifndef VEIERRORUI_H
#define VEIERRORUI_H

// INCLUDES
#include <e32base.h>
#include <ConeResLoader.h>
#include "VideoEditorCommon.h"

// FORWARD DECLARATIONS
class CErrorUI;
class CCoeEnv;

/**	CLASS:	CVeiErrorUI
* 
*	Extended error UI.
*
*/
NONSHARABLE_CLASS( CVeiErrorUI ) : public CBase
{
public:

	/** @name Methods:*/
	//@{

	/**
	* Destructor
	*/
	CVeiErrorUI::~CVeiErrorUI();

	/**
	* Static factory constructor.
	*
	* @param   aCoeEnv
	* @return  created instance
	*/
	IMPORT_C static CVeiErrorUI* NewL( CCoeEnv& aCoeEnv );

	/**
	* Shows global error note for given error.
	* There are empirically tested error codes for which platform shows something
	* For other ones platform does not show anything and for them default string is showed.
	* These error codes must be individually tested for every phone model.
	*
	* @param aError standard error code
	* @return error code if showing the note fails, or KErrNone
	*/
	IMPORT_C TInt ShowGlobalErrorNote( TInt aError ) const;

	/**
	* Shows error note with given message.
	* 
	* @param aCoeEnv No description.
	* @param aResourceId No description.
	* @param aError No description.
	* @return error code if showing the note fails, or KErrNone
	*/
	IMPORT_C static TInt ShowErrorNote( CCoeEnv& aCoeEnv, const TInt aResourceId = 0, TInt aError = 0 );
        
	//@}

private:

    /** @name Methods:*/
    //@{

	/**
	* C++ constructor.
	*/
	CVeiErrorUI( CCoeEnv& aCoeEnv );

	/**
	* Second-phase constructor.
	*/
	void ConstructL();

    //@}

private:

	/** @name Members:*/
	//@{

	CCoeEnv& 				iCoeEnv;
	RConeResourceLoader 	iResLoader;
	CErrorUI* 				iErrorUI;

	//@}
};

#endif

// End of File

