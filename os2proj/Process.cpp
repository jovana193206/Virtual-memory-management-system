#include "Process.h"
#include "KernelProcess.h"

ProcessId Process::getProcessId() const {
	return pProcess->id;
}

Process::Process(ProcessId pid) {
	pProcess = new KernelProcess(pid);
}

Process::~Process() {
	delete pProcess;
}

Status Process::createSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags) {
	return pProcess->createSegment(startAddress, segmentSize, flags);
}

Status Process::loadSegment(VirtualAddress startAddress, PageNum segmentSize, AccessType flags, void* content) {
	return pProcess->loadSegment(startAddress, segmentSize, flags, content);
}

Status Process::deleteSegment(VirtualAddress startAddress) {
	return pProcess->deleteSegment(startAddress);
}

Status Process::pageFault(VirtualAddress address) {
	return pProcess->pageFault(address);
}

PhysicalAddress Process::getPhysicalAddress(VirtualAddress address) {
	return pProcess->getPhysicalAddress(address);
}