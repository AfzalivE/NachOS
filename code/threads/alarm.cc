// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"
#include "list.h"


static int Compare(Thread* x, Thread* y) {

    if (x->waketime < y->waketime) {
        return -1;
    } else if (x->waketime == y->waketime) {
        return 0;
    } else {
        return 1;
    }
}

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
    threadlist = new SortedList<Thread*>(Compare);
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------


void 
Alarm::CallBack() 
{
    DEBUG(dbgThread, "Entering Alarm::CallBack");


    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    
    int wtime = threadlist->Front()->waketime;
    if ((wtime >= kernel->stats->totalTicks)) {
        IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);
        kernel->scheduler->ReadyToRun(threadlist->Front());
        (void) threadlist->RemoveFront();
        (void) kernel->interrupt->SetLevel(oldlevel);
    }

//    if ((interruptedthreads.back().time >= kernel->stats->totalTicks)) {
//        IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);
//        kernel->scheduler->ReadyToRun(interruptedthreads.back().thread1);
//        interruptedthreads.pop_back();
//        (void) kernel->interrupt->SetLevel(oldlevel);
//    }

    if (status != IdleMode) {	// is it time to quit?
	   interrupt->YieldOnReturn();
    }
}

//----------------------------------------------------------------------
// Alarm::GoToSleepFor
//
//----------------------------------------------------------------------

// void Alarm::PrintAll (Thread* t) {
// 	DEBUG(dbgThread, timer.);
// }

void
Alarm::GoToSleepFor(int howLong) {

    DEBUG(dbgThread, "Entering Alarm::GoToSleepFor");

    Thread t;
    t = kernel->currentThread;
    t->waketime = kernel->stats->totalTicks + howLong;
  

    threadlist->Insert(temp);
//    interruptedthreads.push_back(temp);
    

    // sort by time descending, smaller time at the end



//    int i = 0; i < threadlist.Apply(PrintAll);

    IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);
    kernel->currentThread->Sleep(false);
    (void) kernel->interrupt->SetLevel(oldlevel);
}
