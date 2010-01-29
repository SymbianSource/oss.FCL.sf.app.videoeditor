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
* Symbian OS -specific IDCT routines.
*
*/



/*

    1  ABSTRACT


    1.1 Module Type

    type  subroutine


    1.2 Functional Description

    Fixed point arithmetic for fast calculation of IDCT for 8x8 size image
    blocks. The full calculation of the IDCT takes 208 multiplications and
    400 additions.

    The routine optionally checks if AC coefficients are all zero and in that
    case makes a shortcut in IDCT, thus making the calculation faster. This
    feature is activated by defining CHECK_ZERO_AC_COEFFICIENTS_0,
    CHECK_ZERO_AC_COEFFICIENTS_1 or CHECK_ZERO_AC_COEFFICIENTS_2 in envdef.h.


    1.3 Specification/Design Reference

    The algorithm used is the fast algorithm introduced in W.-H. Chen, C. H.
    Smith, and S. C. Fralick, "A fast computational algorithm for the
    Discrete Cosine Transform," IEEE Transactions on Communications, vol.
    COM-25, pp. 1004-1009, 1977.

    This IDCT routine conforms to the accuracy requirements specified in the
    H.261 and H.263 recommendations.

    NRC documentation: Moments: H.263 Decoder - Functional Definition
    Specification

    NRC documentation: Moments: H.263 Decoder - Implementation Design
    Specification


    1.4 Module Test Specification Reference

    TRABANT:h263_test_spec.BASE-TEST


    1.5 Compilation Information

        Compiler:      RISC OS ARM C compiler (Acorn Computers Ltd.)
        Version:       5.05
        Activation:    cc -DBUILD_TARGET=ACORNDEC idcti.c


    1.6 Notes

    This source code can be used in both 16-bit and 32-bit application.

    PREC defines the precision for the fixed point numbers. The best value
    for it depends on several things: You should always have enough room for
    the integer part of the number, and in 386s/486s smaller PREC values are
    faster, but the smaller it is, the poorer is the accuracy.

    The TMPPRECDEC is another adjustable constant. It tells how many bits are
    ripped off the number for the temporary storage. This way the accuracy
    for the multiplications can be better. TMPPREC can be zero. This speeds
    the code up a bit.

    To determine the maximum values for PREC and TMPPRECDEC, count bits you
    need for the integer part anywhere during the calculation, substract that
    from 32, and divide the remaining number by two. This number should be
    >= (2*PREC-TMPPRECDEC)/2, otherwise the results may be corrupted due to
    lost bits.

    For example, if you know that your data will vary from -2048 - 2047, you
    need twelve bits for the integer part. 32-12 = 20, 20 / 2 = 10, so good
    example values for PREC and TMPPRECDEC would be 12 and 4. Also 11 and 2
    would be legal, as would 11 and 3, but 12 and 3 would not ((2*12-3)/2 =
    10.5 > 10).

    NOTE: Several PREC and TMPPRECDEC values were tried in order to meet
    the .
    The requirements could not be met. PREC = 13, TMPPRECDEC = 5 was 
    the closest combination to meet the requirements violating only 
    the overall mean square error requirement.     

    Both the input and output tables are assumed to be normal C ints. Thus,
    in the 16-bit version they are 16-bit integers and in the 32-bit version
    32-bit ones.


    Define CHECK_ZERO_AC_COEFFICIENTS_0, CHECK_ZERO_AC_COEFFICIENTS_1 and
    CHECK_ZERO_AC_COEFFICIENTS_2 in envdef.h if zero AC coefficients checking
    for the whole block, for current row or for current column is desired,
    respectively.
        

*/


/*  2  CONTENTS


        1  ABSTRACT

        2  CONTENTS

        3  GLOSSARY

        4  EXTERNAL RESOURCES
        4.1  Include Files
        4.2  External Data Structures
        4.3  External Function Prototypes

        5  LOCAL CONSTANTS AND MACROS

        6  MODULE DATA STRUCTURES
        6.1  Local Data Structures
        6.2  Local Function Prototypes

        7  MODULE CODE
        7.1  idct
        7.2  firstPass
        7.3  secondPass

*/


