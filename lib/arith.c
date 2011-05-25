/*
 *  arith.c:		Adaptive arithmetic coding and decoding
 *
 *  Written by:		Ullrich Hafner
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

#include "bit-io.h"
#include "misc.h"
#include "arith.h"

/******************************************************************************

				public code
  
******************************************************************************/

arith_t *
alloc_encoder (bitfile_t *output)
/*
 *  Arithmetic coder constructor:
 *  Initialize the arithmetic coder.
 *  
 *  Return value:
 *	A pointer to the new coder structure
 */
{
   arith_t *arith = Calloc (1, sizeof (arith_t));

   assert (output);
   
   arith->low       = LOW;
   arith->high      = HIGH;
   arith->underflow = 0;
   arith->file 	    = output;

   return arith;
}

void
free_encoder (arith_t *arith)
/*
 *  Arithmetic encoder destructor.
 *  Flush the arithmetic coder. Append all remaining bits to the
 *  output stream. Append zero bits to get the output file byte aligned.
 *  
 *  No return value.
 */
{
   u_word_t   low;			/* start of the current code range  */
   u_word_t   high;			/* end of the current code range    */
   u_word_t   underflow;		/* number of underflow bits pending */
   bitfile_t *output;
   
   assert (arith);

   low       = arith->low;
   high      = arith->high;
   underflow = arith->underflow;
   output    = arith->file;
   
   low = high;

   RESCALE_OUTPUT_INTERVAL;

   OUTPUT_BYTE_ALIGN (output);

   Free (arith);
}

real_t
encode_symbol (unsigned symbol, arith_t *arith, model_t *model)
/*
 *  Encode the given 'symbol' using the given probability 'model'.
 *  The current state of the arithmetic coder is given by 'arith'.
 *  Output bits are appended to the stream 'output'.
 *
 *  The model is updated after encoding the symbol (if neccessary the
 *  symbol counts are rescaled).
 *  
 *  Return value:
 *	information content of the encoded symbol.
 *
 *  Side effects:
 *	'model' is updated (probability distribution)
 *	'arith' is updated (coder state)
 */
{
   u_word_t   low_count;		/* lower bound of 'symbol' interval */
   u_word_t   high_count;		/* upper bound of 'symbol' interval */
   u_word_t   scale;			/* range of all 'm' symbol intervals */
   unsigned   range;			/* range of current interval */
   unsigned   index;			/* index of probability model */
   u_word_t   low;			/* start of the current code range  */
   u_word_t   high;			/* end of the current code range    */
   u_word_t   underflow;		/* number of underflow bits pending */
   bitfile_t *output;			/* output file */
   
   assert (model && arith);

   /*
    * Get interval values
    */
   low       = arith->low;
   high      = arith->high;
   underflow = arith->underflow;
   output    = arith->file;

   assert (high > low);
   
   if (model->order > 0)		/* order-'n' model*/
   {
      unsigned power;			/* multiplicator */
      unsigned i;

      /*
       *  Compute index of the probability model to use.
       *  See init_model() for more details.
       */
      power = 1;			/* multiplicator */
      index = 0;			/* address of prob. model */
	 
      for (i = 0; i < model->order; i++) /* genarate a M-nary number */
      {
	 index += model->context [i] * power;	
	 power *= model->symbols;
      }

      index *= model->symbols + 1;	/* we need space for M + 1 elements */

      for (i = 0; i < model->order - 1; i++)
	 model->context [i] = model->context [i + 1];
      model->context [i] = symbol;
   }
   else
      index = 0;

   scale      = model->totals [index + model->symbols];
   low_count  = model->totals [index + symbol];
   high_count = model->totals [index + symbol + 1];

   /*
    *  Compute the new interval depending on the input 'symbol'.
    */
   range = (high - low) + 1;
   high  = low + (u_word_t) ((range * high_count) / scale - 1);
   low   = low + (u_word_t) ((range * low_count) / scale);
   
   RESCALE_OUTPUT_INTERVAL;
   
   if (model->scale > 0)		/* adaptive model */
   {
      unsigned i;

      /*
       *  Update probability model
       */
      for (i = symbol + 1; i <= model->symbols; i++)
	 model->totals [index + i]++;
      if (model->totals [index + model->symbols] > model->scale) /* scaling */
      {
	 for (i = 1; i <= model->symbols; i++)
	 {
	    model->totals [index + i] >>= 1;
	    if (model->totals [index + i] <= model->totals [index + i - 1])
	       model->totals [index + i] = model->totals [index + i - 1] + 1;
	 }
      }
   }

   /*
    *  Store interval values
    */
   arith->low  	    = low;
   arith->high 	    = high;
   arith->underflow = underflow;
   
   return - log2 ((high_count - low_count) / (real_t) scale);
}

