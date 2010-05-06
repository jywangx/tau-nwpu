/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.cs.uoregon.edu/research/tau	           **
*****************************************************************************
**    Copyright 2010  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/****************************************************************************
**	File 		: TauMemoryWrap.cpp  				   **
**	Description 	: TAU Profiling Package				   **
**	Contact		: tau-bugs@cs.uoregon.edu               	   **
**	Documentation	: See http://www.cs.uoregon.edu/research/tau       **
**                                                                         **
**      Description     : LD_PRELOAD memory wrapper                        **
**                                                                         **
****************************************************************************/

#define _GNU_SOURCE
#include <dlfcn.h>

#define _XOPEN_SOURCE 600 /* see: man posix_memalign */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
  
#include <stdarg.h>

#define TAU_LIBRARY_SOURCE
  
#include <TAU.h>
#include <Profile/Profiler.h>

#include <Profile/TauInit.h>
    
#define dprintf TAU_VERBOSE

#include <vector>
using namespace std;


/*********************************************************************
 * This object represents a memory allocation, it consists of 
 * a location (TAU context) and a number of bytes
 ********************************************************************/
class MemoryAllocation {
public:
  size_t numBytes;
  string location;
  MemoryAllocation() {
    numBytes = 0;
    location = "";
  }
  MemoryAllocation(size_t nBytes, string loc) : numBytes(nBytes), location(loc) {
  }
  MemoryAllocation(size_t nBytes) : numBytes(nBytes), location("") {
  }
};


/*********************************************************************
 * set of global data
 ********************************************************************/
class MemoryWrapGlobal {
public:
  int bytesAllocated;
  map<void*,MemoryAllocation> pointerMap;
  void *heapMemoryUserEvent;

  MemoryWrapGlobal() {
    bytesAllocated = 0;
    heapMemoryUserEvent = 0;
    Tau_get_context_userevent(&heapMemoryUserEvent, "Heap Memory Allocated");
  }
  ~MemoryWrapGlobal() {
    Tau_destructor_trigger();
  }
};


/*********************************************************************
 * access to global data
 ********************************************************************/
static MemoryWrapGlobal& global() {
  static MemoryWrapGlobal memoryWrapGlobal;
  return memoryWrapGlobal;
}

/*********************************************************************
 * return whether we should pass through and not track the IO
 ********************************************************************/
static int Tau_iowrap_checkPassThrough() {
  if (Tau_global_get_insideTAU() > 0 || Tau_global_getLightsOut()) {
    return 1;
  } else {
    return 0;
  }
}


/*********************************************************************
 * hook registered to be called at profile write time, we trigger the leaks here
 ********************************************************************/
void Tau_memorywrap_writeHook() {
  if (!TauEnv_get_track_memory_leaks()) {
    return;
  }
  RtsLayer::LockDB();
  
  map<string, TauUserEvent*> userEventMap; // map location to user event

  map<void*,MemoryAllocation>::const_iterator it;
  for (it=global().pointerMap.begin(); it != global().pointerMap.end(); ++it) { // iterate over still-allocated objects
    
    map<string, TauUserEvent*>::const_iterator search = userEventMap.find(it->second.location);
    if (search == userEventMap.end()) { // not found, create a user event for it
      string s (string("MEMORY LEAK! : ")+it->second.location);
      TauUserEvent *leakEvent = new TauUserEvent(s.c_str());
      userEventMap[it->second.location] = leakEvent;
    }

    // trigger the event
    userEventMap[it->second.location]->TriggerEvent(it->second.numBytes);

    //fprintf (stderr, "[%p] leak of %d bytes, allocated at %s\n", it->first, it->second.numBytes, it->second.location.c_str());
  }
  RtsLayer::UnLockDB();
}

/*********************************************************************
 * initializer
 ********************************************************************/
void Tau_memorywrap_checkInit() {
  static int init = 0;
  if (init == 1) {
    return;
  }
  init = 1;

  Tau_global_incr_insideTAU();
  Tau_init_initializeTAU();
  Tau_create_top_level_timer_if_necessary();
  // register write hook
  Tau_global_addWriteHook(Tau_memorywrap_writeHook);
  Tau_global_decr_insideTAU();
}

/*********************************************************************
 * generate context string
 ********************************************************************/
string Tau_memorywrap_getContextString() {
  int tid = RtsLayer::myThread();
  Profiler *current = TauInternal_CurrentProfiler(tid);
  Profiler *p = current;
  int depth = TauEnv_get_callpath_depth();
  string delimiter(" => ");
  string name("");

  while (current != NULL && depth != 0) {
    if (current != p) {
      name = current->ThisFunction->GetName() + string(" ") +
	current->ThisFunction->GetType() + delimiter + name;
    } else {
      name = current->ThisFunction->GetName() + string (" ") +
	current->ThisFunction->GetType();
    }
    current = current->ParentProfiler;
    depth--;
  }
  return name;
}

/*********************************************************************
 * add a pointer to the collection
 ********************************************************************/
