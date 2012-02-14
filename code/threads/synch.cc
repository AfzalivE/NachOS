// synch.cc 
//      Routines for synchronizing threads.  Three kinds of
//      synchronization routines are defined here: semaphores, locks 
//      and condition variables.
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Once we'e implemented one set of higher level atomic operations,
// we can implement others using that implementation.  Here,
// locks and condition variables are implemented on top of 
// semaphores, instead of directly enabling and disabling interrupts.
//
// Locks are implemented using a semaphore to keep track of
// whether the lock is held or not -- a semaphore value of 0 means
// the lock is busy; a semaphore value of 1 means the lock is free.
//
// The implementation of condition variables using semaphores is
// a bit trickier, as explained below under Condition::Wait.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "main.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
//      Initialize a semaphore, so that it can be used for synchronization.
//
//      "debugName" is an arbitrary name, useful for debugging.
//      "initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Lock *lock;
Condition *condition;

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    lock = new Lock("aLock");
    condition = new Condition("aCond");
    // queue = new List<Thread *>;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
//      De-allocate semaphore, when no longer needed.  Assume no one
//      is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    // delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
//      Wait until semaphore value > 0, then decrement.  Checking the
//      value and decrementing must be done atomically, so we
//      need to disable interrupts before checking the value.
//
//      Note that Thread::Sleep assumes that interrupts are disabled
//      when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{

    lock->Acquire();
    if (value <= 0) {
        condition->Wait(Lock);
    } else {
       value--;
    }

    lock->Release();

    // Interrupt *interrupt = kernel->interrupt;
    // Thread *currentThread = kernel->currentThread;
    
    // // disable interrupts
    // IntStatus oldLevel = interrupt->SetLevel(IntOff);        
    
    // if(value <= 0) {                    // semaphore not available
    //     queue->Append(currentThread);   // so go to sleep
    //     currentThread->Sleep(FALSE);
    //     // Thread that woke this thread has decremented value already
    //     // for this thread
    // } else {
    //     value--;            // semaphore available, consume its value
    // }
   
    // re-enable interrupts
//     (void) interrupt->SetLevel(oldLevel);        
}

//----------------------------------------------------------------------
// Semaphore::V
//      Increment semaphore value, waking up a waiter if necessary.
//      As with P(), this operation must be atomic, so we need to disable
//      interrupts.  Scheduler::ReadyToRun() assumes that interrupts
//      are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{

    lock->Acquire();
    value++;
    condition->Signal(aLock);
    lock->Release();

    // Interrupt *interrupt = kernel->interrupt;
    
    // // disable interrupts
    // IntStatus oldLevel = interrupt->SetLevel(IntOff);        
    
    // if (!queue->IsEmpty()) {  // make thread ready.
    //     // There's somebody ready to run, thus they will be given
    //     // possession of the semaphore 
    //     kernel->scheduler->ReadyToRun(queue->RemoveFront());

    // }else {
    //     value++;
    // }
    
    // // re-enable interrupts
    // (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Semaphore::SelfTest, SelfTestHelper
//      Test the semaphore implementation, by using a semaphore
//      to control two threads.  One does P() a couple of
//      times, the other does V().
//----------------------------------------------------------------------

static void
SelfTestHelper (Semaphore *sem) 
{
  sem->V();
  sem->V();
}

void
Semaphore::SelfTest()
{
    Thread *helper = new Thread("semaphore test helper");

    ASSERT(value == 0);                 // otherwise test won't work!
    helper->Fork((VoidFunctionPtr) SelfTestHelper, this);
    this->P();
    this->P();
}

//----------------------------------------------------------------------
// Lock::Lock
//      Initialize a lock, so that it can be used for synchronization.
//      Initially, unlocked.
//
//      "debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

Lock::Lock(char* debugName)
{
    name = debugName;

    queue = new List<Thread *>; // initialize queue // Q3_CHANGE
    value = true;

    lockHolder = NULL;
}

