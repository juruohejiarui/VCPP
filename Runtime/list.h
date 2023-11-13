#pragma once
#include "gloconst.h"

struct ListElement {
    void *Content;
    struct ListElement *Previous, *Next;
};

void List_Init(struct ListElement *_start, struct ListElement *_end);
void List_Insert(struct ListElement *_element, struct ListElement *_p);
void List_Delete(struct ListElement *_element);