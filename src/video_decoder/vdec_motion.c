/*****************************************************************************
 * vdec_motion.c : motion compensation routines
 * (c)1999 VideoLAN
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>

#include "config.h"
#include "common.h"
#include "mtime.h"
#include "vlc_thread.h"

#include "intf_msg.h"
#include "debug.h"                    /* ?? temporaire, requis par netlist.h */

#include "input.h"
#include "input_netlist.h"
#include "decoder_fifo.h"
#include "video.h"
#include "video_output.h"

#include "vdec_idct.h"
#include "video_decoder.h"
#include "vdec_motion.h"

#include "vpar_blocks.h"
#include "vpar_headers.h"
#include "video_fifo.h"
#include "vpar_synchro.h"
#include "video_parser.h"

/*****************************************************************************
 * vdec_MotionComponent : last stage of motion compensation
 *****************************************************************************/
static void __inline__ MotionComponent( yuv_data_t * p_src, yuv_data_t * p_dest,
                                   int i_width, int i_height, int i_x_step,
                                   int i_select )
{
    int i_x, i_y, i_x1, i_y1;
    unsigned int i_dummy;

    switch( i_select )
    {
    case 4:
        /* !xh, !yh, average */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         i_dummy = p_dest[i_x + i_x1] + p_src[i_x + i_x1];
                         p_dest[i_x + i_x1] = (i_dummy + 1) >> 1;
                     }
                }
                p_dest += i_x_step;
                p_src += i_x_step;
            }
        }
        break;

    case 0:
        /* !xh, !yh, !average */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         p_dest[i_x+i_x1] = p_src[i_x+i_x1];
                     }
                }
                p_dest += i_x_step;
                p_src += i_x_step;
            }
        }
        break;

    case 6:
        /* !xh, yh, average */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         i_dummy = p_dest[i_x+i_x1]
                            + ((unsigned int)(p_src[i_x+i_x1] + 1
                                     + p_src[i_x+i_x1 + i_x_step]) >> 1);
                         p_dest[i_x + i_x1] = (i_dummy + 1) >> 1;
                     }
                }
                p_dest += i_x_step;
                p_src += i_x_step;
            }
        }
        break;

    case 2:
        /* !xh, yh, !average */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         p_dest[i_x+i_x1] = (unsigned int)(p_src[i_x+i_x1] + 1
                                            + p_src[i_x+i_x1 + i_x_step])
                                          >> 1;  
                     }
                }
            }
        }
        break;

    case 5:
        /* xh, !yh, average */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         i_dummy = p_dest[i_x+i_x1]
                            + ((unsigned int)(p_src[i_x+i_x1]
                                              + p_src[i_x+i_x1 + 1] + 1) >> 1);
                         p_dest[i_x + i_x1] = (i_dummy + 1) >> 1;
                     }
                }
                p_dest += i_x_step;
                p_src += i_x_step;
            }
        }
        break;

    case 1:
        /* xh, !yh, !average */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         p_dest[i_x+i_x1] = (unsigned int)(p_src[i_x+i_x1]
                                                  + p_src[i_x+i_x1 + 1] + 1)
                                                >> 1;
                     }
                }
            }
        }
        break;

    case 7:
        /* xh, yh, average */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         i_dummy = p_dest[i_x+i_x1]
                            + ((unsigned int)(
                                  p_src[i_x+i_x1]
                                + p_src[i_x+i_x1 + 1]
                                + p_src[i_x+i_x1 + i_x_step]
                                + p_src[i_x+i_x1 + i_x_step + 1]
                                + 2) >> 2);
                         p_dest[i_x + i_x1] = (i_dummy + 1) >> 1;
                     }
                }
                p_dest += i_x_step;
                p_src += i_x_step;
            }
        }
        break;

    default:
        /* xh, yh, !average (3) */
        for( i_y = 0; i_y < i_height; i_y += 4 )
        {
            for( i_y1 = 0; i_y1 < 4; i_y1++ )
            {
                for( i_x = 0; i_x < i_width; i_x += 8 )
                {
                     for( i_x1 = 0; i_x1 < 8; i_x1++ )
                     {
                         p_dest[i_x+i_x1]
                            = ((unsigned int)(
                                  p_src[i_x+i_x1]
                                + p_src[i_x+i_x1 + 1] 
                                + p_src[i_x+i_x1 + i_x_step]
                                + p_src[i_x+i_x1 + i_x_step + 1]
                                + 2) >> 2);
                     }
                }
                p_dest += i_x_step;
                p_src += i_x_step;
            }
        }
        break;
    }
}


