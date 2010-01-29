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


/**************************************************************************
layer3.cpp - MPEG-1, MPEG-2 LSF and MPEG-2.5 layer III bitstream parsing.

  Author(s): Juha Ojanpera
  Copyright (c) 1999-2004 by Nokia Research Center, Speech and Audio Systems.
*************************************************************************/

/**************************************************************************
External Objects Needed
*************************************************************************/

/*-- Project Headers --*/
#include "mpaud.h"
#include "mpheader.h"
#include "mp3tables.h"



//uint8 scalefac_buffer[54];

/**************************************************************************
Title        : III_get_side_info

  Purpose      : Reads Layer III side information from the bitstream.
  
    Usage        : y = III_get_side_info(mp, bs)
    
      Input        : mp - MP3 decoder handle
      bs - input bitstream
      
        Output       : y - TRUE on success, FALSE otherwise
        
          Author(s)    : Juha Ojanpera
*************************************************************************/


BOOL
III_get_side_info(CMP_Stream *mp, TBitStream *bs)
{
    uint16 i, j, k, flag;
    TGranule_Info *gr_info;
    
    /*-- Get the protected bits for the crc error checking. --*/
    if(bs->dsp_buffer)
    {
        int16 sideLen = (GetSideInfoSlots(mp->header) >> 1) + 1;
        
        if(bs->bit_counter == 8) 
            for(i = bs->buf_index, j = 0; j < sideLen; i++, j++)
            {
                DSP_BYTE wTmp = bs->dsp_buffer[MOD_OPCODE(i, bs->buf_mask)] << 8;
                wTmp |= (bs->dsp_buffer[MOD_OPCODE((i + 1), bs->buf_mask)] >> 8) & 255;
                mp->mp3_crc.crc_payload[j + 2] = wTmp;
            }
            else
                for(i = bs->buf_index, j = 0; j < sideLen; i++, j++)
                    mp->mp3_crc.crc_payload[j + 2] = bs->dsp_buffer[MOD_OPCODE(i, bs->buf_mask)];
    }
    
    if(version(mp->header) == MPEG_AUDIO_ID) /* MPEG-1 */
    {
        mp->side_info->main_data_begin = BsGetBits(bs, 9);
        if(channels(mp->header) == 1)
        {
            mp->side_info->private_bits = BsGetBits(bs, 5);
            for(i = 0; i < 4; i++)
                mp->side_info->scfsi[0][i] = BsGetBits(bs, 1);
        }
        else
        {
            mp->side_info->private_bits = BsGetBits(bs, 3);
            for(i = 0; i < 2; i++)
                for(j = 0; j < 4; j++)
                    mp->side_info->scfsi[i][j] = int16(BsGetBits(bs, 1));
        }
        
        for(i = 0; i < 2; i++)
        {
            for(j = 0; j < channels(mp->header); j++)
            {
                gr_info = mp->side_info->ch_info[j]->gr_info[i];
                
                gr_info->part2_3_length = BsGetBits(bs, 12);
                gr_info->big_values = BsGetBits(bs, 9) << 1;
                gr_info->global_gain = BsGetBits(bs, 8);
                gr_info->scalefac_compress = BsGetBits(bs, 4);
                
                gr_info->flags = 0;
                gr_info->flags |= (BsGetBits(bs, 1)) ? WINDOW_SWITCHING_FLAG : 0;
                
                if(win_switch(gr_info))
                {
                    /* block_type */
                    gr_info->flags |= BsGetBits(bs, 2);
                    
                    /* mixed_block_flag */
                    flag = BsGetBits(bs, 1);
                    gr_info->flags |= (flag) ? MIXED_BLOCK_FLAG : 0;
                    
                    for(k = 0; k < 2; k++)
                        gr_info->table_select[k] = BsGetBits(bs, 5);
                    
                    for(k = 0; k < 3; k++)
                        gr_info->subblock_gain[k] = BsGetBits(bs, 3);
                    
                    /* Set region_count parameters since they are implicit in this case. */
                    if((gr_info->flags & 3) == 0)
                        return (FALSE); /* This will trigger resync */
                    else
                        if(short_block(gr_info) && !mixed_block(gr_info))
                        {
                            gr_info->block_mode = SHORT_BLOCK_MODE;
                            gr_info->region0_count = 7;
                        }
                        else
                        {
                            if(short_block(gr_info) && mixed_block(gr_info))
                                return (FALSE); /* mixed block are not allowed => resync */
                            else
                                gr_info->block_mode = LONG_BLOCK_MODE;
                            
                            gr_info->region0_count = 7;
                        }
                        
                    gr_info->region1_count = 20 - gr_info->region0_count;
                }
                else
                {
                    gr_info->block_mode = LONG_BLOCK_MODE;
                    for(k = 0; k < 3; k++)
                        gr_info->table_select[k] = BsGetBits(bs, 5);
                    gr_info->region0_count = BsGetBits(bs, 4);
                    gr_info->region1_count = BsGetBits(bs, 3);
                    
                    gr_info->flags &= ~(uint32)3; /* block_type == 0 (LONG) */
                }
                
                flag = BsGetBits(bs, 3);
                gr_info->flags |= (flag & 4) ? PRE_FLAG : 0;
                gr_info->flags |= (flag & 2) ? SCALEFAC_SCALE : 0;
                gr_info->flags |= (flag & 1) ? COUNT_1_TABLE_SELECT : 0;
            }
        }
    }
    else /* MPEG-2 LSF and MPEG-2.5 */
    {
        mp->side_info->main_data_begin = BsGetBits(bs, 8);
        
        if(channels(mp->header) == 1)
            mp->side_info->private_bits = BsGetBits(bs, 1);
        else
            mp->side_info->private_bits = BsGetBits(bs, 2);
        
        for(i = 0; i < channels(mp->header); i++)
        {
            gr_info = mp->side_info->ch_info[i]->gr_info[0];
            
            gr_info->part2_3_length = BsGetBits(bs, 12);
            gr_info->big_values = BsGetBits(bs, 9) << 1;
            gr_info->global_gain = BsGetBits(bs, 8);
            gr_info->scalefac_compress = BsGetBits(bs, 9);
            gr_info->flags = 0;
            gr_info->flags |= (BsGetBits(bs, 1)) ? WINDOW_SWITCHING_FLAG : 0;
            
            if(win_switch(gr_info))
            {
                /* block_type */
                gr_info->flags |= BsGetBits(bs, 2);
                
                /* mixed_block_flag */
                flag = BsGetBits(bs, 1);
                gr_info->flags |= (flag) ? MIXED_BLOCK_FLAG : 0;
                
                for(k = 0; k < 2; k++)
                    gr_info->table_select[k] = BsGetBits(bs, 5);
                
                for(k = 0; k < 3; k++)
                    gr_info->subblock_gain[k] = BsGetBits(bs, 3);
                
                /* Set region_count parameters since they are implicit in this case. */
                if((gr_info->flags & 3) == 0)
                    return (FALSE);
                else
                    if(short_block(gr_info) && !mixed_block(gr_info))
                    {
                        gr_info->block_mode = SHORT_BLOCK_MODE;
                        gr_info->region0_count = 5;
                    }
                    else
                    {
                        if(short_block(gr_info) && mixed_block(gr_info))
                            return (FALSE);
                        else
                            gr_info->block_mode = LONG_BLOCK_MODE;
                        
                        gr_info->region0_count = 7;
                    }
                    
                gr_info->region1_count = 20 - gr_info->region0_count;
            }
            else
            {
                gr_info->block_mode = LONG_BLOCK_MODE;
                for(k = 0; k < 3; k++)
                    gr_info->table_select[k] = BsGetBits(bs, 5);
                gr_info->region0_count = BsGetBits(bs, 4);
                gr_info->region1_count = BsGetBits(bs, 3);
                
                gr_info->flags &= ~(uint32)3; /* block_type == 0 (LONG) */
            }
            
            flag = BsGetBits(bs, 2);
            gr_info->flags |= (flag & 2) ? SCALEFAC_SCALE : 0;
            gr_info->flags |= (flag & 1) ? COUNT_1_TABLE_SELECT : 0;
        }
    }
    
    return (TRUE);
}

