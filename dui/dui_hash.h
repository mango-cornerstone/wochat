/*-------------------------------------------------------------------------
 *
 * dynahash.c
 *	  dynamic chained hash tables
 *
 * dynahash.c supports both local-to-a-backend hash tables and hash tables in
 * shared memory.  For shared hash tables, it is the caller's responsibility
 * to provide appropriate access interlocking.  The simplest convention is
 * that a single LWLock protects the whole hash table.  Searches (HASH_FIND or
 * hash_seq_search) need only shared lock, but any update requires exclusive
 * lock.  For heavily-used shared tables, the single-lock approach creates a
 * concurrency bottleneck, so we also support "partitioned" locking wherein
 * there are multiple LWLocks guarding distinct subsets of the table.  To use
 * a hash table in partitioned mode, the HASH_PARTITION flag must be given
 * to hash_create.  This prevents any attempt to split buckets on-the-fly.
 * Therefore, each hash bucket chain operates independently, and no fields
 * of the hash header change after init except nentries and freeList.
 * (A partitioned table uses multiple copies of those fields, guarded by
 * spinlocks, for additional concurrency.)
 * This lets any subset of the hash buckets be treated as a separately
 * lockable partition.  We expect callers to use the low-order bits of a
 * lookup key's hash value as a partition number --- this will work because
 * of the way calc_bucket() maps hash values to bucket numbers.
 *
 * For hash tables in shared memory, the memory allocator function should
 * match malloc's semantics of returning NULL on failure.  For hash tables
 * in local memory, we typically use palloc() which will throw error on
 * failure.  The code in this file has to cope with both cases.
 *
 * dynahash.c provides support for these types of lookup keys:
 *
 * 1. Null-terminated C strings (truncated if necessary to fit in keysize),
 * compared as though by strcmp().  This is selected by specifying the
 * HASH_STRINGS flag to hash_create.
 *
 * 2. Arbitrary binary data of size keysize, compared as though by memcmp().
 * (Caller must ensure there are no undefined padding bits in the keys!)
 * This is selected by specifying the HASH_BLOBS flag to hash_create.
 *
 * 3. More complex key behavior can be selected by specifying user-supplied
 * hashing, comparison, and/or key-copying functions.  At least a hashing
 * function must be supplied; comparison defaults to memcmp() and key copying
 * to memcpy() when a user-defined hashing function is selected.
 *
 * Compared to simplehash, dynahash has the following benefits:
 *
 * - It supports partitioning, which is useful for shared memory access using
 *   locks.
 * - Shared memory hashes are allocated in a fixed size area at startup and
 *   are discoverable by name from other processes.
 * - Because entries don't need to be moved in the case of hash conflicts,
 *   dynahash has better performance for large entries.
 * - Guarantees stable pointers to entries.
 *
 * Portions Copyright (c) 1996-2023, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/hash/dynahash.c
 *
 *-------------------------------------------------------------------------
 */

 /*
  * Original comments:
  *
  * Dynamic hashing, after CACM April 1988 pp 446-457, by Per-Ake Larson.
  * Coded into C, with minor code improvements, and with hsearch(3) interface,
  * by ejp@ausmelb.oz, Jul 26, 1988: 13:16;
  * also, hcreate/hdestroy routines added to simulate hsearch(3).
  *
  * These routines simulate hsearch(3) and family, with the important
  * difference that the hash table is dynamic - can grow indefinitely
  * beyond its original size (as supplied to hcreate()).
  *
  * Performance appears to be comparable to that of hsearch(3).
  * The 'source-code' options referred to in hsearch(3)'s 'man' page
  * are not implemented; otherwise functionality is identical.
  *
  * Compilation controls:
  * HASH_DEBUG controls some informative traces, mainly for debugging.
  * HASH_STATISTICS causes HashAccesses and HashCollisions to be maintained;
  * when combined with HASH_DEBUG, these are displayed by hdestroy().
  *
  * Problems & fixes to ejp@ausmelb.oz. WARNING: relies on pre-processor
  * concatenation property, in probably unnecessary code 'optimization'.
  *
  * Modified margo@postgres.berkeley.edu February 1990
  *		added multiple table interface
  * Modified by sullivan@postgres.berkeley.edu April 1990
  *		changed ctl structure for shared memory
  */

#ifndef __WT_DUI_HASH_H__
#define __WT_DUI_HASH_H__

#include "../wochatypes.h"