/*  3  GLOSSARY

    IDCT        Inverse discrete cosine transform

*/


/*  4  EXTERNAL RESOURCES  */


/*  4.1  Include Files  */

#include "h263dconfig.h"

/*  4.2 External Data Structures  */

    /* None */


/*  4.3 External Function Prototypes  */

    /* None */


/*  5  LOCAL CONSTANTS AND MACROS  */

#define PREC        13      /* Fixed point precision */
#define TMPPRECDEC  5      /* Temporary precision decrease */

    /* See note about PREC and TMPPRECDEC above. */

#define TMPPREC     ( PREC - TMPPRECDEC )
#define CDIV        ( 1 << ( 16 - PREC ))
#define ROUNDER     ( 1 << ( PREC - 1 ))

#define f0  (int32)(0xb504 / CDIV)  /* .7071068 = cos( pi / 4 )         */
#define f1  (int32)(0x7d8a / CDIV)  /* .4903926 = 0.5 * cos( 7pi / 16 ) */
#define f2  (int32)(0x7641 / CDIV)  /* .4619398 = 0.5 * cos( 6pi / 16 ) */
#define f3  (int32)(0x6a6d / CDIV)  /* .4157348 = 0.5 * cos( 5pi / 16 ) */
#define f4  (int32)(0x5a82 / CDIV)  /* .3535534 = 0.5 * cos( 4pi / 16 ) */
#define f5  (int32)(0x471c / CDIV)  /* .2777851 = 0.5 * cos( 3pi / 16 ) */
#define f6  (int32)(0x30fb / CDIV)  /* .1913417 = 0.5 * cos( 2pi / 16 ) */
#define f7  (int32)(0x18f8 / CDIV)  /* .0975452 = 0.5 * cos(  pi / 16 ) */

#define f0TMP   (int32)(0xb504 / (1 << (16 - TMPPREC)))



/*  6  MODULE DATA STRUCTURES  */


/*  6.1 Local Data Structures  */

#ifdef _WIN32_EXPLICIT  /* EPOC32_PORT  static data */

static const int    idctZigzag[64] =    /* array of zig-zag positioning */
{  0,  1,  5,  6, 14, 15, 27, 28,       /* of transform coefficients    */
   2,  4,  7, 13, 16, 26, 29, 42,
   3,  8, 12, 17, 25, 30, 41, 43,
   9, 11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63 };

static int32    idctTmpbuf1[64];        /* array for temporary storage of
                                        transform results */
#endif

/*  6.2 Local Function Prototypes  */

static void firstPass   (int *buffer,
                         int32 *tmpbuf);

static void secondPass  (int32 *tmpbuf,
                         int *dest);



/*  7 MODULE CODE  */

/*
=============================================================================
*/

/*  7.1  */

void idct
       (int     *block)

    {


/*  Functional Description

    Fixed point arithmetic for fast calculation of IDCT for 8x8 size image
    blocks.


    Activation

    by function call

    Reentrancy: no


    Inputs

    Parameters:

    *block:         8x8 source block of zigzagged cosine transform
                    coefficients

    Externals:

    None


    Outputs

    Parameters:

    *block:         8x8 destination block of pixel values

    Externals:

    None

    Return Values:

    None


    Exceptional Conditions

    None

-----------------------------------------------------------------------------
*/



/*  Pseudocode

    Calculate 1D-IDCT by rows.
    Calculate 1D-IDCT by columns.

*/



/*  Data Structures  */


/* These are only needed if checking the AC coefficients of the whole block
   is desired. */

    int i = 1;          /* Loop variable */
    int result;         /* Calculation result */


#ifndef _WIN32_EXPLICIT  /* EPOC32_PORT  static data */
    int32     idctTmpbuf1[64];
#endif

/*  Code  */


    /*
     *  Check if the AC coefficients of the whole block are all zero.
     *  In that case the inverse transform is equal to the DC
     *  coefficient with a scale factor.
     */

        while (i < 64 && !block[i++]) {}
        if (i == 64) {
            int *blk = block;
            result = (block[0] + 4) >> 3;
            i = 8;
            while ( i-- )
            {
                blk[0] = result; blk[1] = result; blk[2] = result; blk[3] = result;
                blk[4] = result; blk[5] = result; blk[6] = result; blk[7] = result;
                blk += 8;
            }
            /*
            for (i = 0; i < 64; i++)
            {
                block[i] = result;
            }
            */
        }
        else
        {
            firstPass(block, idctTmpbuf1);
            secondPass(idctTmpbuf1, block);
        }


    }