void Tau_memorywrap_add_ptr (void *ptr, size_t size) {
  if (ptr != NULL) {
    RtsLayer::LockDB();
    if (TauEnv_get_track_memory_leaks()) {
      global().pointerMap[ptr] = MemoryAllocation(size, Tau_memorywrap_getContextString());
    } else {
      global().pointerMap[ptr] = MemoryAllocation(size);
    }
    global().bytesAllocated += size;
    RtsLayer::UnLockDB();
  }
}

/*********************************************************************
 * remove a pointer from the collection
 ********************************************************************/
void Tau_memorywrap_remove_ptr (void *ptr) {
  if (ptr != NULL) {
    RtsLayer::LockDB();
    map<void*,MemoryAllocation>::const_iterator it = global().pointerMap.find(ptr);
    if (it != global().pointerMap.end()) {
      global().bytesAllocated -= global().pointerMap[ptr].numBytes;
      global().pointerMap.erase(ptr);
    }
    RtsLayer::UnLockDB();
  }
}


/*********************************************************************
 * malloc
 ********************************************************************/
void *malloc (size_t size) {
  Tau_memorywrap_checkInit();
  static void* (*_malloc)(size_t size) = NULL;

  if (_malloc == NULL) {
    _malloc = ( void* (*)(size_t size)) dlsym(RTLD_NEXT, "malloc");
  }

  if (Tau_iowrap_checkPassThrough()) {
    return _malloc(size);
  }

  Tau_global_incr_insideTAU();

  void *ptr = _malloc(size);
  Tau_memorywrap_add_ptr(ptr, size);
  TAU_CONTEXT_EVENT(global().heapMemoryUserEvent, global().bytesAllocated);
  Tau_global_decr_insideTAU();
  return ptr;
}

/*********************************************************************
 * calloc
 ********************************************************************/
// void *calloc (size_t nmemb, size_t size) {
//   Tau_memorywrap_checkInit();
//   static void* (*_calloc)(size_t nmemb, size_t size) = NULL;

//   if (_calloc == NULL) {
//     _calloc = ( void* (*)(size_t nmemb, size_t size)) dlsym(RTLD_NEXT, "calloc");
//   }

//   if (Tau_iowrap_checkPassThrough()) {
//     return _calloc(nmemb, size);
//   }

//   Tau_global_incr_insideTAU();

//   void *ptr = _calloc(nmemb, size);
//   Tau_memorywrap_add_ptr(ptr, nmemb * size);
//   TAU_CONTEXT_EVENT(global().heapMemoryUserEvent, global().bytesAllocated);
//   Tau_global_decr_insideTAU();
//   return ptr;
// }


/*********************************************************************
 * realloc
 ********************************************************************/
void *realloc (void *ptr, size_t size) {
  Tau_memorywrap_checkInit();
  static void* (*_realloc)(void *ptr, size_t size) = NULL;

  if (_realloc == NULL) {
    _realloc = ( void* (*)(void *ptr, size_t size)) dlsym(RTLD_NEXT, "realloc");
  }

  if (Tau_iowrap_checkPassThrough()) {
    return _realloc(ptr, size);
  }

  Tau_global_incr_insideTAU();

  void *ret_ptr = _realloc(ptr, size);

  Tau_memorywrap_remove_ptr(ptr);
  Tau_memorywrap_add_ptr(ret_ptr, size);

  TAU_CONTEXT_EVENT(global().heapMemoryUserEvent, global().bytesAllocated);
  Tau_global_decr_insideTAU();
  return ret_ptr;
}


/*********************************************************************
 * posix_memalign
 ********************************************************************/
int posix_memalign (void **memptr, size_t alignment, size_t size) {
  Tau_memorywrap_checkInit();
  static int (*_posix_memalign)(void **memptr, size_t alignment, size_t size) = NULL;

  if (_posix_memalign == NULL) {
    _posix_memalign = ( int (*)(void **memptr, size_t alignment, size_t size)) dlsym(RTLD_NEXT, "posix_memalign");
  }

  if (Tau_iowrap_checkPassThrough()) {
    return _posix_memalign(memptr, alignment, size);
  }

  Tau_global_incr_insideTAU();

  int ret = _posix_memalign(memptr, alignment, size);
  if (ret == 0) {
    Tau_memorywrap_add_ptr(*memptr, size);
  }

  TAU_CONTEXT_EVENT(global().heapMemoryUserEvent, global().bytesAllocated);
  Tau_global_decr_insideTAU();
  return ret;
}



/*********************************************************************
 * free
 ********************************************************************/
void free (void *ptr) {
  Tau_memorywrap_checkInit();
  static void (*_free)(void *ptr) = NULL;

  if (_free == NULL) {
    _free = ( void (*)(void *ptr)) dlsym(RTLD_NEXT, "free");
  }

  if (Tau_iowrap_checkPassThrough()) {
    _free(ptr);
    return;
  }

  Tau_global_incr_insideTAU();

  _free(ptr);

  Tau_memorywrap_remove_ptr(ptr);

  TAU_CONTEXT_EVENT(global().heapMemoryUserEvent, global().bytesAllocated);
  Tau_global_decr_insideTAU();
}





/*********************************************************************
 * EOF
 ********************************************************************/