//----------------------------------------------------------------------
// Lock::~Lock
//      Deallocate a lock
//----------------------------------------------------------------------
Lock::~Lock()
{
    delete queue; // Q3_CHANGE 
}

//----------------------------------------------------------------------
// Lock::Acquire
//
//      Note that Thread::Sleep assumes that interrupts are disabled
//      when it is called.
//      Atomically wait until the lock is free, then set it to busy.
//      Equivalent to Semaphore::P(), with the semaphore value of 0
//      equal to busy, and semaphore value of 1 equal to free.
//----------------------------------------------------------------------

void Lock::Acquire()
{  
    Interrupt *interrupt = kernel->interrupt;
    Thread *currentThread = kernel->currentThread;

    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    if (value == false) {
        queue->Append(currentThread);
        kernel->currentThread->Sleep(false);
    } else {
        value = false; // set lock to busy // Q3_CHANGE
    }


    lockHolder = kernel->currentThread;

    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Release
//      Atomically set lock to be free, waking up a thread waiting
//      for the lock, if any.
//      Equivalent to Semaphore::V(), with the semaphore value of 0
//      equal to busy, and semaphore value of 1 equal to free.
//      
//      Using condition variables. // Q2_CHANGE
//
//      By convention, only the thread that acquired the lock
//      may release it.
//---------------------------------------------------------------------

void Lock::Release()
{
    ASSERT(IsHeldByCurrentThread());
    lockHolder = NULL;
    
    Interrupt *interrupt = kernel->interrupt;
    Thread *currentThread = kernel->currentThread;
   

    if (!queue->IsEmpty()) {
        IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
        kernel->scheduler->ReadyToRun(queue->RemoveFront());
        kernel->interrupt->SetLevel(oldLevel);
    } else {
        value = true;   // Set lock to free // Q3_CHANGE
    }
}

//----------------------------------------------------------------------
// Condition::Condition
//      Initialize a condition variable, so that it can be 
//      used for synchronization.  Initially, no one is waiting
//      on the condition.
//
//      "debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------
Condition::Condition(char* debugName)
{
    name = debugName;
    waitQueue = new List<Thread *>;
}

//----------------------------------------------------------------------
// Condition::Condition
//      Deallocate the data structures implementing a condition variable.
//----------------------------------------------------------------------

Condition::~Condition()
{
    delete waitQueue;
}

//----------------------------------------------------------------------
// Condition::Wait
//      Atomically release monitor lock and go to sleep using condition
//      variables.
//      Q2_CHANGE
// 
//      "conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Wait(Lock* conditionLock) 
{
    Interrupt *interrupt = kernel->interrupt;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    Thread *currentThread = kernel->currentThread;
    
    ASSERT(conditionLock->IsHeldByCurrentThread());
    
    conditionLock->Release();

    waitQueue->Append(currentThread);
    currentThread->Sleep(false);
     
    conditionLock->Acquire();

    (void) interrupt->SetLevel(oldLevel);
     
}

//----------------------------------------------------------------------
// Condition::Signal
//      Wake up a thread waiting on this condition, if any, using
//      condition variable.
//      Q2_CHANGE
//
//      "conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Signal(Lock* conditionLock)
{
    Interrupt *interrupt = kernel->interrupt;
    Thread *currentThread = kernel->currentThread;
    
    ASSERT(conditionLock->IsHeldByCurrentThread());
    
    if (!waitQueue->IsEmpty()) {

        IntStatus oldLevel = interrupt->SetLevel(IntOff);

        kernel->scheduler->ReadyToRun(waitQueue->RemoveFront());

        (void) interrupt->SetLevel(oldLevel);
    }
}

//----------------------------------------------------------------------
// Condition::Broadcast
//      Wake up all threads waiting on this condition, if any.
//
//      "conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Broadcast(Lock* conditionLock) 
{
    while (!waitQueue->IsEmpty()) {
        Signal(conditionLock);
    }
}