typedef struct motion_arg_s
{
    picture_t *     p_source;
    boolean_t       b_source_field;
    boolean_t       b_dest_field;
    int             i_height, i_offset;
    int             i_mv_x, i_mv_y;
    boolean_t       b_average;
} motion_arg_t;

/*****************************************************************************
 * vdec_MotionDummy : motion compensation for an intra macroblock
 *****************************************************************************/
void vdec_MotionDummy( macroblock_t * p_mb )
{
    /* Nothing to do :) */
}

/*****************************************************************************
 * vdec_MotionFieldField : motion compensation for field motion type (field)
 *****************************************************************************/
void vdec_MotionFieldField( macroblock_t * p_mb )
{
    motion_arg_t    args;

    args.i_height = 16;
    args.b_average = 0;
    args.b_dest_field = 0;
    args.i_offset = 0;

    if( p_mb->i_mb_type & MB_MOTION_FORWARD )
    {
        boolean_t   b_current_field;
        
        b_current_field = ( p_mb->i_structure == BOTTOM_FIELD );
        if( p_mb->b_P_coding_type
             && (p_mb->i_current_structure == FRAME_STRUCTURE)
             && (b_current_field != p_mb->ppi_field_select[0][0]) )
            args.p_source = p_mb->p_picture;
        else
            args.p_source = p_mb->p_forward;
        args.b_source_field = p_mb->ppi_field_select[0][0];
        args.i_mv_x = p_mb->pppi_motion_vectors[0][0][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][0][1];
        p_mb->pf_chroma_motion( p_mb, &args );

        args.b_average = 1;
    }

    if( p_mb->i_mb_type & MB_MOTION_BACKWARD )
    {
        args.b_source_field = p_mb->ppi_field_select[0][1];
        args.i_mv_x = p_mb->pppi_motion_vectors[0][1][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][1][1];
        p_mb->pf_chroma_motion( p_mb, &args );
    }
}

/*****************************************************************************
 * vdec_MotionField16x8 : motion compensation for 16x8 motion type (field)
 *****************************************************************************/
