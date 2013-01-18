/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/tau	           **
*****************************************************************************
**    Copyright 2010  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/****************************************************************************
**	File 		      : memory_wrapper_static.c
**	Description 	: TAU Profiling Package
**	Contact		    : tau-bugs@cs.uoregon.edu
**	Documentation	: See http://www.cs.uoregon.edu/research/tau
**
**  Description   : TAU memory profiler and debugger
**
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <TAU.h>
#include <Profile/Profiler.h>
#include <Profile/TauMemory.h>
#include <memory_wrapper.h>

void * __real_malloc(size_t size);
void * __real_calloc(size_t count, size_t size);
void __real_free(void * ptr);
#ifdef HAVE_MEMALIGN
void * __real_memalign(size_t alignment, size_t size);
#endif
int __real_posix_memalign(void **ptr, size_t alignment, size_t size);
void * __real_realloc(void * ptr, size_t size);
void * __real_valloc(size_t size);
#ifdef HAVE_PVALLOC
void * __real_pvalloc(size_t size);
#endif


int Tau_memory_wrapper_init(void)
{
  static int init = 0;
  if (init) return 0;
  init = 1;

  Tau_global_incr_insideTAU();
  Tau_init_initializeTAU();
  Tau_create_top_level_timer_if_necessary();
  Tau_set_memory_wrapper_present(1);
  Tau_global_decr_insideTAU();
  return 0;
}

int Tau_memory_wrapper_passthrough(void)
{
  return Tau_global_get_insideTAU()
      || !Tau_init_check_initialized()
      || Tau_global_getLightsOut();
}


malloc_t Tau_get_system_malloc()
{
  return __real_malloc;
}

calloc_t Tau_get_system_calloc()
{
  return __real_calloc;
}

realloc_t Tau_get_system_realloc()
{
  return __real_realloc;
}

#ifdef HAVE_MEMALIGN
memalign_t Tau_get_system_memalign()
{
  return __real_memalign;
}
#endif

posix_memalign_t Tau_get_system_posix_memalign()
{
  return __real_posix_memalign;
}

valloc_t Tau_get_system_valloc()
{
  return __real_valloc;
}

#ifdef HAVE_PVALLOC
pvalloc_t Tau_get_system_pvalloc()
{
  return __real_pvalloc;
}
#endif

free_t Tau_get_system_free()
{
  return __real_free;
}


void * __wrap_malloc(size_t size)
{
  return malloc_handle(size);
}

void * __wrap_calloc(size_t count, size_t size)
{
  return calloc_handle(count, size);
}

void __wrap_free(void * ptr)
{
  return free_handle(ptr);
}

#ifdef HAVE_MEMALIGN
void * __wrap_memalign(size_t alignment, size_t size)
{
  return memalign_handle(alignment, size);
}
#endif

int __wrap_posix_memalign(void **ptr, size_t alignment, size_t size)
{
  return posix_memalign_handle(ptr, alignment, size);
}

void * __wrap_realloc(void * ptr, size_t size)
{
  return realloc_handle(ptr, size);
}

void * __wrap_valloc(size_t size)
{
  return valloc_handle(size);
}

#ifdef HAVE_PVALLOC
void * __wrap_pvalloc(size_t size)
{
  return pvalloc_handle(size);
}
#endif

/*********************************************************************
 * EOF
 ********************************************************************/