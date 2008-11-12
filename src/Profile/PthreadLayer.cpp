/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/tau	           **
*****************************************************************************
**    Copyright 1997  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: PthreadLayer.cpp				  **
**	Description 	: TAU Profiling Package RTS Layer definitions     **
**			  for supporting pthreads 			  **
**	Author		: Sameer Shende					  **
**	Contact		: sameer@cs.uoregon.edu sameer@acl.lanl.gov 	  **
**	Flags		: Compile with				          **
**			  -DPROFILING_ON to enable profiling (ESSENTIAL)  **
**			  -DPROFILE_STATS for Std. Deviation of Excl Time **
**			  -DSGI_HW_COUNTERS for using SGI counters 	  **
**			  -DPROFILE_CALLS  for trace of each invocation   **
**			  -DSGI_TIMERS  for SGI fast nanosecs timer	  **
**			  -DTULIP_TIMERS for non-sgi Platform	 	  **
**			  -DPOOMA_STDSTL for using STD STL in POOMA src   **
**			  -DPOOMA_TFLOP for Intel Teraflop at SNL/NM 	  **
**			  -DPOOMA_KAI for KCC compiler 			  **
**			  -DDEBUG_PROF  for internal debugging messages   **
**                        -DPROFILE_CALLSTACK to enable callstack traces  **
**	Documentation	: See http://www.cs.uoregon.edu/research/tau      **
***************************************************************************/


//////////////////////////////////////////////////////////////////////
// Include Files 
//////////////////////////////////////////////////////////////////////

//#define DEBUG_PROF
#ifdef TAU_DOT_H_LESS_HEADERS
#include <iostream>
using namespace std;
#else /* TAU_DOT_H_LESS_HEADERS */
#include <iostream.h>
#endif /* TAU_DOT_H_LESS_HEADERS */
#include "Profile/Profiler.h"

#include <stdlib.h>

#ifdef TAU_CHARM
extern "C" {
#include <cpthreads.h>
}
#else 
#include <pthread.h>
#endif

/////////////////////////////////////////////////////////////////////////
// Member Function Definitions For class PthreadLayer
// This allows us to get thread ids from 0..N-1 instead of long nos 
// as generated by pthread_self() 
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// Define the static private members of PthreadLayer  
/////////////////////////////////////////////////////////////////////////

pthread_key_t PthreadLayer::tauPthreadId;
pthread_mutex_t PthreadLayer::tauThreadcountMutex;
pthread_mutexattr_t PthreadLayer::tauThreadcountAttr;
pthread_mutex_t     PthreadLayer::tauDBMutex;  
pthread_mutex_t     PthreadLayer::tauEnvMutex;  
pthread_mutexattr_t PthreadLayer::tauDBAttr;

int PthreadLayer::tauThreadCount = 0; 

////////////////////////////////////////////////////////////////////////
// RegisterThread() should be called before any profiling routines are
// invoked. This routine sets the thread id that is used by the code in
// FunctionInfo and Profiler classes. This should be the first routine a 
// thread should invoke from its wrapper. Note: main() thread shouldn't
// call this routine. 
////////////////////////////////////////////////////////////////////////
int PthreadLayer::RegisterThread(void)
{
  int *id = (int *) pthread_getspecific(tauPthreadId);
  
  if (id != NULL) {
    return 0;
  }

  int *threadId = new int;

  pthread_mutex_lock(&tauThreadcountMutex);
  tauThreadCount ++;
  // A thread should call this routine exactly once. 
  *threadId = tauThreadCount;
  DEBUGPROFMSG("Thread id "<< tauThreadCount<< " Created! "<<endl;);

  pthread_mutex_unlock(&tauThreadcountMutex);
  pthread_setspecific(tauPthreadId, threadId);

  return 0;
}


////////////////////////////////////////////////////////////////////////
// GetThreadId returns an id in the range 0..N-1 by looking at the 
// thread specific data. Since a getspecific has to be preceeded by a 
// setspecific (that all threads besides main do), we get a null for the
// main thread that lets us identify it as thread 0. It is the only 
// thread that doesn't do a PthreadLayer::RegisterThread(). 
////////////////////////////////////////////////////////////////////////
int PthreadLayer::GetThreadId(void) {
#ifdef TAU_CHARM
  if (RtsLayer::myNode() == -1)
    return 0;
#endif

  static int initflag = PthreadLayer::InitializeThreadData();
  // if its in here the first time, setup mutexes etc.

  int *id = (int *) pthread_getspecific(tauPthreadId);
  
  if (id == NULL) {
    return 0; // main() thread 
  } else { 
    return *id;
  }
}


void PthreadLayer::SetThreadId(int tid) {
  static int initflag = PthreadLayer::InitializeThreadData();
  int *id = new int;
  *id = tid;
  pthread_setspecific(tauPthreadId, id);
  return;
}


