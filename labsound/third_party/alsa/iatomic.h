#ifndef __ALSA_IATOMIC_H
#define __ALSA_IATOMIC_H

#ifdef __i386__
#define mb() 	__asm__ __volatile__ ("lock; addl $0,0(%%esp)": : :"memory")
#define rmb()	mb()
#define wmb()	__asm__ __volatile__ ("": : :"memory")
#define IATOMIC_DEFINED		1
#endif 

#ifdef __x86_64__
#define mb() 	asm volatile("mfence":::"memory")
#define rmb()	asm volatile("lfence":::"memory")
#define wmb()	asm volatile("sfence":::"memory")
#define IATOMIC_DEFINED		1
#endif

#ifdef __ia64__
/*
 * Macros to force memory ordering.  In these descriptions, "previous"
 * and "subsequent" refer to program order; "visible" means that all
 * architecturally visible effects of a memory access have occurred
 * (at a minimum, this means the memory has been read or written).
 *
 *   wmb():	Guarantees that all preceding stores to memory-
 *		like regions are visible before any subsequent
 *		stores and that all following stores will be
 *		visible only after all previous stores.
 *   rmb():	Like wmb(), but for reads.
 *   mb():	wmb()/rmb() combo, i.e., all previous memory
 *		accesses are visible before all subsequent
 *		accesses and vice versa.  This is also known as
 *		a "fence."
 *
 * Note: "mb()" and its variants cannot be used as a fence to order
 * accesses to memory mapped I/O registers.  For that, mf.a needs to
 * be used.  However, we don't want to always use mf.a because (a)
 * it's (presumably) much slower than mf and (b) mf.a is supported for
 * sequential memory pages only.
 */
#define mb()	__asm__ __volatile__ ("mf" ::: "memory")
#define rmb()	mb()
#define wmb()	mb()

#define IATOMIC_DEFINED		1

#endif /* __ia64__ */

#ifdef __alpha__

#define mb() \
__asm__ __volatile__("mb": : :"memory")

#define rmb() \
__asm__ __volatile__("mb": : :"memory")

#define wmb() \
__asm__ __volatile__("wmb": : :"memory")

#define IATOMIC_DEFINED		1

#endif /* __alpha__ */

#ifdef __powerpc__

/*
 * Memory barrier.
 * The sync instruction guarantees that all memory accesses initiated
 * by this processor have been performed (with respect to all other
 * mechanisms that access memory).  The eieio instruction is a barrier
 * providing an ordering (separately) for (a) cacheable stores and (b)
 * loads and stores to non-cacheable memory (e.g. I/O devices).
 *
 * mb() prevents loads and stores being reordered across this point.
 * rmb() prevents loads being reordered across this point.
 * wmb() prevents stores being reordered across this point.
 *
 * We can use the eieio instruction for wmb, but since it doesn't
 * give any ordering guarantees about loads, we have to use the
 * stronger but slower sync instruction for mb and rmb.
 */
#define mb()  __asm__ __volatile__ ("sync" : : : "memory")
#define rmb()  __asm__ __volatile__ ("sync" : : : "memory")
#define wmb()  __asm__ __volatile__ ("eieio" : : : "memory")

#define IATOMIC_DEFINED		1

#endif /* __powerpc__ */

#ifndef IATOMIC_DEFINED

/* Generic __sync_synchronize is available from gcc 4.1 */

#define mb() __sync_synchronize()
#define rmb() mb()
#define wmb() mb()

#define IATOMIC_DEFINED		1

#endif /* IATOMIC_DEFINED */

/*
 *  Atomic read/write
 *  Copyright (c) 2001 by Abramo Bagnara <abramo@alsa-project.org>
 */

/* Max number of times we must spin on a spin-lock calling sched_yield().
   After MAX_SPIN_COUNT iterations, we put the calling thread to sleep. */

#ifndef MAX_SPIN_COUNT
#define MAX_SPIN_COUNT 50
#endif

/* Duration of sleep (in nanoseconds) when we can't acquire a spin-lock
   after MAX_SPIN_COUNT iterations of sched_yield().
   This MUST BE > 2ms.
   (Otherwise the kernel does busy-waiting for real-time threads,
    giving other threads no chance to run.) */

#ifndef SPIN_SLEEP_DURATION
#define SPIN_SLEEP_DURATION 2000001
#endif

typedef struct {
	unsigned int begin, end;
} snd_atomic_write_t;

typedef struct {
	volatile const snd_atomic_write_t *write;
	unsigned int end;
} snd_atomic_read_t;

void snd_atomic_read_wait(snd_atomic_read_t *t);

static __inline__ void snd_atomic_write_init(snd_atomic_write_t *w)
{
	w->begin = 0;
	w->end = 0;
}

static __inline__ void snd_atomic_write_begin(snd_atomic_write_t *w)
{
	w->begin++;
	wmb();
}

static __inline__ void snd_atomic_write_end(snd_atomic_write_t *w)
{
	wmb();
	w->end++;
}

static __inline__ void snd_atomic_read_init(snd_atomic_read_t *r, snd_atomic_write_t *w)
{
	r->write = w;
}

static __inline__ void snd_atomic_read_begin(snd_atomic_read_t *r)
{
	r->end = r->write->end;
	rmb();
}

static __inline__ int snd_atomic_read_ok(snd_atomic_read_t *r)
{
	rmb();
	return r->end == r->write->begin;
}

#endif /* __ALSA_IATOMIC_H */
