#include "KernelProcess.h"
#include "KernelSystem.h"
#include "ResourceAllocator.h"
#include "global.h"
#include "List.h"
#include <iostream>

using namespace std;


KernelProcess::KernelProcess(ProcessId pid) {
	id = pid;
	pmtp = nullptr;
	sys = nullptr;
}

KernelProcess::~KernelProcess() {
	//prvo prodje kroz pmt i oslobodi sve stranice vm memorije i klastere particije koji su bili zauzeti
	descriptor* pmt = (descriptor*)pmtp;
	for (int i = 0; i < PMT_ENTRY_NUM; i++) {
		if ((pmt[i].bits & S_BIT) != 0) {
			if ((pmt[i].bits & V_BIT) != 0) {
				Status st = sys->allocator->deallocVMPage(pmt[i].vmPage);
				if (st == TRAP) cout << "Error, wrong page number!" <<  endl; 
			}
			else {
				Status st = sys->allocator->deallocCluster(pmt[i].cluster);
				if (st == TRAP) cout << "Error, wrong cluster number!" << endl;
			}
		}
	}
	PageNum pmtStart = (PageNum)(((char*)pmtp - (char*)sys->pmtSpace)/PAGE_SIZE);
	Status st = sys->allocator->deallocPMT(pmtStart);
	if (st == TRAP) cout << "Error, wrong page number!" << endl;
	sys->processes->deleteElem(id);
}

int KernelProcess::getOffset(VirtualAddress address) {
	return address % PAGE_SIZE;
}

PageNum KernelProcess::getPageNum(VirtualAddress address) {
	return address / PAGE_SIZE;
}

Status KernelProcess::checkOverlaps(PageNum startPage, PageNum segmentSize) {
	descriptor* pmt = (descriptor*)pmtp;
	for (PageNum i = startPage; i < (startPage + segmentSize); i++) {
		if (pmt[i].bits & S_BIT) return TRAP;
	}
	return OK;
}

Status KernelProcess::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags) {

	if (getOffset(startAddress) != 0) {
		cout << "Error: requested segment is not straightened to the beginning of a page!" << endl;
		return TRAP;
	}
	PageNum startPage = getPageNum(startAddress);
	Status st = checkOverlaps(startPage, segmentSize);
	if (st == TRAP) {
		cout << "Error: requested segment is overlaping with an existing segment!" << endl;
		return TRAP;
	}
	if (sys->allocator->numFreePart < segmentSize) {
		cout << "Error: not enough disk space for new segment!" << endl;
		return TRAP;
	}

	descriptor* pmt = (descriptor*)pmtp;
	pmt[startPage].bits |= SEGSTART_BIT;
	int bitsMask = S_BIT;
	if ((flags == READ) || (flags == READ_WRITE)) bitsMask |= READ_BIT;
	if ((flags == WRITE) || (flags == READ_WRITE)) bitsMask |= WRITE_BIT;
	if (flags == EXECUTE) bitsMask |= EXEC_BIT;

	for (unsigned long i = startPage; i < (startPage + segmentSize); i++) {
		ClusterNo cluster = sys->allocator->allocCluster();
		pmt[i].cluster = cluster;
		pmt[i].bits |= bitsMask;
		if (i == (startPage + segmentSize - 1)) pmt[i].bits |= SEGEND_BIT;
	}
	return OK;
}

Status KernelProcess::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void *content) {

	if (getOffset(startAddress) != 0) {
		cout << "Error: requested segment is not straightened to the beginning of a page!" << endl;
		return TRAP;
	}
	PageNum startPage = getPageNum(startAddress);
	Status st = checkOverlaps(startPage, segmentSize);
	if (st == TRAP) {
		cout << "Error: requested segment is overlaping with an existing segment!" << endl;
		return TRAP;
	}
	if (sys->allocator->numFreePart < segmentSize) {
		cout << "Error: not enough disk space for new segment!" << endl;
		return TRAP;
	}

	descriptor* pmt = (descriptor*)pmtp;
	char* buffer = (char*)content;
	pmt[startPage].bits |= SEGSTART_BIT;
	int bitsMask = S_BIT;
	if ((flags == READ) || (flags == READ_WRITE)) bitsMask |= READ_BIT;
	if ((flags == WRITE) || (flags == READ_WRITE)) bitsMask |= WRITE_BIT;
	if (flags == EXECUTE) bitsMask |= EXEC_BIT;

	for (unsigned long i = startPage; i < (startPage + segmentSize); i++) {
		ClusterNo cluster = sys->allocator->loadToDisk(buffer);
		buffer += PAGE_SIZE;
		pmt[i].cluster = cluster;
		pmt[i].bits |= bitsMask;
		if (i == (startPage + segmentSize - 1)) pmt[i].bits |= SEGEND_BIT;
	}
	return OK;
}

Status KernelProcess::deleteSegment(VirtualAddress startAddress) {
	PageNum startPage = getPageNum(startAddress);
	descriptor* pmt = (descriptor*)pmtp;
	if ((pmt[startPage].bits & (S_BIT | SEGSTART_BIT)) == 0) {
		cout << "Error: startAddress is not the start of a segment!" << endl;
		return TRAP;
	}
	PageNum i = startPage;
	while((pmt[i].bits & SEGEND_BIT) == 0) {
		if ((pmt[i].bits & V_BIT) != 0) {
			Status st = sys->allocator->deallocVMPage(pmt[i].vmPage);
			if (st == TRAP) cout << "Error, wrong page number!" << endl;
		}
		else {
			Status st = sys->allocator->deallocCluster(pmt[i].cluster);;
			if (st == TRAP) cout << "Error, wrong cluster number!" << endl;
		}
		pmt[i].bits = 0;
		i++;
	}
	//iz petlje se izaslo kada smo dosli do poslednjeg deskriptora ovog segmenta, treba osloboditi i taj deskriptor
	if ((pmt[i].bits & V_BIT) != 0) {
		Status st = sys->allocator->deallocVMPage(pmt[i].vmPage);
		if (st == TRAP) cout << "Error, wrong page number!" << endl;
	}
	else {
		Status st = sys->allocator->deallocCluster(pmt[i].cluster);;
		if (st == TRAP) cout << "Error, wrong cluster number!" << endl;
	}
	pmt[i].bits = 0;
	return OK;
}

PhysicalAddress KernelProcess::getPhysicalAddress(VirtualAddress address) {
	PageNum page = getPageNum(address);
	descriptor* pmt = (descriptor*)pmtp;
	return (PhysicalAddress)((char*)sys->processVMSpace + (pmt[page].vmPage * PAGE_SIZE) + getOffset(address));
}

Status KernelProcess::pageFault(VirtualAddress address) {
	PageNum page = getPageNum(address);
	descriptor* pmt = (descriptor*)pmtp;
	if ((pmt[page].bits & V_BIT) || ((pmt[page].bits & S_BIT) == 0)) return TRAP;
	char *content = new char[ClusterSize];
	int st = sys->partition->readCluster(pmt[page].cluster, content);
	if (st == 0) {
		cout << "Error reading from the disk!" << endl;
		delete content;
		return TRAP;
	}
	PageNum num = sys->allocator->loadVMPage(content, id);
	pmt[page].vmPage = num;
	pmt[page].bits |= V_BIT;
	return OK;
}