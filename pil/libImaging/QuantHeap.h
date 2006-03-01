/*
 * The Python Imaging Library
 * $Id$
 *
 * image quantizer
 *
 * Written by Toby J Sargeant <tjs@longford.cs.monash.edu.au>.
 *
 * See the README file for information on usage and redistribution.
 */

#ifndef __HEAP_H__
#define __HEAP_H__

#include "QuantTypes.h"

void heap_free(Heap);
int heap_remove(Heap,void **);
int heap_add(Heap,void *);
int heap_top(Heap,void **);
Heap *heap_new(HeapCmpFunc);

#endif