void
encode_array (bitfile_t *output, const unsigned *data, const unsigned *context,
	      const unsigned *c_symbols, unsigned n_context, unsigned n_data,
	      unsigned scaling)
/*
 *  Arithmetic coding of #'n_data' symbols given in the array 'data'.
 *  If 'n_context' > 1 then a number (context [n]) is assigned to every
 *  data element n, specifying which context (i.e. number of symbols given by
 *  c_symbols [context [n]] and adaptive probability model) must be used.
 *  Rescale probability models if range > 'scaling'.
 *
 *  No return value.
 */
{
   u_word_t **totals;			/* probability model */

   if (!n_context)
      n_context = 1;			/* always use one context */

   assert (output && c_symbols && data);
   assert (n_context == 1 || context);
   
   /*
    *  Allocate probability models, start with uniform distribution
    */
   totals = Calloc (n_context, sizeof (u_word_t *));
   {
      unsigned c;
      
      for (c = 0; c < n_context; c++)
      {
	 unsigned i;
      
	 totals [c]    = Calloc (c_symbols [c] + 1, sizeof (u_word_t));
	 totals [c][0] = 0;
      
	 for (i = 0; i < c_symbols [c]; i++)
	    totals [c][i + 1] = totals [c][i] + 1;
      }
   }

   /*
    *  Encode array elements
    */
   {
      u_word_t low  	 = 0;		/* Start of the current code range */
      u_word_t high 	 = 0xffff;	/* End of the current code range */
      u_word_t underflow = 0;		/* Number of underflow bits pending */
      unsigned n;
      
      for (n = 0; n < n_data; n++)
      {
	 u_word_t low_count;		/* lower bound of 'symbol' interval */
	 u_word_t high_count;		/* upper bound of 'symbol' interval */
	 u_word_t scale;		/* range of all 'm' symbol intervals */
	 unsigned range;		/* current range */
	 int	  d;			/* current data symbol */
	 int	  c;			/* context of current data symbol */

	 d = data [n];
	 c = n_context > 1 ? context [n] : 0; 
      
	 scale	    = totals [c][c_symbols [c]];
	 low_count  = totals [c][d];
	 high_count = totals [c][d + 1];

	 /*
	  * Rescale high and low for the new symbol.
	  */
	 range = (high - low) + 1;
	 high  = low + (u_word_t) ((range * high_count) / scale - 1);
	 low   = low + (u_word_t) ((range * low_count) / scale);
	 RESCALE_OUTPUT_INTERVAL;
      
	 /*
	  *  Update probability models
	  */
	 {
	    unsigned i;

	    for (i = d + 1; i < c_symbols [c] + 1; i++)
	       totals [c][i]++;
	 
	    if (totals [c][c_symbols [c]] > scaling) /* scaling */
	       for (i = 1; i < c_symbols [c] + 1; i++)
	       {
		  totals [c][i] >>= 1;
		  if (totals [c][i] <= totals [c][i - 1])
		     totals [c][i] = totals [c][i - 1] + 1;
	       }
	 }
      }
      /*
       *  Flush arithmetic encoder
       */
      low = high;
      RESCALE_OUTPUT_INTERVAL;
      OUTPUT_BYTE_ALIGN (output);
   }
   
   /*
    *  Cleanup ...
    */
   {
      unsigned c;
      for (c = 0; c < n_context; c++)
	 Free (totals [c]);
      Free (totals);
   }
}

