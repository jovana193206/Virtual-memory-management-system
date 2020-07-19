#pragma once

#include <mutex>

using namespace std;

class KernelProcess;

class List {

private:
	struct Elem {
		KernelProcess *data;
		Elem *next;
		Elem(KernelProcess* t, Elem *n = 0) { data = t; next = n; }
	};

	Elem *first, *last;
	mutex mutex;

	void deleteList();
	
	friend class ResourceAllocator;


public:
	List();
	~List();
	List& add(KernelProcess* newElem);
	KernelProcess* find(int id);
	void deleteElem(int id);
};
