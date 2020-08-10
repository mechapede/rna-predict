/* ArrayList header */

#ifndef _ADTARRAYLIST_H
#define _ADTARRAYLIST_H

#include <stdio.h>

typedef struct ADTarraylist{
    int num;
    void * * vals; //values can be indexed directly
    int vals_length;
} ADTarraylist;

//caller responsible for all memory free/allocations

/* Initiate the list structure to safe default values */
void adtInitiateArrayList(ADTarraylist * list);

/* Cleanup the linkedlist structure of all memory to store ellements, but not he node itself */
void adtFreeArrayList(ADTarraylist * list);

/*Add node to linked list. Returns -1 or failure. 0 otherwise*/
int adtAddArrayValue(ADTarraylist * list, void * val, int index);

/* Add a node to end of linked list */
int adtAddEndArrayValue(ADTarraylist * list, void * val);

/* Find the index of a value in linked list if it exsists. Must provide compare function. 
 * Returns -1 if not found 
 * */
int adtFindArrayValue(ADTarraylist * list, void * val, int (*compare)(void * val1, void * val2) );

/* Get node from list, freeing will cauce corruption in the list 
 * Returns NULL on failure*/
void * adtPeakArrayValue(ADTarraylist * list, int index);

/*Remove node from list. Caller responsible for free. 
 * Returns NULL on failure*/
void * adtPopArrayValue(ADTarraylist * list, int index);


#endif
