/*
 *  arith.h
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

#ifndef _ARITH_H
#define _ARITH_H

#include "types.h"
#include "bit-io.h"

typedef struct model
{
   unsigned  symbols;			/* number of symbols in the alphabet */
   unsigned  scale;			/* if totals > scale rescale totals */
   unsigned  order;			/* order of the probability model */
   unsigned *context;			/* context of the model */
   unsigned *totals;			/* the totals */
} model_t;

typedef struct arith
{
   u_word_t   low;			/* start of the current code range */
   u_word_t   high;			/* end of the current code range */
   u_word_t   underflow;		/* number of underflow bits pending */
   u_word_t   code;			/* the present input code value */
   bitfile_t *file;			/* I/O stream */
} arith_t;

enum interval {LOW = 0x0000, FIRST_QUARTER = 0x4000, HALF = 0x8000,
	       THIRD_QUARTER = 0xc000, HIGH = 0xffff};

arith_t *
alloc_encoder (bitfile_t *file);
void
free_encoder (arith_t *arith);
real_t
encode_symbol (unsigned symbol, arith_t *arith, model_t *model);
void
encode_array (bitfile_t *output, const unsigned *data, const unsigned *context,
	      const unsigned *c_symbols, unsigned n_context, unsigned n_data,
	      unsigned scaling);
arith_t *
alloc_decoder (bitfile_t *input);
void
free_decoder (arith_t *arith);
unsigned
decode_symbol (arith_t *arith, model_t *model);
unsigned *
decode_array (bitfile_t *input, const unsigned *context,
	      const unsigned *c_symbols, unsigned n_context,
	      unsigned n_data, unsigned scaling);
model_t *
alloc_model (unsigned m, unsigned scale, unsigned n, unsigned *totals);
void
free_model (model_t *model);

#define RESCALE_INPUT_INTERVAL  for (;;)                                      \
                                   if ((high >= HALF) && (low < HALF) &&      \
                                      ((low & FIRST_QUARTER) != FIRST_QUARTER \
				       || (high & FIRST_QUARTER) != 0))       \
                                   {                                          \
                                      break;                                  \
                                   }                                          \
                                   else if ((high < HALF) || (low >= HALF))   \
                                   {                                          \
                                      low  <<= 1;                             \
                                      high <<= 1;                             \
                                      high  |= 1;                             \
                                      code <<= 1;                             \
                                      code  += get_bit (input);               \
                                   }                                          \
                                   else                                       \
                                   {                                          \
                                      code  ^= FIRST_QUARTER;                 \
                                      low   &= FIRST_QUARTER - 1;             \
                                      low  <<= 1;                             \
                                      high <<= 1;                             \
                                      high  |= HALF + 1;                      \
                                      code <<= 1;                             \
                                      code  += get_bit (input);               \
                                   }                                          
        								   
#define RESCALE_OUTPUT_INTERVAL  for (;;)                                     \
                                 {                                            \
                                    if (high < HALF)                          \
                                    {                                         \
                                       put_bit (output, 0);                   \
                                       for (; underflow; underflow--)         \
                                          put_bit (output, 1);                \
                                    }                                         \
                                    else if (low >= HALF)                     \
                                    {                                         \
                                       put_bit (output, 1);                   \
                                       for (; underflow; underflow--)         \
                                          put_bit (output, 0);                \
                                    }                                         \
                                    else if (high < THIRD_QUARTER &&          \
                                             low >= FIRST_QUARTER)            \
                                    {                                         \
                                       underflow++;                           \
                                       high |= FIRST_QUARTER;                 \
                                       low  &= FIRST_QUARTER - 1;             \
                                    }                                         \
                                    else                                      \
                                       break;                                 \
                                    high <<= 1;                               \
                                    high  |= 1;                               \
                                    low  <<= 1;                               \
                                 }                                             
					 
#endif /* not _ARITH_H */

