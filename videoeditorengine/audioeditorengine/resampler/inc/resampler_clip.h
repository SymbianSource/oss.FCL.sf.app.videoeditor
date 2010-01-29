#ifndef __RESAMPLER_CLIP_H__
#define __RESAMPLER_CLIP_H__
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


#include "resampler_common_defs.h"
#include "resampler_data_types.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file 

  @ingroup common_dsp

  Helper functions to limit output to different numerical ranges.

  The header file 'resampler_clip.h' contains helper (inline) functions for 
  limiting the input to a specified numerical range, or more specifically,
  to a specified number of bits. These functions are especially useful in 
  fixed point DSP units for limiting the output range.
*/

/** Clip function for signed 16-bit integer range.

  This function limits its input to the range -32768 <= output <= 32767.
  The implementation uses the traditional conditionals to check for 
  overflows..
*/
static FORCEINLINE int32 RESAMPLER_Clip16(int32 input)
{
    input = input < (-32768) ? (-32768) : input;
    input = input > 32767 ?  32767 : input;
    return input;
}

/** Shift-and-Clip function for signed 16-bit integer range.

  This function limits its input to the range -32768 <= output <= 32767
  after first shifting the input to the right by the given number of bits.
*/
static FORCEINLINE int32 RESAMPLER_RightShiftAndClip16(int32 input, int rightShift)
{
    input = input >> rightShift;
    return RESAMPLER_Clip16(input);
}

/** Shift-round-and-clip function for signed 16-bit integer range.

  This function limits its input to the range -32768 <= output <= 32767
  after first shifting the input to the right by the given number of bits
  with rounding.
*/
static FORCEINLINE int32 RESAMPLER_RightShiftRoundAndClip16(int32 input, int rightShift)
{
    if (rightShift > 0)
    {
        input += (int32)1 << (rightShift - 1);
    }
    input = input >> rightShift;
    return RESAMPLER_Clip16(input);
}

/** Saturated 16-bit absolute value.

  This function calculates a saturated 16-bit output value from a 16-bit 
  input RESAMPLER_AbsAndClip16(0x8000) => 0x7FFF.
*/
static FORCEINLINE int16 RESAMPLER_AbsSaturate16(int16 input)
{
    return (int16)RESAMPLER_Clip16(labs((long)input));
}

/** Saturated addition of two 16-bit values.
*/
static FORCEINLINE int16 RESAMPLER_AddSaturate16(int16 input1, int16 input2)
{
    return (int16)RESAMPLER_Clip16((int32)input1 + input2);
}


/** Saturated subtraction of two 16-bit values.
*/
static FORCEINLINE int16 RESAMPLER_SubSaturate16(int16 input1, int16 input2)
{
    return (int16)RESAMPLER_Clip16((int32)input1 - input2);
}

/** Saturated addition of two 32-bit values.
*/
static FORCEINLINE int32 RESAMPLER_AddSaturate32(int32 input1, int32 input2)
{
    /* int40 is the shortest "long long" available on all 
     * the current platforms 
     */
    int40 res = input1;
    res += input2;
    if (res > (int40)(2147483647L))
    {
        res = (int40)(2147483647L);
    }
    if (res < (int40)(-2147483647L - 1))
    {
        res = (int40)(-2147483647L - 1);
    }
    return (int32)res;
}

/** Saturated subtraction of two 32-bit values.
*/
static FORCEINLINE int32 RESAMPLER_SubSaturate32(int32 input1, int32 input2)
{
    int40 res = input1;
    res -= input2;
    if (res > (int40)(2147483647L))
    {
        res = (int40)(2147483647L);
    }
    if (res < (int40)(-2147483647L - 1))
    {
        res = (int40)(-2147483647L - 1);
    }
    return (int32)res;
}

#ifdef __cplusplus
}
#endif

#endif  /* __RESAMPLER_CLIP_H__ */
