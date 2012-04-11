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

// A3
void pcUp()
{
        int pc = kernel->machine->ReadRegister(PCReg);
        kernel->machine->WriteRegister(PrevPCReg,pc);
        pc = kernel->machine->ReadRegister(NextPCReg);
        kernel->machine->WriteRegister(PCReg,pc);
        pc+=4;
        kernel->machine->WriteRegister(NextPCReg,pc);
}

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
    int sec;
    switch (which) {
        case SyscallException:
        	int va;
        	char name;
        	int id;
        	char buff;
        	int size;
        	OpenFile *F;

            switch(type) {
                case SC_Halt:
                    DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
                    kernel->interrupt->Halt();
                    break;
                		case SC_Create:
                        bool res;
                        va = kernel->machine->ReadRegister(4);
                        kernel->machine->Translate(va, &sec, 1, false);
                        name = &machine->mainMemory[sec];
                        res = FileSystem->Create(name,64);
                        kernel->machine->WriteRegister(2,(int)res);
                        pcUp();
                        break;
// A3                        	
                // case SC_Open:
                //         va = kernel->machine->ReadRegister(4);
                //         kernel->machine->Translate(va, &sec, 1, false);
                //         name = &machine->mainMemory[sec];
                //         *F = FileSystem->Open(name);
                //         id = ftable->append(name,F);
                //         kernel->currentThread->appendFile(id);
                //         kernel->machine->WriteRegister(2,id);
                //         pcUp();
                //         break;

                case SC_Read:
                        int num;
                        va = kernel->machine->ReadRegister(4);
                        kernel->machine->Translate(va,&sec, 1, false);
                        buff = &machine->mainMemory[sec];
                        size = machine->ReadRegister(5);
                        id = machine->ReadRegister(6);
                        if (id == ConsoleInput) //console output 
                        {
                                r->Acquire();
                                num = 1;
                                *buff = SyCn->ReadChar();
                                r->Release();
                        } else {
                                if(currentThread->findFile(id))
                                        F = ftable->find(id);
                                else
                                        F=NULL;
                                if(F!= NULL)
                                {
                                        num = F->Read(buff,size);
                                        ftable->releaseLock(id);
                                }
                                else
                                {
                                        num = -1;

                                }
                        }
                        machine->WriteRegister(4,(int)buff);
                        machine->WriteRegister(2,num);
                        pcUp();
                        break;

                case SC_Write:
                        va = machine->ReadRegister(4);
                        machine->Translate(va,&sec, 1, false);
                        buff = &kernel->machine->mainMemory[sec];
                        size = kernel->machine->ReadRegister(5);
                        id = kernel->machine->ReadRegister(6);
                        // code = currentThread->ID;

                        /*fout.open("old.txt");
                        int nume;
                        nume = currentThread->space->numPages;
                        for(i=0; i <(int) currentThread->space->numPages*PageSize;i++)
                        {
                                machine->Translate(i,&sec,1,false);
                                va=machine->mainMemory[sec];
                                fout<<i<<" "<<setiosflags(ios::oct)<<va<<endl;
                        }
                        fout.close();*/



                        // if (id == ConsoleOutput) //console input
                        // {
                        //         w->Acquire();
                        //         if(size>1) SyCn->WriteLine(buff,size);
                        //         else SyCn->WriteChar(buff[0]);
                        //         w->Release();
                        // }
                        // else
                        // {
                                if(kernel->currentThread->findFile(id))
                                        F = ftable->find(id);
                                else
                                        F=NULL;
                                if(F!= NULL)
                                        //ftable->getLock(id);
                                        F->Write(buff,size);
                                ftable->releaseLock(id);
                        // }
                        pcUp();
                        break;

                case SC_Close:
                        cout<<"closing"<<endl;
                        int id = machine->ReadRegister(4);
                        kernel->currentThread->closeFile(id);
                        ftable->remove(id);
                        pcUp();
                        break;
                default:
                    cerr << "Unexpected system call " << type << "\n";
                    break;
            }
            break;
// A3 commented A2 code
	// case PageFaultException:
	// 	int virtAddr = kernel->machine->ReadRegister(39);
	// 	unsigned int vpn = (unsigned) virtAddr/PageSize;
	// 	for (int i=0; i < NumPhysPages; i++) {
	// 		if(kernel->ipt[i].vPage == vpn && kernel->ipt[i].Process_Id == kernel->currentThread->space) {
	// 			IntStatus oldlevel = kernel->interrupt->SetLevel(IntOff); //interrupt disable
	// 			kernel->machine->tlb[kernel->whichTLBPage].virtualPage = kernel->ipt[i].vPage;
	// 			kernel->machine->tlb[kernel->whichTLBPage].physicalPage = kernel->ipt[i].pPage;
	// 			kernel->machine->tlb[kernel->whichTLBPage].valid = TRUE;
	// 			kernel->machine->tlb[kernel->whichTLBPage].use = FALSE;
	// 			kernel->machine->tlb[kernel->whichTLBPage].dirty = FALSE;
	// 			kernel->machine->tlb[kernel->whichTLBPage].readOnly = FALSE;
	// 			kernel->whichTLBPage = (kernel->whichTLBPage + 1) % TLBSize;
	// 			(void) kernel->interrupt->SetLevel(oldlevel); //interrupt enable
	// 		}
	// 	}
	// 	kernel->machine->WriteRegister(NextPCReg, PCReg);
	// 	break;

        default:
            cerr << "Unexpected user mode exception" << (int)which << "\n";
            break;
    }
    ASSERTNOTREACHED();
}
