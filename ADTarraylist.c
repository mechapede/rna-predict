/* ArrayList implementation code */

#include <stdio.h>
#include <assert.h>

#include "utils.h"
#include "ADTarraylist.h"


/* Initiate the list structure to safe default values */
void adtInitiateArrayList(ADTarraylist * list){
    list->num =0;
    list->vals = (void **) xmalloc(sizeof(void *) * 40);
    list->vals_length = 40;
}


/* Cleanup the linkedlist structure of all memory to store ellements, but not the ellements itself */
void adtFreeArrayList(ADTarraylist * list){
    xfree(list->vals);
}


/*Add node to linked list. Returns -1 or failure. 0 otherwise*/
int adtAddArrayValue(ADTarraylist * list, void * val, int index){
    if( list->num >= list->vals_length ){
        list->vals = (void **) xrealloc(list->vals, sizeof(void *) * list->vals_length * 2);
        list->vals_length *= 2;
    }
    assert(index <= list->num); 
    
    for( int i = list->num; i > index; i--){
        list->vals[i] = list->vals[i-1];
    }
    list->vals[index] = val;
    list->num++;
    return 0;
}


/* Add a node to end of linked list */
int adtAddEndArrayValue(ADTarraylist * list, void * val){
    if( list->num >= list->vals_length ){
        list->vals = (void **) xrealloc(list->vals, sizeof(void *) * list->vals_length * 2);
        list->vals_length *= 2;
    }
    list->vals[list->num] = val;
    list->num++;
    return 0;
}


/* Find the index of a value in linked list if it exsists. Must provide compare function. 
 * Returns -1 if not found 
 * */
int adtFindArrayValue(ADTarraylist * list, void * val, int (*compare)(void * val1, void * val2) ){
    for( int i = 0; i < list->num; i++){
        if( compare(list->vals[i],val) ){
            return i;
        }
    }
    return -1;
}


/*Remove node from list. Caller responsible for free. 
 * Returns NULL on failure*/
void * adtPopArrayValue(ADTarraylist * list, int index){
    assert( index < list->num && index >= 0 );
    void * val = list->vals[index];
    for( int i = index; i < list->num-1; i++){
        list->vals[i] = list->vals[i+1];
    }
    list->num--;
    
    return val;
}