#ifdef __cplusplus
extern "C" {
#endif

   /*
	* Constants
	*
	* A hash table has a top-level "directory", each of whose entries points
	* to a "segment" of ssize bucket headers.  The maximum number of hash
	* buckets is thus dsize * ssize (but dsize may be expansible).  Of course,
	* the number of records in the table can be larger, but we don't want a
	* whole lot of records per bucket or performance goes down.
	*
	* In a hash table allocated in shared memory, the directory cannot be
	* expanded because it must stay at a fixed address.  The directory size
	* should be selected using hash_select_dirsize (and you'd better have
	* a good idea of the maximum number of entries!).  For non-shared hash
	* tables, the initial directory size can be left at the default.
	*/
#define DEF_SEGSIZE			   256
#define DEF_SEGSIZE_SHIFT	   8	/* must be log2(DEF_SEGSIZE) */
#define DEF_DIRSIZE			   256

/* Number of freelists to be used for a partitioned hash table. */
#define NUM_FREELISTS			32

/*
 * HASHELEMENT is the private part of a hashtable entry.  The caller's data
 * follows the HASHELEMENT structure (on a MAXALIGN'd boundary).  The hash key
 * is expected to be at the start of the caller's hash entry data structure.
 */
typedef struct HASHELEMENT
{
	struct HASHELEMENT* link;	/* link to next entry in same bucket */
	uint32		hashvalue;		/* hash function result for this entry */
} HASHELEMENT;

/* A hash bucket is a linked list of HASHELEMENTs */
typedef HASHELEMENT* HASHBUCKET;

/* A hash segment is an array of bucket headers */
typedef HASHBUCKET* HASHSEGMENT;

/*
 * Per-freelist data.
 *
 * In a partitioned hash table, each freelist is associated with a specific
 * set of hashcodes, as determined by the FREELIST_IDX() macro below.
 * nentries tracks the number of live hashtable entries having those hashcodes
 * (NOT the number of entries in the freelist, as you might expect).
 *
 * The coverage of a freelist might be more or less than one partition, so it
 * needs its own lock rather than relying on caller locking.  Relying on that
 * wouldn't work even if the coverage was the same, because of the occasional
 * need to "borrow" entries from another freelist; see get_hash_entry().
 *
 * Using an array of FreeListData instead of separate arrays of mutexes,
 * nentries and freeLists helps to reduce sharing of cache lines between
 * different mutexes.
 */
typedef struct
{
	long		nentries;		/* number of entries in associated buckets */
	HASHELEMENT* freeList;		/* chain of free elements */
} FreeListData;

/*
 * Header structure for a hash table --- contains all changeable info
 *
 * In a shared-memory hash table, the HASHHDR is in shared memory, while
 * each backend has a local HTAB struct.  For a non-shared table, there isn't
 * any functional difference between HASHHDR and HTAB, but we separate them
 * anyway to share code between shared and non-shared tables.
 */
struct HASHHDR
{
	/*
	 * The freelist can become a point of contention in high-concurrency hash
	 * tables, so we use an array of freelists, each with its own mutex and
	 * nentries count, instead of just a single one.  Although the freelists
	 * normally operate independently, we will scavenge entries from freelists
	 * other than a hashcode's default freelist when necessary.
	 *
	 * If the hash table is not partitioned, only freeList[0] is used and its
	 * spinlock is not used at all; callers' locking is assumed sufficient.
	 */
	FreeListData freeList[NUM_FREELISTS];

	/* These fields can change, but not in a partitioned table */
	/* Also, dsize can't change in a shared table, even if unpartitioned */
	long		dsize;			/* directory size */
	long		nsegs;			/* number of allocated segments (<= dsize) */
	uint32		max_bucket;		/* ID of maximum bucket in use */
	uint32		high_mask;		/* mask to modulo into entire table */
	uint32		low_mask;		/* mask to modulo into lower half of table */

	/* These fields are fixed at hashtable creation */
	Size		keysize;		/* hash key length in bytes */
	Size		entrysize;		/* total user element size in bytes */
	long		num_partitions; /* # partitions (must be power of 2), or 0 */
	long		max_dsize;		/* 'dsize' limit if directory is fixed size */
	long		ssize;			/* segment size --- must be power of 2 */
	int			sshift;			/* segment shift = log2(ssize) */
	int			nelem_alloc;	/* number of entries to allocate at once */

#ifdef HASH_STATISTICS
	/*
	 * Count statistics here.  NB: stats code doesn't bother with mutex, so
	 * counts could be corrupted a bit in a partitioned table.
	 */
	long		accesses;
	long		collisions;
#endif
};

typedef struct HASHHDR HASHHDR;

/*
 * Hash functions must have this signature.
 */
typedef uint32(*HashValueFunc) (const void* key, Size keysize);

/*
 * Key comparison functions must have this signature.  Comparison functions
 * return zero for match, nonzero for no match.  (The comparison function
 * definition is designed to allow memcmp() and strncmp() to be used directly
 * as key comparison functions.)
 */
typedef int (*HashCompareFunc) (const void* key1, const void* key2,	Size keysize);

/*
 * Key copying functions must have this signature.  The return value is not
 * used.  (The definition is set up to allow memcpy() and strlcpy() to be
 * used directly.)
 */
typedef void* (*HashCopyFunc) (void* dest, const void* src, Size keysize);

/*
 * Space allocation function for a hashtable --- designed to match malloc().
 * Note: there is no free function API; can't destroy a hashtable unless you
 * use the default allocator.
 */
typedef void* (*HashAllocFunc) (Size request);

/*
 * Top control structure for a hashtable --- in a shared table, each backend
 * has its own copy (OK since no fields change at runtime)
 */
struct HTAB
{
	HASHHDR* hctl;			/* => shared control information */
	HASHSEGMENT* dir;			/* directory of segment starts */
	HashValueFunc hash;			/* hash function */
	HashCompareFunc match;		/* key comparison function */
	HashCopyFunc keycopy;		/* key copying function */
	HashAllocFunc alloc;		/* memory allocator */
	MemoryPoolContext hcxt;			/* memory context if default allocator used */
	char* tabname;		/* table name (for error messages) */
	bool		isshared;		/* true if table is in shared memory */
	bool		isfixed;		/* if true, don't enlarge */

	/* freezing a shared table isn't allowed, so we can keep state here */
	bool		frozen;			/* true = no more inserts allowed */

	/* We keep local copies of these fixed values to reduce contention */
	Size		keysize;		/* hash key length in bytes */
	long		ssize;			/* segment size --- must be power of 2 */
	int			sshift;			/* segment shift = log2(ssize) */
};

/* Hash table control struct is an opaque type known only within dynahash.c */
typedef struct HTAB HTAB;

/* Flag bits for hash_create; most indicate which parameters are supplied */
#define HASH_PARTITION	0x0001	/* Hashtable is used w/partitioned locking */
#define HASH_SEGMENT	0x0002	/* Set segment size */
#define HASH_DIRSIZE	0x0004	/* Set directory size (initial and max) */
#define HASH_ELEM		0x0008	/* Set keysize and entrysize (now required!) */
#define HASH_STRINGS	0x0010	/* Select support functions for string keys */
#define HASH_BLOBS		0x0020	/* Select support functions for binary keys */
#define HASH_FUNCTION	0x0040	/* Set user defined hash function */
#define HASH_COMPARE	0x0080	/* Set user defined comparison function */
#define HASH_KEYCOPY	0x0100	/* Set user defined key-copying function */
#define HASH_ALLOC		0x0200	/* Set memory allocator */
#define HASH_CONTEXT	0x0400	/* Set memory allocation context */
#define HASH_SHARED_MEM 0x0800	/* Hashtable is in shared memory */
#define HASH_ATTACH		0x1000	/* Do not initialize hctl */
#define HASH_FIXED_SIZE 0x2000	/* Initial size is a hard limit */

/* Parameter data structure for hash_create */
/* Only those fields indicated by hash_flags need be set */
typedef struct HASHCTL
{
	/* Used if HASH_PARTITION flag is set: */
	long		num_partitions; /* # partitions (must be power of 2) */
	/* Used if HASH_SEGMENT flag is set: */
	long		ssize;			/* segment size */
	/* Used if HASH_DIRSIZE flag is set: */
	long		dsize;			/* (initial) directory size */
	long		max_dsize;		/* limit to dsize if dir size is limited */
	/* Used if HASH_ELEM flag is set (which is now required): */
	Size		keysize;		/* hash key length in bytes */
	Size		entrysize;		/* total user element size in bytes */
	/* Used if HASH_FUNCTION flag is set: */
	HashValueFunc hash;			/* hash function */
	/* Used if HASH_COMPARE flag is set: */
	HashCompareFunc match;		/* key comparison function */
	/* Used if HASH_KEYCOPY flag is set: */
	HashCopyFunc keycopy;		/* key copying function */
	/* Used if HASH_ALLOC flag is set: */
	HashAllocFunc alloc;		/* memory allocator */
	/* Used if HASH_CONTEXT flag is set: */
	MemoryPoolContext hcxt;			/* memory context to use for allocations */
	/* Used if HASH_SHARED_MEM flag is set: */
	HASHHDR* hctl;			/* location of header in shared mem */
} HASHCTL;

/* hash_search operations */
typedef enum
{
	HASH_FIND,
	HASH_ENTER,
	HASH_REMOVE,
	HASH_ENTER_NULL
} HASHACTION;

uint32 hash_bytes(const void* key, Size keysize);

HTAB* hash_create(const char* tabname, long nelem, const HASHCTL* info, int flags);

void* hash_search(HTAB* hashp, const void* keyPtr, HASHACTION action, bool* foundPtr);

void hash_destroy(HTAB* hashp);

#ifdef __cplusplus
}
#endif

#endif // __WT_DUI_HASH_H__