////////////////////////////////////////////////////////////////////////
// InitializeThreadData is called before any thread operations are performed. 
// It sets the default values for static private data members of the 
// PthreadLayer class.
////////////////////////////////////////////////////////////////////////
int PthreadLayer::InitializeThreadData(void) {
  static int initflag = 0;
  if (initflag == 0) {
    initflag = 1;
    // Initialize the mutex
    pthread_key_create(&tauPthreadId, NULL);
    pthread_mutexattr_init(&tauThreadcountAttr);
    pthread_mutex_init(&tauThreadcountMutex, &tauThreadcountAttr);
    //cout << "PthreadLayer::Initialize() done! " <<endl;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////
int PthreadLayer::InitializeDBMutexData(void)
{
  // For locking functionDB 
  pthread_mutexattr_init(&tauDBAttr);  
  pthread_mutex_init(&tauDBMutex, &tauDBAttr); 
  
  //cout <<" Initialized the functionDB Mutex data " <<endl;
  return 1;
}

////////////////////////////////////////////////////////////////////////
// LockDB locks the mutex protecting TheFunctionDB() global database of 
// functions. This is required to ensure that push_back() operation 
// performed on this is atomic (and in the case of tracing this is 
// followed by a GetFunctionID() ). This is used in 
// FunctionInfo::FunctionInfoInit().
////////////////////////////////////////////////////////////////////////
int PthreadLayer::LockDB(void)
{
  static int initflag=InitializeDBMutexData();
  // Lock the functionDB mutex
  pthread_mutex_lock(&tauDBMutex);
  return 1;
}

////////////////////////////////////////////////////////////////////////
// UnLockDB() unlocks the mutex tauDBMutex used by the above lock operation
////////////////////////////////////////////////////////////////////////
int PthreadLayer::UnLockDB(void)
{
  // Unlock the functionDB mutex
  pthread_mutex_unlock(&tauDBMutex);
  return 1;
}  


////////////////////////////////////////////////////////////////////////
int PthreadLayer::InitializeEnvMutexData(void)
{
  // For locking functionEnv 
  pthread_mutexattr_init(&tauDBAttr);  
  pthread_mutex_init(&tauEnvMutex, &tauDBAttr); 
  
  //cout <<" Initialized the functionEnv Mutex data " <<endl;
  return 1;
}

////////////////////////////////////////////////////////////////////////
// LockEnv locks the mutex protecting TheFunctionEnv() global database of 
// functions. This is required to ensure that push_back() operation 
// performed on this is atomic (and in the case of tracing this is 
// followed by a GetFunctionID() ). This is used in 
// FunctionInfo::FunctionInfoInit().
////////////////////////////////////////////////////////////////////////
int PthreadLayer::LockEnv(void)
{
  static int initflag=InitializeEnvMutexData();
  // Lock the functionEnv mutex
  pthread_mutex_lock(&tauEnvMutex);
  return 1;
}

////////////////////////////////////////////////////////////////////////
// UnLockEnv() unlocks the mutex tauEnvMutex used by the above lock operation
////////////////////////////////////////////////////////////////////////
int PthreadLayer::UnLockEnv(void)
{
  // Unlock the functionEnv mutex
  pthread_mutex_unlock(&tauEnvMutex);
  return 1;
}  

/* Below is the pthread_create wrapper */
typedef struct tau_pthread_pack {
  void *(*start_routine) (void *);
  void *arg;
  int id;
} tau_pthread_pack;

extern "C" void *tau_pthread_function (void *arg) {
  tau_pthread_pack *pack = (tau_pthread_pack*)arg;
  if (pack->id != -1) {
    TAU_PROFILE_SET_THREAD(pack->id);
  } else {
    TAU_REGISTER_THREAD();
  }
  return pack->start_routine(pack->arg);
}

extern "C" int tau_pthread_create (pthread_t * threadp,
		    const pthread_attr_t *attr,
		    void *(*start_routine) (void *),
		    void *arg) {
  tau_pthread_pack *pack = (tau_pthread_pack*) malloc (sizeof(tau_pthread_pack));
  pack->start_routine = start_routine;
  pack->arg = arg;
  pack->id = -1; // none specified
  return pthread_create(threadp, (pthread_attr_t*) attr, tau_pthread_function, (void*)pack);
}

extern "C" void tau_pthread_exit (void *value_ptr) {
  TAU_PROFILE_EXIT("pthread_exit");
  pthread_exit(value_ptr);
}


extern "C" int tau_track_pthread_create (pthread_t * threadp,
			  const pthread_attr_t *attr,
			  void *(*start_routine) (void *),
			  void *arg, int id) {
  tau_pthread_pack *pack = (tau_pthread_pack*) malloc (sizeof(tau_pthread_pack));
  pack->start_routine = start_routine;
  pack->arg = arg;
  pack->id = id; // set to the specified id
  return pthread_create(threadp, (pthread_attr_t*) attr, tau_pthread_function, (void*)pack);
}


#ifdef TAU_PTHREAD_PRELOAD
#include <dlfcn.h>

static int (*_pthread_create) (pthread_t* thread, const pthread_attr_t* attr, 
			       void *(*start_routine)(void*), void* arg) = NULL;
static void (*_pthread_exit) (void *value_ptr) = NULL;

extern "C" int pthread_create (pthread_t* thread, const pthread_attr_t* attr, 
		    void *(*start_routine)(void*), void* arg) {
  if (_pthread_create == NULL) {
    _pthread_create = (int (*) (pthread_t* thread, const pthread_attr_t* attr, void *(*start_routine)(void*), void* arg)) dlsym(RTLD_NEXT, "pthread_create");
  }

  tau_pthread_pack *pack = (tau_pthread_pack*) malloc (sizeof(tau_pthread_pack));
  pack->start_routine = start_routine;
  pack->arg = arg;
  return _pthread_create(thread, (pthread_attr_t*) attr, tau_pthread_function, (void*)pack);
}

extern "C" void pthread_exit (void *value_ptr) {

  if (_pthread_exit == NULL) {
    _pthread_exit = (void (*) (void *value_ptr)) dlsym(RTLD_NEXT, "pthread_exit");
  }
  TAU_PROFILE_EXIT("pthread_exit");
  _pthread_exit(value_ptr);
}
#endif

/***************************************************************************
 * $RCSfile: PthreadLayer.cpp,v $   $Author: amorris $
 * $Revision: 1.19 $   $Date: 2008/11/12 01:08:49 $
 * POOMA_VERSION_ID: $Id: PthreadLayer.cpp,v 1.19 2008/11/12 01:08:49 amorris Exp $
 ***************************************************************************/
