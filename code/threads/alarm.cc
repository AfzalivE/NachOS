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

int Alarm::ThreadCompare(Threadstruct x, Threadstruct y) {
    if (x.time < y.time) return -1;
    else if (x.time == y.time) return 0;
    else return 1;
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
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    
    if (status != IdleMode) {	// is it time to quit?
	   interrupt->YieldOnReturn();
    }

    if (interruptedthreads->Front->time > kernel->stats->totalTicks) {
        kernel->scheduler->ReadyToRun(interruptedthreads->Front->thread1);
        interruptedthreads->RemoveFront();
    } 

    // if ((interruptedthreads.back().time > kernel->stats->totalTicks)) {
    //     kernel->scheduler->ReadyToRun(interruptedthreads.back().thread1);
    //     interruptedthreads.pop_back();
    // }
}

//----------------------------------------------------------------------
// Alarm::GoToSleepFor
//
//----------------------------------------------------------------------
void
Alarm::GoToSleepFor(int howLong)
{
	Threadstruct temp;
	temp.thread1 = kernel->currentThread;
    temp.time = kernel->stats->totalTicks + howLong;


    interruptedthreads->Insert(temp);   
    
    // if (interruptedthreads.empty()) {
    //     interruptedthreads.push_back(temp);
    // }

    // int i, j;
    // Threadstruct newValue;

    // sort by time descending
    // for (i = 1; i < interruptedthreads.size(); i++) {
    //     newValue = interruptedthreads.at(i);
    //     j = i;
    //     while (j > 0 && interruptedthreads.at(j - 1).time > newValue.time) {
    //           interruptedthreads.at(j) = interruptedthreads.at(j - 1);
    //           j--;
    //     }
    //     interruptedthreads.at(j) = newValue;
    // }

    IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff);
    kernel->currentThread->Sleep(true);
    (void) kernel->interrupt->SetLevel(oldlevel);
}
