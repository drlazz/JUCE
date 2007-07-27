/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-7 by Raw Material Software ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the
   GNU General Public License, as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   JUCE is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with JUCE; if not, visit www.gnu.org/licenses or write to the
   Free Software Foundation, Inc., 59 Temple Place, Suite 330, 
   Boston, MA 02111-1307 USA

  ------------------------------------------------------------------------------

   If you'd like to release a closed-source product which uses JUCE, commercial
   licenses are also available: visit www.rawmaterialsoftware.com/juce for
   more information.

  ==============================================================================
*/

#ifndef __JUCE_THREAD_JUCEHEADER__
#define __JUCE_THREAD_JUCEHEADER__

#include "juce_WaitableEvent.h"
#include "juce_CriticalSection.h"


//==============================================================================
/**
    Encapsulates a thread.

    Subclasses derive from Thread and implement the run() method, in which they
    do their business. The thread can then be started with the startThread() method
    and controlled with various other methods.

    This class also contains some thread-related static methods, such
    as sleep(), yield(), getCurrentThreadId() etc.

    @see CriticalSection, WaitableEvent, Process, ThreadWithProgressWindow,
         MessageManagerLock
*/
class JUCE_API  Thread
{
public:
    //==============================================================================
    /**
        Creates a thread.

        When first created, the thread is not running. Use the startThread()
        method to start it.
    */
    Thread (const String& threadName);

    /** Destructor.

        Deleting a Thread object that is running will only give the thread a
        brief opportunity to stop itself cleanly, so it's recommended that you
        should always call stopThread() with a decent timeout before deleting,
        to avoid the thread being forcibly killed (which is a Bad Thing).
    */
    virtual ~Thread();

    //==============================================================================
    /** Must be implemented to perform the thread's actual code.

        Remember that the thread must regularly check the threadShouldExit()
        method whilst running, and if this returns true it should return from
        the run() method as soon as possible to avoid being forcibly killed.

        @see threadShouldExit, startThread
    */
    virtual void run() = 0;

    //==============================================================================
    // Thread control functions..

    /** Starts the thread running.

        This will start the thread's run() method.
        (if it's already started, startThread() won't do anything).

        @see stopThread
    */
    void startThread() throw();

    /** Starts the thread with a given priority.

        Launches the thread with a given priority, where 0 = lowest, 10 = highest.
        If the thread is already running, its priority will be changed.

        @see startThread, setPriority
    */
    void startThread (const int priority) throw();

    /** Attempts to stop the thread running.

        This method will cause the threadShouldExit() method to return true
        and call notify() in case the thread is currently waiting.

        Hopefully the thread will then respond to this by exiting cleanly, and
        the stopThread method will wait for a given time-period for this to
        happen.

        If the thread is stuck and fails to respond after the time-out, it gets
        forcibly killed, which is a very bad thing to happen, as it could still
        be holding locks, etc. which are needed by other parts of your program.

        @param timeOutMilliseconds  The number of milliseconds to wait for the
                                    thread to finish before killing it by force. A negative
                                    value in here will wait forever.
        @see signalThreadShouldExit, threadShouldExit, waitForThreadToExit, isThreadRunning
    */
    void stopThread (const int timeOutMilliseconds) throw();

    //==============================================================================
    /** Returns true if the thread is currently active */
    bool isThreadRunning() const throw();

    /** Sets a flag to tell the thread it should stop.

        Calling this means that the threadShouldExit() method will then return true.
        The thread should be regularly checking this to see whether it should exit.

        @see threadShouldExit
        @see waitForThreadToExit
    */
    void signalThreadShouldExit() throw();

    /** Checks whether the thread has been told to stop running.

        Threads need to check this regularly, and if it returns true, they should
        return from their run() method at the first possible opportunity.

        @see signalThreadShouldExit
    */
    inline bool threadShouldExit() const throw()  { return threadShouldExit_; }

    /** Waits for the thread to stop.

        This will waits until isThreadRunning() is false or until a timeout expires.

        @param timeOutMilliseconds  the time to wait, in milliseconds. If this value
                                    is less than zero, it will wait forever.
        @returns    true if the thread exits, or false if the timeout expires first.
    */
    bool waitForThreadToExit (const int timeOutMilliseconds) const throw();

    //==============================================================================
    /** Changes the thread's priority.

        @param priority     the new priority, in the range 0 (lowest) to 10 (highest). A priority
                            of 5 is normal.
    */
    void setPriority (const int priority) throw();

    /** Changes the priority of the caller thread.

        Similar to setPriority(), but this static method acts on the caller thread.

        @see setPriority
    */
    static void setCurrentThreadPriority (const int priority) throw();

    //==============================================================================
    /** Sets the affinity mask for the thread.

        This will only have an effect next time the thread is started - i.e. if the
        thread is already running when called, it'll have no effect.

        @see setCurrentThreadAffinityMask
    */
    void setAffinityMask (const uint32 affinityMask) throw();

    /** Changes the affinity mask for the caller thread.

        This will change the affinity mask for the thread that calls this static method.

        @see setAffinityMask
    */
    static void setCurrentThreadAffinityMask (const uint32 affinityMask) throw();

    //==============================================================================
    // this can be called from any thread that needs to pause..
    static void JUCE_CALLTYPE sleep (int milliseconds) throw();

    /** Yields the calling thread's current time-slot. */
    static void JUCE_CALLTYPE yield() throw();

    //==============================================================================
    /** Makes the thread wait for a notification.

        This puts the thread to sleep until either the timeout period expires, or
        another thread calls the notify() method to wake it up.

        @returns    true if the event has been signalled, false if the timeout expires.
    */
    bool wait (const int timeOutMilliseconds) const throw();

    /** Wakes up the thread.

        If the thread has called the wait() method, this will wake it up.

        @see wait
    */
    void notify() const throw();

    //==============================================================================
    /** Returns an id that identifies the caller thread.

        To find the ID of a particular thread object, use getThreadId().

        @returns    a unique identifier that identifies the calling thread.
        @see getThreadId
    */
    static int getCurrentThreadId() throw();

    /** Finds the thread object that is currently running.

        Note that the main UI thread (or other non-Juce threads) don't have a Thread
        object associated with them, so this will return 0.
    */
    static Thread* getCurrentThread() throw();

    /** Returns the ID of this thread.

        That means the ID of this thread object - not of the thread that's calling the method.

        This can change when the thread is started and stopped, and will be invalid if the
        thread's not actually running.

        @see getCurrentThreadId
    */
    int getThreadId() const throw();

    /** Returns the name of the thread.

        This is the name that gets set in the constructor.
    */
    const String getThreadName() const throw()                      { return threadName_; }

    //==============================================================================
    /** Returns the number of currently-running threads.

        @returns  the number of Thread objects known to be currently running.
        @see stopAllThreads
    */
    static int getNumRunningThreads() throw();

    /** Tries to stop all currently-running threads.

        This will attempt to stop all the threads known to be running at the moment.
    */
    static void stopAllThreads (const int timeoutInMillisecs) throw();


    //==============================================================================
    juce_UseDebuggingNewOperator

private:
    const String threadName_;
    void* volatile threadHandle_;
    CriticalSection startStopLock;
    WaitableEvent startSuspensionEvent_, defaultEvent_;

    int threadPriority_, threadId_;
    uint32 affinityMask_;
    bool volatile threadShouldExit_;

    friend void JUCE_API juce_threadEntryPoint (void*);
    static void threadEntryPoint (Thread* thread) throw();

    Thread (const Thread&);
    const Thread& operator= (const Thread&);
};

#endif   // __JUCE_THREAD_JUCEHEADER__
