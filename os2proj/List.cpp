#include "List.h"
#include <mutex>
#include "KernelProcess.h"

using namespace std;


List::List() : mutex() {
	first = nullptr;
	last = nullptr;
}

List& List::add(KernelProcess *newElem) {
	Elem* temp = new Elem(newElem);
	mutex.lock();
	last = (!first ? first : last->next) = temp;
	mutex.unlock();
	return *this;
}

void List::deleteList() {
	while (first) {
		Elem *old = first;
		first = first->next;
		delete old;
	}
	last = nullptr;
}

List::~List() {
	deleteList();
}

KernelProcess* List::find(int id) {
	for (Elem* cur = first; cur != nullptr; cur = cur->next) {
		if (cur->data->id == id) return cur->data;
	}
	return nullptr;
}

void List::deleteElem(int id) {
	Elem* prev = nullptr;
	for (Elem* cur = first; cur != nullptr; prev = cur, cur = cur->next) {
		if (cur->data->id != id) continue;
		mutex.lock();
		if (cur == first) {
			first = first->next;
			if (first == nullptr) last = nullptr;
		}
		else {
			if (cur == last) last = prev;
			prev->next = cur->next;
		}
		cur->next = nullptr;
		delete cur;
		mutex.unlock();
		break;
	}
}