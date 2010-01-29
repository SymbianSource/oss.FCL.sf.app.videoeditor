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


#ifndef VEIREMCONTARGET_H
#define VEIREMCONTARGET_H

//  INCLUDES
#include <remconcoreapitargetobserver.h>

// FORWARD DECLARATIONS
class CRemConCoreApiTarget;
class CRemConInterfaceSelector;

// CLASS DECLARATION

class MVeiMediakeyObserver
	{
	public:
		 
     	virtual void HandleVolumeUpL() = 0;	
     	virtual void HandleVolumeDownL() = 0;
	};

/**
*  mgx listener for RemCon commands.
*/
NONSHARABLE_CLASS( CVeiRemConTarget ) :	public CBase, public MRemConCoreApiTargetObserver
    {
    public:  // Methods

	// Constructors and destructor

        /**
        * Static constructor.
        */
        IMPORT_C static CVeiRemConTarget* NewL(MVeiMediakeyObserver& aObserver);

        /**
        * Destructor.
        */
        virtual ~CVeiRemConTarget();

	// Methods from base classes

		/**
		* From MRemConCoreApiTargetObserver MrccatoCommand.
		* A command has been received. 
		* @param aOperationId The operation ID of the command.
		* @param aButtonAct The button action associated with the command.
		*/
		void MrccatoCommand(TRemConCoreApiOperationId aOperationId, TRemConCoreApiButtonAction aButtonAct);

		/**
		* From MRemConCoreApiTargetObserver MrccatoPlay.
		* A 'play' command has been received. 
		* @param aSpeed The playback speed.
		* @param aButtonAct The button action associated with the command.
		*/
		void MrccatoPlay(TRemConCoreApiPlaybackSpeed aSpeed, TRemConCoreApiButtonAction aButtonAct);

		/**
		* From MRemConCoreApiTargetObserver MrccatoTuneFunction.
		* A 'tune function' command has been received.
		* @param aTwoPart If EFalse, only aMajorChannel is to be used. Otherwise, 
		* both aMajorChannel and aMinorChannel are to be used.
		* @param aMajorChannel The major channel number.
		* @param aMinorChannel The minor channel number.
		* @param aButtonAct The button action associated with the command.
		*/
		void MrccatoTuneFunction(TBool aTwoPart, TUint aMajorChannel, TUint aMinorChannel, TRemConCoreApiButtonAction aButtonAct);

		/**
		* From MRemConCoreApiTargetObserver MrccatoSelectDiskFunction.
		* A 'select disk function' has been received.
		* @param aDisk The disk.
		* @param aButtonAct The button action associated with the command.
		*/
		void MrccatoSelectDiskFunction(TUint aDisk, TRemConCoreApiButtonAction aButtonAct);

		/**
		* From MRemConCoreApiTargetObserver MrccatoSelectAvInputFunction.
		* A 'select AV input function' has been received.
		* @param aAvInputSignalNumber The AV input.
		* @param aButtonAct The button action associated with the command.
		*/
		void MrccatoSelectAvInputFunction(TUint8 aAvInputSignalNumber, TRemConCoreApiButtonAction aButtonAct);

		/**
		* From MRemConCoreApiTargetObserver MrccatoSelectAudioInputFunction.
		* A 'select audio input function' has been received.
		* @param aAudioInputSignalNumber The audio input.
		* @param aButtonAct The button action associated with the command.
		*/
		void MrccatoSelectAudioInputFunction(TUint8 aAudioInputSignalNumber, TRemConCoreApiButtonAction aButtonAct);

    protected:  // Methods

    private: //Methods

		/**
        * C++ default constructor.
        */
        CVeiRemConTarget(MVeiMediakeyObserver& aObserver);

        /**
        * By default Symbian 2nd phase constructor is private.
        */
        void ConstructL();

    private:    // Data
    	
    	MVeiMediakeyObserver& iObserver;
		// RemCon interface selector.
    	CRemConInterfaceSelector*	iInterfaceSelector;
		// RemCon Core API target class.
		CRemConCoreApiTarget*		iCoreTarget;
	    };

#endif      //VEIREMCONTARGET_H
            
// End of File
