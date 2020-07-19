#pragma once

#include "Process.h"

class KernelSystem;


class KernelProcess {
public:
	KernelProcess(ProcessId pid);
	~KernelProcess();

	Status createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags);
	Status loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void *content);
	Status deleteSegment(VirtualAddress startAddress);

	Status pageFault(VirtualAddress address);
	PhysicalAddress getPhysicalAddress(VirtualAddress address);

	static int getOffset(VirtualAddress address);
	static PageNum getPageNum(VirtualAddress address);

private:
	
	Status checkOverlaps(PageNum startPage, PageNum segmentSize);

	int id;
	PhysicalAddress pmtp;
	KernelSystem* sys;
	friend class Process;
	friend class List;
	friend class KernelSystem;
	friend class ResourceAllocator;
};
