#pragma once
#include "gloconst.h"

struct ListElement {
    void *Content;
    struct ListElement *Previous, *Next;
};

static inline void List_Init(struct ListElement *_start, struct ListElement *_end) {
    _start->Previous = _end->Next = NULL;
    _start->Next = _end;
    _end->Previous = _start;
}


static inline void List_Insert(struct ListElement *_element, struct ListElement *_position) {
    _element->Previous = _position->Previous;
    _element->Next = _position;
    _position->Previous->Next = _element;
    _position->Previous = _element;
}

static inline void List_Delete(struct ListElement *_element) {
    _element->Previous->Next = _element->Next;
    _element->Next->Previous = _element->Previous;
    _element->Previous = _element->Next = NULL;
}

static inline void List_Clean(struct ListElement *_start, struct ListElement *_end) {
    for (struct ListElement *_first = _start->Next, *_second; _first != _end; _first = _second) {
        _second = _first->Next;
        List_Delete(_first);
    }
}