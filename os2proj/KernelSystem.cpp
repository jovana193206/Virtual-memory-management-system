#include <iostream>
#include "KernelSystem.h"
#include "ResourceAllocator.h"
#include "global.h"
#include "List.h"
#include "KernelProcess.h"

using namespace std;

KernelSystem::KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
						   PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition) {
	lastId = 0;
	processes = new List();
	this->processVMSpace = processVMSpace;
	this->processVMSpaceSize = processVMSpaceSize;
	this->pmtSpace = pmtSpace;
	this->pmtSpaceSize = pmtSpaceSize;
	this->partition = partition;
	partitionSize = partition->getNumOfClusters();

	allocator = new ResourceAllocator(processVMSpaceSize, pmtSpaceSize, partition->getNumOfClusters(), this);
	
}

KernelSystem::~KernelSystem() {
	delete processes;
	delete allocator;
}

Process* KernelSystem::createProcess() {
	//alociraj pmt za novi proces ako ima dovoljno prostora u pmtSpace
	PhysicalAddress pmtp = allocator->allocPMT();
	if (pmtp == nullptr) {
		cout << "Error: pmtSpace is full!" << endl;
		return nullptr;
	}
	Process *ret = new Process(++lastId);
	ret->pProcess->pmtp = pmtp;
	ret->pProcess->sys = this;
	//prodji kroz pmt novog procesa i postavi s bit na 0 u svakom deskriptoru (segment nije alociran);
	descriptor* pmt = (descriptor*)pmtp;
	for (int i = 0; i < PMT_ENTRY_NUM; i++) pmt[i].bits = 0;   //postavlja sve bite na 0
	processes->add(ret->pProcess);
	return ret;
}

Status KernelSystem::access(ProcessId pid, VirtualAddress address, AccessType type) {
	descriptor* pmt = (descriptor*)(processes->find(pid)->pmtp);
	PageNum page = KernelProcess::getPageNum(address);
	if ((pmt[page].bits & S_BIT) == 0) return TRAP;
	if ((pmt[page].bits & V_BIT) == 0) return PAGE_FAULT;
	if ((type == READ) && ((pmt[page].bits & READ_BIT) == 0)) return TRAP;
	if (type == WRITE) {
		if ((pmt[page].bits & WRITE_BIT) == 0) return TRAP;
		else pmt[page].bits |= D_BIT;
	}
	if ((type == EXECUTE) && ((pmt[page].bits & EXEC_BIT) == 0)) return TRAP;
	return OK;
}