/***
* cqueue.c - c circle queue c file
*/

#include "cqueue.h"
#include <stdio.h>

#include "hal_uart.h"

//#define DEBUG_CQUEUE 1

int InitCQueue(CQueue *Q, unsigned int size, CQItemType *buf)
{
	Q->size = size;
    Q->base = buf;
    Q->len  = 0;
    if (!Q->base)
		return CQ_ERR;

    Q->read = Q->write = 0;
	return CQ_OK;
}  
  
int IsEmptyCQueue(CQueue *Q)
{
    if (Q->len == 0)
        return CQ_OK;
    else
        return CQ_ERR;
}

int LengthOfCQueue(CQueue *Q)
{
    return Q->len;
}

int AvailableOfCQueue(CQueue *Q)
{
    return (Q->size - Q->len);
}
  
int EnCQueue(CQueue *Q, CQItemType *e, unsigned int len)
{
	if (AvailableOfCQueue(Q) < len) {
		return CQ_ERR;
	}

	Q->len += len;

	while(len > 0) {
		Q->base[Q->write] = *e;

		++Q->write;
		++e;
		--len;

		if(Q->write >= Q->size)
			Q->write = 0;
	}

    return CQ_OK;
}

int EnCQueueFront(CQueue *Q, CQItemType *e, unsigned int len)
{
	if (AvailableOfCQueue(Q) < len) {
		return CQ_ERR;
	}

	Q->len += len;

	/* walk to last item , revert write */
	e = e + len - 1;

	if(Q->read == 0)
		Q->read = Q->size - 1;
	else
		Q->read--;

	while(len > 0) {
		Q->base[Q->read] = *e;

		--Q->read;
		--e;
		--len;

		if(Q->read < 0)
			Q->read = Q->size - 1;
	}

	/* we walk one more, walk back */
	if(Q->read == Q->size - 1)
		Q->read = 0;
	else
		++Q->read;

    return CQ_OK;
}
  
int DeCQueue(CQueue *Q, CQItemType *e, unsigned int len)
{
	if(LengthOfCQueue(Q) < len)
		return CQ_ERR;

	Q->len -= len;

	if(e != NULL) {
		while(len > 0) {
			*e = Q->base[Q->read];

			++Q->read;
			++e;
			--len;

			if(Q->read == Q->size)
				Q->read = 0;
		}
	}
	else {
		Q->read = (Q->read+len)%Q->size;
	}

    return CQ_OK;
}

int PeekCQueue(CQueue *Q, unsigned int len_want, CQItemType **e1, unsigned int *len1, CQItemType **e2, unsigned int *len2)
{
	if(LengthOfCQueue(Q) < len_want) {
		return CQ_ERR;
    }

	*e1 = &(Q->base[Q->read]);
	if((Q->write > Q->read) || (Q->size - Q->read > len_want)) {
		*len1 = len_want;
		*e2   = NULL;
		*len2 = 0;
		return CQ_OK;
	}
	else {
		*len1 = Q->size - Q->read;
		*e2   = &(Q->base[0]);
		*len2 = len_want - *len1;
		return CQ_OK;
	}

	return CQ_ERR;
}

#ifdef DEBUG_CQUEUE
int DumpCQueue(CQueue *Q)
{
	CQItemType e;
	int len = 0, i = 0;

	len = LengthOfCQueue(Q);

	if(len <= 0)
		return CQ_ERR;

	i = Q->read;
	while(len > 0) {
		e = Q->base[i];

		TRACE(stderr, "-0x%x", e);
		TRACE(stderr, "-%c", e);

		++i;
		--len;
		if(i == Q->size)
			i = 0;
	}

	return CQ_ERR;
}
#endif
