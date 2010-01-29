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
* H.263 bitstream parsing.
*
*/



/*
 * Includes
 */

#include "h263dConfig.h"
#include "viddemux.h"
#include "vdxint.h"
#include "debug.h"
#include "biblin.h"
/* MVE */
#include "MPEG4Transcoder.h"

/*
 * Defines
 */

#ifdef DEB_STDOUT
/* Use back-channel debug output file when printing back-channel related 
   messages. */
#include <stdio.h>
extern FILE *bcmDebFile;
#endif

 
/*
 * Module-scope typedefs
 */

/* Another type for VLC (variable length code) lookup tables used in 
   Annex I implementation.*/
typedef struct {
   u_char LAST;   /* see section 5.4.2 of the H.263 recommendation */
   u_char RUN;    /* to understand LAST, RUN and LEVEL */
   u_char LEVEL;
   u_char len;    /* actual length of code in bits */
} vdxVLCTableNew_t;


/*
 * Global constants
 */

/* Used to convert a luminance block index defined in section 4.2.1
   of the H.263 recommendation to a coded block pattern mask (see sections
   5.3.4 and 5.3.5 of the H.263 recommendation. 
   See also macros section in viddemux.h. */
const int vdxBlockIndexToCBPYMask[5] = {0, 8, 4, 2, 1};
const int vdxYBlockIndexToCBPBMask[5] = {0, 32, 16, 8, 4};


/*
 * Local function prototypes
 */

/* Picture Layer */

static int vdxActAfterIncorrectSEI(
   bibBuffer_t *inBuffer,
   int fPLUSPTYPE,
   int *fLast,
   int *bitErrorIndication);

static void vdxStandardSourceFormatToFrameSize(int sourceFormat, int *width, int *height);

/* Slice Layer */
int vdxFrameSizeToPictureFormat(int width, int height);

/* Macroblock Layer */

static int vdxGetIntraMode(bibBuffer_t *inBuffer,int *index,
   int *bitErrorIndication);

static int vdxUMVGetMVD(bibBuffer_t *inBuffer,int *mvdx10,
   int *bitErrorIndication);

static int vdxGetNormalMODB(bibBuffer_t *inBuffer, int *index, 
   int *bitErrorIndication);

static int vdxGetImpPBMODB(bibBuffer_t *inBuffer, int *index,
   int *bitErrorIndication);




/*
 * Picture Layer Global Functions
 */