/*
* Writes layer 3 side information (excluding granule specific info) 
* to the specified bitstream. For writing granule specific 
* parameters 'L3WriteGranule()' function should be used.
*/
INLINE void
L3WriteCommonSideInfo(TBitStream *bs, CIII_Side_Info *sideInfo, TMPEG_Header *header)
{
    int16 bits;
    uint32 dWord;
    
    bits = 9;
    dWord = sideInfo->main_data_begin;
    
    /*-- MPEG-1. --*/
    if(version(header) == MPEG_AUDIO_ID)
    {
        if(channels(header) == 1)
        {
            bits = 18;
            dWord <<= 5; dWord |= sideInfo->private_bits;
            dWord <<= 1; dWord |= sideInfo->scfsi[0][0];
            dWord <<= 1; dWord |= sideInfo->scfsi[0][1];
            dWord <<= 1; dWord |= sideInfo->scfsi[0][2];
            dWord <<= 1; dWord |= sideInfo->scfsi[0][3];
        }
        else
        {
            bits = 20;
            dWord <<= 3; dWord |= sideInfo->private_bits;
            dWord <<= 1; dWord |= sideInfo->scfsi[0][0];
            dWord <<= 1; dWord |= sideInfo->scfsi[0][1];
            dWord <<= 1; dWord |= sideInfo->scfsi[0][2];
            dWord <<= 1; dWord |= sideInfo->scfsi[0][3];
            dWord <<= 1; dWord |= sideInfo->scfsi[1][0];
            dWord <<= 1; dWord |= sideInfo->scfsi[1][1];
            dWord <<= 1; dWord |= sideInfo->scfsi[1][2];
            dWord <<= 1; dWord |= sideInfo->scfsi[1][3];
        }
    }
    
    /*-- MPEG-2 LSF, MPEG-2.5. --*/
    else
    {
        if(channels(header) == 1)
        { bits = 9;  dWord <<= 1; }
        else 
        { bits = 10; dWord <<= 2; }
        
        dWord |= sideInfo->private_bits;
    }
    
    BsPutBits(bs, bits, dWord);
}

