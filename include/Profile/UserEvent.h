/****************************************************************************
**			TAU Portable Profiling Package			   **
**			http://www.acl.lanl.gov/tau		           **
*****************************************************************************
**    Copyright 1997  						   	   **
**    Department of Computer and Information Science, University of Oregon **
**    Advanced Computing Laboratory, Los Alamos National Laboratory        **
****************************************************************************/
/***************************************************************************
**	File 		: UserEvent.h					  **
**	Description 	: TAU Profiling Package				  **
**	Author		: Sameer Shende					  **
**	Contact		: sameer@cs.uoregon.edu sameer@acl.lanl.gov 	  **
**	Flags		: Compile with				          **
**			  -DPROFILING_ON to enable profiling (ESSENTIAL)  **
**			  -DPROFILE_STATS for Std. Deviation of Excl Time **
**			  -DSGI_HW_COUNTERS for using SGI counters 	  **
**			  -DPROFILE_CALLS  for trace of each invocation   **
**                        -DSGI_TIMERS  for SGI fast nanosecs timer       **
**			  -DTULIP_TIMERS for non-sgi Platform	 	  **
**			  -DPOOMA_STDSTL for using STD STL in POOMA src   **
**			  -DPOOMA_TFLOP for Intel Teraflop at SNL/NM 	  **
**			  -DPOOMA_KAI for KCC compiler 			  **
**			  -DDEBUG_PROF  for internal debugging messages   **
**                        -DPROFILE_CALLSTACK to enable callstack traces  **
**	Documentation	: See http://www.acl.lanl.gov/tau	          **
***************************************************************************/

//////////////////////////////////////////////////////////////////////
// Include Files 
//////////////////////////////////////////////////////////////////////

#define TAU_EVENT_DATATYPE  double

class TauUserEvent {
    
  public: 
    STORAGE(TAU_EVENT_DATATYPE, MinValue);
    STORAGE(TAU_EVENT_DATATYPE, MaxValue);
    STORAGE(TAU_EVENT_DATATYPE, SumValue);
    STORAGE(TAU_EVENT_DATATYPE, SumSqrValue); 
    STORAGE(TAU_EVENT_DATATYPE, LastValueRecorded);
    STORAGE(TAU_EVENT_DATATYPE, UserFunctionValue);
    STORAGE(long, NumEvents);
    bool DisableMin, DisableMax, DisableMean, DisableStdDev;
    string EventName;

    void AddEventToDB();
    TauUserEvent();
    TauUserEvent(const char * EName);
    TauUserEvent(TauUserEvent& );
    TauUserEvent& operator= (const TauUserEvent& );
    void TriggerEvent(TAU_EVENT_DATATYPE data, int tid = RtsLayer::myThread());
    ~TauUserEvent();
    TAU_EVENT_DATATYPE GetMin(int tid = RtsLayer::myThread());
    TAU_EVENT_DATATYPE GetMax(int tid = RtsLayer::myThread());
    TAU_EVENT_DATATYPE GetSumValue(int tid = RtsLayer::myThread());
    TAU_EVENT_DATATYPE GetMean(int tid = RtsLayer::myThread());
    double  GetSumSqr(int tid = RtsLayer::myThread());
    long    GetNumEvents(int tid = RtsLayer::myThread());
    const char *  GetEventName (void) const;
    void SetEventName(const char * newname); 
    void SetEventName(string newname); 
    bool GetDisableMin(void);
    bool GetDisableMax(void);
    bool GetDisableMean(void);
    bool GetDisableStdDev(void);
    void SetDisableMin(bool value);
    void SetDisableMax(bool value);
    void SetDisableMean(bool value);
    void SetDisableStdDev(bool value);
    static void ReportStatistics(bool ForEachThread = false); 
};


TAU_STD_NAMESPACE vector<TauUserEvent*>& TheEventDB(void);
/*    
#ifdef PROFILING_ON
#define TAU_REGISTER_EVENT(event, name)  	TauUserEvent event(name);
#define TAU_EVENT(event, data) 		 	(event).TriggerEvent(data);
#define TAU_EVENT_DISABLE_MIN(event) 		(event).SetDisableMin(true);
#define TAU_EVENT_DISABLE_MAX(event) 		(event).SetDisableMax(true);
#define TAU_EVENT_DISABLE_MEAN(event) 		(event).SetDisableMean(true);
#define TAU_EVENT_DISABLE_STDDEV(event) 	(event).SetDisableStdDev(true);
#define TAU_REPORT_STATISTICS()			TauUserEvent::ReportStatistics()
#define TAU_REPORT_THREAD_STATISTICS()		TauUserEvent::ReportStatistics(true)

#else // PROFILING is disabled
#define TAU_REGISTER_EVENT(event, name)
#define TAU_EVENT(event, data)
#define TAU_EVENT_DISABLE_MIN(event)
#define TAU_EVENT_DISABLE_MAX(event)
#define TAU_EVENT_DISABLE_MEAN(event)
#define TAU_EVENT_DISABLE_STDDEV(event)
#define TAU_STORE_ALL_EVENTS
#define TAU_REPORT_STATISTICS()
#define TAU_REPORT_THREAD_STATISTICS()


#endif // PROFILING_ON 
*/

/***************************************************************************
 * $RCSfile: UserEvent.h,v $   $Author: sameer $
 * $Revision: 1.7 $   $Date: 2002/11/08 02:25:06 $
 * POOMA_VERSION_ID: $Id: UserEvent.h,v 1.7 2002/11/08 02:25:06 sameer Exp $ 
 ***************************************************************************/