/*
=============================================================================
*/



/*  7.2  */

static void firstPass
        (int     *buffer,
         int32   *tmpbuf)

    {


/*  Functional Description

    Local function: Calculate 1D-IDCT for the rows of the 8x8 block.


    Activation

    by function call

    Reentrancy: no


    Inputs

    Parameters:

    *block:         8x8 block of cosine transform coefficients

    Externals:

    None


    Outputs

    Parameters:

    *tmpbuf         Temporary storage for the results of the first pass.

    Externals:

    None

    Return Values:

    None


    Exceptional Conditions

    None

-----------------------------------------------------------------------------
*/



/*  Pseudocode

    Calculate 1D-IDCT by rows.

*/



/*  Data Structures  */

    int     row;                    /* Loop variable */
    int32   e, f, g, h;             /* Temporary storage */
    int32   t0, t1, t2, t3, t5, t6; /* Temporary storage */
    int32   bd2, bd3;               /* Temporary storage */

#ifndef _WIN32_EXPLICIT  /* EPOC32_PORT  static data */
    static const int     idctZigzag[64] =
{  0,  1,  5,  6, 14, 15, 27, 28,
   2,  4,  7, 13, 16, 26, 29, 42,
   3,  8, 12, 17, 25, 30, 41, 43,
   9, 11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63 };

    const int *zz = idctZigzag;
#else
    int     *zz = idctZigzag;
#endif


/*  Code  */

#define ZZ(x) ((int32)buffer[zz[x]])

        for( row = 0; row < 8; row++ )
        {


        /*
         *  Check if the AC coefficients on the current row are all zero.
         *  In that case the inverse transform is equal to the DC
         *  coefficient with a scale factor.
         */

            if ((ZZ(1) | ZZ(2) | ZZ(3) | ZZ(4) | ZZ(5) | ZZ(6) | ZZ(7)) == 0)
            {
                tmpbuf[7] =
                tmpbuf[6] =
                tmpbuf[5] =
                tmpbuf[4] =
                tmpbuf[3] =
                tmpbuf[2] =
                tmpbuf[1] =
                tmpbuf[0] = (ZZ(0) * f4) >> TMPPRECDEC;

                tmpbuf += 8;
                zz += 8;
                continue;
            }

            t0 = t3 = (ZZ(0) + ZZ(4)) * f4;
            bd3 = ZZ(6) * f6 + ZZ(2) * f2;
            t0 += bd3;
            t3 -= bd3;

            t1 = t2 = (ZZ(0) - ZZ(4)) * f4;
            bd2 = ZZ(2) * f6 - ZZ(6) * f2;
            t1 += bd2;
            t2 -= bd2;

            e = h = (ZZ(1) + ZZ(7)) * f7;
            h += ZZ(1) * ( -f7+f1 );
            f = g = (ZZ(5) + ZZ(3)) * f3;
            g += ZZ(5) * ( -f3+f5 );

            tmpbuf[0] = ( t0 + ( h + g )) >> TMPPRECDEC;
            tmpbuf[7] = ( t0 - ( h + g )) >> TMPPRECDEC;

            f += ZZ(3) * ( -f3-f5 );
            e += ZZ(7) * ( -f7-f1 );

            tmpbuf[3] = ( t3 + ( e + f )) >> TMPPRECDEC;
            tmpbuf[4] = ( t3 - ( e + f )) >> TMPPRECDEC;

            t6 = ( h - g + e - f ) * f0TMP >> TMPPREC;
            t5 = ( h - g - e + f ) * f0TMP >> TMPPREC;

            tmpbuf[1] = ( t1 + t6 ) >> TMPPRECDEC;
            tmpbuf[6] = ( t1 - t6 ) >> TMPPRECDEC;
            tmpbuf[2] = ( t2 + t5 ) >> TMPPRECDEC;
            tmpbuf[5] = ( t2 - t5 ) >> TMPPRECDEC;

            tmpbuf += 8;
            zz += 8;
        }
    }

