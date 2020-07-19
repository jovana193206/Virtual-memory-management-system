#pragma once

#include "System.h"
#include "part.h"

class List;
class ResourceAllocator;


class KernelSystem {
public:
	KernelSystem(PhysicalAddress processVMSpace, PageNum processVMSpaceSize,
				 PhysicalAddress pmtSpace, PageNum pmtSpaceSize, Partition* partition);
	~KernelSystem();

	Process* createProcess();

	//Hardware job
	Status access(ProcessId pid, VirtualAddress address, AccessType type);





private:
	int lastId;
	List* processes;
	ResourceAllocator* allocator;   //3 niza za evidenciju slobodnog prostora
	PhysicalAddress processVMSpace;
	PageNum processVMSpaceSize;
	PhysicalAddress pmtSpace;
	PageNum pmtSpaceSize;
	Partition* partition;
	ClusterNo partitionSize;

	friend class ResourceAllocator;
	friend class KernelProcess;
};