/*
* Writes granule specific parameters to the specified bitstream.
* Please note that the MPEG frame may contain more than one granule
* depending on the number of channels and version. So this function
* may need to be called multiple times.
*/
INLINE void
L3WriteGranule(TBitStream *bs, TGranule_Info *gr_info, BOOL mpeg1)
{
    uint32 dWord;
    
    dWord = gr_info->part2_3_length << 9;
    dWord |= gr_info->big_values >> 1;
    dWord <<= 8; dWord |= gr_info->global_gain;
    
    BsPutBits(bs, 29, dWord);
    
    dWord = gr_info->scalefac_compress << 1;
    if(win_switch(gr_info))
    {
        dWord |= 1;
        dWord <<= 2; dWord |= (gr_info->flags & 3);
        dWord <<= 1; if(mixed_block(gr_info)) dWord |= 1;
        dWord <<= 5; dWord |= gr_info->table_select[0];
        dWord <<= 5; dWord |= gr_info->table_select[1];
        dWord <<= 3; dWord |= gr_info->subblock_gain[0];
        dWord <<= 3; dWord |= gr_info->subblock_gain[1];
        dWord <<= 3; dWord |= gr_info->subblock_gain[2];
    }
    else
    {
        dWord <<= 5; dWord |= gr_info->table_select[0];
        dWord <<= 5; dWord |= gr_info->table_select[1];
        dWord <<= 5; dWord |= gr_info->table_select[2];
        dWord <<= 4; dWord |= gr_info->region0_count; 
        dWord <<= 3; dWord |= gr_info->region1_count;
    }
    
    if(mpeg1) { dWord <<= 1; if(pre_flag(gr_info)) dWord |= 1; }
    else      { BsPutBits(bs, 32, dWord); dWord = 0; }
    
    dWord <<= 1; if(scalefac_scale(gr_info)) dWord |= 1;
    dWord <<= 1; if(gr_info->flags & COUNT_1_TABLE_SELECT) dWord |= 1;
    
    BsPutBits(bs, (mpeg1) ? 30 : 2, dWord);
}

