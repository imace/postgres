/*-------------------------------------------------------------------------
 *
 * array.h
 *	  Utilities for the new array code. Contains prototypes from the
 *	  following files:
 *				utils/adt/arrayfuncs.c
 *				utils/adt/arrayutils.c
 *				utils/adt/chunk.c
 *
 *
 * Portions Copyright (c) 1996-2000, PostgreSQL, Inc
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $Id: array.h,v 1.26 2000/07/17 03:05:32 tgl Exp $
 *
 * NOTES
 *	  XXX the data array should be MAXALIGN'd -- notice that the array
 *	  allocation code does not allocate the extra space required for this,
 *	  even though the array-packing code does the MAXALIGNs.
 *
 *-------------------------------------------------------------------------
 */
#ifndef ARRAY_H
#define ARRAY_H

#include "fmgr.h"

/*
 * Arrays are varlena objects, so must meet the varlena convention that
 * the first int32 of the object contains the total object size in bytes.
 */
typedef struct
{
	int32		size;			/* total array size (varlena requirement) */
	int			ndim;			/* # of dimensions */
	int			flags;			/* implementation flags */
} ArrayType;

/*
 * fmgr macros for array objects
 */
#define DatumGetArrayTypeP(X)         ((ArrayType *) PG_DETOAST_DATUM(X))
#define DatumGetArrayTypePCopy(X)     ((ArrayType *) PG_DETOAST_DATUM_COPY(X))
#define PG_GETARG_ARRAYTYPE_P(n)      DatumGetArrayTypeP(PG_GETARG_DATUM(n))
#define PG_GETARG_ARRAYTYPE_P_COPY(n) DatumGetArrayTypePCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_ARRAYTYPE_P(x)      PG_RETURN_POINTER(x)

/*
 * bitmask of ArrayType flags field:
 * 1st bit - large object flag
 * 2nd bit - chunk flag (array is chunked if set)
 * 3rd,4th,&5th bit - large object type (used only if bit 1 is set)
 */
#define ARR_LOB_FLAG	(0x1)
#define ARR_CHK_FLAG	(0x2)
#define ARR_OBJ_MASK	(0x1c)

#define ARR_SIZE(a)				(((ArrayType *) a)->size)
#define ARR_NDIM(a)				(((ArrayType *) a)->ndim)
#define ARR_FLAGS(a)			(((ArrayType *) a)->flags)

#define ARR_IS_LO(a) \
		(((ArrayType *) a)->flags & ARR_LOB_FLAG)
#define SET_LO_FLAG(f,a) \
		(((ArrayType *) a)->flags |= ((f) ? ARR_LOB_FLAG : 0x0))

#define ARR_IS_CHUNKED(a) \
		(((ArrayType *) a)->flags & ARR_CHK_FLAG)
#define SET_CHUNK_FLAG(f,a) \
		(((ArrayType *) a)->flags |= ((f) ? ARR_CHK_FLAG : 0x0))

#define ARR_OBJ_TYPE(a) \
		((ARR_FLAGS(a) & ARR_OBJ_MASK) >> 2)
#define SET_OBJ_TYPE(f,a) \
		((ARR_FLAGS(a)&= ~ARR_OBJ_MASK), (ARR_FLAGS(a)|=((f<<2)&ARR_OBJ_MASK)))

/*
 * ARR_DIMS returns a pointer to an array of array dimensions (number of
 * elements along the various array axes).
 *
 * ARR_LBOUND returns a pointer to an array of array lower bounds.
 *
 * That is: if the third axis of an array has elements 5 through 10, then
 * ARR_DIMS(a)[2] == 6 and ARR_LBOUND[2] == 5.
 *
 * Unlike C, the default lower bound is 1.
 */
#define ARR_DIMS(a) \
		((int *) (((char *) a) + sizeof(ArrayType)))
