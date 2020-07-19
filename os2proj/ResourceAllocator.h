#pragma once

#include "vm_declarations.h"
#include "part.h"
#include <mutex>

using namespace std;

class KernelSystem;

class ResourceAllocator {
private:
	PageNum fifo; //za VMSpace, na pocetku 0
	ProcessId *occupierId;    // 0 => page is free
	bool *freePMTPages;      //true=free, false=occupied
	bool *freeClusters;
	unsigned int numFreeVM, numFreePMT, numFreePart;
	mutex fifoMutex, vmMutex, pmtMutex, partMutex;
	KernelSystem *sys;

	friend class KernelProcess;

public:
	ResourceAllocator(PageNum processVMSpaceSize, PageNum pmtSpaceSize, ClusterNo partitionSize, KernelSystem *system);
	~ResourceAllocator();

	PhysicalAddress allocPMT();
	Status deallocPMT(PageNum start);
	Status deallocVMPage(PageNum num);
	Status deallocCluster(ClusterNo num);
	ClusterNo allocCluster();
	ClusterNo storeToDisk(PageNum num); //It stores the content of num page from processVMSpace to a free cluster on the disk. 
	//There should be enough space on the disk bu if there is not return error.
	//If the operation succeeds return the number of the cluster you stored data to.
	ClusterNo loadToDisk(char* content); //Return the number of the cluster you loaded the data to.
	PageNum loadVMPage(char* content, ProcessId occupier);
	PageNum getSwapPage();
};
