#ifndef PTRLIST_H_
#define PTRLIST_H_

typedef struct ptrlist{
	void* ptr;
	struct ptrlist* next;
} PtrList;


PtrList* ptrlist_cnext(PtrList *head, PtrList *current);
PtrList* ptrlist_findtail(PtrList *head);
PtrList* ptrlist_find(PtrList *head, void* ptr);
PtrList* ptrlist_add(PtrList *head, void* ptr);
int ptrlist_remove(PtrList *head, PtrList *to_remove);
void ptrlist_destroy(PtrList *head);


#endif /* PTRLIST_H_ */