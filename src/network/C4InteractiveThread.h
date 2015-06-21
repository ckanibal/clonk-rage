
#ifndef C4INTERACTIVETHREAD_H
#define C4INTERACTIVETHREAD_H

#include "StdScheduler.h"
#include "StdSync.h"

// Event types
enum C4InteractiveEventType {
  Ev_None = 0,

  Ev_Log,
  Ev_LogSilent,
  Ev_LogFatal,

  Ev_FileChange,

  Ev_HTTP_Response,

  Ev_IRC_Message,

  Ev_Net_Conn,
  Ev_Net_Disconn,
  Ev_Net_Packet,

  Ev_Last = Ev_Net_Packet,
};

// Collects StdSchedulerProc objects and executes them in a seperate thread
// Provides an event queue for the procs to communicate with the main thread
class C4InteractiveThread {
 public:
  C4InteractiveThread();
  ~C4InteractiveThread();

  // Event callback interface
  class Callback {
   public:
    virtual void OnThreadEvent(C4InteractiveEventType eEvent,
                               void *pEventData) = 0;
    virtual ~Callback() {}
  };

 private:
  // the thread itself
  StdSchedulerThread Scheduler;

  // event queue (signals to main thread)
  struct Event {
    C4InteractiveEventType Type;
    void *Data;
#ifdef _DEBUG
    int Time;
#endif
    Event *Next;
  };
  Event *pFirstEvent, *pLastEvent;
  CStdCSec EventPushCSec, EventPopCSec;

  // callback objects for events of special types
  Callback *pCallbacks[Ev_Last + 1];

 public:
  // process management
  bool AddProc(StdSchedulerProc *pProc);
  void RemoveProc(StdSchedulerProc *pProc);

  // event queue
  bool PushEvent(C4InteractiveEventType eEventType, void *pData = NULL);
  void ProcessEvents();  // by main thread

  // special events
  bool ThreadLog(const char *szMessage, ...) GNUC_FORMAT_ATTRIBUTE_O;
  bool ThreadLogFatal(const char *szMessage, ...) GNUC_FORMAT_ATTRIBUTE_O;
  bool ThreadLogS(const char *szMessage, ...) GNUC_FORMAT_ATTRIBUTE_O;

  // event handlers
  void SetCallback(C4InteractiveEventType eEvent, Callback *pnNetworkCallback) {
    pCallbacks[eEvent] = pnNetworkCallback;
  }
  void ClearCallback(C4InteractiveEventType eEvent,
                     Callback *pnNetworkCallback) {
    if (pCallbacks[eEvent] == pnNetworkCallback) pCallbacks[eEvent] = NULL;
  }

 private:
  bool PopEvent(C4InteractiveEventType *pEventType,
                void **ppData);  // by main thread
};

#endif  // C4INTERACTIVETHREAD_H