arith_t *
alloc_decoder (bitfile_t *input)
/*
 *  Arithmetic decoder constructor:
 *  Initialize the arithmetic decoder with the first
 *  16 input bits from the stream 'input'.
 *  
 *  Return value:
 *	A pointer to the new decoder structure
 */

{
   arith_t *arith = Calloc (1, sizeof (arith_t));
   
   assert (input);
   
   arith->low  = LOW;
   arith->high = HIGH;
   arith->code = get_bits (input, 16);
   arith->file = input;

   return arith;
}

void
free_decoder (arith_t *arith)
/*
 *  Arithmetic decoder destructor:
 *  Flush the arithmetic decoder, i.e., read bits to get the input
 *  file byte aligned. 
 *  
 *  No return value.
 *
 *  Side effects:
 *	structure 'arith' is discarded.
 */
{
   assert (arith);

   INPUT_BYTE_ALIGN (arith->file);

   Free (arith);
}

unsigned
decode_symbol (arith_t *arith, model_t *model)
/*
 *  Decode the next symbol - the state of the arithmetic decoder
 *  is given in 'arith'. Read refinement bits from the stream 'input'
 *  and use the given probability 'model'. Update the probability model after
 *  deconding the symbol (if neccessary also rescale the symbol counts).
 *  
 *  Return value:
 *	decoded symbol
 *
 *  Side effects:
 *	'model' is updated (probability distribution)
 *	'arith' is updated (decoder state)
 */
{
   unsigned   range;			/* range of current interval */
   unsigned   count;			/* value in the current interval */
   unsigned   index;			/* index of probability model */
   unsigned   symbol;			/* decoded symbol */
   u_word_t   scale;			/* range of all 'm' symbol intervals */
   u_word_t   low;			/* start of the current code range  */
   u_word_t   high;			/* end of the current code range    */
   u_word_t   code;			/* the present input code value */
   bitfile_t *input;			/* input file */

   assert (arith && model);
   
   /*
    * Get interval values
    */
   low   = arith->low;
   high  = arith->high;
   code  = arith->code;
   input = arith->file;

   assert (high > low);
   
   if (model->order > 0)		/* order-'n' model */
   {
      unsigned power;			/* multiplicator */
      unsigned i;
      
      /*
       *  Compute index of the probability model to use.
       *  See init_model() for more details.
       */
      power = 1;			/* multiplicator */
      index = 0;			/* address of prob. model */
	 
      for (i = 0; i < model->order; i++) /* genarate a m-nary number */
      {
	 index += model->context[i] * power;	
	 power *= model->symbols;
      }

      index *= model->symbols + 1;	/* we need space for m + 1 elements */
   }
   else
      index = 0;

   scale = model->totals [index + model->symbols];
   range = (high - low) + 1;
   count = ((code - low + 1) * scale - 1) / range;

   for (symbol = model->symbols; count < model->totals [index + symbol];
	symbol--)
      ;

   if (model->order > 0)		/* order-'n' model */
   {
      unsigned i;
      
      for (i = 0; i < model->order - 1; i++)
	 model->context [i] = model->context [i + 1];
      model->context [i] = symbol;
   }

   /*
    *  Compute interval boundaries
    */
   {
      u_word_t low_count;		/* lower bound of 'symbol' interval */
      u_word_t high_count;		/* upper bound of 'symbol' interval */
      
      low_count  = model->totals [index + symbol];
      high_count = model->totals [index + symbol + 1];
      high       = low + (u_word_t) ((range * high_count) / scale - 1 );
      low        = low + (u_word_t) ((range * low_count) / scale );
   }
   
   RESCALE_INPUT_INTERVAL;
   
   if (model->scale > 0)		/* adaptive model */
   {
      unsigned i;

      /*
       *  Update probability model
       */
      for (i = symbol + 1; i <= model->symbols; i++)
	 model->totals [index + i]++;
      if (model->totals [index + model->symbols] > model->scale) /* scaling */
      {
	 for (i = 1; i <= model->symbols; i++)
	 {
	    model->totals [index + i] >>= 1;
	    if (model->totals [index + i] <= model->totals [index + i - 1])
	       model->totals [index + i] = model->totals [index + i - 1] + 1;
	 }
      }
   }
   
   /*
    *  Store interval values
    */
   arith->low  = low;
   arith->high = high;
   arith->code = code;

   return symbol;
}

