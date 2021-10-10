// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "List.h"

ListMessage::ListMessage(void) {
	this->type = -1; // say
} // ListMessage::ListMessage

ListMessage::~ListMessage(void) {
	// nix
} // ListMessage::~ListMessage


ListElem::ListElem(void) {
	this->prev = nullptr;
	this->next = nullptr;
} // ListElem::ListElem

void ListElem::Clone(ListElem **into) {
	if (!*into) *into = new ListElem;
//	super-class->Clone(into);
	(*into)->prev = nullptr;
	(*into)->next = nullptr;
} // ListElem::Clone

ListElem::~ListElem(void) {
	// nix
} // ListElem::~ListElem

void ListElem::Receive(ListMessage *message, bool forward) {
	// abstract
} // ListElem::Receive


ListStruct::ListStruct(void) {
	this->first = nullptr;
	this->last  = nullptr;
	this->elems = 0;
} // ListStruct::ListStruct

void ListStruct::Clone(ListStruct **into) {
	ListElem *orig,*copy;
	int32_t elem;
	
	if (!*into) *into = new ListStruct;
//	super-class->Clone(into);
	(*into)->first = nullptr;
	(*into)->last  = nullptr;
	(*into)->elems = 0;
	for (orig = this->first, elem = 0; elem < this->elems; orig = orig->next, elem++) {
		copy = nullptr;
		orig->Clone(&copy);
		(*into)->InsertAtEnd(copy);
	}
} // ListStruct::Clone

ListStruct::~ListStruct(void) {
	ListElem *curr = nullptr,*prev = nullptr;

	for (curr = this->last; this->elems > 0; this->elems--) {
		if (this->elems > 1) // silence BC
			prev = curr->prev;
		delete curr;
		if (this->elems > 1) // silence BC
			curr = prev;
	}
	this->first = this->last  = nullptr;
} // ListStruct::~ListStruct

void ListStruct::InsertAtStart(ListElem *that) {
	// abstract
} // ListStruct::InsertAtStart

void ListStruct::InsertBefore(ListElem *that, ListElem *before) {
	// abstract
} // ListStruct::InsertBefore

void ListStruct::InsertAfter (ListElem *that, ListElem *after) {
	// abstract
} // ListStruct::InsertAfter

void ListStruct::InsertAtEnd(ListElem *that) {
	// abstract
} // ListStruct::InsertAtEnd

void ListStruct::Remove(ListElem *that) {
	// abstract
} // ListStruct::Remove

void ListStruct::Broadcast(ListMessage *message, bool forward) {
	ListElem *orig;
	int32_t elem;
	
	if (forward)
		for (orig = this->first, elem = 0; elem < this->elems; orig = orig->next, elem++) orig->Receive(message,forward);
	else
		for (orig = this->last, elem = 0; elem < this->elems; orig = orig->prev, elem++) orig->Receive(message,forward);
} // ListStruct::Broadcast

void ListStruct::Delete(ListStruct **list) {
	ListElem *curr = nullptr,*prev = nullptr;

	for (curr = (*list)->last; (*list)->elems > 0; (*list)->elems--) {
		if ((*list)->elems > 1) // silence BC
			prev = curr->prev;

		delete& curr;
#
		if ((*list)->elems > 1) // silence BC
			curr = prev;
	}
	(*list)->first = (*list)->last = nullptr;
	delete *list; *list = nullptr;
} // ListStruct::Delete


LinearListStruct::LinearListStruct(void) {
	// nothing else
} // LinearListStruct::LinearListStruct

void LinearListStruct::Clone(ListStruct **into) {
	if (!*into) *into = new LinearListStruct;
	ListStruct::Clone(into);
} // LinearListStruct::Clone

LinearListStruct::~LinearListStruct(void) {
	// nothing else
} // LinearListStruct::LinearListStruct

void LinearListStruct::InsertAtStart(ListElem *that) {
	that->prev = nullptr; that->next = this->first; this->first = that;
	if (that->next) that->next->prev = that; else this->last = that;
	this->elems++;
} // LinearListStruct::InsertAtStart

