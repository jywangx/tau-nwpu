/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/paracomp/tau
*****************************************************************************
**    Copyright 2004  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: TauUtil.h					  **
**	Description 	: TAU Utilities 
**	Author		: Sameer Shende					  **
**	Contact		: sameer@cs.uoregon.edu 
**	Documentation	: See http://www.cs.uoregon.edu/research/paracomp/tau
***************************************************************************/

#ifndef _TAU_UTIL_H_
#define _TAU_UTIL_H_

#include <stdlib.h> 

/* The following macros help create a local array and assign to elements of 
   the local C array, values from Fortran array after conversion using f2c 
   MPI macros. Need to optimize the implementation. Use static array instead
   of malloc */
   
#define TAU_DECL_LOCAL(mtype, l) mtype *l
#define TAU_ALLOC_LOCAL(mtype, l, size) l = (mtype *) malloc(sizeof(mtype) * size)
#define TAU_DECL_ALLOC_LOCAL(mtype, l, size) TAU_DECL_LOCAL(mtype, l) = TAU_ALLOC_LOCAL(mtype, l, size) 
#define TAU_ASSIGN_VALUES(dest, src, size, func) { int i; for (i = 0; i < size; i++) dest[i] = func(src[i]); }
#define TAU_ASSIGN_STATUS(dest, src, size, func) { int i; for (i = 0; i < size; i++) func(&src[i], &dest[i]); }
#define TAU_FREE_LOCAL(l) free(l)

/******************************************************/

#endif /* _TAU_UTIL_H_ */