/*
* Writes layer 3 side info to the specified bitstream. The side info
* in this context includes the header and the actual side info
* parameters.
*/
void
L3WriteSideInfo(TBitStream *bs, CIII_Side_Info *sideInfo, TMPEG_Header *header)
{
    BOOL mpeg1;
    int16 i, j, max_gr;
    
    mpeg1 = (version(header) == MPEG_AUDIO_ID) ? TRUE : FALSE;
    max_gr = (mpeg1) ? 2 : 1;
    
    /*-- Write common side info. --*/
    L3WriteCommonSideInfo(bs, sideInfo, header);
    
    /*-- Write granule parameters. --*/
    for(i = 0; i < max_gr; i++)
        for(j = 0; j < channels(header); j++)
        {
            TGranule_Info *gr_info = sideInfo->ch_info[j]->gr_info[i];
            L3WriteGranule(bs, gr_info, mpeg1);
        }
}

/**************************************************************************
Title        : III_get_LSF_scale_data

  Purpose      : Decodes scalafactors of MPEG-2 LSF and MPEG-2.5 bitstreams.
  
    Usage        : III_get_LSF_scale_data(mp, gr, ch)
    
      Input        : mp - MP3 stream parameters
      gr - granule number
      ch - channel number (left or right)
      
        Explanation  : -
        
          Author(s)    : Juha Ojanpera
*************************************************************************/

static void
III_get_LSF_scale_data(CMP_Stream *mp, int16 gr, int16 ch, uint8* scalefac_buffer)
{
    int16 i, j, k, m = 0;
    int16 blocktypenumber = 0, blocknumber = 0;
    int16 scalefac_comp, int_scalefac_comp, new_slen[4] = {0, 0, 0, 0};
    TGranule_Info *gr_info;
    
    gr_info = mp->side_info->ch_info[ch]->gr_info[gr];
    scalefac_comp = gr_info->scalefac_compress;
    
    switch(gr_info->block_mode)
    {
    case SHORT_BLOCK_MODE:
        blocktypenumber = 1;
        break;
        
    case LONG_BLOCK_MODE:
        blocktypenumber = 0;
        break;
        
    default:
        break;
    }
    
    if(!((mode_extension(mp->header) == 1 || 
        mode_extension(mp->header) == 3) && ch == 1))
    {    
        
        if(scalefac_comp < 400)
        {
            new_slen[0] = (scalefac_comp >> 4) / 5;
            new_slen[1] = (scalefac_comp >> 4) % 5;
            new_slen[2] = (scalefac_comp & 15) >> 2;
            new_slen[3] = (scalefac_comp & 3);
            
            blocknumber = 0;
            m = 4;
        }
        else if(scalefac_comp < 500)
        {
            scalefac_comp -= 400;
            
            new_slen[0] = (scalefac_comp >> 2) / 5;
            new_slen[1] = (scalefac_comp >> 2) % 5;
            new_slen[2] = scalefac_comp & 3;
            
            blocknumber = 1;
            m = 3;
        }
        else /*if(scalefac_comp < 512)*/
        {
            scalefac_comp -= 500;
            
            new_slen[0] = scalefac_comp / 3;
            new_slen[1] = scalefac_comp % 3;
            
            gr_info->flags |= (uint32)PRE_FLAG; /* pre_flag = 1 */
            
            blocknumber = 2;
            m = 2;
        }
    }
    
    if(((mode_extension(mp->header) == 1 || 
        mode_extension(mp->header) == 3) && ch == 1))
    {
        
        int_scalefac_comp = scalefac_comp >> 1;
        
        if(int_scalefac_comp < 180)
        {
            int16 tmp = int_scalefac_comp % 36;
            
            new_slen[0] = int_scalefac_comp / 36;
            new_slen[1] = tmp / 6;
            new_slen[2] = tmp % 6;
            
            blocknumber = 3;
            m = 3;
        }
        else if(int_scalefac_comp < 244)
        {
            int_scalefac_comp -= 180;
            
            new_slen[0] = (int_scalefac_comp & 63) >> 4;
            new_slen[1] = (int_scalefac_comp & 15) >> 2;
            new_slen[2] = int_scalefac_comp & 3;
            
            blocknumber = 4;
            m = 3;
        }
        else /*if(int_scalefac_comp < 255)*/
        {
            int_scalefac_comp -= 244;
            
            new_slen[0] = int_scalefac_comp / 3;
            new_slen[1] = int_scalefac_comp % 3;
            
            blocknumber = 5;
            m = 2;
        }
        
        TIS_Info *is_info = &mp->side_info->is_info;
        is_info->is_len[0] = (1 << new_slen[0]) - 1;
        is_info->is_len[1] = (1 << new_slen[1]) - 1;
        is_info->is_len[2] = (1 << new_slen[2]) - 1;
        is_info->nr_sfb[0] = nr_of_sfb_block[blocknumber][blocktypenumber][0];
        is_info->nr_sfb[1] = nr_of_sfb_block[blocknumber][blocktypenumber][1];
        is_info->nr_sfb[2] = nr_of_sfb_block[blocknumber][blocktypenumber][2];
        
    }
    
    
    Mem::Fill(scalefac_buffer, 54, 0);
    
    
    for(i = k = 0; i < m; i++)
    {
        
        if(new_slen[i] != 0)
            for(j = 0; j < nr_of_sfb_block[blocknumber][blocktypenumber][i]; j++, k++)
            {
                
                
                
                scalefac_buffer[k] = BsGetBits(mp->br, new_slen[i]);
                
                
            }
            else
            {
                
                
                if (i < 4)
                {
                    k += nr_of_sfb_block[blocknumber][blocktypenumber][i];
                }
                else
                {
                    k +=  0;
                }            
                
            }      
            
    }
    
}

