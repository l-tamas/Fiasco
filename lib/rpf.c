/*
 *  rpf.c:		Conversion of float to reduced precision format values
 *
 *  Written by:		Stefan Frank
 *			Richard Krampfl
 *			Ullrich Hafner
 *		
 *  This file is part of FIASCO («F»ractal «I»mage «A»nd «S»equence «CO»dec)
 *  Copyright (C) 1994-2000 Ullrich Hafner <hafner@bigfoot.de>
 */

/*
 *  $Date: 2000/06/14 20:49:37 $
 *  $Author: hafner $
 *  $Revision: 5.1 $
 *  $State: Exp $
 */

#include "config.h"

#include "types.h"
#include "macros.h"
#include "error.h"

#include "misc.h"
#include "rpf.h"

/* 
 * CAUTION: The IEEE float format must be used by your compiler,
 *          or all following code is void!
 */

#ifdef WORDS_BIGENDIAN
/*
 *  Big-Endian Architecture (e.g. SUN, Motorola)
 *  Memory representation of integer 0x00112233 is 00,11,22,33
 */

enum real_bytes {BYTE_0, BYTE_1, BYTE_2, BYTE_3};

#else  /* not WORDS_BIGENDIAN */
/*
 *  Little-Endian Architecture (e.g. Intel, VAX, Alpha)
 *  Memory representation of integer 0x00112233 is 33,22,11,00
 */

enum real_bytes {BYTE_3, BYTE_2, BYTE_1, BYTE_0};

#endif /* not WORDS_BIGENDIAN */

const int RPF_ZERO = -1;

/*****************************************************************************

			       private code
  
*****************************************************************************/

int
rtob (real_t f, const rpf_t *rpf)
/*
 *  Convert real number 'f' into fixed point format.
 *  The real number in [-'range'; +'range'] is scaled to [-1 ; +1].
 *  Sign and the first 'precision' - 1 bits of the mantissa are
 *  packed into one integer.  
 *
 *  Return value:
 *	real value in reduced precision format
 */
{  
   unsigned int	mantissa;
   int		exponent, sign;
   union
   {
      float f;
      unsigned char c[4];
   } v;					/* conversion dummy */

   f  /= rpf->range;			/* scale f to [-1,+1] */	
   v.f = f;

   /*
    *  Extract mantissa (23 Bits), exponent (8 Bits) and sign (1 Bit)
    */

   mantissa = ((((v.c[BYTE_1] & 127) << 8 ) | v.c[BYTE_2]) << 8) | v.c[BYTE_3];
   exponent = (((v.c[BYTE_0] & 127) << 1) | (v.c[BYTE_1] & 128 ? 1 : 0)) - 126;
   sign     = v.c[BYTE_0] & 128 ? 1 : 0;
		
   /*
    *  Generate reduced precision mantissa.
    */
   mantissa >>= 1;				/* shift 1 into from left */
   mantissa  |= (1 << 22);
   if (exponent > 0) 
      mantissa <<= exponent;
   else
      mantissa >>= -exponent;  
   
   mantissa >>= (23 - rpf->mantissa_bits - 1);

   mantissa +=  1;			/* Round last bit. */
   mantissa >>= 1;
   
   if (mantissa == 0)			/* close to zero */
      return RPF_ZERO;
   else if (mantissa >= (1U << rpf->mantissa_bits)) /* overflow */
      return sign;
   else
      return ((mantissa & ((1U << rpf->mantissa_bits) - 1)) << 1) | sign;
}

float
btor (int binary, const rpf_t *rpf)
/*
 *  Convert value 'binary' in reduced precision format to a real value.
 *  For more information refer to function lin_rtob() above.
 *
 *  Return value:
 *	converted value
 */
{
   unsigned int	mantissa;
   int		sign, exponent;
   union
   {
      float f;
      unsigned char c[4];
   } value;

   if (binary == RPF_ZERO)
      return 0;

   if (binary < 0 || binary >= 1 << (rpf->mantissa_bits + 1))
      error ("Reduced precision format: value %d out of range.", binary);

   /*
    *  Restore IEEE float format:
    *  mantissa (23 Bits), exponent (8 Bits) and sign (1 Bit)
    */
   
   sign       = binary & 1;
   mantissa   = (binary & ((1 << (rpf->mantissa_bits + 1)) - 1)) >> 1; 
   mantissa <<= (23 - rpf->mantissa_bits);
   exponent   = 0;

   if (mantissa == 0)
   {
      value.f = (sign ? -1.0 : 1.0);
   }
   else
   {
      while (!(mantissa & (1 << 22)))	/* normalize mantissa */
      {
	 exponent--;
	 mantissa <<= 1;
      }
      mantissa <<= 1;

      value.c[BYTE_0] = (sign << 7) | ((exponent + 126) >> 1);
      value.c[BYTE_1] = (((exponent + 126) & 1) << 7)
			| ((mantissa  >> 16) & 127);
      value.c[BYTE_2] = (mantissa >> 8) & 255;
      value.c[BYTE_3] = mantissa & 255;
   }
   
   return value.f * rpf->range;		/* expand [ -1 ; +1 ] to
					   [ -range ; +range ] */
}

rpf_t *
alloc_rpf (unsigned mantissa, fiasco_rpf_range_e range)
/*
 *  Reduced precision format constructor.
 *  Allocate memory for the rpf_t structure.
 *  Number of mantissa bits is given by `mantissa'.
 *  The range of the real values is in the interval [-`range', +`range'].
 *  In case of invalid parameters, a structure with default values is
 *  returned. 
 *
 *  Return value
 *	pointer to the new rpf structure
 */
{
   rpf_t *rpf = Calloc (1, sizeof (rpf_t));
   
   if (mantissa < 2)
   {
      warning (_("Size of RPF mantissa has to be in the interval [2,8]. "
		 "Using minimum value 2.\n"));
      mantissa = 2;
   }
   else if (mantissa > 8)
   {
      warning (_("Size of RPF mantissa has to be in the interval [2,8]. "
		 "Using maximum value 8.\n"));
      mantissa = 2;
   }

   rpf->mantissa_bits = mantissa;
   rpf->range_e       = range;
   switch (range)
   {
      case FIASCO_RPF_RANGE_0_75:
	 rpf->range = 0.75;
	 break;
      case FIASCO_RPF_RANGE_1_50:
	 rpf->range = 1.50;
	 break;
      case FIASCO_RPF_RANGE_2_00:
	 rpf->range = 2.00;
	 break;
      case FIASCO_RPF_RANGE_1_00:
	 rpf->range = 1.00;
	 break;
      default:
	 warning (_("Invalid RPF range specified. Using default value 1.0."));
	 rpf->range   = 1.00;
	 rpf->range_e = FIASCO_RPF_RANGE_1_00;
	 break;
   }
   return rpf;
}