#undef  ZZ


/*
=============================================================================
*/



/*  7.3  */

static void secondPass
        (int32   *tmpbuf,
         int     *dest)

    {


/*  Functional Description

    Local function: Calculate 1D-IDCT for the columns of the 8x8 block.


    Activation

    by function call

    Reentrancy: no


    Inputs

    Parameters:

    *tmpbuf         Temporary storage for the results of the first pass.

    Externals:

    None


    Outputs

    Parameters:

    *block:         8x8 block of pixel values

    Externals:

    None

    Return Values:

    None


    Exceptional Conditions

    None

-----------------------------------------------------------------------------
*/


/*  Pseudocode

    Calculate 1D-IDCT by columns.

*/


/*  Data Structures  */

    int     col;                    /* Loop variable */
    int32   e, f, g, h;             /* Temporary storage */
    int32   t0, t1, t2, t3, t5, t6; /* Temporary storage */
    int32   bd2, bd3;               /* Temporary storage */


/*  Code  */

#define ZZ(x) tmpbuf[x * 8]

        for( col = 0; col < 8; col++ )
        {

            t0 = t3 = ((ZZ(0) + ZZ(4)) * f4 ) >> TMPPREC;
            bd3 = ( ZZ(6) * f6 + ZZ(2) * f2 ) >> TMPPREC;
            t0 += bd3;
            t3 -= bd3;

            t1 = t2 = ((ZZ(0) - ZZ(4)) * f4 ) >> TMPPREC;
            bd2 = ( ZZ(2) * f6 - ZZ(6) * f2 ) >> TMPPREC;
            t1 += bd2;
            t2 -= bd2;

            e = h = (ZZ(1) + ZZ(7)) * f7;
            h += (ZZ(1) * ( -f7+f1 ));
            h >>= TMPPREC;
            f = g = (ZZ(5) + ZZ(3)) * f3;
            g += (ZZ(5) * ( -f3+f5 ));
            g >>= TMPPREC;

            dest[0*8] = (int) (( t0 + ( h + g ) + ROUNDER ) >> PREC);
            dest[7*8] = (int) (( t0 - ( h + g ) + ROUNDER ) >> PREC);

            f += ZZ(3) * ( -f3-f5 );
            f >>= TMPPREC;
            e += ZZ(7) * ( -f7-f1 );
            e >>= TMPPREC;

            dest[3*8] = (int) (( t3 + ( e + f ) + ROUNDER ) >> PREC);
            dest[4*8] = (int) (( t3 - ( e + f ) + ROUNDER ) >> PREC);

            t6 = ( h - g + e - f ) * f0TMP >> TMPPREC;
            t5 = ( h - g - e + f ) * f0TMP >> TMPPREC;

            dest[1*8] = (int) (( t1 + t6 + ROUNDER ) >> PREC);
            dest[6*8] = (int) (( t1 - t6 + ROUNDER ) >> PREC);
            dest[2*8] = (int) (( t2 + t5 + ROUNDER ) >> PREC);
            dest[5*8] = (int) (( t2 - t5 + ROUNDER ) >> PREC);

            tmpbuf++;
            dest++;
        }
    }


/*
=============================================================================
*/

// End of File