/**************************************************************************
Title        : III_get_scale_factors

Purpose      : Reads the scale factors of layer III.
  
Usage        : III_get_scale_factors(mp, gr, ch)
    
Input        : mp - MP3 stream parameters
gr - granule number
ch - channel number (left or right)
      
Author(s)    : Juha Ojanpera
*************************************************************************/
        
void III_get_scale_factors(CMP_Stream *mp, int16 gr, int16 ch)
    {
    uint8 *sf[3];
    int16 i, sfb, bits, idx;
    TGranule_Info *gr_info;
    CIII_Scale_Factors *scale_fac;
    
    uint8* scalefac_buffer = NULL;
    TRAPD(error, scalefac_buffer = new (ELeave) uint8[54]);
    if (error != KErrNone)
        return;
    
    Mem::Fill(scalefac_buffer, 54, 0);
    
    gr_info = mp->side_info->ch_info[ch]->gr_info[gr];
    
    scale_fac = mp->side_info->ch_info[ch]->scale_fac;
    
    idx = 0;
    if(mp->side_info->lsf)
        {
        III_get_LSF_scale_data(mp, gr, ch, scalefac_buffer);
        }
        
    
    switch(gr_info->block_mode)
    {
    case SHORT_BLOCK_MODE:
        sf[0] = scale_fac->scalefac_short[0];
        sf[1] = scale_fac->scalefac_short[1];
        sf[2] = scale_fac->scalefac_short[2];
        if(mp->side_info->lsf)
            for(sfb = 0; sfb < 12; sfb++)
            {
                *sf[0]++ = scalefac_buffer[idx++];
                *sf[1]++ = scalefac_buffer[idx++];
                *sf[2]++ = scalefac_buffer[idx++];
            }
            else
                for(i = 0; i < 2; i++)
                {
                    bits = slen[i][gr_info->scalefac_compress];
                    if(bits)
                        for(sfb = sfbtable.s[i]; sfb < sfbtable.s[i + 1]; sfb++)
                        {
                            *sf[0]++ = BsGetBits(mp->br, bits);
                            *sf[1]++ = BsGetBits(mp->br, bits);
                            *sf[2]++ = BsGetBits(mp->br, bits);
                        }
                        else
                            for(sfb = sfbtable.s[i]; sfb < sfbtable.s[i + 1]; sfb++)
                            {
                                *sf[0]++ = 0; *sf[1]++ = 0; *sf[2]++ = 0;
                            }
                }
            break;
                
    case LONG_BLOCK_MODE:
        sf[0] = scale_fac->scalefac_long;
        
        
        if(mp->side_info->lsf)
            for(i = 0; i < 21; i++)
                *sf[0]++ = scalefac_buffer[idx++];
            else
                for(i = 0; i < 4; i++)
                {
                    
                    if(mp->side_info->scfsi[ch][i] == 0 || gr == 0)
                    {
                        bits = slen[i >> 1][gr_info->scalefac_compress];
                        if(bits)
                            for(sfb = sfbtable.l[i]; sfb < sfbtable.l[i + 1]; sfb++)
                            {
                                
                                *sf[0]++ = BsGetBits(mp->br, bits);
                                
                            }
                            else
                                for(sfb = sfbtable.l[i]; sfb < sfbtable.l[i + 1]; sfb++)
                                    *sf[0]++ = 0;
                    }
                    else
                        sf[0] += sfbtable.l[i + 1] - sfbtable.l[i];
                }
            break;
                
    default:
        break;
    }
    
    delete[] scalefac_buffer;
    
}