/* {{-output"vdxGetPictureHeader.txt"}} */
/*
 * vdxGetPictureHeader
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    inpParam                   input parameters
 *    header                     output parameters: picture header
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the H.263 picture header.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxGetPictureHeader(
   bibBuffer_t *inBuffer, 
   const vdxGetPictureHeaderInputParam_t *inpParam,
   vdxPictureHeader_t *header,
   int *bitErrorIndication)
/* {{-output"vdxGetPictureHeader.txt"}} */
{
   bibFlushBits_t
      flushBits;
   bibGetBits_t
      getBits;
   int
      numBitsGot,
      bits1To5OfPTYPE,
      sourceFormat;
   int16
      bibError = 0;
   u_int32 tmp = 0;          /* temporary variable for reading some redundant bits */

   vdxAssert(inBuffer != NULL);
   vdxAssert(inpParam != NULL);
   vdxAssert(inpParam->flushBits != NULL);
   vdxAssert(inpParam->getBits != NULL);
   vdxAssert(header != NULL);
   vdxAssert(bitErrorIndication != NULL);

   flushBits = inpParam->flushBits;
   getBits = inpParam->getBits;

   memset(header, 0, sizeof(vdxPictureHeader_t));
   *bitErrorIndication = 0;

   /* Assume that the existence of a PSC has been checked,
      it is the next code in the buffer, and
      its byte alignment has been checked */

   /* Flush stuffing (PSTUF) */
   if (inpParam->numStuffBits) {
      flushBits(inpParam->numStuffBits, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
   }

   /* Flush PSC */
   flushBits(22, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
   /* PSC cannot contain fatal bit errors (checked earlier) */
   header->numBits += 22;

   *bitErrorIndication = 0;   

   /* TR */
   header->tr = (int) getBits(8, inBuffer, &numBitsGot, bitErrorIndication,
      &bibError);
   header->numBits += 8;


   #ifdef DEB_STDOUT
      if (bcmDebFile)
         deb1f(bcmDebFile, "TR %d\n", header->tr);
   #endif

   /* The first 5 bits from PTYPE */
   bits1To5OfPTYPE = (int) getBits(5, inBuffer, &numBitsGot,
      bitErrorIndication, &bibError);
   header->numBits += 5;


   /* PTYPE bit 1 should be 1 */
   if ((bits1To5OfPTYPE & 16) == 0 ) {
      deb0p("ERROR. PTYPE bit 1 invalid.\n");
      goto exitAfterBitError;
   }

   /* PTYPE bit 2 should be 0 for distinction with H.261 */
   if (bits1To5OfPTYPE & 8) {
      deb0p("ERROR. PTYPE bit 2 indicates H.261.\n");
      goto exitAfterBitError;
   }

   /* PTYPE bit 3, Split Screen Indicator */
   header->fSplitScreen = ((bits1To5OfPTYPE & 4) > 0);

   /* PTYPE bit 4, Document camera indicator */
   header->fDocumentCamera = ((bits1To5OfPTYPE & 2) > 0);

   /* PTYPE bit 5, Freeze Picture Release */
   header->fFreezePictureRelease = (bits1To5OfPTYPE & 1);

   /* PTYPE bits 6 - 8, Source Format */
   sourceFormat = (int) getBits(3, inBuffer, &numBitsGot,
      bitErrorIndication, &bibError);
   header->numBits += 3;


   /* If H.263 version 1 */
   if (sourceFormat >= 1 && sourceFormat <= 5) {
      int bits9To13OfPTYPE;

      header->fPLUSPTYPE = 0;

      vdxStandardSourceFormatToFrameSize(sourceFormat, &header->lumWidth, &header->lumHeight);

      /* PTYPE bits 9 - 13 */
      bits9To13OfPTYPE = (int) getBits(5, inBuffer, &numBitsGot,
         bitErrorIndication, &bibError);
      header->numBits += 5;


      /* PTYPE bit 9, Picture coding type */
      header->pictureType = ((bits9To13OfPTYPE & 16) > 0);

      /* PTYPE bit 10, Unrestricted motion vector mode */
      header->fUMV = ((bits9To13OfPTYPE & 8) > 0);

      /* PTYPE bit 11, Syntax-based Arithmetic Coding mode */
      header->fSAC = ((bits9To13OfPTYPE & 4) > 0);

      /* PTYPE bit 12, Advanced prediction mode */
      header->fAP = ((bits9To13OfPTYPE & 2) > 0);

      /* PTYPE bit 13, PB-frames mode */

      /* If PTYPE bit 9 indicates a P-picture */
      if (header->pictureType) {
         /* Check if it is actually a PB-picture */
         if (bits9To13OfPTYPE & 1)
            header->pictureType = VDX_PIC_TYPE_PB;
      }

      /* Else PTYPE bit 9 indicates an I-picture */
      else {
         /* Check that bit 13 is 0 */
         if (bits9To13OfPTYPE & 1) {
            deb0p("ERROR. PTYPE bit 9 and 13 mismatch.\n");
            goto exitAfterBitError;
         }
      }
   }

   /* Else if H.263 version 2 (PLUSPTYPE) */
   else if (sourceFormat == 7) {
      int bits4To9OfMPPTYPE;

      header->fPLUSPTYPE = 1;

      /* UFEP */
      header->ufep = (int) getBits(3, inBuffer, &numBitsGot,
         bitErrorIndication, &bibError);
      header->numBits += 3;


      if (header->ufep > 1) {
         deb0p("ERROR. UFEP illegal.\n");
         goto exitAfterBitError;
      }

      /* If UFEP = '001' */
      if (header->ufep) {
         int bits4To18OfOPPTYPE;

         /* OPPTYPE bits 1 - 3, Source format */
         sourceFormat = (int) getBits(3, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 3;



         if (sourceFormat >= 1 && sourceFormat <= 5) {
            header->fCustomSourceFormat = 0;
            vdxStandardSourceFormatToFrameSize(sourceFormat, 
               &header->lumWidth, &header->lumHeight);
         }

         else if (sourceFormat == 6)
            header->fCustomSourceFormat = 1;

         else {
            deb0p("ERROR. Source format illegal.\n");
            goto exitAfterBitError;
         }

         /* OPPTYPE bits 4 - 18 */
         bits4To18OfOPPTYPE = (int) getBits(15, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 15;


         /* OPPTYPE bit 4, Custom PCF */
         header->fCustomPCF = ((bits4To18OfOPPTYPE & 0x4000) > 0);

         /* OPPTYPE bit 5, Unrestricted Motion Vector mode */
         header->fUMV = ((bits4To18OfOPPTYPE & 0x2000) > 0);

         /* OPPTYPE bit 6, Syntax-based Arithmetic Coding mode */
         header->fSAC = ((bits4To18OfOPPTYPE & 0x1000) > 0);

         /* OPPTYPE bit 7, Advanced Prediction mode */
         header->fAP = ((bits4To18OfOPPTYPE & 0x0800) > 0);

         /* OPPTYPE bit 8, Advanced INTRA Coding mode */
         header->fAIC = ((bits4To18OfOPPTYPE & 0x0400) > 0);

         /* OPPTYPE bit 9, Deblocking filter mode */
         header->fDF = ((bits4To18OfOPPTYPE & 0x0200) > 0);

         /* OPPTYPE bit 10, Slice Structured mode */
         header->fSS = ((bits4To18OfOPPTYPE & 0x0100) > 0);

         /* OPPTYPE bit 11, Reference Picture Selection mode */
         header->fRPS = ((bits4To18OfOPPTYPE & 0x0080) > 0);

         /* OPPTYPE bit 12, Independent Segment Decoding mode */
         header->fISD = ((bits4To18OfOPPTYPE & 0x0040) > 0);

         /* OPPTYPE bit 13, Alternative Inter VLC mode */
         header->fAIV = ((bits4To18OfOPPTYPE & 0x0020) > 0);

         /* OPPTYPE bit 14, Modified Quantization mode */
         header->fMQ = ((bits4To18OfOPPTYPE & 0x0010) > 0);

         /* OPPTYPE bits 15 - 18 must be '1000' */
         if ((bits4To18OfOPPTYPE & 0x000F) != 8) {
            deb0p("ERROR. OPPTYPE bits 15 - 18 illegal.\n");
            goto exitAfterBitError;
         }

         /* Mode interaction restrictions, see section 5.1.4.6 of 
            the H.263 recommendation */
         if (header->fSAC && (header->fAIV || header->fMQ || header->fUMV)) {
            deb0p("ERROR. Illegal bit pattern (section 5.1.4.6).\n");
            goto exitAfterBitError;
         }
      }

      /* MPPTYPE, bits 1 - 3, Picture type code */
      header->pictureType = (int) getBits(3, inBuffer, &numBitsGot,
         bitErrorIndication, &bibError);
      header->numBits += 3;


      if (header->pictureType >= 6) {
         deb0p("ERROR. Picture type code illegal.\n");
         goto exitAfterBitError;
      }

      /* MPPTYPE, bits 4 - 9 */
      bits4To9OfMPPTYPE = (int) getBits(6, inBuffer, &numBitsGot,
         bitErrorIndication, &bibError);
      header->numBits += 6;


      /* MPPTYPE bit 4, Reference Picture Resampling mode */
      header->fRPR = ((bits4To9OfMPPTYPE & 32) > 0);

      /* MPPTYPE bit 5, Reduced-Resolution Update mode */
      header->fRRU = ((bits4To9OfMPPTYPE & 16) > 0);

      /* RPR/RRU must not be set for I or EI-pictures.
         (See section 5.1.4.5 of the H.263 recommendation.) */
      if ((header->fRPR || header->fRRU) &&
         (header->pictureType == VDX_PIC_TYPE_I || 
            header->pictureType == VDX_PIC_TYPE_EI)) {
         deb0p("ERROR. RPR or RRU is set for I or EI.\n");
         goto exitAfterBitError;
      }

      /* MPPTYPE bit 6, Rounding type */
      header->rtype = ((bits4To9OfMPPTYPE & 8) > 0);

      /* RTYPE must be 0 if other than P, Improved PB or EP picture
         (see section 5.1.4.3 of the H.263 recommendation */

      /* MPPTYPE bits 7 - 9, must be '001' */
      if ((bits4To9OfMPPTYPE & 7) != 1) {
         deb0p("ERROR. MPPTYPE bits 7 - 9 illegal.\n");
         goto exitAfterBitError;
      }

      /* CPM */
      header->cpm = (int) getBits(1, inBuffer, &numBitsGot,
         bitErrorIndication, &bibError);
      header->numBits += 1;


      if (header->cpm) {
         /* PSBI */
         header->psbi = (int) getBits(2, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 2;


      }

      if (header->fCustomSourceFormat) {
         int parCode, fExtendedPAR = 0, pwi, phi;

         /* CPFMT bits 1 - 4, Pixel Aspect Ratio code */
         parCode = (int) getBits(4, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 4;



         switch (parCode) {

            case 1: /* Square */
               header->parWidth = 1;
               header->parHeight = 1;
               break;

            case 2: /* CIF for 4:3 picture */
               header->parWidth = 12;
               header->parHeight = 11;
               break;

            case 3: /* 525-type for 4:3 picture */
               header->parWidth = 10;
               header->parHeight = 11;
               break;

            case 4: /* CIF stretched for 16:9 picture */
               header->parWidth = 16;
               header->parHeight = 11;
               break;

            case 5: /* 525-type stretched for 16:9 picture */
               header->parWidth = 40;
               header->parHeight = 33;
               break;

            case 15: /* extended PAR */
               fExtendedPAR = 1;
               break;

            default:
               deb0p("ERROR. PAR code illegal.\n");
               goto exitAfterBitError;
         }

         /* CPFMT bits 5 - 13, picture width indication */
         pwi = (int) getBits(9, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 9;


         header->lumWidth = ((pwi + 1) <<2 /** 4*/);

         /* CPFMT bit 14 must be 1 */
         tmp = getBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         header->numBits += 1;


         if ( tmp == 0 ) {
            deb0p("ERROR. CPFMT bit 14 is 0.\n");
            goto exitAfterBitError;
         }

         /* CPFMT bits 15 - 23, picture height indication */
         phi = (int) getBits(9, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 9;


         if (phi == 0 || phi > 288) {
            deb0p("ERROR. PHI illegal.\n");
            goto exitAfterBitError;
         }
         header->lumHeight = (phi <<2 /** 4*/);

         if (fExtendedPAR) {
            /* EPAR bits 1 - 8, PAR Width */
            header->parWidth = (int) getBits(8, inBuffer, &numBitsGot,
               bitErrorIndication, &bibError);
            header->numBits += 8;


            if (header->parWidth == 0) {
               deb0p("ERROR. PAR width illegal.\n");
               goto exitAfterBitError;
            }

            /* EPAR bits 9 - 16, PAR Height */
            header->parHeight = (int) getBits(8, inBuffer, &numBitsGot,
               bitErrorIndication, &bibError);
            header->numBits += 8;


            if (header->parHeight == 0) {
               deb0p("ERROR. PAR height illegal.\n");
               goto exitAfterBitError;
            }
         }
      } /* endif (customSourceFormat) */

      if (header->fCustomPCF) {
         int clockConversionCode;
         u_int32 clockDivisor, conversionFactor;

         /* CPCFC bit 1, Clock conversion code */
         clockConversionCode = (int) getBits(1, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 1;


         if (clockConversionCode)
            conversionFactor = 1001;
         else
            conversionFactor = 1000;

         /* CPCFC bits 2 - 8, Clock divisor */
         clockDivisor = getBits(7, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 7;


         if (clockDivisor == 0) {
            deb0p("ERROR. Illegal clock divisor.\n");
            goto exitAfterBitError;
         }
 
         header->pcfRate = 1800000LU;
         header->pcfScale = clockDivisor * conversionFactor;
      }

      else {
         /* CIF clock frequency */
         header->pcfRate = 2997;
         header->pcfScale = 100;
      }

      if (header->fCustomPCF || (!header->ufep && inpParam->fCustomPCF)) {
         int etr;

         /* ETR */
         etr = (int) getBits(2, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 2;



         header->tr |= (etr << 8);
      }

      if (header->fUMV) {
         /* UUI */
         tmp = getBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
         header->numBits += 1;


         if ( tmp )
            header->fUMVLimited = 1;

         else {
            tmp = getBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
            header->numBits += 1;


            if ( tmp )
               header->fUMVLimited = 0;
            else {
               /* '00' forbidden */
               deb0p("ERROR. Illegal UUI.\n");
               goto exitAfterBitError;
            }
         }


      }

      if (header->fSS) {
         /* SSS */
         tmp = (int) getBits(1, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);

         header->fRectangularSlices = tmp;
         header->numBits += 1;

         header->fArbitrarySliceOrdering = (int) getBits(1, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 1;


         /* Mode interaction restriction, see section 5.1.4.6 of 
            the H.263 recommendation */
         if (header->fISD && !header->fRectangularSlices) {
            deb0p("ERROR. Illegal bit pattern (section 5.1.4.6).\n");
            goto exitAfterBitError;
         }
      }

      if (inpParam->fScalabilityMode) {
         /* ELNUM */
         header->elnum = (int) getBits(4, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 4;

      }

      if (inpParam->fScalabilityMode && header->ufep) {
         /* RLNUM */
         header->rlnum = (int) getBits(4, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 4;

      }

      if (header->fRPS) {
         /* RPSMF */
         header->rpsMode = (int) getBits(3, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 3;


         if (header->rpsMode < 4) {
            deb0p("ERROR. Illegal RPSMF.\n");
            goto exitAfterBitError;
         }

         header->rpsMode -= 4; /* 4..7 --> 0..3 */
      }

      /* If no OPPTYPE but RPS was previously on or RPS signaled in OPPTYPE */
      if ((inpParam->fRPS && header->ufep == 0) || header->fRPS) {
         /* TRPI */
         header->trpi = (int) getBits(1, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 1;


         if (header->trpi) {
            /* TRP */
            header->trp = (int) getBits(10, inBuffer, &numBitsGot,
               bitErrorIndication, &bibError);
            header->numBits += 10;


            deb2f(bcmDebFile, "TRPI in picture header. TR = %d, TRP = %d.\n", 
               header->tr, header->trp);            
         }

            /* Code following the standard */

            /* BCI */
            tmp = getBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
            header->numBits += 1;


            if ( tmp ) {
               /* BCM not supported */
               deb0p("ERROR. BCM not supported.\n");
               goto exitAfterBitError;
            }

            else {
               tmp = getBits(1, inBuffer, &numBitsGot,bitErrorIndication, &bibError);
               header->numBits += 1;


               if ( !tmp ) {
                  /* BCI '00' is illegal */
                  deb0p("ERROR. Illegal BCI.\n");
                  goto exitAfterBitError;
               }
            }

      }

      if (header->fRPR) {
         /* RPRP not supported */
         deb0p("ERROR. RPRP not supported.\n");
         goto exitAfterBitError;
      }
   }

   else {
      deb0p("ERROR. Source format illegal.\n");
      goto exitAfterBitError;
   }

   /* PQUANT */
   header->pquantPosition = header->numBits;
   header->pquant = (int) getBits(5, inBuffer, &numBitsGot,
      bitErrorIndication, &bibError);
   header->numBits += 5;


   if (header->pquant == 0) {
      deb0p("ERROR. PQUANT illegal.\n");
      goto exitAfterBitError;
   }

   if (!header->fPLUSPTYPE) {
      /* CPM */
      header->cpm = (int) getBits(1, inBuffer, &numBitsGot,
         bitErrorIndication, &bibError);
      header->numBits += 1;


      if (header->cpm) {
         /* PSBI */
         header->psbi = (int) getBits(2, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError); 
         header->numBits += 2;


      }
   }

   /* If PB-frame */
   if (header->pictureType == VDX_PIC_TYPE_PB || 
      header->pictureType == VDX_PIC_TYPE_IPB) {

      /* TRB */

      /* If custom picture clock frequence is used */
      if (header->fCustomPCF || inpParam->fCustomPCF) {
         header->trb = (int) getBits(5, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 5;


      }

      else {
         header->trb = (int) getBits(3, inBuffer, &numBitsGot,
            bitErrorIndication, &bibError);
         header->numBits += 3;


      }

      if (header->trb == 0) {
         deb0p("ERROR. TRB illegal.\n");
         goto exitAfterBitError;
      }

      /* DBQUANT */
      header->dbquant = (int) getBits(2, inBuffer, &numBitsGot,
         bitErrorIndication, &bibError);
      header->numBits += 2;


   }


   /* Check success and return */



   /* If no error in bit buffer functions */
   if (!bibError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;

   exitAfterBitError:
      if (bibError && bibError != ERR_BIB_NOT_ENOUGH_DATA)
         return VDX_ERR_BIB;
      return VDX_OK_BUT_BIT_ERROR;
}


/* {{-output"vdxFlushSEI.txt"}} */
/*
 * vdxFlushSEI
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function discards (flushes) all consequent PEI/PSUPP pairs.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxFlushSEI(
   bibBuffer_t *inBuffer,
   int *bitErrorIndication )
/* {{-output"vdxFlushSEI.txt"}} */
{
   int
      numBitsGot;
   int16
      bibError = 0;
   u_int32 
      pei,
      psupp;

   vdxAssert(inBuffer != NULL);
   vdxAssert(bitErrorIndication != NULL);

   do {
      /* PEI */
      pei = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


      if (pei) {
         /* PSUPP */
         psupp = bibGetBits(8, inBuffer, &numBitsGot, bitErrorIndication, 
            &bibError);
         // ignore the value of psupp; this is flush-function
         if ( psupp )
            {
            }


      }
   } while (pei);   

   return VDX_OK;

}


/* {{-output"vdxGetSEI.txt"}} */
/*
 * vdxGetSEI
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *
 *    ftype                      FTYPE field as defined in table L.1 of H.263,
 *                               -1 if the value is not valid
 *
 *    dsize                      DSIZE as defined in section L.2 of H.263
 *
 *    parameterData              an array of (min) 16 entries,
 *                               filled with dsize octets of parameter data
 *
 *    fLast                      set to 1 if the first PEI indicates that
 *                               no PSUPPs follow. Otherwise 0.
 *
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function gets supplemental enhancement information as defined
 *    in Annex L of H.263.
 *
 * Note:
 *    The start code emulation prevention necessity using "Do Nothing" function
 *    is not checked. See section L.3 of H.263 for more details.
 *    
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxGetSEI(
   bibBuffer_t *inBuffer,
   int *ftype,
   int *dsize,
   u_char *parameterData,
   int *fLast,
   int *bitErrorIndication)
/* {{-output"vdxGetSEI.txt"}} */
{
   int
      numBitsGot,
      paramNum,
      lftype, /* local FTYPE */
      ldsize; /* local DSZIE */
   int16
      bibError = 0;
   u_int32 
      pei;

   vdxAssert(inBuffer != NULL);
   vdxAssert(ftype != NULL);
   vdxAssert(dsize != NULL);
   vdxAssert(parameterData != NULL);
   vdxAssert(fLast != NULL);
   vdxAssert(bitErrorIndication != NULL);

   *ftype = -1;
   *dsize = 0;   

   /* PEI */
   pei = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


   if (pei) {
      *fLast = 0;

      /* FTYPE */
      lftype = (int) bibGetBits(4, inBuffer, &numBitsGot, bitErrorIndication, 
         &bibError);


      /* DSIZE */
      ldsize = (int) bibGetBits(4, inBuffer, &numBitsGot, bitErrorIndication, 
         &bibError);

   }

   else {
      *fLast = 1;
      return VDX_OK;
   }

   for (paramNum = 0; paramNum < ldsize; paramNum++) {
      /* PEI */
      pei = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


      if (!pei) {
         deb0p("ERROR. DSIZE does not match with PEI.\n");
         *fLast = 1;
         return VDX_OK_BUT_BIT_ERROR;
      }

      /* PSUPP containing parameter data */
      parameterData[paramNum] = (u_char) bibGetBits(8, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);

   }

   *ftype = lftype;
   *dsize = ldsize;
     
   return VDX_OK;

}


/* {{-output"vdxGetAndParseSEI.txt"}} */
/*
 * vdxGetAndParseSEI
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *
 *    fPLUSPTYPE                 indicates if PLUSPTYPE is in use
 *
 *    numScalabilityLayers       -1 if the very first picture,
 *                               0 if Annex N scalability layers not in use,
 *                               2..15 if Annex N scalability layers in use
 *
 *    sei                        parsed SEI data
 *
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function gets supplemental enhancement information as defined
 *    in Annex L and W of H.263 and returns the parsed data.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 */

int vdxGetAndParseSEI(
   bibBuffer_t *inBuffer,
   int fPLUSPTYPE,
   int numScalabilityLayers,
   vdxSEI_t *sei,
   int *bitErrorIndication)
/* {{-output"vdxGetAndParseSEI.txt"}} */
{
   int 
       ret = VDX_OK,              /* Temporary variable to carry return values */
      fSecondFTYPESet = 0,       /* 1 = Extended FTYPE in use, 
                                    0 = Annex L/W FTYPE in use,
                                    see L.15 for further details */
      ftype,                     /* FTYPE as in section L.2 of H.263 */
      dsize,                     /* DSIZE as in section L.2 of H.263 */
      fLast,                     /* 1 if PEI is zero, 0 otherwise */
      cont = 0,                  /* CONT as in section W.6 of H.263 */ 
      ebit,                      /* EBIT as in section W.6 of H.263 */
      mtype,                     /* MTYPE as in section W.6 of H.263 */
      prevCONT = 0,              /* CONT field in the previous picture msg */
      prevMTYPE = -1;            /* MTYPE in the previous picture msg */
      
   u_char
      parameterData[16];

   /* Initialize output data */
   sei->fFullPictureFreezeRequest = 0;
   sei->fFullPictureSnapshot = 0;
   sei->snapshotId = 0;
   sei->snapshotStatus = 255;
   sei->scalabilityLayer = -1;
   sei->numScalabilityLayers = 0;
   memset(sei->prevPicHeader, 0, VDX_MAX_BYTES_IN_PIC_HEADER);
   sei->numBytesInPrevPicHeader = 0;
   sei->numBitsInLastByteOfPrevPicHeader = 0;
   sei->fPrevPicHeaderTooLarge = 0;

   do {
      /* Get supplemental enhancement information */
      ret = vdxGetSEI(inBuffer, &ftype, &dsize, parameterData, &fLast, 
         bitErrorIndication);

      /* If fatal error or bit error */
      if (ret != VDX_OK)
         return ret;

      /* If the previous FTYPE indicated Extended Function Type */
      if (fSecondFTYPESet) {

         /* Let's discard FTYPE/DSIZE and parameters as suggested in 
            section L.15 of H.263 Recommendation to allow backward
            compatibility to to-be-defined set of extended FTYPEs. */
            
         /* The next expected FTYPE is a normal one defined in Annex L/W. */
         fSecondFTYPESet = 0;

         continue;
      }

      switch (ftype) {

         /* If "Reserved" */
         case 0:
            ret = vdxActAfterIncorrectSEI(
               inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            break;

         /* If "Do Nothing" */
         case 1:

            if (dsize != 0) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Full-Picture Freeze Request */
         case 2:
            if (dsize != 0) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            else
               sei->fFullPictureFreezeRequest = 1;

            break;

         /* If Partial-Picture Freeze Request */
         case 3:
            if (dsize != 4) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Resizing Partial-Picture Freeze Request */
         case 4:
            if (dsize != 8) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Partial-Picture Freeze-Release Request */
         case 5:
            if (dsize != 4) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Full-Picture Snapshot Tag */
         case 6:
            if (dsize != 4) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            else {
               int i;
               sei->fFullPictureSnapshot = 1;
               /* store 32-bit snapshot ID, first byte is the least significant */
               for (i = 0; i < 4; i++)
                  sei->snapshotId |= (parameterData[i] << (i<<3 /**8*/));
            }

            break;

         /* If Partial-Picture Snapshot Tag */
         case 7:
            if (dsize != 8) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Video Time Segment Start Tag */
         case 8:
            if (dsize != 4) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Video Time Segment End Tag */
         case 9:
            if (dsize != 4) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Progressive Refinement Segment Start Tag */
         case 10:
            if (dsize != 4) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Progressive Refinement Segment End Tag */
         case 11:
            if (dsize != 4) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Chroma Keying Information */
         case 12:
            if (dsize < 1 || dsize > 9) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Fixed-Point IDCT */
         case 13:
            if (dsize != 1) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            break;

         /* If Picture Message */
         case 14:
            /* DSIZE shall be at least 1 to carry CONT, EBIT, and MTYPE */
            if (dsize < 1) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            else {
               cont = ( (parameterData[0] & 0x80) >> 7 ); 
               ebit = ( (parameterData[0] & 0x70) >> 4 ); 
               mtype = (parameterData[0] & 0x0f);

               if (mtype < 1 || mtype > 5) {
                  /* Non-text message, check restriction in W.6.2 */
                  if (ebit != 0 && (cont == 1 || dsize == 1)) {
                     ret = vdxActAfterIncorrectSEI(
                        inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
                     break;
                  }
               }

               /* If the previous picture message indicated that the data
                  continues in the next picture message, but the current
                  message type differs from the previous one
                  (restricted in section W.6.1 of the H.263 Recommendation */
               if (prevCONT && mtype != prevMTYPE) {
                  ret = vdxActAfterIncorrectSEI(
                     inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
                  break;
               }
            
               /* If arbitrary binary data */
               if (mtype == 0) {

                  /* Proprietary snapshot status indication */
                  if (parameterData[1] == 83 && parameterData[2] == 115) {
                     if (dsize != 4) {
                        ret = vdxActAfterIncorrectSEI(
                           inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
                        break;
                     }
                     sei->snapshotStatus = parameterData[3];
                  }

                  /* Proprietary Annex N scalability layer 
                     indication */
                  else if (parameterData[1] == 83 && parameterData[2] == 108) {
                     if (dsize != 4) {
                        ret = vdxActAfterIncorrectSEI(
                           inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
                        break;
                     }
                     sei->scalabilityLayer = parameterData[3] >> 4;
                     sei->numScalabilityLayers = parameterData[3] & 15;

                     /* If less than two scalability layers or
                        max number of scalability layers changes during
                        the sequence */
                     if (sei->numScalabilityLayers < 2 ||
                        (numScalabilityLayers >= 0 &&
                           sei->numScalabilityLayers != 
                           numScalabilityLayers)) {

                        ret = vdxActAfterIncorrectSEI(
                           inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
                        break;
                     } 
                  }
               }

               /* Else if Previous Picture Header Repetition */
               else if (mtype == 7) {
                  int
                     firstIndexToWrite,
                     numBytesToWrite;
                  
                  if (sei->numBytesInPrevPicHeader == 0)
                     /* The first two bytes of PSC = 0x00 0x00 */
                     sei->numBytesInPrevPicHeader = 2;
                     
                  firstIndexToWrite = sei->numBytesInPrevPicHeader;
                  numBytesToWrite = dsize - 1;
                  sei->numBitsInLastByteOfPrevPicHeader = 8 - ebit;

                  /* If buffer would overflow */
                  if (firstIndexToWrite + numBytesToWrite > 
                     VDX_MAX_BYTES_IN_PIC_HEADER) {
                     numBytesToWrite = 
                        VDX_MAX_BYTES_IN_PIC_HEADER - firstIndexToWrite;
                     sei->numBitsInLastByteOfPrevPicHeader = 8;
                     sei->fPrevPicHeaderTooLarge = 1;
                  }

                  if (numBytesToWrite) {
                     memcpy(
                        &sei->prevPicHeader[firstIndexToWrite], 
                        &parameterData[1],
                        numBytesToWrite);

                     sei->numBytesInPrevPicHeader += numBytesToWrite;
                  }
               }

               prevCONT = cont;
               prevMTYPE = mtype;
            }
            break;

         /* If Extended Function Type */
         case 15:
            if (dsize != 0) {
               ret = vdxActAfterIncorrectSEI(
                  inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
            }

            fSecondFTYPESet = 1;

            break;
      }
   } while (!fLast && ret == VDX_OK);

   /* If a picture message was not completed and the fault has not been
      tracked earlier */
   if (prevCONT && ret == VDX_OK) {
      ret = vdxActAfterIncorrectSEI(
         inBuffer, fPLUSPTYPE, &fLast, bitErrorIndication);
   }

   return ret;
}


/*
 * GOB Layer Global Functions
 */

/* {{-output"vdxGetGOBHeader.txt"}} */
/*
 * vdxGetGOBHeader
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    inpParam                   input parameters
 *    header                     output parameters: GOB header
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 * Function:
 *    This function reads the H.263 GOB header.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxGetGOBHeader(
   bibBuffer_t *inBuffer, 
   const vdxGetGOBHeaderInputParam_t *inpParam,
   vdxGOBHeader_t *header,
   int *bitErrorIndication,
   int aColorEffect,
   int* aStartByteIndex,
   int* aStartBitIndex, CMPEG4Transcoder *hTranscoder)
/* {{-output"vdxGetGOBHeader.txt"}} */
{
   int
      numBitsGot;
   int16
      bibError = 0;
   u_int32 
      tmp = 0;          /* temporary variable for reading some redundant bits */

   vdxAssert(inBuffer != NULL);
   vdxAssert(inpParam != NULL);
   vdxAssert(header != NULL);
   vdxAssert(bitErrorIndication != NULL);

   memset(header, 0, sizeof(vdxGOBHeader_t));
   *bitErrorIndication = 0;

   /* Assume that the existence of a GBSC has been checked, and
      it is the next code in the buffer */

   /* Flush stuffing (GSTUF) */
   if (inpParam->numStuffBits)
   {
      bibFlushBits(inpParam->numStuffBits, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
      // if chroma has been removed and GSTUF is there, skip GSTUF bits to point to byte-aligned GBSC
      if(aColorEffect==1 || aColorEffect==2)
      {
         (*aStartByteIndex)++;
         *aStartBitIndex = 7;
      }
   }

   /* MVE */
     hTranscoder->H263GOBSliceHeaderBegin();

    /* Flush GBSC */
   bibFlushBits(17, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


   /* GBSC cannot contain fatal bit errors (checked earlier) */

   /* GN */
   header->gn = (int) bibGetBits(5, inBuffer, &numBitsGot, 
      bitErrorIndication, &bibError);


   /* If the GN field contains a fatal bit error or
      the value of GN is not in the valid range 1..24.
      This range is defined in section 5.2.3 of the standard.

      Notice that in general case one cannot assume that the maximum GN
      of the picture is the same as in the previous picture (GOB) since
      a picture header may have been lost and the header could have contained
      an indication of a picture format change. Therefore we have to stick
      to the overall maximum GN, equal to 24, here. */

   if ( header->gn == 0 /* PSC */ || 
      header->gn > 24) {
      deb0p("ERROR. GN illegal.\n");
      goto exitAfterBitError;
   }

   if (inpParam->fCPM) {
      /* GSBI */
      header->gsbi = (int) bibGetBits(2, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


   }
   /* GFID */
   header->gfid = (int) bibGetBits(2, inBuffer, &numBitsGot, 
      bitErrorIndication, &bibError);


   /* GQUANT */
   header->gquant = (int) bibGetBits(5, inBuffer, &numBitsGot, 
      bitErrorIndication, &bibError);


   if (header->gquant == 0) {
      deb0p("ERROR. Illegal GQUANT.\n");
      goto exitAfterBitError;
   }

   if (inpParam->fRPS) {
      /* TRI */
      header->tri = (int) bibGetBits(1, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


      /* If TR present */
      if (header->tri) {
         /* TR */
         if (inpParam->fCustomPCF)
            header->tr = (int) bibGetBits(10, inBuffer, &numBitsGot, 
               bitErrorIndication, &bibError);
         else
            header->tr = (int) bibGetBits(8, inBuffer, &numBitsGot, 
               bitErrorIndication, &bibError);


      }

      /* TRPI */
      header->trpi = (int) bibGetBits(1, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


      /* If TRP present */
      if (header->trpi) {
         /* TRP */
         header->trp = (int) bibGetBits(10, inBuffer, &numBitsGot, 
            bitErrorIndication, &bibError);


         deb2f(bcmDebFile, "TRPI in GOB header. GN = %d, TRP = %d.\n", 
            header->gn, header->trp);         
      }

      /* BCI */
         /* Code following the standard */

         /* BCI */
         tmp = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


         if ( tmp ) {
            /* BCM not supported */
            deb0p("ERROR. BCM not supported.\n");
            goto exitAfterBitError;
         }

         else {
            tmp = bibGetBits(1, inBuffer, &numBitsGot,bitErrorIndication, &bibError);


            if ( !tmp ) {
               /* BCI '00' is illegal */
               deb0p("ERROR. Illegal BCI.\n");
               goto exitAfterBitError;
            }
         }


   }

   /* Check success and return */


   /* If no error in bit buffer functions */
   if (!bibError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;

   exitAfterBitError:
      if (bibError && bibError != ERR_BIB_NOT_ENOUGH_DATA)
         return VDX_ERR_BIB;

      return VDX_OK_BUT_BIT_ERROR;
}

/*
 * Slice Layer Global Functions
 */

/* {{-output"vdxGetSliceHeader.txt"}} */
/*
 * vdxGetSliceHeader
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    inpParam                   input parameters
 *    header                     output parameters: Slice header
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 * Function:
 *    This function reads the H.263 Slice header.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 */

int vdxGetSliceHeader(
   bibBuffer_t *inBuffer, 
   const vdxGetSliceHeaderInputParam_t *inpParam,
   vdxSliceHeader_t *header,
   int *bitErrorIndication)
/* {{-output"vdxGetSliceHeader.txt"}} */
{
   int
      numBitsGot;
   int16
      bibError = 0;
   u_int32 
      tmp = 0;          /* temporary variable for reading some redundant bits */


   vdxAssert(inBuffer != NULL);
   vdxAssert(inpParam != NULL);
   vdxAssert(header != NULL);
   vdxAssert(bitErrorIndication != NULL);

   memset(header, 0, sizeof(vdxSliceHeader_t));
   *bitErrorIndication = 0;

   if (!inpParam->sliceHeaderAfterPSC)  {
      /* Flush stuffing (GSTUF) */
      if (inpParam->numStuffBits)
         bibFlushBits(inpParam->numStuffBits, inBuffer, &numBitsGot, bitErrorIndication, &bibError);

      /* Flush SSC */
      /* Assume that the existence of a SSC has been checked, and
         it is the next code in the buffer */

      bibFlushBits(17, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


      /* SSC cannot contain fatal bit errors (checked earlier) */
   }

   /* Read SEPB1 (K.2.3) */
   tmp = (int) bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                           &bibError);
   

   if (tmp != 1)  {
      deb0p("ERROR. SEPB1 illegal.\n");
      goto exitAfterBitError;
   }

   /* SSBI */
   if ((inpParam->fCPM) && (!inpParam->sliceHeaderAfterPSC))  {
      header->ssbi = (int) bibGetBits(4, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


      /* If the SSBI field contains a fatal bit error or
      the value of SSBI is not in the valid range 9..12.
      This range is defined in Table K.1/H.263 */
      if ( tmp < 9  || 
         tmp > 12) {
         deb0p("ERROR. SSBI illegal.\n");
         goto exitAfterBitError;
      }
      /* Set emulated GN value */
      header->gn = header->ssbi + 16;
      /* Set sub-bitstream number */
      if (header->gn < 29) 
         header->sbn = header->ssbi - 9;
      else
         header->sbn = 3;
   }


   /* MBA */
   header->mba = (int) bibGetBits(inpParam->mbaFieldWidth, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


   /* If the MBA field contains a fatal bit error or
      the value of MBA is larger than Max Value defined in Table K.2/H.263 */
   if ( header->mba > inpParam->mbaMaxValue) {
      deb0p("ERROR. MBA illegal.\n");
      goto exitAfterBitError;
   }

   /* SEPB2 */
   if (inpParam->sliceHeaderAfterPSC)  {
      if (inpParam->fRectangularSlices)   {
         tmp = (int) bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                              &bibError);


         if (tmp != 1)  {
            deb0p("ERROR. SEPB1 illegal.\n");
            goto exitAfterBitError;
         }
      }
   }
   else  {
      if (inpParam->fCPM)  {
         if (inpParam->mbaFieldWidth > 9) {
            tmp = (int) bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                              &bibError);


            if (tmp != 1)  {
               deb0p("ERROR. SEPB1 illegal.\n");
               goto exitAfterBitError;
            }
         }
      }
      else  {
         if (inpParam->mbaFieldWidth > 11) {
            tmp = (int) bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                              &bibError);


            if (tmp != 1)  {
               deb0p("ERROR. SEPB1 illegal.\n");
               goto exitAfterBitError;
            }
         }
      }
   }

   /* SQUANT */
   if (!inpParam->sliceHeaderAfterPSC)  {
      header->squant = (int) bibGetBits(5, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);
      if ( bibError )
            return VDX_ERR_BIB;

      /* If the SQUANT field contains a fatal bit error or
         the value of SQUANT is between 1 and 31 */
      if ( header->squant == 0) {
         deb0p("ERROR. SQUANT illegal.\n");
         goto exitAfterBitError;
      }
   }

   /* SWI */
   if (inpParam->fRectangularSlices)   {
      header->swi = (int) bibGetBits(inpParam->swiFieldWidth, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


      /* If the MBA field contains a fatal bit error or
         the value of MBA is larger than Max Value defined in Table K.2/H.263 */
      if ( header->swi > inpParam->swiMaxValue) {
         deb0p("ERROR. SWI illegal.\n");
         goto exitAfterBitError;
      }
   }

   /* Read SEPB3 */
   tmp = (int) bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                           &bibError);


   if (tmp != 1)  {
      deb0p("ERROR. SEPB3 illegal.\n");
      goto exitAfterBitError;
   }

   /* GFID */
   if (!inpParam->sliceHeaderAfterPSC)  {
      header->gfid = (int) bibGetBits(2, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


   }

   if (!inpParam->sliceHeaderAfterPSC && inpParam->fRPS) {
      /* TRI */
      header->tri = (int) bibGetBits(1, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


      /* If TR present */
      if (header->tri) {
         /* TR */
/*         if (inpParam->fCustomPCF)
            header->tr = (int) bibGetBits(10, inBuffer, &numBitsGot, 
               bitErrorIndication, &bibError);
         else*/
            header->tr = (int) bibGetBits(8, inBuffer, &numBitsGot, 
               bitErrorIndication, &bibError);


      }

      /* TRPI */
      header->trpi = (int) bibGetBits(1, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


      /* If TRP present */
      if (header->trpi) {
         /* TRP */
         header->trp = (int) bibGetBits(10, inBuffer, &numBitsGot, 
            bitErrorIndication, &bibError);


         deb2f(bcmDebFile, "TRPI in GOB header. GN = %d, TRP = %d.\n", 
            header->gn, header->trp);         
      }

      /* BCI */
         /* Code following the standard */

         /* BCI */
         tmp = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


         if ( tmp ) {
            /* BCM not supported */
            deb0p("ERROR. BCM not supported.\n");
            goto exitAfterBitError;
         }

         else {
            tmp = bibGetBits(1, inBuffer, &numBitsGot,bitErrorIndication, &bibError);


            if ( !tmp ) {
               /* BCI '00' is illegal */
               deb0p("ERROR. Illegal BCI.\n");
               goto exitAfterBitError;
            }
         }


   }

   /* Check success and return */


   /* If no error in bit buffer functions */
   if (!bibError)
      return VDX_OK;

   /* Else if ran out of data (i.e. decoding out of sync) */
   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else other error in bit buffer functions */
   else
      return VDX_ERR_BIB;
   exitAfterBitError:
      if (bibError && bibError != ERR_BIB_NOT_ENOUGH_DATA)
         return VDX_ERR_BIB;

      return VDX_OK_BUT_BIT_ERROR;
}
/* {{-output"vdxGetMBAandSWIValues.txt"}} */
/*
 * vdxGetIMBLayer
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    inpParam                   input parameters
 *    outParam                   output parameters
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 * Function:
 *    This function reads the macroblock layer data of an INTRA macroblock.
 *
 * Returns:
 *    Nothing
 *
 */

void vdxGetMBAandSWIValues(
   int width,
   int height,
   int fRRU,
   int *mbaFieldWidth,
   int *mbaMaxValue,
   int *swiFieldWidth,
   int *swiMaxValue
   )
{
   int 
      pictureFormat;

   pictureFormat = vdxFrameSizeToPictureFormat(width, height);
 
   if (fRRU)  {
      switch (pictureFormat)  {
      case 0:
         /* sub-QCIF */
         *mbaFieldWidth = 5;
         *mbaMaxValue = 11;
         *swiFieldWidth = 3;
         *swiMaxValue = 3;
         break;
      case 1:
         /* QCIF */
         *mbaFieldWidth = 6;
         *mbaMaxValue = 29;
         *swiFieldWidth = 3;
         *swiMaxValue = 5;
         break;
      case 2:
         /* CIF */
         *mbaFieldWidth = 7;
         *mbaMaxValue = 98;
         *swiFieldWidth = 4;
         *swiMaxValue = 10;
         break;
      case 3:
         /* 4CIF */
         *mbaFieldWidth = 9;
         *mbaMaxValue = 395;
         *swiFieldWidth = 5;
         *swiMaxValue = 21;
         break;
      case 4:
         /* 16CIF */
         *mbaFieldWidth = 11;
         *mbaMaxValue = 1583;
         *swiFieldWidth = 6;
         *swiMaxValue = 43;
         break;
      case 5:
         /* 2048x1152 */
         *mbaFieldWidth = 12;
         *mbaMaxValue = 2303;
         *swiFieldWidth = 6;
         *swiMaxValue = 63;
         break;
      }
   }
   else  {
      switch (pictureFormat)  {
      case 0:
         /* sub-QCIF */
         *mbaFieldWidth = 6;
         *mbaMaxValue = 47;
         *swiFieldWidth = 4;
         *swiMaxValue = 7;
         break;
      case 1:
         /* QCIF */
         *mbaFieldWidth = 7;
         *mbaMaxValue = 98;
         *swiFieldWidth = 4;
         *swiMaxValue = 10;
         break;
      case 2:
         /* CIF */
         *mbaFieldWidth = 9;
         *mbaMaxValue = 395;
         *swiFieldWidth = 5;
         *swiMaxValue = 21;
         break;
      case 3:
         /* 4CIF */
         *mbaFieldWidth = 11;
         *mbaMaxValue = 1583;
         *swiFieldWidth = 6;
         *swiMaxValue = 43;
         break;
      case 4:
         /* 16CIF */
         *mbaFieldWidth = 13;
         *mbaMaxValue = 6335;
         *swiFieldWidth = 7;
         *swiMaxValue = 87;
         break;
      case 5:
         /* 2048x1152 */
         *mbaFieldWidth = 14;
         *mbaMaxValue = 9215;
         *swiFieldWidth = 7;
         *swiMaxValue = 127;
         break;
      }
   }
}

/*
 * Macroblock Layer Global Functions
 */

/* {{-output"vdxGetIMBLayer.txt"}} */
/*
 * vdxGetIMBLayer
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    inpParam                   input parameters
 *    outParam                   output parameters
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 * Function:
 *    This function reads the macroblock layer data of an INTRA macroblock.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

   
int vdxGetIMBLayer(
   bibBuffer_t *inBuffer, 
   bibBuffer_t *outBuffer, 
   bibBufferEdit_t *bufEdit,
   int /*aColorEffect*/, 
   int * /*aStartByteIndex*/, 
   int * /*aStartBitIndex*/, 
   TBool /*aGetDecodedFrame*/,
   const vdxGetIMBLayerInputParam_t *inpParam,
   vdxIMBLayer_t *outParam,
   int *bitErrorIndication, CMPEG4Transcoder *hTranscoder)

/* {{-output"vdxGetIMBLayer.txt"}} */
{
   int
      mcbpcIndex,
      cbpyIndex,
      retValue = VDX_OK,
      fDQUANT = 0,
      fINTRAMODE,
      bitsGot = 0;
   int16 bibError = 0;

   int StartByteIndex; 
   int StartBitIndex;

   vdxAssert(inBuffer != NULL);
   vdxAssert(inpParam != NULL);
   vdxAssert(outParam != NULL);
   vdxAssert(bitErrorIndication != NULL);
     
   /* MVE */
     vdxIMBListItem_t *MBinstance;
     // Create new MBInstance for the next MB 
     MBinstance = (vdxIMBListItem_t *) malloc(sizeof(vdxIMBListItem_t));
     if (!MBinstance)
     {
         deb("ERROR - vdxGetIMBLayer::MBinstance creation failed\n");
         return H263D_ERROR;         
     }
     memset(MBinstance, 0, sizeof(vdxIMBListItem_t));
     
   /* Loop while MCBPC indicates stuffing */
   do {
     
        StartByteIndex = inBuffer->getIndex; 
        StartBitIndex  = inBuffer->bitIndex;
         
        /* MVE */
        VDT_SET_START_POSITION(MBinstance,0,inBuffer->getIndex,inBuffer->bitIndex); // 0: mcbpc
         
        retValue = vdxGetMCBPCIntra(inBuffer, &mcbpcIndex, bitErrorIndication);
        if (retValue != VDX_OK)
             goto exitFunction;
         
        /* MVE */
        /* remember to output the MB stufffing bits!! */
        if (mcbpcIndex == 8) 
        {
            /* copy data from inbuffer to outbuffer  */
            bufEdit->copyMode = CopyWhole; // copy whole
            CopyStream(inBuffer,outBuffer,bufEdit,StartByteIndex,StartBitIndex);
        }
   } while (mcbpcIndex == 8);
     
    /* MVE */
    VDT_SET_END_POSITION(MBinstance,0,inBuffer->getIndex,inBuffer->bitIndex); // 0: mcbpc
     
   /* CBPC (2 LSBs of MCBPC) */
   outParam->cbpc = mcbpcIndex & 3;

   /* DQUANT is given for MCBPC indexes 4..7 */
   fDQUANT = mcbpcIndex & 4;

   /* MB Type*/
   outParam->mbType = (mcbpcIndex <4)?3:4;
   
   /* INTRA_MODE */
   if (inpParam->fAIC)
   {
      retValue = vdxGetIntraMode(inBuffer,&fINTRAMODE,bitErrorIndication);
      if (retValue != VDX_OK)
         goto exitFunction;
      outParam->predMode = fINTRAMODE;
   }

   /* ac_pred_flag (1 bit) */
   if (inpParam->fMPEG4) {
      outParam->ac_pred_flag = (u_char) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &bibError);

   }

     /* MVE */
   VDT_SET_START_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); // 2: cbpy

   /* CBPY */
   retValue = vdxGetCBPY(inBuffer, &cbpyIndex, bitErrorIndication);
   if (retValue != VDX_OK)
      goto exitFunction;
   outParam->cbpy = cbpyIndex;

     /* MVE */
   VDT_SET_END_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); // 2: cbpy
   VDT_SET_START_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant
     
   if (fDQUANT) {
         retValue = vdxUpdateQuant(inBuffer, inpParam->fMQ, inpParam->quant,
             &outParam->quant, bitErrorIndication);
         if (retValue != VDX_OK)
             goto exitFunction;
   }
     
   /* Else no DQUANT */
   else
         outParam->quant = inpParam->quant;
   
   /* Color Toning */
   hTranscoder->AfterMBLayer(outParam->quant);
     
exitFunction:
     
     /* MVE */
   VDT_SET_END_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant
   outParam->mcbpc = mcbpcIndex;
     
   MBinstance->mcbpc = mcbpcIndex;
   MBinstance->quant = outParam->quant;
   MBinstance->cbpc  = outParam->cbpc;
   MBinstance->cbpy  = outParam->cbpy;
   MBinstance->ac_pred_flag = outParam->ac_pred_flag; 
   MBinstance->dquant       = fDQUANT ?  outParam->quant - inpParam->quant : 0;
     
   hTranscoder->OneIMBDataStarted(MBinstance);
   free(MBinstance);
     
   return retValue;
}


/* {{-output"vdxGetPPBMBLayer.txt"}} */
/*
 * vdxGetPPBMBLayer
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    inpParam                   input parameters
 *    outParam                   output parameters
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 * Function:
 *    This function reads the macroblock layer data of an INTER macroblock.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */


int vdxGetPPBMBLayer(
   bibBuffer_t *inBuffer, 
   bibBuffer_t *outBuffer, 
   bibBufferEdit_t *bufEdit,
   int /*aColorEffect*/,
   int */*aStartByteIndex*/, 
   int */*aStartBitIndex*/, 
   TBool /*aGetDecodedFrame*/, 
   int * /*bwMBType*/, 
   const vdxGetPPBMBLayerInputParam_t *inpParam,
   vdxPPBMBLayer_t *outParam,
   int *bitErrorIndication, CMPEG4Transcoder *hTranscoder)

/* {{-output"vdxGetPPBMBLayer.txt"}} */
{
   static const int mbTypeToMBClass[6] = 
      {VDX_MB_INTER, VDX_MB_INTER, VDX_MB_INTER, 
       VDX_MB_INTRA, VDX_MB_INTRA, VDX_MB_INTER};

   static const int mbTypeToDQUANTI[6] =
      {0, 1, 0, 0, 1, 1};

   int
      numBitsGot,
      retValue = VDX_OK,
      mcbpc,
      mbType,
      mbClass,
      fDQUANT = 0,
      fMVD,
      numMVs,
      fCBPB,
      fMVDB,
      cbpyIndex,
      fINTRAMODE;
   int16
      bibError = 0;
   u_int32 
      bits;

   int StartByteIndex; 
   int StartBitIndex;

    vdxAssert(inBuffer != NULL);
    vdxAssert(inpParam != NULL);
    vdxAssert(inpParam->pictureType == VDX_PIC_TYPE_P ||
        inpParam->pictureType == VDX_PIC_TYPE_PB||
        inpParam->pictureType == VDX_PIC_TYPE_IPB);
    vdxAssert(outParam != NULL);
    vdxAssert(bitErrorIndication != NULL);
     
    /* MVE */
//    int startByteIndex = inBuffer->getIndex;
//    int startBitIndex  = inBuffer->bitIndex;
     
    vdxPMBListItem_t *MBinstance;
     // Create new MBInstance for the next MB 
     MBinstance = (vdxPMBListItem_t *) malloc(sizeof(vdxPMBListItem_t));
     if (!MBinstance)
     {
         deb("ERROR - vdxGetPMBLayer::MBinstance creation failed\n");
         goto exitFunction;
     }
     memset(MBinstance, 0, sizeof(vdxPMBListItem_t));
     VDT_SET_START_POSITION(MBinstance,11,0,7); // 11: MB stuffing bits
     VDT_SET_END_POSITION(MBinstance,11,0,7); // 11: MB stuffing bits
     
   /* Set default output values */
   memset(outParam, 0, sizeof(vdxPPBMBLayer_t));
   outParam->quant = inpParam->quant;
     
   /* Loop while MCBPC indicates stuffing */
   do {
         
         // note: stufffing includes COD+MCBPC in P frames
         StartByteIndex = inBuffer->getIndex; 
         StartBitIndex  = inBuffer->bitIndex;
         
         /* COD */
         bits = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
             &bibError);
         
         
         outParam->fCodedMB = (int) bits ^ 1;
         
         /* If not coded (i.e. if COD == 1) */
         if (bits)
             goto exitFunction;
         
         /* Else coded (i.e. if COD == 0) */
         else {
            /* MCBPC */

            /* MVE */
            VDT_SET_START_POSITION(MBinstance,0,inBuffer->getIndex,inBuffer->bitIndex); // 0: mcbpc

            retValue = vdxGetMCBPCInter(
                 inBuffer, 
                 inpParam->fPLUSPTYPE,
                 inpParam->fAP || inpParam->fDF,
                 inpParam->fFirstMBOfPicture,
                 &mcbpc, 
                 bitErrorIndication);

            if (retValue != VDX_OK)
                 goto exitFunction;

            /* See section 5.3.2 of the H.263 recommendation to find out 
            the details of this illegal codeword check. */
            if (mcbpc == 20 &&
                 inpParam->pictureType == VDX_PIC_TYPE_IPB &&
                 inpParam->fCustomSourceFormat &&
                 inpParam->fFirstMBOfPicture) 
            {
                 deb0p("ERROR. Illegal MCBPC stuffing.\n");
                 retValue = VDX_OK_BUT_BIT_ERROR;
                 goto exitFunction;
            }

            /* MVE */
            /* remember to output the MB stufffing bits!! */
            if (mcbpc == 20) 
            {
                 // copy data from inbuffer to outbuffer 
                 bufEdit->copyMode = CopyWhole; // copy whole
                 CopyStream(inBuffer,outBuffer,bufEdit,StartByteIndex,StartBitIndex);
            }
         }
   } while (mcbpc == 20);

   /* Decrease indexes > 20 to enable consistent MCBPC handling */
   if (mcbpc > 20)
         mcbpc--;
     
    /* MVE */
    VDT_SET_END_POSITION(MBinstance,0,inBuffer->getIndex,inBuffer->bitIndex); // 0: mcbpc
   
   /* CBPC (2 LSBs of MCBPC) */
   outParam->cbpc = mcbpc & 3;
     
   /* MCBPC --> MB type & included data elements */
   mbType = outParam->mbType = mcbpc / 4;
   mbClass = outParam->mbClass = mbTypeToMBClass[mbType];
   fDQUANT = mbTypeToDQUANTI[mbType];
   /* MVD is included always for PB-frames and always if MB type is INTER */
   fMVD = inpParam->pictureType != VDX_PIC_TYPE_P || mbClass == VDX_MB_INTER;
   numMVs = outParam->numMVs = 
      (fMVD) ?
         ((mbType == 2 || mbType == 5) ? 4 : 1) :
         0;

   /* 4 MVs can be present only when in Advanced Prediction or Deblocking 
      Filter mode */
   if (numMVs == 4 && !inpParam->fAP && !inpParam->fDF) {
      deb0p("ERROR. Illegal MCBPC.\n");
      retValue = VDX_OK_BUT_BIT_ERROR;
      goto exitFunction;
   }

   /* INTRA_MODE */
   if ((inpParam->fAIC)&&((mbType == 3) || (mbType == 4)))
   {
      retValue = vdxGetIntraMode(inBuffer,&fINTRAMODE,bitErrorIndication);
      if (retValue != VDX_OK)
         goto exitFunction;
      outParam->predMode = fINTRAMODE;
   }

   if (inpParam->pictureType == VDX_PIC_TYPE_PB) {
      int modbIndex;
      /* MODB */
      retValue = vdxGetNormalMODB(inBuffer, &modbIndex, bitErrorIndication);
      if (retValue != VDX_OK)
         goto exitFunction;
      fCBPB = (int) (modbIndex & 2);
      fMVDB = outParam->fMVDB = (int) (modbIndex > 0);
   }
   else if (inpParam->pictureType == VDX_PIC_TYPE_IPB)
   {
      int modbIndex;
      /* MODB in Improved PB mode */
      retValue = vdxGetImpPBMODB(inBuffer,&modbIndex,bitErrorIndication);
      fCBPB = (int) (modbIndex & 1);
      fMVDB = outParam->fMVDB = (int) ((modbIndex & 2)>>1);
      outParam->IPBPredMode = (int) (modbIndex / 2);
   }
   else {
      fCBPB = 0;
      fMVDB = outParam->fMVDB = 0;
   }

   if (fCBPB) {
      /* CBPB */
      outParam->cbpb = (int) bibGetBits(6, inBuffer, &numBitsGot, 
         bitErrorIndication, &bibError);


   }
   else
      outParam->cbpb = 0;

   /* ac_pred_flag (1 bit) */
   if (inpParam->fMPEG4) {
      if (mbClass == VDX_MB_INTRA) {
         outParam->ac_pred_flag = (u_char) bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


      }
   }

   /* MVE */
   VDT_SET_START_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); // 2: cbpy

   /* CBPY */
   retValue = vdxGetCBPY(inBuffer, &cbpyIndex, bitErrorIndication);
   if (retValue != VDX_OK)
      goto exitFunction;

   if (mbClass == VDX_MB_INTER)
      /* Convert index to INTER CBPY */
      outParam->cbpy = 15 - cbpyIndex;

   else
      outParam->cbpy = cbpyIndex;

   /* MVE */
   VDT_SET_END_POSITION(MBinstance,2,inBuffer->getIndex,inBuffer->bitIndex); // 2: cbpy
   VDT_SET_START_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant

   if (fDQUANT) {
      /* Get DQUANT and update QUANT */
      retValue = vdxUpdateQuant(inBuffer, inpParam->fMQ, inpParam->quant,
         &outParam->quant, bitErrorIndication);
      if (retValue != VDX_OK)
         goto exitFunction;
   }

   /* MVE */
   VDT_SET_END_POSITION(MBinstance,1,inBuffer->getIndex,inBuffer->bitIndex); // 1: dquant
   VDT_SET_START_POSITION(MBinstance,10,inBuffer->getIndex,inBuffer->bitIndex); // 10: mv

   /* Color Toning */
   hTranscoder->AfterMBLayer(outParam->quant);
     

   if (fMVD) {
         int i;
         
         /* If no new Annex D is in use */
         if (!inpParam->fPLUSPTYPE || !inpParam->fUMV) {
             
             if (inpParam->fMPEG4) {
                 for (i = 0; i < numMVs; i++) {
                     retValue = vdxGetScaledMVD(inBuffer,inpParam->f_code,&outParam->mvdx[i],bitErrorIndication);
                     if (retValue != VDX_OK)
                         goto exitFunction;
                     
                     retValue = vdxGetScaledMVD(inBuffer,inpParam->f_code,&outParam->mvdy[i],bitErrorIndication);
                     if (retValue != VDX_OK)
                         goto exitFunction;
                 }
             } else {
                 for (i = 0; i < numMVs; i++) {
                     retValue = vdxGetMVD(inBuffer, &outParam->mvdx[i], bitErrorIndication);
                     if (retValue != VDX_OK)
                         goto exitFunction;
                     
                     retValue = vdxGetMVD(inBuffer, &outParam->mvdy[i], bitErrorIndication);
                     if (retValue != VDX_OK)
                         goto exitFunction;
                 }
             }
         }
         
         else if (inpParam->fPLUSPTYPE && inpParam->fUMV)   /* Annex D */
         {
             for (i = 0; i < numMVs; i++)
             {
                 retValue = vdxUMVGetMVD(inBuffer,&outParam->mvdx[i],bitErrorIndication);
                 if (retValue != VDX_OK)
                     goto exitFunction;
                 
                 retValue = vdxUMVGetMVD(inBuffer,&outParam->mvdy[i],bitErrorIndication);
                 if (retValue != VDX_OK)
                     goto exitFunction;
                 
                 if ((outParam->mvdx[i] == 5) && (outParam->mvdy[i] == 5))
                     /* Read "Prevent Start Code Emulation bit" if 000000 occurs */
                 {
                     bits = bibGetBits(1,inBuffer,&numBitsGot,bitErrorIndication,
                         &bibError);
                     
                     
                     if (!bits) {
                         retValue = VDX_OK_BUT_BIT_ERROR;
                         goto exitFunction;
                     }
                 }
             }
         }
   }
     
   if (fMVDB) {
         int i;
         if (!inpParam->fPLUSPTYPE || !inpParam->fUMV) 
         {
             retValue = vdxGetMVD(inBuffer,&outParam->mvdbx,bitErrorIndication);
             if (retValue != VDX_OK)
                 goto exitFunction;
             
             retValue = vdxGetMVD(inBuffer,&outParam->mvdby,bitErrorIndication);
             if (retValue != VDX_OK)
                 goto exitFunction;
         }
         else if (inpParam->fPLUSPTYPE && inpParam->fUMV)   /* Annex D */
         {
             for (i = 0; i < numMVs; i++)
             {
                 retValue = vdxUMVGetMVD(inBuffer,&outParam->mvdbx,bitErrorIndication);
                 if (retValue != VDX_OK)
                     goto exitFunction;
                 
                 retValue = vdxUMVGetMVD(inBuffer,&outParam->mvdby,bitErrorIndication);
                 if (retValue != VDX_OK)
                     goto exitFunction;
                 
                 if ((outParam->mvdbx == 5) && (outParam->mvdby == 5))
                     /* Read "Prevent Start Code Emulation bit" if 000000 occurs */
                 {
                     bits = bibGetBits(1,inBuffer,&numBitsGot,bitErrorIndication,
                         &bibError);
                     
                     if (!bits) {
                         retValue = VDX_OK_BUT_BIT_ERROR;
                         goto exitFunction;
                     }
                 }
             }
         }
   }
     
     
     
exitFunction:
     
   /* MVE */
   /* PB frame is not allowed !!! */
   VDT_SET_END_POSITION(MBinstance,10,inBuffer->getIndex,inBuffer->bitIndex); // 10: mv
   outParam->mcbpc = mcbpc;   

   for (int i = 0; i < outParam->numMVs; i++)
   {
         MBinstance->mvx[i] = outParam->mvdx[i];
         MBinstance->mvy[i] = outParam->mvdy[i];
   }
     
   MBinstance->mcbpc        = outParam->mcbpc;
   MBinstance->fCodedMB     = (unsigned char)outParam->fCodedMB ;           
   MBinstance->mbType       = outParam->mbType;              
   MBinstance->mbClass      = outParam->mbClass;             
   MBinstance->quant        = outParam->quant;               
   MBinstance->cbpc         = outParam->cbpc;                
   MBinstance->cbpy         = outParam->cbpy;                
   MBinstance->ac_pred_flag = outParam->ac_pred_flag;
   MBinstance->numMVs       = outParam->numMVs; 
   MBinstance->dquant       = fDQUANT ?  outParam->quant - inpParam->quant : 0;
     
   hTranscoder->OnePMBDataStarted(MBinstance);
   free(MBinstance);
     
   return retValue;
}


/*
 * Block Layer Global Functions
 */

/* {{-output"vdxGetIntraDCTBlock.txt"}} */
/*
 * vdxGetIntraDCTBlock
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    fCodedBlock                0 if COD is 1, 1 if COD is 0
 *    block                      DCT coefficients of the block 
 *                               in zigzag order
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *    fMQ                        flag for Modified Quantization
 *    qp                         quantization parameter
 *
 * Function:
 *    This function gets the DCT coefficients for one INTRA block.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxGetIntraDCTBlock(
   bibBuffer_t *inBuffer, 
   int fCodedBlock,
   int *block,
   int *bitErrorIndication,
   int fMQ,
   int qp)
/* {{-output"vdxGetIntraDCTBlock.txt"}} */
{
   int
      numBitsGot,
      retValue = VDX_OK;
   int16
      bibError = 0;
   u_int32 
      bits;

   vdxAssert(inBuffer != NULL);
   vdxAssert(block != NULL);
   vdxAssert(bitErrorIndication != NULL);

   /* MVE */
   int fEscapeCodeUsed = 0;

   /* INTRADC */
   bits = bibGetBits(8, inBuffer, &numBitsGot, bitErrorIndication, &bibError);

   block[0] = (int) (bits << 3);

   /* If (FLC == 1111 1111) */
   if (block[0] == 2040)
      block[0] = 1024;

   
   if (fCodedBlock)
      retValue = vdxGetDCTBlock(inBuffer, 1, 0, block, bitErrorIndication, fMQ,qp, &fEscapeCodeUsed);

   else
      memset(block + 1, 0, 63 * sizeof(int));

   return retValue;


}


/* {{-output"vdxGetDCTBlock.txt"}} */
/*
 * vdxGetDCTBlock
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    startIndex                 the first index in block where to put data
 *    fMPEG4EscapeCodes          use MPEG-4 escape codes (3 alternatives)
 *    block                      array for block (length 64)
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *    fMQ
 *    qp
 *    fEscapeCodeUsed            
 * Function:
 *    This function reads a block from bit buffer using Huffman codes listed
 *    in TCOEF table. The place, where the block is read is given as a
 *    pointer parameter.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 *
 */

int vdxGetDCTBlock(
   bibBuffer_t *inBuffer, 
   int startIndex,
   u_char fMPEG4EscapeCodes,
   int *block,
   int *bitErrorIndication,
   int fMQ,
   int qp,
   int *fEscapeCodeUsed)
/* {{-output"vdxGetDCTBlock.txt"}} */
{
   /* Lookup tables: val field contains
      1 bit for LAST, 8 bits for RUN and 4 bits for LEVEL */
   static const vdxVLCTable_t tcoefTab0[] = {
      {4225,7}, {4209,7}, {4193,7}, {4177,7}, {193,7}, {177,7},
      {161,7}, {4,7}, {4161,6}, {4161,6}, {4145,6}, {4145,6},
      {4129,6}, {4129,6}, {4113,6}, {4113,6}, {145,6}, {145,6},
      {129,6}, {129,6}, {113,6}, {113,6}, {97,6}, {97,6},
      {18,6}, {18,6}, {3,6}, {3,6}, {81,5}, {81,5},
      {81,5}, {81,5}, {65,5}, {65,5}, {65,5}, {65,5},
      {49,5}, {49,5}, {49,5}, {49,5}, {4097,4}, {4097,4},
      {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4}, {4097,4},
      {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2},
      {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2},
      {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2},
      {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2},
      {1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,2},
      {1,2}, {1,2}, {17,3}, {17,3}, {17,3}, {17,3},
      {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3},
      {17,3}, {17,3}, {17,3}, {17,3}, {17,3}, {17,3},
      {33,4}, {33,4}, {33,4}, {33,4}, {33,4}, {33,4},
      {33,4}, {33,4}, {2,4}, {2,4}, {2,4}, {2,4},
      {2,4}, {2,4}, {2,4}, {2,4}
   };

   static const vdxVLCTable_t tcoefTab1[] = {
      {9,10}, {8,10}, {4481,9}, {4481,9}, {4465,9}, {4465,9},
      {4449,9}, {4449,9}, {4433,9}, {4433,9}, {4417,9}, {4417,9},
      {4401,9}, {4401,9}, {4385,9}, {4385,9}, {4369,9}, {4369,9},
      {4098,9}, {4098,9}, {353,9}, {353,9}, {337,9}, {337,9},
      {321,9}, {321,9}, {305,9}, {305,9}, {289,9}, {289,9},
      {273,9}, {273,9}, {257,9}, {257,9}, {241,9}, {241,9},
      {66,9}, {66,9}, {50,9}, {50,9}, {7,9}, {7,9},
      {6,9}, {6,9}, {4353,8}, {4353,8}, {4353,8}, {4353,8},
      {4337,8}, {4337,8}, {4337,8}, {4337,8}, {4321,8}, {4321,8},
      {4321,8}, {4321,8}, {4305,8}, {4305,8}, {4305,8}, {4305,8},
      {4289,8}, {4289,8}, {4289,8}, {4289,8}, {4273,8}, {4273,8},
      {4273,8}, {4273,8}, {4257,8}, {4257,8}, {4257,8}, {4257,8},
      {4241,8}, {4241,8}, {4241,8}, {4241,8}, {225,8}, {225,8},
      {225,8}, {225,8}, {209,8}, {209,8}, {209,8}, {209,8},
      {34,8}, {34,8}, {34,8}, {34,8}, {19,8}, {19,8},
      {19,8}, {19,8}, {5,8}, {5,8}, {5,8}, {5,8}
   };

   static const vdxVLCTable_t tcoefTab2[] = {
      {4114,11}, {4114,11}, {4099,11}, {4099,11}, {11,11}, {11,11},
      {10,11}, {10,11}, {4545,10}, {4545,10}, {4545,10}, {4545,10},
      {4529,10}, {4529,10}, {4529,10}, {4529,10}, {4513,10}, {4513,10},
      {4513,10}, {4513,10}, {4497,10}, {4497,10}, {4497,10}, {4497,10},
      {146,10}, {146,10}, {146,10}, {146,10}, {130,10}, {130,10},
      {130,10}, {130,10}, {114,10}, {114,10}, {114,10}, {114,10},
      {98,10}, {98,10}, {98,10}, {98,10}, {82,10}, {82,10},
      {82,10}, {82,10}, {51,10}, {51,10}, {51,10}, {51,10},
      {35,10}, {35,10}, {35,10}, {35,10}, {20,10}, {20,10},
      {20,10}, {20,10}, {12,11}, {12,11}, {21,11}, {21,11},
      {369,11}, {369,11}, {385,11}, {385,11}, {4561,11}, {4561,11},
      {4577,11}, {4577,11}, {4593,11}, {4593,11}, {4609,11}, {4609,11},
      {22,12}, {36,12}, {67,12}, {83,12}, {99,12}, {162,12},
      {401,12}, {417,12}, {4625,12}, {4641,12}, {4657,12}, {4673,12},
      {4689,12}, {4705,12}, {4721,12}, {4737,12}, {7167,7},
      {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7},
      {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7},
      {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7},
      {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7},
      {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7}, {7167,7},
      {7167,7}
   };

   static const int inter_max_level[2][64] = {
                                     {12,  6,  4,  3,  3,  3,  3,  2,
                                       2,  2,  2,  1,  1,  1,  1,  1,
                                       1,  1,  1,  1,  1,  1,  1,  1,
                                       1,  1,  1,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0},
 
                                      {3,  2,  1,  1,  1,  1,  1,  1,
                                       1,  1,  1,  1,  1,  1,  1,  1,
                                       1,  1,  1,  1,  1,  1,  1,  1,
                                       1,  1,  1,  1,  1,  1,  1,  1,
                                       1,  1,  1,  1,  1,  1,  1,  1,
                                       1,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0,
                                       0,  0,  0,  0,  0,  0,  0,  0}
                                    };

   static const int inter_max_run0[13] = { 999,
                                   26, 10,  6,  2,  1,  1,
                                    0,  0,  0,  0,  0,  0
                                };
  
   static const int inter_max_run1[4] = { 999, 40,  1,  0 };

   int
      numBitsGot, /* number of bits got from bit buffer */
      retValue = VDX_OK,
      code,       /* bits got from bit buffer */
      index,      /* index to zigzag table running from 1 to 63 */
      run,        /* RUN code */
      level;      /* LEVEL code */

   int16
      bibError = 0;

   u_int32 
      last,       /* LAST code (see standard) */
      sign,       /* sign for level */
      tmp;        /* temporary variable for reading some redundant bits */

   vdxVLCTable_t const *tab; /* pointer to lookup table */

   vdxAssert(inBuffer != NULL);
   vdxAssert(startIndex == 0 || startIndex == 1);
   vdxAssert(block != NULL);
   vdxAssert(bitErrorIndication != NULL);

   /* Reset all coefficients to zero in order to avoid writing
      zeros one by one during run-length decoding */
   memset(&block[startIndex], 0, (64 - startIndex) * sizeof(int));

   index = startIndex;

   /* MVE */
   *fEscapeCodeUsed = 0;

   do {
      /* Read next codeword */
      code = (int) bibShowBits(12, inBuffer, &numBitsGot, bitErrorIndication,
         &bibError);


      /* Select the right table and index for the codeword */
      if (code >= 512)
         tab = &tcoefTab0[(code >> 5) - 16];
      else if (code >= 128)
         tab = &tcoefTab1[(code >> 2) - 32];
      else if (code >= 8)
         tab = &tcoefTab2[code - 8];
      else {
         deb("ERROR - illegal TCOEF\n");
         retValue = VDX_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Flush the codeword from the buffer */
      bibFlushBits(tab->len, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


      if (tab->val == 7167)     /* ESCAPE */
      {

          
        /* the following is modified for 3-mode escape */
        if(fMPEG4EscapeCodes) {

          int run_offset=0,
              level_offset=0;

          code = (int) bibShowBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


          if (code<=2) {

              /* escape modes: level or run is offset */
              if (code==2) run_offset=1;
              else level_offset=1;

              /* Flush the escape code from the buffer */
              if (run_offset)
                  bibFlushBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);
              else
                  bibFlushBits(1, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


              /* Read next codeword */
              code = (int) bibShowBits(12, inBuffer, &numBitsGot, bitErrorIndication,
                  &bibError);



              /* Select the right table and index for the codeword */
              if (code >= 512)
                  tab = &tcoefTab0[(code >> 5) - 16];
              else if (code >= 128)
                  tab = &tcoefTab1[(code >> 2) - 32];
              else if (code >= 8)
                  tab = &tcoefTab2[code - 8];
              else {
                  deb("ERROR - illegal TCOEF\n");
                  retValue = VDX_OK_BUT_BIT_ERROR;
                  goto exitFunction;
              }

              bibFlushBits(tab->len, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


              run = (tab->val >> 4) & 255;
              level = tab->val & 15;
              last = (tab->val >> 12) & 1;

              /* need to add back the max level */
              if (level_offset)
                  level = level + inter_max_level[last][run];
              else if (last)
                  run = run + inter_max_run1[level]+1;
              else
                  run = run + inter_max_run0[level]+1;

              sign = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,
                  &bibError);


              
              if (sign)
                  level = -level;

          } else {
              
              /* Flush the codeword from the buffer */
              bibFlushBits(2, inBuffer, &numBitsGot, bitErrorIndication, &bibError);


              /* LAST */
              last = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                  &bibError);



              /* RUN */
              run = (int) bibGetBits(6, inBuffer, &numBitsGot, bitErrorIndication, 
                  &bibError);


              /* MARKER BIT */
              tmp = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError);


              if(!tmp) {
                  retValue = VDX_OK_BUT_BIT_ERROR;
                  goto exitFunction;
              }
              /* LEVEL */
              level = (int) bibGetBits(12, inBuffer, &numBitsGot, bitErrorIndication, 
                  &bibError);


              /* MARKER BIT */
              tmp = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,&bibError);


              if ( !tmp ) {
                  retValue = VDX_OK_BUT_BIT_ERROR;
                  goto exitFunction;
              }

              /* 0000 0000 0000 and 1000 0000 0000 is forbidden unless in MQ mode */
              if (level == 0 || level == 2048) {
                  deb("ERROR - illegal level.\n");
                  retValue = VDX_OK_BUT_BIT_ERROR;
                  goto exitFunction;
              }

              /* Codes 1000 0000 0001 .. 1111 1111 1111 */
              if (level > 2048)
                  level -= 4096;
              
          } /* flc */

        } else { /* !fMPEG4EscapeCodes */

            /* MVE */
            *fEscapeCodeUsed = 1;
            
            /* LAST */
            last = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication, 
                &bibError);


            /* RUN */
            run = (int) bibGetBits(6, inBuffer, &numBitsGot, bitErrorIndication, 
                &bibError);


            /* LEVEL */
            level = (int) bibGetBits(8, inBuffer, &numBitsGot, bitErrorIndication, 
                &bibError);
            

            /* Codes 1000 0001 .. 1111 1111 */
            if (level > 128)
                level -= 256;


            /* 0000 0000 is forbidden, 1000 0000 is forbidden unless in MQ mode */
            if (level == 0) {
                deb("ERROR - illegal level.\n");
                retValue = VDX_OK_BUT_BIT_ERROR;
                goto exitFunction;
            }
            if (level == 128)
            {
               if (fMQ)
               {
                  level = (int) bibGetBits(11,inBuffer,&numBitsGot,
                     bitErrorIndication,&bibError);


                  level = ((level&0x003F) <<5) | (level >> 6);
                  if (level>1023) level -= 2048;

                  /* See section T.5 of the H.263 recommendation to understand
                     this restriction. */
                    if (((level > -127) && (level < 127)) || (qp >= 8))
                    {
                        deb("ERROR - illegal extended level.\n");
                        retValue = VDX_OK_BUT_BIT_ERROR;
                        goto exitFunction;
                    }
                }
                else
                {
                    deb("ERROR - illegal level.\n");
                    retValue = VDX_OK_BUT_BIT_ERROR;
                    goto exitFunction;
                }
            }
        } /* End fMPEG4EscapeCodes switch */        
      }

      else {
         run = (tab->val >> 4) & 255;
         level = tab->val & 15;
         last = (tab->val >> 12) & 1;

         sign = bibGetBits(1, inBuffer, &numBitsGot, bitErrorIndication,
            &bibError);


         if (sign)
            level = -level;
      }

      /* If too many coefficients */
      if (index + run > 63) {
         deb("ERROR - too many TCOEFs.\n");
         retValue = VDX_OK_BUT_BIT_ERROR;
         goto exitFunction;
      }

      /* Do run-length decoding */

      /* Note: No need to set coeffs to zero since they are already reset
         in the beginning of the function */
      index += run;

         block[index++] = level;

   } while (!last);

   exitFunction:

   /* Note: No need to set the rest of the coefficients to zero since
      they are already reset in the beginning of the function */

   if (!bibError)
      return retValue;

   else if (bibError == ERR_BIB_NOT_ENOUGH_DATA) {
      return VDX_OK_BUT_BIT_ERROR;
   }
   else if ( *bitErrorIndication ) {
      return VDX_OK_BUT_BIT_ERROR;
   }
   else
      return VDX_ERR_BIB;
}



/*
 * Picture Layer Local Functions
 */

/*
 * vdxActAfterIncorrectSEI
 *
 * Parameters:
 *    inBuffer                   B: Bit Buffer instance
 *    fPLUSPTYPE                 I: signals the existence of PLUSPTYPE
 *    fLast                      B: set to 1 if SEI flushed, 
 *                                  otherwise not changed
 *    bitErrorIndication         B: bit error indication, see biterr.h for
 *                                  the possible values
 *
 * Function:
 *    Problem: H.263 v1 did not specify FTYPE/DSIZE values, but rather
 *    any data can be transfered in PSUPP. Section 5.1.3 of 
 *    H.263 Recommendation specifies that Annex L and Annex W supplemental
 *    enhancement functions can be used also if PLUSPTYPE is not in use.
 *    Consequently, if PLUSPTYPE is not in use and if the bit-stream
 *    violates FTYPE/DSIZE (or any other) rules defined in Annex L or 
 *    Annex W, we do not know if the far-end encoder is transmitting
 *    proprietary PSUPP values (allowed in H.263 v1) or if a bit error 
 *    has hit the SEI data that actually follows Annex L/W. 
 *
 *    This function is called after an illegal Annex L/W parameter has
 *    been detected. The function acts as follows:
 *    - If PLUSPTYPE is in use, there must be
 *      a bit error, and the function signals the error.
 *    - If PLUSPTYPE is not in use, there is
 *      a bit error or the far-end encoder uses proprietary PSUPP values.
 *      The function flushes all PSUPP fields and returns without error
 *      indication.
 *      NOTE: This scheme could be improved if the decoder knows that
 *      the far-end encoder is always following Annex L/W.
 *
 * Returns:
 *    VDX_OK
 *    VDX_OK_BUT_BIT_ERROR
 *    VDX_ERR
 */

static int vdxActAfterIncorrectSEI(
   bibBuffer_t *inBuffer,
   int fPLUSPTYPE,
   int *fLast,
   int *bitErrorIndication)
{
   /* If Annex L/W is in use for sure */
   if (fPLUSPTYPE) {

      /* Return indication of bit error */
      return VDX_OK_BUT_BIT_ERROR;
   }

   /* Else Annex L/W is not necessarily on */
   else {
      
      /* We are out of sync due to bit error or 
         do not understand the non-standard PEI/PSUPP syntax
         --> flush the rest of PEI/PSUPP pairs */

      *fLast = 1;

      return vdxFlushSEI(inBuffer, bitErrorIndication);
   }
}


/*
 * vdxStandardSourceFormatToFrameSize
 *    
 *
 * Parameters:
 *    sourceFormat               the three source format bits
 *    width                      luminance image width
 *    height                     luminance image height
 *
 * Function:
 *    This function converts the source format bits to luminance image size.
 *
 * Returns:
 *    Nothing.
 *
 *    
 *
 */

static void vdxStandardSourceFormatToFrameSize(int sourceFormat, int *width, int *height)
{
   vdxAssert(sourceFormat >= 1 && sourceFormat <= 5);

   switch (sourceFormat) {
      case 1:  /* sub-QCIF */
         *width = 128;
         *height = 96;
         break;
      case 2:  /* QCIF */
         *width = 176;
         *height = 144;
         break;
      case 3:  /* CIF */
         *width = 352;
         *height = 288;
         break;
      case 4:  /* 4CIF */
         *width = 704;
         *height = 576;
         break;
      case 5:  /* 16 CIF */
         *width = 1408;
         *height = 1152;
         break;
   }
}


/*
 * Slice Layer Local Functions
 */

/*
 * vdxFrameSizeToPictureFormat
 *    
 *
 * Parameters:
 *    width                      luminance image width
 *    height                     luminance image height
 *
 * Function:
 *    This function converts luminance image size to picture format.
 *
 * Returns:
 *    Source format
 *
 *
 */

int vdxFrameSizeToPictureFormat(int width, int /*height*/)
{
   if (width <= 128) 
      return 0;   /* sub-QCIF */
   else if (width <= 176)
      return 1;   /* QCIF */
   else if (width <= 352)
      return 2;   /* CIF */
   else if (width <= 704)
      return 3;   /* 4CIF */
   else if (width <= 1408)
      return 4;   /* 16CIF */
   else if (width <= 2048)
      return 5;   /* 2048x1152 */
   else
      return -1;  /* Should never happen */
}

/*
 * Macroblock Layer Local Functions
 */

/*
 * vdxGetCBPY
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    index                      index to the CBPY table of H.263
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the CBPY code.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxGetCBPY(bibBuffer_t *inBuffer, int *index, 
   int *bitErrorIndication)
{
   static const vdxVLCTable_t tabCBPY[48] = {
      {-1,0}, {-1,0}, {6,6}, {9,6}, {8,5}, {8,5}, {4,5}, {4,5},
      {2,5}, {2,5}, {1,5}, {1,5}, {0,4}, {0,4}, {0,4}, {0,4},
      {12,4}, {12,4}, {12,4}, {12,4}, {10,4},{10,4},{10,4},{10,4},
      {14,4}, {14,4}, {14,4}, {14,4}, {5,4}, {5,4}, {5,4}, {5,4},
      {13,4}, {13,4}, {13,4}, {13,4}, {3,4}, {3,4}, {3,4}, {3,4},
      {11,4}, {11,4}, {11,4}, {11,4}, {7,4}, {7,4}, {7,4}, {7,4}
   };

   int bitsGot, code;
   int16 ownError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(index != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibShowBits(6, inBuffer, &bitsGot, bitErrorIndication, 
      &ownError);


   if (code >= 48) {
      /* bit pattern = 11xxxx */
      bibFlushBits(2, inBuffer, &bitsGot, bitErrorIndication, &ownError);
      *index = 15;
      return VDX_OK;
   }

   if (code < 2) {
      deb("vlcGetCBPY: ERROR - illegal code.\n");
      return VDX_OK_BUT_BIT_ERROR;
   }

   bibFlushBits(tabCBPY[code].len, inBuffer, &bitsGot, bitErrorIndication, &ownError);

   *index = tabCBPY[code].val;

   if (ownError)
      return VDX_ERR_BIB;

   return VDX_OK;
}


/*
 * vdxGetMCBPCInter
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    fPLUSPTYPE                 flag to indicate if PLUSPTYPE is present
 *    fFourMVsPossible           flag to indicate if four motion vectors per
 *                               macroblock is allowed
 *    fFirstMBOfPicture          flag to indicate if the current macroblock
 *                               is the first one of the picture in scan-order
 *    index                      index to the MCBPC table for P-pictures
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the MCBPC code for P-pictures.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 *    
 */

int vdxGetMCBPCInter(
   bibBuffer_t *inBuffer, 
   int fPLUSPTYPE,
   int fFourMVsPossible,
   int fFirstMBOfPicture,
   int *index, 
   int *bitErrorIndication)
{
   static const vdxVLCTable_t tabMCBPCInter[256] = {
      {-1,0}, /* 0 = illegal */
      {20,9}, /* 1 = 0000 0000 1 = stuffing */
      {19,9}, /* 2 = 0000 0001 0 */
      {18,9}, /* 3 = 0000 0001 1 */
      {17,9}, /* 4 = 0000 0010 0 */
      {7,9},  /* 5 = 0000 0010 1 */
      {14,8}, {14,8}, /* 6..7 = 0000 0011 x */
      {13,8}, {13,8}, /* 8..9 = 0000 0100 x */
      {11,8}, {11,8}, /* 10..11 = 0000 0101 x */
      {15,7}, {15,7}, {15,7}, {15,7}, /* 12..15 = 0000 011x x */
      {10,7}, {10,7}, {10,7}, {10,7}, /* 16..19 = 0000 100x x */
      { 9,7}, { 9,7}, { 9,7}, { 9,7}, /* 20..23 = 0000 101x x */
      { 6,7}, { 6,7}, { 6,7}, { 6,7}, /* 24..27 = 0000 110x x */
      { 5,7}, { 5,7}, { 5,7}, { 5,7}, /* 28..31 = 0000 111x x */
      {16,6}, {16,6}, {16,6}, {16,6},
      {16,6}, {16,6}, {16,6}, {16,6}, /* 32..39 = 0001 00xx x */
      { 3,6}, { 3,6}, { 3,6}, { 3,6},
      { 3,6}, { 3,6}, { 3,6}, { 3,6}, /* 40..47 = 0001 01xx x */
      {12,5}, {12,5}, {12,5}, {12,5}, {12,5}, {12,5},
      {12,5}, {12,5}, {12,5}, {12,5}, {12,5}, {12,5},
      {12,5}, {12,5}, {12,5}, {12,5}, /* 48..63 = 0001 1xxx x */
      { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4},
      { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4},
      { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4},
      { 2,4}, { 2,4}, { 2,4}, { 2,4}, { 2,4}, /* 64..95 = 0010 xxxx x */
      { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4},
      { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4},
      { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4},
      { 1,4}, { 1,4}, { 1,4}, { 1,4}, { 1,4}, /* 96..127 = 0011 xxxx x */
      {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3},
      {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3},
      {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3},
      {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3},
      {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3},
      {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3},
      {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3}, {8,3},
      {8,3}, {8,3}, /* 128..191 = 010x xxxx x */
      {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3},
      {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3},
      {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3},
      {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3},
      {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3},
      {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3},
      {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3}, {4,3},
      {4,3}, {4,3}, {4,3} /* 192..255 = 011x xxxx x */
   };

   /* Indices 21 - 24 of the MCBPC table for P-pictures,
      4 least significant bits from the maximum of 13 are used for indexing */
   static const vdxVLCTable_t tabMCBPCInterIndices21To24[16] = {
      {-1,0}, {-1,0}, {-1,0}, {-1,0}, 
      {-1,0}, {-1,0}, {-1,0}, {-1,0},        /* 0000 0000 00xx x, illegal */
      {21,11}, {21,11}, {21,11}, {21,11},    /* 0000 0000 010x x */
      {22,13},                               /* 0000 0000 0110 0 */
      {-1,0},                                /* 0000 0000 0110 1, illegal */
      {23,13},                               /* 0000 0000 0111 0 */
      {24,13}                                /* 0000 0000 0111 1 */
   };

   int bitsGot, code;
   int16 ownError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(index != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibShowBits(9, inBuffer, &bitsGot, bitErrorIndication, 
      &ownError);


   /* If index == 0 */
   if (code >= 256) {
      /* bit pattern = 1xxx xxxx x */
      bibFlushBits(1, inBuffer, &bitsGot, bitErrorIndication, &ownError);
      *index = 0;
      return VDX_OK;
   }

   /* If illegal code or indices 21 - 24 */
   if (code == 0) {
      /* 0000 0000 0 (indices 21 - 24 in MCBPC table for P-pictures)
         can only be present if the conditions listed in  section 5.3.2 of 
         the H.263 recommendation are fulfilled. Otherwise, the bit pattern
         is considered an illegal codeword. */

      /* If indices 21 - 24 */
      if (fPLUSPTYPE && fFourMVsPossible && !fFirstMBOfPicture) {

         /* Note: The following restriction is given in section 5.3.2 of 
            the H.263 recommendation: "Also, encoders shall not allow an MCBPC
            code for macroblock type 5 to immediately follow seven consecutive 
            zeros in the bitstream (as can be caused by particular INTRADC codes
            followed by COD=0), in order to prevent start code emulation."
            This condition is not checked in the decoder, since it would require
            relatively complex code structure and it is considered relatively
            improbable. */

         /* Read 13 bits */
         code = (int) bibShowBits(13, inBuffer, &bitsGot, bitErrorIndication, 
            &ownError);


         /* Note: We already know that the first 9 bits are zero, thus code
            cannot be larger than 15. */

         *index = tabMCBPCInterIndices21To24[code].val;

         /* If illegal bit pattern */
         if (*index == -1) {
            deb("vlcGetMCBPCInter: ERROR - illegal code.\n");
            return VDX_OK_BUT_BIT_ERROR;
         }

         bibFlushBits(tabMCBPCInterIndices21To24[code].len, inBuffer, &bitsGot, 
            bitErrorIndication, &ownError);
      }

      /* Else illegal code */
      else {
         deb("vlcGetMCBPCInter: ERROR - illegal code.\n");
         return VDX_OK_BUT_BIT_ERROR;
      }
   }

   /* Else indices 1 - 20 */
   else {
      bibFlushBits(tabMCBPCInter[code].len, inBuffer, &bitsGot, bitErrorIndication, &ownError);
   
      *index = tabMCBPCInter[code].val;
   }


   if (ownError)
      return VDX_ERR_BIB;

   return VDX_OK;
}


/*
 * vdxGetMCBPCIntra
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    index                      index to the MCBPC table for I-pictures
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the MCBPC code for I-pictures.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxGetMCBPCIntra(bibBuffer_t *inBuffer, int *index, 
   int *bitErrorIndication)
{
   static const vdxVLCTable_t tabMCBPCIntra[64] = {
      {-1,0}, /* illegal */
      {5,6},
      {6,6},
      {7,6},
      {4,4}, {4,4}, {4,4}, {4,4},
      {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3}, {1,3},
      {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3}, {2,3},
      {3,3}, {3,3}, {3,3}, {3,3}, {3,3}, {3,3}, {3,3}, {3,3}
   };

   int bitsGot, code;
   int16 ownError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(index != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibShowBits(9, inBuffer, &bitsGot, bitErrorIndication, 
      &ownError);


   if (code == 1) {
      /* macroblock stuffing */
      bibFlushBits(9, inBuffer, &bitsGot, bitErrorIndication, &ownError);
      *index = 8;
      return VDX_OK;
   }

   code >>= 3; /* remove unnecessary bits */

   if (code == 0) {
      deb("vlcGetMCBPCIntra: ERROR - illegal code.\n");
      return VDX_OK_BUT_BIT_ERROR;
   }

   if (code >= 32) {
      /* bit pattern = 1xxxxx */
      bibFlushBits(1, inBuffer, &bitsGot, bitErrorIndication, &ownError);
      *index = 0;
      return VDX_OK;
   }

   bibFlushBits(tabMCBPCIntra[code].len, inBuffer, &bitsGot, bitErrorIndication, &ownError);

   *index = tabMCBPCIntra[code].val;

   if (ownError)
      return VDX_ERR_BIB;

   return VDX_OK;
}


/*
 * vdxGetMVD
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    mvdx10                     the leftmost vector value from the motion
 *                               vector VLC table multiplied by 10 to avoid 
 *                               non-integer numbers.
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the MVD code.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

int vdxGetMVD(bibBuffer_t *inBuffer, int *mvdx10, 
   int *bitErrorIndication)
{
   static const vdxVLCTable_t tabMVD0[14] = {
      {15,4},  /* 0001 0 */
      {-15,4}, /* 0001 1 */
      {10,3}, {10,3},   /* 0010 */
      {-10,3}, {-10,3}, /* 0011 */
      {5,2}, {5,2}, {5,2}, {5,2},    /* 010 */
      {-5,2}, {-5,2}, {-5,2}, {-5,2} /* 011 */
   };

   static const vdxVLCTable_t tabMVD1[96] = {
      {60,10}, {-60,10},
      {55,10}, {-55,10},
      {50,9}, {50,9}, {-50,9}, {-50,9},
      {45,9}, {45,9}, {-45,9}, {-45,9},
      {40,9}, {40,9}, {-40,9}, {-40,9},
      {35,7}, {35,7}, {35,7}, {35,7}, {35,7}, {35,7}, {35,7}, {35,7},
      {-35,7}, {-35,7}, {-35,7}, {-35,7}, {-35,7}, {-35,7}, {-35,7}, {-35,7},
      {30,7}, {30,7}, {30,7}, {30,7}, {30,7}, {30,7}, {30,7}, {30,7},
      {-30,7}, {-30,7}, {-30,7}, {-30,7}, {-30,7}, {-30,7}, {-30,7}, {-30,7},
      {25,7}, {25,7}, {25,7}, {25,7}, {25,7}, {25,7}, {25,7}, {25,7},
      {-25,7}, {-25,7}, {-25,7}, {-25,7}, {-25,7}, {-25,7}, {-25,7}, {-25,7},
      {20,6}, {20,6}, {20,6}, {20,6}, {20,6}, {20,6}, {20,6}, {20,6},
      {20,6}, {20,6}, {20,6}, {20,6}, {20,6}, {20,6}, {20,6}, {20,6},
      {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6},
      {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6}, {-20,6}
   };

   static const vdxVLCTable_t tabMVD2[] = {
      {160,12}, {-160,12}, {155,12}, {-155,12},
      {150,11}, {150,11}, {-150,11}, {-150,11},
      {145,11}, {145,11}, {-145,11}, {-145,11},
      {140,11}, {140,11}, {-140,11}, {-140,11},
      {135,11}, {135,11}, {-135,11}, {-135,11},
      {130,11}, {130,11}, {-130,11}, {-130,11},
      {125,11}, {125,11}, {-125,11}, {-125,11},
      {120,10}, {120,10}, {120,10}, {120,10},
      {-120,10}, {-120,10}, {-120,10}, {-120,10},
      {115,10}, {115,10}, {115,10}, {115,10},
      {-115,10}, {-115,10}, {-115,10}, {-115,10},
      {110,10}, {110,10}, {110,10}, {110,10},
      {-110,10}, {-110,10}, {-110,10}, {-110,10},
      {105,10}, {105,10}, {105,10}, {105,10},
      {-105,10}, {-105,10}, {-105,10}, {-105,10},
      {100,10}, {100,10}, {100,10}, {100,10},
      {-100,10}, {-100,10}, {-100,10}, {-100,10},
      {95,10}, {95,10}, {95,10}, {95,10},
      {-95,10}, {-95,10}, {-95,10}, {-95,10},
      {90,10}, {90,10}, {90,10}, {90,10},
      {-90,10}, {-90,10}, {-90,10}, {-90,10},
      {85,10}, {85,10}, {85,10}, {85,10},
      {-85,10}, {-85,10}, {-85,10}, {-85,10},
      {80,10}, {80,10}, {80,10}, {80,10},
      {-80,10}, {-80,10}, {-80,10}, {-80,10},
      {75,10}, {75,10}, {75,10}, {75,10},
      {-75,10}, {-75,10}, {-75,10}, {-75,10},
      {70,10}, {70,10}, {70,10}, {70,10},
      {-70,10}, {-70,10}, {-70,10}, {-70,10},
      {65,10}, {65,10}, {65,10}, {65,10},
      {-65,10}, {-65,10}, {-65,10}, {-65,10}
   };
   int code, bitsGot;
   int16 ownError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(mvdx10 != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibGetBits(1, inBuffer, &bitsGot, bitErrorIndication, &ownError);


   if (code) {
      *mvdx10 = 0;
      return VDX_OK;
   }

   code = (int) bibShowBits(12, inBuffer, &bitsGot, bitErrorIndication, 
      &ownError);

   
   if (code >= 512) {
      code = (code >> 8) - 2;
      bibFlushBits(tabMVD0[code].len, inBuffer, &bitsGot, bitErrorIndication, &ownError);

      *mvdx10 = tabMVD0[code].val;
      return VDX_OK;
   }

   if (code >= 128) {
      code = (code >> 2) - 32;
      bibFlushBits(tabMVD1[code].len, inBuffer, &bitsGot, bitErrorIndication, &ownError);


      *mvdx10 = tabMVD1[code].val;
      return VDX_OK;
   }

   /* If illegal code, return bit error.
      In Table 14/H.263, the illegal codes are 0000 0000 000x x and 0000 0000 0010 0.
      In Table B-12/MPEG-4 Visual, Section B.1.3, 0000 0000 0010 0 is legal.
      To simplify the source code, 0000 0000 0010 0 is considered legal in  H.263 too */
   if ((code -= 4) < 0) {
      deb("vlcGetMVD: ERROR - illegal code.\n");
      return VDX_OK_BUT_BIT_ERROR;
   }

   bibFlushBits(tabMVD2[code].len, inBuffer, &bitsGot, bitErrorIndication, &ownError);


   *mvdx10 = tabMVD2[code].val;
   return VDX_OK;
}


/*
 * vdxUMVGetMVD
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    mvdx10                     the leftmost vector value from the motion
 *                               vector VLC table multiplied by 10 to avoid 
 *                               non-integer numbers.
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the MVD code when unrestricted motion vector mode
 *    is in use and PLUSTYPE is present.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally,but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 */

static int vdxUMVGetMVD(bibBuffer_t *inBuffer,int *mvdx10,
   int *bitErrorIndication)
{
   int code,bitsGot;
   int16 ownError = 0;
   int sign;

   vdxAssert(inBuffer != NULL);
   vdxAssert(mvdx10 != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,&ownError);


   if (code) {
      *mvdx10 = 0;
      return VDX_OK;
   }

   code = (int) bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,&ownError);

   code += 2;

   while (bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,&ownError) 
   )
   {
      code <<=1;
      code = code + bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,&ownError);
   }

   
   sign = (code & 0x0001)?-5:5;
   code >>=1;
   *mvdx10 = sign*code;
   return VDX_OK;
}

/*
 * vdxGetNormalMODB
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    index                      index to the MODB table
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the MODB code. The function should be used
 *    in PB-frames mode (Annex G).
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 */

static int vdxGetNormalMODB(bibBuffer_t *inBuffer, int *index, 
   int *bitErrorIndication)
{
   int bitsGot, code;
   int16 ownError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(index != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibShowBits(2, inBuffer, &bitsGot, bitErrorIndication, 
      &ownError);


   if (code < 2) {
      /* code = 0 */
      bibFlushBits(1, inBuffer, &bitsGot, bitErrorIndication, &ownError);
      *index = 0;
      return VDX_OK;
   }
   else {
      /* code = 10 or 11 */
      bibFlushBits(2, inBuffer, &bitsGot, bitErrorIndication, &ownError);
      *index = code - 1;
      return VDX_OK;
   }
}

/*
 * vdxGetImpPBMODB
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    index                      index to the MODB table
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the MODB code. This function should be used
 *    in improved PB-frames mode (Annex M).
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally,but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 */

static int vdxGetImpPBMODB(bibBuffer_t *inBuffer,int *index,
   int *bitErrorIndication)
{
   int bitsGot,code;
   int16 ownError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(index != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,
      &ownError);


   
   if (code == 0)
   {
      /* Bidirectional Prediction: code = 0 */
      *index = 0;
      return VDX_OK;      
   }
   else
   {
      code = (int) bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,
         &ownError);

      if (code == 0)
      {
         /* Bidirectional Prediction: code = 10 */
         *index = 1;
         return VDX_OK;
      }
      else
      {
         code = (int) bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,
            &ownError);


         if (code == 0)
         {
            /* Forward Prediction: code = 110 */
            *index = 2;
            return VDX_OK;
         }
         else
         {
            code = (int) bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,
               &ownError);


            if (code == 0)
            {
               /* Forward Prediction: code = 1110 */
               *index = 3;
               return VDX_OK;
            }
            else
            {
               code = (int) bibGetBits(1,inBuffer,&bitsGot,bitErrorIndication,
                  &ownError);


               /* Backward Prediction: code = 11110 or 11111 */
               *index = 4+code;
               return VDX_OK;
            }
         }
      }
    }
}

/*
 * vdxUpdateQuant
 *    
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    fMQ                        non-zero if the Modified Quantization mode
 *                               (Annex T) is in use, otherwise zero
 *    quant                      the quantization parameter for the previous
 *                               macroblock
 *    newQuant                   the updated quantization parameter for
 *                               the current macroblock
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the DQUANT field and updated the current quantization
 *    parameter appropriately.
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally, but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 *    
 *
 */

int vdxUpdateQuant(
   bibBuffer_t *inBuffer, 
   int fMQ,
   int quant,
   int *newQuant,
   int *bitErrorIndication)
{
   int
      numBitsGot;
   int16
      bibError = 0;
   u_int32 
      bits;
   static const int changeOfQuant[2][32] = 
      {{0,2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2,-2,-2,-2,-2,-2,
        -2,-2,-2,-2,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3},
       {0,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,2,1,-5}};

   vdxAssert(inBuffer != NULL);
   vdxAssert(newQuant != NULL);
   vdxAssert(bitErrorIndication != NULL);

   /* DQUANT */
   if (!fMQ) {
      bits = bibGetBits(2, inBuffer, &numBitsGot, bitErrorIndication, 
         &bibError);


      switch (bits) {
         case 0: *newQuant = quant - 1; break;
         case 1: *newQuant = quant - 2; break;
         case 2: *newQuant = quant + 1; break;
         case 3: *newQuant = quant + 2; break;
      }

      /* Clip QUANT to legal range */
      if (*newQuant < 1)
         *newQuant = 1;
      else if (*newQuant > 31)
         *newQuant = 31;
   }
   else
   {
      bits = bibGetBits(1,inBuffer,&numBitsGot,bitErrorIndication,
         &bibError);


      if (bits)
      {
         bits = bibGetBits(1,inBuffer,&numBitsGot,bitErrorIndication,
            &bibError);

         *newQuant = quant + changeOfQuant[bits][quant];
      }
      else
      {
         bits = bibGetBits(5,inBuffer,&numBitsGot,bitErrorIndication,
            &bibError);

         *newQuant = bits;
      }
   }

   return VDX_OK;
}


/*
 * vdxGetIntraMode
 *
 * Parameters:
 *    inBuffer                   pointer to bit buffer instance
 *    index                      index to the INTRA_MODE parameter for I-pictures
 *    bitErrorIndication         non-zero if a bit error has been detected
 *                               within the bits accessed in this function,
 *                               see biterr.h for possible values
 *
 * Function:
 *    This function reads the INTRA_MODE field from the MB header..
 *    This field exists iff annex I is in use..
 *
 * Returns:
 *    VDX_OK                     the function was successful
 *    VDX_OK_BUT_BIT_ERROR       the function behaved normally,but a bit error
 *                               occured
 *    VDX_ERR_BIB                an error occured when accessing bit buffer
 *
 */


static int vdxGetIntraMode(bibBuffer_t *inBuffer,int *index,
   int *bitErrorIndication)

{
   int bitsGot,code;
   int16 ownError = 0;

   vdxAssert(inBuffer != NULL);
   vdxAssert(index != NULL);
   vdxAssert(bitErrorIndication != NULL);

   code = (int) bibShowBits(2,inBuffer,&bitsGot,bitErrorIndication,
      &ownError);


   if (code > 1) {
      /* Bitpattern 1x */
      bibFlushBits(2,inBuffer,&bitsGot,bitErrorIndication,&ownError);
      *index = code - 1;
      return VDX_OK;
   }

   bibFlushBits(1,inBuffer,&bitsGot,bitErrorIndication,&ownError);
   *index = 0;
   return VDX_OK;
}


/*
 * Block Layer Local Functions
 */





int vdxChangeBlackAndWhiteHeaderIntraIMB(bibBufferEdit_t *bufEdit, 
                                                 int mcbpcIndex,
                                                 int StartByteIndex, 
                                                 int StartBitIndex)
{
    int k, cur_index, new_index, cur_len, new_len, new_val;
    
    const tVLCTable sCBPCIType[9] = 
    {
    {1, 1}, {1, 3}, {2, 3}, {3, 3}, {1, 4},
    {1, 6}, {2, 6}, {3, 6}, {1, 9}
    };
    
     // evaluate MCBPC parameters
    int cur_cbpc = mcbpcIndex & 3;      
    int new_cbpc = 0;       // cpbc=0 indicates chroma is 0
    int mbType = (mcbpcIndex <4)?3:4;

    // evaluate indices in table
    cur_index = (mbType == 3 ? 0 : 4) + cur_cbpc;   
    new_index = (mbType == 3 ? 0 : 4) + new_cbpc;   
    
    // retrieve values
    cur_len = sCBPCIType[cur_index].length;
    new_len = sCBPCIType[new_index].length;
    new_val = sCBPCIType[new_index].code;
    
    // allocate memory for storing the parameters
    if (!bufEdit->editParams)
    {
                bufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
                bufEdit->numChanges=1;
                bufEdit->copyMode = CopyWithEdit/*CopyWithEdit*/;
    }
    k=bufEdit->numChanges-1;
    bufEdit->copyMode = CopyWithEdit; 
    // store the new parameters
    bufEdit->editParams[k].StartByteIndex = StartByteIndex; 
    bufEdit->editParams[k].StartBitIndex = StartBitIndex; 
    bufEdit->editParams[k].curNumBits = cur_len; 
    bufEdit->editParams[k].newNumBits = new_len; 
    bufEdit->editParams[k].newValue = new_val; 

    return 0;
}

int vdxChangeBlackAndWhiteHeaderInterPMB(bibBufferEdit_t *bufEdit, 
                                                 int mcbpcIndex,
                                                 int StartByteIndex, 
                                                 int StartBitIndex)
{
    int k, cur_index, new_index, cur_len, new_len, new_val;
        
    const tVLCTable sCBPCPType[21] = 
    {
    {1, 1}, {3, 4}, {2, 4}, {5, 6},
    {3, 3}, {7, 7}, {6, 7}, {5, 9}, 
    {2, 3}, {5, 7}, {4, 7}, {5, 8},
    {3, 5}, {4, 8}, {3, 8}, {3, 7},
    {4, 6}, {4, 9}, {3, 9}, {2, 9}, 
    {1, 9}
    };
    
     // evaluate MCBPC parameters
    int cur_cbpc = mcbpcIndex & 3;      
    int new_cbpc = 0;       // cpbc=0 indicates chroma is 0
    int mbType; 

    mbType = mcbpcIndex / 4;
     
    // evaluate indices in table
    cur_index = mbType * 4 + cur_cbpc;  
    new_index = mbType * 4 + new_cbpc;  
    
    // retrieve values
    cur_len = sCBPCPType[cur_index].length;
    new_len = sCBPCPType[new_index].length;
    new_val = sCBPCPType[new_index].code;
    
    // allocate memory for storing the parameters
    if (!bufEdit->editParams)
    {
                bufEdit->editParams = (bibEditParams_t *) malloc(sizeof(bibEditParams_t));
                bufEdit->numChanges=1;
                bufEdit->copyMode = CopyWithEdit/*CopyWithEdit*/; 
    }
    k=bufEdit->numChanges-1;
    bufEdit->copyMode = CopyWithEdit/*CopyWithEdit*/; 
    // store the new parameters
    bufEdit->editParams[k].StartByteIndex = StartByteIndex; 
    bufEdit->editParams[k].StartBitIndex = StartBitIndex; 
    bufEdit->editParams[k].curNumBits = cur_len; 
    bufEdit->editParams[k].newNumBits = new_len; 
    bufEdit->editParams[k].newValue = new_val; 

    return mbType;
}


// End of File
