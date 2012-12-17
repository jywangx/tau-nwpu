/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/tau	           **
*****************************************************************************
**    Copyright 1997  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: pthread_wrap.c				  **
**	Description 	: TAU Profiling Package RTS Layer definitions     **
**			  for wrapping syscalls like exit                 **
**	Contact		: tau-team@cs.uoregon.edu 		 	  **
**	Documentation	: See http://www.cs.uoregon.edu/research/tau      **
***************************************************************************/


// Include Files 
//////////////////////////////////////////////////////////////////////

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdio.h>
#include <TAU.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <Profile/TauEnv.h>

#define dprintf TAU_VERBOSE 

#if (defined (TAU_BGP) || defined(TAU_XLC))
#define TAU_DISABLE_SYSCALL_WRAPPER
#endif /* TAU_BGP || TAU_XLC */

#ifndef TAU_PTHREAD_BARRIER_AVAILABLE
#define TAU_PTHREAD_BARRIER_AVAILABLE
#endif /* TAU_PTHREAD_BARRIER_AVAILABLE */

typedef int (*pthread_create_call_p) 
	(pthread_t *threadp,
	const pthread_attr_t *attr,
	void *(*start_routine) (void *),
	void *arg);

extern int tau_pthread_create_wrapper (pthread_create_call_p pthread_create_call,
pthread_t *threadp, const pthread_attr_t *attr, void *(*start_routine) (void *),
void *arg);

/********************************/
/* LD_PRELOAD wrapper functions */
/********************************/

#ifdef TAU_PRELOAD_LIB
static int (*_pthread_create) (pthread_t* thread, const pthread_attr_t* attr, 
			       void *(*start_routine)(void*), void* arg) = NULL;
static void (*_pthread_exit) (void *value_ptr) = NULL;
static int (*_pthread_join) (pthread_t thread, void ** retval) = NULL;
extern void *tau_pthread_function (void *arg);
typedef struct tau_pthread_pack {
  void *(*start_routine) (void *);
  void *arg;
  int id;
} tau_pthread_pack;


#ifdef TAU_PTHREAD_BARRIER_AVAILABLE
static int (*_pthread_barrier_wait) (pthread_barrier_t *barrier) = NULL;
#endif /* TAU_PTHREAD_BARRIER_AVAILABLE */

int pthread_create (pthread_t* thread, const pthread_attr_t* attr, 
		    void *(*start_routine)(void*), void* arg) {
  if (_pthread_create == NULL) {
    _pthread_create = (int (*) (pthread_t* thread, const pthread_attr_t* attr, void *(*start_routine)(void*), void* arg)) dlsym(RTLD_NEXT, "pthread_create");
  }
	/*
  tau_pthread_pack *pack = (tau_pthread_pack*) malloc (sizeof(tau_pthread_pack));
  pack->start_routine = start_routine;
  pack->arg = arg;
  pack->id = -1;
	*/
  return tau_pthread_create_wrapper(_pthread_create, thread, attr, start_routine, arg);
}

int pthread_join (pthread_t thread, void **retval) {
  int ret;
  if (_pthread_join == NULL) {
    _pthread_join = (int (*) (pthread_t, void **)) dlsym(RTLD_NEXT, "pthread_join"); 
  }
   TAU_PROFILE_TIMER(timer, "pthread_join()", "", TAU_DEFAULT);
   TAU_PROFILE_START(timer);
   ret= _pthread_join(thread, retval); 
   TAU_PROFILE_STOP(timer);
   return ret;
}
void pthread_exit (void *value_ptr) {

  if (_pthread_exit == NULL) {
    _pthread_exit = (void (*) (void *value_ptr)) dlsym(RTLD_NEXT, "pthread_exit");
  }

  TAU_PROFILE_EXIT("pthread_exit");
  _pthread_exit(value_ptr);
}

#ifdef TAU_PTHREAD_BARRIER_AVAILABLE
extern int pthread_barrier_wait(pthread_barrier_t *barrier) {
  int retval;
  if (_pthread_barrier_wait == NULL) {
    _pthread_barrier_wait = (int (*) (pthread_barrier_t *barrier)) dlsym(RTLD_NEXT, "pthread_barrier_wait");
  }
  TAU_PROFILE_TIMER(timer, "pthread_barrier_wait", "", TAU_DEFAULT);
  TAU_PROFILE_START(timer);
  retval = _pthread_barrier_wait (barrier);
  TAU_PROFILE_STOP(timer);
  return retval;
}
#endif /* TAU_PTHREAD_BARRIER_AVAILABLE */

#else // Wra via the the link line.
/*********************************/
/* LD wrappers                   */
/*********************************/
/////////////////////////////////////////////////////////////////////////
// Define PTHREAD wrappers
/////////////////////////////////////////////////////////////////////////

extern void *tau_pthread_function (void *arg);
typedef struct tau_pthread_pack {
  void *(*start_routine) (void *);
  void *arg;
  int id;
} tau_pthread_pack;


int __real_pthread_create (pthread_t* thread, const pthread_attr_t* attr, 
		    void *(*start_routine)(void*), void* arg);
extern int __wrap_pthread_create (pthread_t* thread, const pthread_attr_t* attr, 
		    void *(*start_routine)(void*), void* arg) {
	/*
  tau_pthread_pack *pack = (tau_pthread_pack*) malloc (sizeof(tau_pthread_pack));
  pack->start_routine = start_routine;
  pack->arg = arg;
  pack->id = -1;
	*/
  return tau_pthread_create_wrapper(__real_pthread_create, thread, attr, start_routine, arg);
}

int __real_pthread_join (pthread_t thread, void **retval);
extern int __wrap_pthread_join (pthread_t thread, void **retval) {
  int ret;
   TAU_PROFILE_TIMER(timer, "pthread_join()", "", TAU_DEFAULT);
   TAU_PROFILE_START(timer);
   ret= __real_pthread_join(thread, retval); 
   TAU_PROFILE_STOP(timer);
   return ret;
}
void __real_pthread_exit (void *value_ptr);
extern void __wrap_pthread_exit (void *value_ptr) {

  TAU_PROFILE_EXIT("pthread_exit");
  __real_pthread_exit(value_ptr);
}

#ifdef TAU_PTHREAD_BARRIER_AVAILABLE
int __real_pthread_barrier_wait(pthread_barrier_t *barrier);
int __wrap_pthread_barrier_wait(pthread_barrier_t *barrier) {
  int retval;
  TAU_PROFILE_TIMER(timer, "pthread_barrier_wait", "", TAU_DEFAULT);
  TAU_PROFILE_START(timer);
  retval = __real_pthread_barrier_wait (barrier);
  TAU_PROFILE_STOP(timer);
  return retval;
}
#endif /* TAU_PTHREAD_BARRIER_AVAILABLE */

#endif //TAU_PRELOAD_LIB


/***************************************************************************
 * $RCSfile: TauWrapSyscalls.cpp,v $   $Author: sameer $
 * $Revision: 1.6 $   $Date: 2010/06/10 12:46:53 $
 * TAU_VERSION_ID: $Id: TauWrapSyscalls.cpp,v 1.6 2010/06/10 12:46:53 sameer Exp $
 ***************************************************************************/