#define ARR_LBOUND(a) \
		((int *) (((char *) a) + sizeof(ArrayType) + \
				  (sizeof(int) * (((ArrayType *) a)->ndim))))

/*
 * Returns a pointer to the actual array data.
 */
#define ARR_DATA_PTR(a) \
		(((char *) a) + \
		 MAXALIGN(sizeof(ArrayType) + 2 * (sizeof(int) * (a)->ndim)))

/*
 * The total array header size for an array of dimension n (in bytes).
 */
#define ARR_OVERHEAD(n) \
		(MAXALIGN(sizeof(ArrayType) + 2 * (n) * sizeof(int)))

/*------------------------------------------------------------------------
 * Miscellaneous helper definitions and routines for arrayfuncs.c
 *------------------------------------------------------------------------
 */

#define RETURN_NULL(type)  do { *isNull = true; return (type) 0; } while (0)

#define NAME_LEN	30

typedef struct
{
	char		lo_name[NAME_LEN];
	int			C[MAXDIM];
} CHUNK_INFO;

/*
 * prototypes for functions defined in arrayfuncs.c
 */
extern Datum array_in(PG_FUNCTION_ARGS);
extern Datum array_out(PG_FUNCTION_ARGS);
extern Datum array_eq(PG_FUNCTION_ARGS);
extern Datum array_dims(PG_FUNCTION_ARGS);

extern Datum array_ref(ArrayType *array, int nSubscripts, int *indx,
					   bool elmbyval, int elmlen,
					   int arraylen, bool *isNull);
extern ArrayType *array_set(ArrayType *array, int nSubscripts, int *indx,
							Datum dataValue,
							bool elmbyval, int elmlen,
							int arraylen, bool *isNull);
extern ArrayType *array_clip(ArrayType *array, int nSubscripts,
							 int *upperIndx, int *lowerIndx,
							 bool elmbyval, int elmlen, bool *isNull);
extern ArrayType *array_assgn(ArrayType *array, int nSubscripts,
							  int *upperIndx, int *lowerIndx,
							  ArrayType *newArr,
							  bool elmbyval, int elmlen, bool *isNull);
extern Datum array_map(FunctionCallInfo fcinfo, Oid inpType, Oid retType);

extern ArrayType *construct_array(Datum *elems, int nelems,
								  bool elmbyval, int elmlen, char elmalign);
extern void deconstruct_array(ArrayType *array,
							  bool elmbyval, int elmlen, char elmalign,
							  Datum **elemsp, int *nelemsp);

extern int _LOtransfer(char **destfd, int size, int nitems, char **srcfd,
			int isSrcLO, int isDestLO);
extern char *_array_newLO(int *fd, int flag);


/*
 * prototypes for functions defined in arrayutils.c
 * [these names seem to be too generic. Add prefix for arrays? -- AY]
 */

extern int	GetOffset(int n, int *dim, int *lb, int *indx);
extern int	getNitems(int n, int *a);
extern int	compute_size(int *st, int *endp, int n, int base);
extern void mda_get_offset_values(int n, int *dist, int *PC, int *span);
extern void mda_get_range(int n, int *span, int *st, int *endp);
extern void mda_get_prod(int n, int *range, int *P);
extern int	tuple2linear(int n, int *tup, int *scale);
extern void array2chunk_coord(int n, int *C, int *a_coord, int *c_coord);
extern int	next_tuple(int n, int *curr, int *span);

/*
 * prototypes for functions defined in chunk.c
 */
extern char *_ChunkArray(int fd, FILE *afd, int ndim, int *dim, int baseSize,
			int *nbytes, char *chunkfile);
extern int _ReadChunkArray(int *st, int *endp, int bsize, int fp,
			 char *destfp, ArrayType *array, int isDestLO, bool *isNull);
extern struct varlena *_ReadChunkArray1El(int *st, int bsize, int fp,
				   ArrayType *array, bool *isNull);


#endif	 /* ARRAY_H */