void vdec_MotionField16x8( macroblock_t * p_mb )
{
    motion_arg_t    args;

    args.i_height = 8;
    args.b_average = 0;
    args.b_dest_field = 0;
    args.i_offset = 0;

    if( p_mb->i_mb_type & MB_MOTION_FORWARD )
    {
        boolean_t   b_current_field;

        b_current_field = ( p_mb->i_structure == BOTTOM_FIELD );
        if( p_mb->b_P_coding_type
             && (p_mb->i_current_structure == FRAME_STRUCTURE)
             && (b_current_field != p_mb->ppi_field_select[0][0]) )
            args.p_source = p_mb->p_picture;
        else
            args.p_source = p_mb->p_forward;
        args.b_source_field = p_mb->ppi_field_select[0][0];
        args.i_mv_x = p_mb->pppi_motion_vectors[0][0][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][0][1];
        p_mb->pf_chroma_motion( p_mb, &args );

        if( p_mb->b_P_coding_type
             && (p_mb->i_current_structure == FRAME_STRUCTURE)
             && (b_current_field != p_mb->ppi_field_select[1][0]) )
            args.p_source = p_mb->p_picture;
        else
            args.p_source = p_mb->p_forward;
        args.b_source_field = p_mb->ppi_field_select[1][0];
        args.i_mv_x = p_mb->pppi_motion_vectors[1][0][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[1][0][1];
        args.i_offset = 8;
        p_mb->pf_chroma_motion( p_mb, &args );

        args.b_average = 1;
        args.i_offset = 0;
    }

    if( p_mb->i_mb_type & MB_MOTION_BACKWARD )
    {
        args.p_source = p_mb->p_backward;
        args.b_source_field = p_mb->ppi_field_select[0][1];
        args.i_mv_x = p_mb->pppi_motion_vectors[0][1][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][1][1];
        p_mb->pf_chroma_motion( p_mb, &args );

        args.b_source_field = p_mb->ppi_field_select[1][1];
        args.i_mv_x = p_mb->pppi_motion_vectors[1][1][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[1][1][1];
        args.i_offset = 8;
        p_mb->pf_chroma_motion( p_mb, &args );
    }
}

/*****************************************************************************
 * vdec_MotionFieldDMV : motion compensation for dmv motion type (field)
 *****************************************************************************/
void vdec_MotionFieldDMV( macroblock_t * p_mb )
{
    /* This is necessarily a MOTION_FORWARD only macroblock */
    fprintf(stderr, "DMV pas code !!!\n");
}

/*****************************************************************************
 * vdec_MotionFrameFrame : motion compensation for frame motion type (frame)
 *****************************************************************************/
void vdec_MotionFrameFrame( macroblock_t * p_mb )
{
    motion_arg_t    args;

    args.b_source_field = args.b_dest_field = 0;
    args.i_height = 16;
    args.b_average = 0;
    args.i_offset = 0;

    if( p_mb->i_mb_type & MB_MOTION_FORWARD )
    {
        args.p_source = p_mb->p_forward;
        args.i_mv_x = p_mb->pppi_motion_vectors[0][0][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][0][1];
        p_mb->pf_chroma_motion( p_mb, &args );

        args.b_average = 1;
    }

    if( p_mb->i_mb_type & MB_MOTION_BACKWARD ) 
    {
        /* Necessarily MB_MOTION_BACKWARD */
        args.p_source = p_mb->p_backward;
        args.i_mv_x = p_mb->pppi_motion_vectors[0][1][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][1][1];
        p_mb->pf_chroma_motion( p_mb, &args );
    }
}

/*****************************************************************************
 * vdec_MotionFrameField : motion compensation for field motion type (frame)
 *****************************************************************************/
void vdec_MotionFrameField( macroblock_t * p_mb )
{
    motion_arg_t    args;

    args.i_height = 8;
    args.b_average = 0;
    args.i_offset = 0;

    if( p_mb->i_mb_type & MB_MOTION_FORWARD )
    {
        args.p_source = p_mb->p_forward;

        args.b_source_field = p_mb->ppi_field_select[0][0];
        args.b_dest_field = 0;
        args.i_mv_x = p_mb->pppi_motion_vectors[0][0][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][0][1] >> 1;
        p_mb->pf_chroma_motion( p_mb, &args );

        args.b_source_field = p_mb->ppi_field_select[1][0];
        args.b_dest_field = 1;
        args.i_mv_x = p_mb->pppi_motion_vectors[1][0][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[1][0][1] >> 1;
        p_mb->pf_chroma_motion( p_mb, &args );

        args.b_average = 1;
    }

    if( p_mb->i_mb_type & MB_MOTION_BACKWARD )
    {
        args.p_source = p_mb->p_backward;

        args.b_source_field = p_mb->ppi_field_select[0][1];
        args.b_dest_field = 0;
        args.i_mv_x = p_mb->pppi_motion_vectors[0][1][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[0][1][1] >> 1;
        p_mb->pf_chroma_motion( p_mb, &args );

        args.b_source_field = p_mb->ppi_field_select[1][1];
        args.b_dest_field = 1;
        args.i_mv_x = p_mb->pppi_motion_vectors[1][1][0];
        args.i_mv_y = p_mb->pppi_motion_vectors[1][1][1] >> 1;
        p_mb->pf_chroma_motion( p_mb, &args );
    }
}

/*****************************************************************************
 * vdec_MotionFrameDMV : motion compensation for dmv motion type (frame)
 *****************************************************************************/
void vdec_MotionFrameDMV( macroblock_t * p_mb )
{
    /* This is necessarily a MOTION_FORWARD only macroblock */
    fprintf(stderr, "DMV pas codee 2 !!!!!\n");
}

/*****************************************************************************
 * vdec_Motion420 : motion compensation for a 4:2:0 macroblock
 *****************************************************************************/
void vdec_Motion420( macroblock_t * p_mb, motion_arg_t * p_motion )
{
    p_motion->i_mv_x = p_motion->i_mv_y = 0;
    /* Luminance */
    MotionComponent( /* source */
                     p_motion->p_source->p_y
                       + (p_mb->i_l_x + (p_motion->i_mv_x >> 1))
                       + ((p_mb->i_l_y + p_motion->i_offset
                          + (p_motion->i_mv_y >> 1))
                          + (p_motion->b_source_field - p_motion->b_dest_field))
                         * p_mb->p_picture->i_width,
                     /* destination */
                     p_mb->p_picture->p_y
                       + (p_mb->i_l_x)
                       + (p_mb->i_l_y) * p_mb->p_picture->i_width,
                     /* prediction width and height */
                     16, p_motion->i_height,
                     /* step */
                     p_mb->i_l_stride,
                     /* select */
                     (p_motion->b_average << 2)
                       | ((p_motion->i_mv_y & 1) << 1)
                       | (p_motion->i_mv_x & 1) );

    /* Chrominance Cr */
    MotionComponent( p_motion->p_source->p_u
                       + (p_mb->i_c_x + ((p_motion->i_mv_x/2) >> 1))
                       + ((p_mb->i_c_y + (p_motion->i_offset >> 1)
                          + ((p_motion->i_mv_y/2) >> 1))
                          + (p_motion->b_source_field - p_motion->b_dest_field))
                         * p_mb->p_picture->i_chroma_width,
                     p_mb->p_picture->p_u
                       + (p_mb->i_c_x)
                       + (p_mb->i_c_y) * p_mb->p_picture->i_chroma_width,
                     8, p_motion->i_height >> 1, p_mb->i_c_stride,
                     (p_motion->b_average << 2)
                       | (((p_motion->i_mv_y/2) & 1) << 1)
                       | ((p_motion->i_mv_x/2) & 1) );

    /* Chrominance Cb */
    MotionComponent( p_motion->p_source->p_v
                       + (p_mb->i_c_x + ((p_motion->i_mv_x/2) >> 1))
                       + ((p_mb->i_c_y + (p_motion->i_offset >> 1)
                          +((p_motion->i_mv_y/2) >> 1))
                          + (p_motion->b_source_field - p_motion->b_dest_field))
                         * p_mb->p_picture->i_chroma_width,
                     p_mb->p_picture->p_v
                       + (p_mb->i_c_x)
                       + (p_mb->i_c_y) * p_mb->p_picture->i_chroma_width,
                     8, p_motion->i_height >> 1, p_mb->i_c_stride,
                     (p_motion->b_average << 2)
                       | (((p_motion->i_mv_y/2) & 1) << 1)
                       | ((p_motion->i_mv_x/2) & 1) );

}

/*****************************************************************************
 * vdec_Motion422 : motion compensation for a 4:2:2 macroblock
 *****************************************************************************/
void vdec_Motion422( macroblock_t * p_mb, motion_arg_t * p_motion )
{
    fprintf(stderr, "La chrominance va chier dans la colle.\n");
}

/*****************************************************************************
 * vdec_Motion444 : motion compensation for a 4:4:4 macroblock
 *****************************************************************************/
void vdec_Motion444( macroblock_t * p_mb, motion_arg_t * p_motion )
{
    fprintf(stderr, "La chrominance va chier dans le pastis.\n");
}