unsigned *
decode_array (bitfile_t *input, const unsigned *context,
	      const unsigned *c_symbols, unsigned n_context,
	      unsigned n_data, unsigned scaling)
/*
 *  Arithmetic decoding of #'n_data' symbols.
 *  If 'n_context' > 1 then a number (context [n]) is assigned to every
 *  data element n, specifying which context (i.e. number of symbols given by
 *  c_symbols [context [n]] and adaptive probability model) must be used.
 *  Rescale probability models if range > 'scaling'.
 *
 *  Return value:
 *	pointer to array containing the decoded symbols
 */
{
   unsigned  *data;			/* array to store decoded symbols */
   u_word_t **totals;			/* probability model */
   
   if (n_context < 1)
      n_context = 1;			/* always use one context */
   assert (input && c_symbols);
   assert (n_context == 1 || context);

   data = Calloc (n_data, sizeof (unsigned));
   
   /*
    *  Allocate probability models, start with uniform distribution
    */
   totals = Calloc (n_context, sizeof (u_word_t *));
   {
      unsigned c;
      
      for (c = 0; c < n_context; c++)
      {
	 unsigned i;
      
	 totals [c]    = Calloc (c_symbols [c] + 1, sizeof (u_word_t));
	 totals [c][0] = 0;
      
	 for (i = 0; i < c_symbols [c]; i++)
	    totals [c][i + 1] = totals [c][i] + 1;
      }
   }

   /*
    *  Fill array 'data' with decoded values
    */
   {
      u_word_t code = get_bits (input, 16); /* The present input code value */
      u_word_t low  = 0;		/* Start of the current code range */
      u_word_t high = 0xffff;		/* End of the current code range */
      unsigned n;
      
      for (n = 0; n < n_data; n++) 
      {
	 u_word_t scale;		/* range of all 'm' symbol intervals */
	 u_word_t low_count;		/* lower bound of 'symbol' interval */
	 u_word_t high_count;		/* upper bound of 'symbol' interval */
	 unsigned count;		/* value in the current interval */
	 unsigned range;		/* current interval range */
	 unsigned d;			/* current data symbol */
	 unsigned c;			/* context of current data symbol */

	 c = n_context > 1 ? context [n] : 0; 

	 assert (high > low);
	 scale = totals [c][c_symbols [c]];
	 range = (high - low) + 1;
	 count = (((code - low) + 1 ) * scale - 1) / range;
      
	 for (d = c_symbols [c]; count < totals [c][d]; d--) /* next symbol */
	    ;
	 low_count  = totals [c][d];
	 high_count = totals [c][d + 1];

	 high = low + (u_word_t) ((range * high_count) / scale - 1 );
	 low  = low + (u_word_t) ((range * low_count) / scale );
	 RESCALE_INPUT_INTERVAL;

	 /*
	  *  Updata probability models
	  */
	 {
	    unsigned i;

	    for (i = d + 1; i < c_symbols [c] + 1; i++)
	       totals [c][i]++;
	 
	    if (totals [c][c_symbols [c]] > scaling) /* scaling */
	       for (i = 1; i < c_symbols [c] + 1; i++)
	       {
		  totals [c][i] >>= 1;
		  if (totals [c][i] <= totals [c][i - 1])
		     totals [c][i] = totals [c][i - 1] + 1;
	       }
	 }
	 data [n] = d;
      }
      INPUT_BYTE_ALIGN (input);
   }

   /*
    *  Cleanup ...
    */
   {
      unsigned c;
      
      for (c = 0; c < n_context; c++)
	 Free (totals [c]);
      Free (totals);
   }
   
   return data;
}