void
    init_III_reorder(int16 reorder_idx[2][MAX_MONO_SAMPLES], int16 *sfb_table, 
    int16 *sfb_width_table)
{
    int32 sfb, sfb_start, i;
    int32 window, freq, src_line, des_line;
    
    for(i = sfb = 0; sfb < MAX_SHORT_SFB_BANDS; sfb++)
    {
        sfb_start = sfb_table[sfb];
        
        for(window = 0; window < 3; window++)
            for(freq = 0; freq < sfb_width_table[sfb]; freq++)
            {
                src_line = sfb_start * 3 + window * sfb_width_table[sfb] + freq;
                des_line = (sfb_start * 3) + window + (freq * 3);
                
                reorder_idx[0][i] =
                    ((des_line / SSLIMIT) * SSLIMIT) + (des_line % SSLIMIT);
                
                reorder_idx[1][i++] =
                    ((src_line / SSLIMIT) * SSLIMIT) + (src_line % SSLIMIT);
            }
    }
}



/**************************************************************************
Title        : III_reorder

  Purpose      : Re-orders the input frame if short blocks are present.
  
    Usage        : III_reorder(mp, gr, ch)
    
      Input        : mp - MP3 stream parameters
      ch - channel number (left or right)
      gr - granule number
      
        Explanation  : -
        
          Author(s)    : Juha Ojanpera
*************************************************************************/

void
    III_reorder(CMP_Stream *mp, int16 ch, int16 gr)
{
    int16 i, sb_start;
    register int16 *id1, *id2;
    
    FLOAT* xr = NULL;
    TRAPD(error, xr = new (ELeave) FLOAT[MAX_MONO_SAMPLES]);
    if (error != KErrNone)
        return;
    
    register FLOAT *src, *dst;
    
    if(mp->side_info->ch_info[ch]->gr_info[gr]->block_mode == LONG_BLOCK_MODE)
    {
        delete[] xr;
        return;
        
    }
    
    sb_start = 0;
    
    id1 = &mp->reorder_idx[0][sb_start];
    id2 = &mp->reorder_idx[1][sb_start];
    
    /*
    * First re-order the short block to a temporary buffer
    * and then copy back to the input buffer. Not fully optimal,
    * a better way would be perhaps to do the re-ordering during the
    * dequantization but to my opinion that would complicate the code
    * too much. We also have to remember that short blocks do not occur
    * very frequently, so the penalty of having a separate re-ordering
    * routine is not so time consuming from the overall decoder complexity
    * point of view.
    */
    src = &mp->buffer->ch_reconstructed[ch][0];
    dst = &xr[0];
    
    for(i = sb_start; i < MAX_MONO_SAMPLES; i++)
        dst[*id1++] = src[*id2++];
    
    /* Copy back. */
    for(i = sb_start; i < MAX_MONO_SAMPLES; i++)
        src[i] = dst[i];
    
    delete[] xr;
}
