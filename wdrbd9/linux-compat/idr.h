﻿/*
	* include/linux/idr.h
	*
	* 2002-10-18  written by Jim Houston jim.houston@ccur.com
	*	Copyright (C) 2002 by Concurrent Computer Corporation
	*	Distributed under the GNU GPL license version 2.
	*
	* Small id to pointer translation service avoiding fixed sized
	* tables.
	*/

#ifndef __IDR_H__
#define __IDR_H__
#include "drbd_windrv.h"


#if BITS_PER_LONG == 32
# define IDR_BITS 5
# define IDR_FULL 0xfffffffful
	/* We can only use two of the bits in the top level because there is
	only one possible bit in the top level (5 bits * 7 levels = 35
	bits, but you only use 31 bits in the id). */
# define TOP_LEVEL_FULL (IDR_FULL >> 30)
#elif BITS_PER_LONG == 64
# define IDR_BITS 6
# define IDR_FULL 0xfffffffffffffffful
	/* We can only use two of the bits in the top level because there is
	only one possible bit in the top level (6 bits * 6 levels = 36
	bits, but you only use 31 bits in the id). */
# define TOP_LEVEL_FULL (IDR_FULL >> 62)
#else
# error "BITS_PER_LONG is not 32 or 64"
#endif

#define IDR_SIZE (1 << IDR_BITS)
#define IDR_MASK ((1 << IDR_BITS)-1)

#define MAX_ID_SHIFT (sizeof(int)*8 - 1)
#define MAX_ID_BIT (1U << MAX_ID_SHIFT)
#define MAX_ID_MASK (MAX_ID_BIT - 1)

	/* Leave the possibility of an incomplete final layer */
#define MAX_LEVEL (MAX_ID_SHIFT + IDR_BITS - 1) / IDR_BITS

	/* Number of id_layer structs to leave in free list */
#define IDR_FREE_MAX MAX_LEVEL + MAX_LEVEL

struct idr_layer {
#ifdef _WIN32
	ULONG_PTR			bitmap; /* A zero bit means "space here" */
#else
	unsigned long		bitmap; /* A zero bit means "space here" */
#endif
	struct idr_layer	*ary[1 << IDR_BITS];
	int			 count;	 /* When zero, we can release it */
	int			 layer;	 /* distance from leaf */
};

struct idr {
	struct idr_layer *top;
	struct idr_layer *id_free;
	int		  layers; /* only valid without concurrent changes */
	int		  id_free_cnt;
	KSPIN_LOCK	  lock;
};

#ifdef _WIN32
	// unused
#else
#define IDR_INIT(name)						\
{								\
	.top = NULL, \
	.id_free = NULL, \
	.layers = 0, \
	.id_free_cnt = 0, \
	.lock = __SPIN_LOCK_UNLOCKED(name.lock), \
}
#define DEFINE_IDR(name)	struct idr name = IDR_INIT(name)
/* Actions to be taken after a call to _idr_sub_alloc */
#define IDR_NEED_TO_GROW -2
#define IDR_NOMORE_SPACE -3


#define _idr_rc_to_errno(rc) ((rc) == -1 ? -EAGAIN : -ENOSPC)
#endif

/**
* idr synchronization (stolen from radix-tree.h)
*
* idr_find() is able to be called locklessly, using RCU. The caller must
* ensure calls to this function are made within rcu_read_lock() regions.
* Other readers (lock-free or otherwise) and modifications may be running
* concurrently.
*
* It is still required that the caller manage the synchronization and
* lifetimes of the items. So if RCU lock-free lookups are used, typically
* this would mean that the items have their own locks, or are amenable to
* lock-free access; and that the items are freed by RCU (or only freed after
* having been deleted from the idr tree *and* a synchronize_rcu() grace
* period).
*/

/*
* This is what we export.
*/

extern void *idr_find(struct idr *idp, int id);
extern int idr_pre_get(struct idr *idp, gfp_t gfp_mask);
extern int idr_get_new(struct idr *idp, void *ptr, int *id);
extern int idr_get_new_above(struct idr *idp, void *ptr, int starting_id, int *id);
extern int idr_for_each(struct idr *idp,
	int (*fn)(int id, void *p, void *data), void *data);
extern void *idr_get_next(struct idr *idp, int *nextid);
extern void *idr_replace(struct idr *idp, void *ptr, int id);
extern void idr_remove(struct idr *idp, int id);
extern void idr_remove_all(struct idr *idp);
extern void idr_destroy(struct idr *idp);
extern void idr_init(struct idr *idp);


/*
* IDA - IDR based id allocator, use when translation from id to
* pointer isn't necessary.
*/
#define IDA_CHUNK_SIZE		128	/* 128 bytes per chunk */
#ifdef _WIN32
#define IDA_BITMAP_LONGS	(128 / sizeof(LONG_PTR) - 1)
#define IDA_BITMAP_BITS		(IDA_BITMAP_LONGS * sizeof(LONG_PTR) * 8)
#else
#define IDA_BITMAP_LONGS	(128 / sizeof(long) - 1) 
#define IDA_BITMAP_BITS		(IDA_BITMAP_LONGS * sizeof(long) * 8)
#endif


struct ida_bitmap {
	long			nr_busy;
	unsigned long		bitmap[IDA_BITMAP_LONGS];
};

struct ida {
	struct idr		idr;
	struct ida_bitmap	*free_bitmap;
};

#ifdef _WIN32
	// unused
#else
#define IDA_INIT(name)		{ .idr = IDR_INIT(name), .free_bitmap = NULL, }
#define DEFINE_IDA(name)	struct ida name = IDA_INIT(name)
#endif

#ifndef _WIN32
extern int ida_pre_get(struct ida *ida, gfp_t gfp_mask);
extern int ida_get_new_above(struct ida *ida, int starting_id, int *p_id);
extern int ida_get_new(struct ida *ida, int *p_id);
extern void ida_remove(struct ida *ida, int id);
extern void ida_destroy(struct ida *ida);
extern void ida_init(struct ida *ida);

int ida_simple_get(struct ida *ida, unsigned int start, unsigned int end,
	gfp_t gfp_mask);
void ida_simple_remove(struct ida *ida, unsigned int id);
#endif

#ifdef _WIN32
	// unused
#else
void __init idr_init_cache(void);
#endif

#endif /* __IDR_H__ */