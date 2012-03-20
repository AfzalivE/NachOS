// exception.cc 
//      Entry point into the Nachos kernel from user programs.
//      There are two kinds of things that can cause control to
//      transfer back to here from user code:
//
//      syscall -- The user code explicitly requests to call a procedure
//      in the Nachos kernel.  Right now, the only function we support is
//      "Halt".
//
//      exceptions -- The user code does something that the CPU can't handle.
//      For instance, accessing memory that doesn't exist, arithmetic errors,
//      etc.  
//
//      Interrupts (which can also cause control to transfer from user
//      code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
//      Entry point into the Nachos kernel.  Called when a user program
//      is executing, and either does a syscall, or generates an addressing
//      or arithmetic exception.
//
//      For system calls, the following is the calling convention:
//
//      system call code -- r2
//             arg1 -- r4
//             arg2 -- r5
//             arg3 -- r6
//             arg4 -- r7
//
//      The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//      "which" is the kind of exception.  The list of possible exceptions 
//      is in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    switch (which) {
        case SyscallException:
            switch(type) {
                case SC_Halt:
                    DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
                    kernel->interrupt->Halt();
                    break;
                default:
                    cerr << "Unexpected system call " << type << "\n";
                    break;
            }
            break;
	    //
	case PageFaultException:
		int virtAddr = kernel->machine->ReadRegister(39);
		unsigned int vpn = (unsigned) virtAddr/PageSize;
		for(int i=0; i < NumPhysPages; i++) 		{
		if(kernel->ipt[i].vPage == vpn && kernel->ipt[i].Process_Id == kernel->currentThread->space)
		{
		IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff); //interrupt disable
		kernel->machine->tlb[kernel->whichTLBPage].virtualPage = kernel->ipt[i].vPage;
		kernel->machine->tlb[kernel->whichTLBPage].physicalPage = kernel->ipt[i].pPage;
		kernel->machine->tlb[kernel->whichTLBPage].valid = TRUE;
		kernel->machine->tlb[kernel->whichTLBPage].use = FALSE;
		kernel->machine->tlb[kernel->whichTLBPage].dirty = FALSE;
		kernel->machine->tlb[kernel->whichTLBPage].readOnly = FALSE;
		kernel->whichTLBPage = (kernel->whichTLBPage + 1) % TLBSize;
		(void) kernel->interrupt->SetLevel(oldlevel); //interrupt enable
		}
		}
		
	break;
        default:
            cerr << "Unexpected user mode exception" << (int)which << "\n";
            break;
    }
    ASSERTNOTREACHED();
}