void LinearListStruct::InsertBefore(ListElem *that, ListElem *before) {
	if (before == nullptr || before == this->first) {
		this->InsertAtStart(that);
	} else { // before != NULL && before->prev != NULL, assuming "before" is in "this" at all
		that->next = before; that->prev = before->prev;
		before->prev->next = that; before->prev = that;
		this->elems++;
	}
} // LinearListStruct::InsertBefore

void LinearListStruct::InsertAfter (ListElem *that, ListElem *after) {
	if (after == nullptr || after == this->last) {
		this->InsertAtEnd(that);
	} else { // after != NULL && after->next != NULL, assuming "after" is in "this" at all
		that->prev = after; that->next = after->next;
		after->next->prev = that; after->next = that;
		this->elems++;
	}
} // LinearListStruct::InsertAfter

void LinearListStruct::InsertAtEnd(ListElem *that) {
	that->next = nullptr; that->prev = this->last; this->last = that;
	if (that->prev) that->prev->next = that; else this->first = that;
	this->elems++;
} // LinearListStruct::InsertAtEnd

void LinearListStruct::Remove(ListElem *that) { // assuming 'that' is an element of 'this'...
	this->elems--;
	if (that->prev) that->prev->next = that->next; else this->first = that->next;
	if (that->next) that->next->prev = that->prev; else this->last  = that->prev;
	that->prev = nullptr;
	that->next = nullptr;
} // LinearListStruct::Remove


CircularListStruct::CircularListStruct(void) {
	// nothing else
} // CircularListStruct::CircularListStruct

void CircularListStruct::Clone(ListStruct **into) {
	if (!*into) *into = new CircularListStruct;
	ListStruct::Clone(into);
} // CircularListStruct::Clone

CircularListStruct::~CircularListStruct(void) {
	// nothing else
} // CircularListStruct::CircularListStruct

void CircularListStruct::InsertAtStart(ListElem *that) {
	if (this->elems > 0)
		this->InsertBefore(that,this->first);
	else {
		this->first = this->last = that->prev = that->next = that;
		this->elems++;
	};
} // CircularListStruct::InsertAtStart

void CircularListStruct::InsertBefore(ListElem *that, ListElem *before) {
	if (this->elems == 0 || before == nullptr)
		this->InsertAtStart(that);
	else {
		that->next = before;
		that->prev = before->prev;
		that->prev->next = that->next->prev = that;
		if (before == this->first) this->first = that;
		this->elems++;
	}
} // CircularListStruct::InsertBefore

void CircularListStruct::InsertAfter (ListElem *that, ListElem *after) {
	if (this->elems == 0 || after == nullptr)
		this->InsertAtEnd(that);
	else {
		that->prev = after;
		that->next = after->next;
		that->prev->next = that->next->prev = that;
		if (after == this->last) this->last = that;
		this->elems++;
	}
} // CircularListStruct::InsertAfter

void CircularListStruct::InsertAtEnd(ListElem *that) {
	if (this->elems > 0)
		this->InsertAfter(that,this->last);
	else {
		this->first = this->last = that->prev = that->next = that;
		this->elems++;
	}
} // CircularListStruct::InsertAtEnd

void CircularListStruct::Remove(ListElem *that) { // assuming 'that' is an element of 'this'...
	this->elems--;
	if (that == this->last) this->last = that->prev;
	else if (that == this->first) this->first = that->next;
	if (this->elems == 0) {
		this->first = nullptr;
		this->last = nullptr;
	} else {
		that->prev->next = that->next;
		that->next->prev = that->prev;
	}
	that->prev = nullptr;
	that->next = nullptr;
} // CircularListStruct::Remove

void CircularListStruct::Reverse(void) {
	ListElem *curr,*next,*temp;
	int32_t elems;
	
	elems = 0; curr = this->first;
	while (elems < this->elems) {
		next = curr->next;
		temp = curr->next; curr->next = curr->prev; curr->prev = temp;
		curr = next;
		elems++;
	}
	temp = this->first; this->first = this->last; this->last = temp;
} // CircularListStruct::Reverse