model_t *
alloc_model (unsigned m, unsigned scale, unsigned n, unsigned *totals)
/*
 *  Model constructor:
 *  allocate and initialize an order-'n' probability model.
 *  The size of the source alphabet is 'm'. Rescale the symbol counts after
 *  'scale' symbols are encoded/decoded. The initial probability of every
 *  symbol is 1/m.
 *  If 'scale' = 0 then use static modeling (p = 1/n).
 *  If 'totals' is not NULL then use this array of 'm' values to set
 *  the initial counts.
 *
 *  Return value:
 *	a pointer to the new probability model structure.
 *  
 *  Note: We recommend a small size of the alphabet because no escape codes
 *  are used to encode/decode previously unseen symbols.
 *  
 */
{
   model_t  *model;			/* new probability model */
   unsigned  num;			/* number of contexts to allocate */
   bool_t    cont;			/* continue flag */
   bool_t    dec;			/* next order flag */
   unsigned  i;

   /*
    *  Allocate memory for the structure
    */
   model          = Calloc (1, sizeof (model_t));
   model->symbols = m;
   model->scale   = scale;
   model->order   = n;
   model->context = n > 0 ? Calloc (n, sizeof (unsigned)) : NULL;
   /*
    *  Allocate memory for the probabilty model.
    *  Each of the m^n different contexts requires its own probability model.
    */
   for (num = 1, i = 0; i < model->order; i++)
      num *= model->symbols;

   model->totals = Calloc (num * (model->symbols + 1), sizeof (unsigned));

   for (i = 0; i < model->order; i++)
      model->context[i] = 0;		/* start with context 0,0, .. ,0 */
   cont = YES;
   while (cont)				/* repeat while context != M ... M */
   {
      int	power;			/* multiplicator */
      int	index;			/* index of probability model */
      /*
       *  There are m^n different contexts:
       *  Let "context_1 context_2 ... context_n symbol" be the current input
       *  stream then the index of the probability model is given by:
       *  index = context_1 * M^0 + context_2 * M^1 + ... + context_n * M^(n-1)
       */
      power = 1;			/* multiplicator */
      index = 0;			/* address of prob. model */
	 
      for (i = 0; i < model->order; i++)	/* genarate a m-nary number */
      {
	 index += model->context[i] * power;	
	 power *= model->symbols;
      }

      index *= model->symbols + 1;	/* size of each model is m + 1 */

      model->totals [index + 0] = 0;	/* always zero */
	 
      for (i = 1; i <= model->symbols; i++) /* prob of each symbol is 1/m or
					       as given in totals */
	 model->totals[index + i] = model->totals [index + i - 1]
				    + (totals ? totals [i - 1] : 1);

      if (model->order == 0)		/* order-0 model */
	 cont = NO;
      else				/* try next context */
	 for (i = model->order - 1, dec = YES; dec; i--)
	 {
	    dec = NO;
	    model->context[i]++;
	    if (model->context[i] >= model->symbols) 
	    {
	       /* change previous context */
	       model->context[i] = 0;
	       if (i > 0)		/* there's still a context remaining */
		  dec = YES;
	       else
		  cont = NO;		/* all context models initilized */
	    }
	 }
   }
   for (i = 0; i < model->order; i++)
      model->context[i] = 0;		/* start with context 0,0, .. ,0 */

   return model;
}

void
free_model (model_t *model)
/*
 *  Model destructor:
 *  Free memory allocated by the arithmetic 'model'.
 *
 *  No return value.
 *
 *  Side effects:
 *	struct 'model' is discarded
 */
{
   if (model != NULL)
   {
      if (model->context != NULL)
	 Free (model->context);
      Free (model->totals);
      Free (model);
   }
   else
      warning ("Can't free model <NULL>.");
}
