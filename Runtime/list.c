#include "list.h"

void List_Init(struct ListElement *_start, struct ListElement *_end) {
    _start->Previous = _end->Next = NULL;
    _start->Next = _end;
    _end->Previous = _start;
}

void InsertElement(struct ListElement *_element, struct ListElement *_position) {
    _element->Previous = _position->Previous;
    _element->Next = _position;
    _position->Previous->Next = _element;
    _position->Previous = _element;
}

void DeleteElement(struct ListElement *_element) {
    _element->Previous->Next = _element->Next;
    _element->Next->Previous = _element->Previous;
    _element->Previous = _element->Next = NULL;
}