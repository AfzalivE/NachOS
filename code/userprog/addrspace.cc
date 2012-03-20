// addrspace.cc 
//      Routines to manage address spaces (executing user programs).
//
//      In order to run a user program, you must:
//
//      1. link with the -n -T 0 option 
//      2. run coff2noff to convert the object file to Nachos format
//         (Nachos object code format is essentially just a simpler
//         version of the UNIX executable object code format)
//      3. load the NOFF file into the Nachos file system
//         (if you are using the "stub" file system, you
//         don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"

#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
//      Do little endian to big endian conversion on the bytes in the 
//      object file header, in case the file was generated on a little
//      endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//      Create an address space to run a user program.
//      Set up the translation from program memory to physical 
//      memory.  For now, this is really simple (1:1), since we are
//      only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace()
{
    pageTable = new TranslationEntry[NumPhysPages];
    for (int i = 0; i < (signed)NumPhysPages; i++) {
        pageTable[i].virtualPage = i;  // for now virt page # = phys page #
        pageTable[i].physicalPage = i;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;  
    }
    
    // zero out the entire address space
    bzero(kernel->machine->mainMemory, MemorySize);
    
}

//----------------------------------------------------------------------


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//      Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
   delete pageTable;
}

IptEntry::IptEntry() {
kernel->ipt = new IptEntry[NumPhysPages];
	for (int i = 0; i < NumPhysPages; i++) {
	   kernel->ipt[i]->valid = FALSE;
	}
}

IptEntry::~IptEntry() {
    delete kernel->ipt;
}



//----------------------------------------------------------------------
// AddrSpace::Load
//      Load a user program into memory from a file.
//
//      Assumes that the page table has been initialized, and that
//      the object code file is in NOFF format.
//
//      "fileName" -- the file containing the object code to load into 
//                    memory
//----------------------------------------------------------------------

bool 
AddrSpace::Load(char *fileName) 
{
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;
    unsigned int size;

    if (executable == NULL) {
        cerr << "Unable to open file " << fileName << "\n";
        return FALSE;
    }

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
                (WordToHost(noffH.noffMagic) == NOFFMAGIC))
            SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
                        + UserStackSize;   // we need to increase the size
                                           // to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);      // check we're not trying
                                           // to run anything too big --
                                           // at least until we have
                                           // virtual memory

    DEBUG(dbgAddr, "Initializing address space: "<< numPages << ", " << size);

    // then, copy in the code and data segments into memory
    // Note: this code assumes that virtual address = physical address
    if (noffH.code.size > 0) {
        DEBUG(dbgAddr, "Initializing code segment.");
        DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        executable->ReadAt(
                &(kernel->machine->mainMemory[noffH.code.virtualAddr]), 
                        noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initializing data segment.");
        DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        executable->ReadAt(
                &(kernel->machine->mainMemory[noffH.initData.virtualAddr]),
                        noffH.initData.size, noffH.initData.inFileAddr);
    }
    delete executable;                  // close file
    return TRUE;                        // success
}

//----------------------------------------------------------------------
// AddrSpace::Execute
//      Run a user program using the current thread
//
//      The program is assumed to have already been loaded into
//      the address space
//
//----------------------------------------------------------------------

void 
AddrSpace::Execute() 
{

    kernel->currentThread->space = this;

    this->InitRegisters();              // set the initial register values
    this->RestoreState();               // load page table register

    kernel->machine->Run();             // jump to the user progam

    ASSERTNOTREACHED();                 // machine->Run never returns;
                                        // the address space exits
                                        // by doing the syscall "exit"
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//      Set the initial values for the user-level register set.
//
//      We write these directly into the "machine" registers, so
//      that we can immediately jump to user code.  Note that these
//      will be saved/restored into the currentThread->userRegisters
//      when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start", which
    //  is assumed to be virtual address zero
    machine->WriteRegister(PCReg, 0);        

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    // Since instructions occupy four bytes each, the next instruction
    // after start will be at virtual address four.
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages*PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//      On a context switch, save any machine state, specific
//      to this address space, that needs saving.
//
//      For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{

}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//      On a context switch, restore the machine state so that
//      this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    //kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
    if(kernel->currentThread->space != kernel->scheduler->oldThread2->space) {
        for (int i = 0; i < (signed)NumPhysPages; i++) {
        	kernel->ipt[i].vPage = kernel->machine->tlb[kernel->whichTLBPage].virtualPage;
        	kernel->ipt[i].pPage = kernel->machine->tlb[kernel->whichTLBPage].physicalPage;
        	kernel->ipt[i].valid = kernel->machine->tlb[kernel->whichTLBPage].valid;
        	kernel->ipt[i].use = kernel->machine->tlb[kernel->whichTLBPage].use;
        	kernel->ipt[i].dirty = kernel->machine->tlb[kernel->whichTLBPage].dirty;
        	kernel->ipt[i].replacing = kernel->machine->tlb[kernel->whichTLBPage].readOnly;
        	kernel->machine->tlb[i].valid = FALSE;
        }
	}
}
