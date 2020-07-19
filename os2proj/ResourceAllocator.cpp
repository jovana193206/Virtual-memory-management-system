#include <iostream>
#include "ResourceAllocator.h"
#include "global.h"
#include "KernelSystem.h"
#include "KernelProcess.h"
#include "List.h"
#include "vm_declarations.h"

using namespace std;

ResourceAllocator::ResourceAllocator(PageNum processVMSpaceSize, PageNum pmtSpaceSize, ClusterNo partitionSize, 
									 KernelSystem *system) : fifoMutex(), vmMutex(), pmtMutex(), partMutex() {
	fifo = 0;
	occupierId = new ProcessId[numFreeVM = processVMSpaceSize];
	freePMTPages = new bool[numFreePMT = pmtSpaceSize];
	freeClusters = new bool[numFreePart = partitionSize];
	for (unsigned int i = 0; i < processVMSpaceSize; i++) occupierId[i] = 0;
	for (unsigned int i = 0; i < pmtSpaceSize; i++) freePMTPages[i] = true;
	for (unsigned int i = 0; i < partitionSize; i++) freeClusters[i] = true;
	sys = system;
}


ResourceAllocator::~ResourceAllocator() {
	delete occupierId;
	delete freePMTPages;
	delete freeClusters;
}

PhysicalAddress ResourceAllocator::allocPMT() {
	if (numFreePMT < PMT_SIZE) return nullptr;
	pmtMutex.lock();
	PageNum i = 0;
	while ((i < sys->pmtSpaceSize) && (freePMTPages[i] == false)) i++;  //trazi prvu slobodnu stranicu
	for (PageNum j = i; j < (i + PMT_SIZE); j++) freePMTPages[j] = false;     //zauzima PMT_SIZE stranica
	numFreePMT -= PMT_SIZE;
	pmtMutex.unlock();
	PhysicalAddress ret = (PhysicalAddress)((char*)sys->pmtSpace + i*PAGE_SIZE);
	return ret;
}

Status ResourceAllocator::deallocPMT(PageNum start) {
	if ((start < 0) || (start >= sys->pmtSpaceSize)) return TRAP;
	pmtMutex.lock();
	for (unsigned int i = start; i < start + PMT_SIZE; i++) freePMTPages[i] = true;
	numFreePMT += PMT_SIZE;
	pmtMutex.unlock();
	return OK;
}

Status ResourceAllocator::deallocVMPage(PageNum num) {
	if ((num < 0) || (num >= sys->processVMSpaceSize)) return TRAP;
	vmMutex.lock();
	occupierId[num] = 0;  //stranica je slobodna (ProcessId-evi krecu od 1)
	numFreeVM++;
	vmMutex.unlock();
	return OK;
}

Status ResourceAllocator::deallocCluster(ClusterNo num) {
	if ((num < 0) || (num >= sys->partition->getNumOfClusters())) return TRAP;
	partMutex.lock();
	freeClusters[num] = true;
	numFreePart++;
	partMutex.unlock();
	return OK;
}

ClusterNo ResourceAllocator::loadToDisk(char* content) {
	if (numFreePart == 0) return -1;
	ClusterNo i = 0;
	partMutex.lock();
	while((i < sys->partitionSize) && (freeClusters[i] == false)) i++;
	int st = sys->partition->writeCluster(i, content);
	if (st == 0) {
		cout << "Error writing to the disk!" << endl;
		return -1;
	}
	freeClusters[i] = false;
	numFreePart--;
	partMutex.unlock();
	return i;
}

PageNum ResourceAllocator::loadVMPage(char* content, ProcessId occupier) {
	PageNum swapPage = 0;
	if (numFreeVM == 0) { //Ako nema slobodne stranice, nalazimo stranicu za zamenu i ako je D=1 njen sadrzaj snimamo u odgovarajuci klaster
		fifoMutex.lock();
		swapPage = fifo;
		fifo = (fifo + 1) % sys->processVMSpaceSize;
		fifoMutex.unlock();
		vmMutex.lock();
		descriptor* pmt = (descriptor*)(sys->processes->find(occupierId[swapPage])->pmtp);
		occupierId[swapPage] = occupier;
		vmMutex.unlock();
		PageNum i = 0;
		for (; i < PMT_ENTRY_NUM; i++) {
			if ((pmt[i].bits & (S_BIT | V_BIT)) && (pmt[i].vmPage == swapPage)) break;
		}
		if (pmt[i].bits & D_BIT) {
			char* content = (char*)(sys->processVMSpace) + swapPage*PAGE_SIZE;
			int st = sys->partition->writeCluster(pmt[i].cluster, content);
			if (st == 0) {
				cout << "Error writing to the disk!" << endl;
				return -1;
			}
			pmt[i].bits &= ~D_BIT;
		}
		pmt[i].bits &= ~V_BIT;
	}
	else {   //Ako postoji slobodna stranica, nadjemo je i odredimo je za swapPage, tj. za ucitavanje nove stranice
		vmMutex.lock();
		while ((swapPage < sys->processVMSpaceSize) && (occupierId[swapPage] != 0)) swapPage++;
		occupierId[swapPage] = occupier;
		numFreeVM--;
		vmMutex.unlock();
	}
	//Ucitavamo trazenu stranicu u swapPage
	vmMutex.lock();
	char* page = (char*)(sys->processVMSpace) + swapPage*PAGE_SIZE;
	for (int j = 0; j < PAGE_SIZE; j++) page[j] = content[j];
	vmMutex.unlock();
	return swapPage;
}

ClusterNo ResourceAllocator::allocCluster() {
	if (numFreePart == 0) return -1;
	ClusterNo i = 0;
	partMutex.lock();
	while ((i < sys->partitionSize) && (freeClusters[i] == false)) i++;
	freeClusters[i] = false;
	numFreePart--;
	partMutex.unlock();
	return i;
}

PageNum ResourceAllocator::getSwapPage() {
	fifoMutex.lock();
	PageNum ret = fifo;
	fifo = (fifo + 1) % sys->processVMSpaceSize;
	fifoMutex.unlock();
	return ret;
}