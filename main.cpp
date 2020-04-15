#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cmath>
using namespace std;
#endif /* __PROGTEST__ */

struct HeapMemory{
    size_t index;   ////store index of free heap
    int size;
    bool isFree;
    HeapMemory * prev;
    uint8_t * next;

    HeapMemory(size_t index, int size, bool isFree, HeapMemory * prev, uint8_t * next) :
    index(index), size(size), isFree(isFree), prev(prev), next(next) {}

    HeapMemory(){}
};

class MemManager {

     uint8_t * localMemPool;
     uint8_t structSize;
     uint8_t countDone;
     HeapMemory* tmp;

    MemManager(){}

    void        collectFree         ( HeapMemory* fromAdd,  HeapMemory* toAdd);
    void        collectFreeAll      ( HeapMemory* prev,  HeapMemory* now, HeapMemory* next);
    void        makeNull            ( HeapMemory* m);
public:

    static  MemManager  &instance ( )   {   static MemManager mm;   return mm;    }

    void *      Alloc           ( int    size );
    void        Init            ( void * memPool, int memSize);
    bool        Free            ( void * blk );

    size_t      getCountDone    (  )       { return countDone;}

};

void MemManager::Init(void * memPool, int memSize){
    localMemPool =(uint8_t *) memPool;

//    localMemPool[0] = 0;
//    localMemPool[1] = sizeof(HeapMemory);

    countDone = 0;
    structSize = sizeof(HeapMemory);

    HeapMemory freeMem = HeapMemory(structSize, memSize - structSize, true, NULL, NULL);
    memcpy(localMemPool,(uint8_t*) &freeMem, sizeof(freeMem));
}

void * MemManager::Alloc ( int size ){
    tmp = (HeapMemory*)localMemPool;

    while(true){
        if((*tmp).size >= size && (*tmp).isFree) {
            ///Creates next free node
            if((*tmp).size > size) {
                HeapMemory next = HeapMemory(tmp->index + size + structSize, tmp->size - size - structSize, true, tmp, NULL);   ///MAYBE +1
                memcpy(localMemPool + ( *tmp ).index + size, (uint8_t *) &next, sizeof(next));                          ///////problem
                tmp->size = size;
                tmp->next = localMemPool+(*tmp).index+size;
            }

            tmp->isFree = false;
            ++countDone;
            return localMemPool+(*tmp).index;
        }
        if(tmp->next != NULL)
            tmp = (HeapMemory *)tmp->next;
        else
            return NULL;
    }
}

void    MemManager::collectFree(HeapMemory* fromAdd,  HeapMemory* toAdd){
    if(fromAdd->isFree){
        toAdd->size += fromAdd->size+ structSize;
        toAdd->next = fromAdd->next;
        makeNull(fromAdd);
    }
}

void    MemManager::collectFreeAll ( HeapMemory* prev,  HeapMemory* now, HeapMemory* next) {
    if(prev->isFree && next->isFree){                                                       //////problem
        prev->size += now->size + next->size + structSize + structSize;                     //////problem
        prev->next = next->next;

        makeNull(next);
        makeNull(now);
    }
    else if(prev->isFree)
        collectFree(now, prev);
    else if(next->isFree)
        collectFree(next, now);
}

void    MemManager::makeNull      ( HeapMemory* m){
    m->next = NULL;
    m->index = 0;
    m->size = 0;
    m->prev = NULL;
    m->isFree = 0;
}

bool    MemManager::Free    ( void * blk ){
    tmp = (HeapMemory*)blk;
    tmp--;

    if( tmp->isFree || tmp->index == 0)
        return false;

    tmp->isFree = true;

    if(tmp->prev != NULL && tmp->next != NULL){
        collectFreeAll((HeapMemory*)tmp->prev, tmp, (HeapMemory*)tmp->next);
    }
    else if(tmp->prev != NULL)
        collectFree(tmp, (HeapMemory*)tmp->prev);
    else if(tmp->next != NULL)
        collectFree((HeapMemory*)tmp->next, tmp);

    --countDone;
    return true;
}

void   HeapInit    ( void * memPool, int memSize )
{
    MemManager::instance().Init(memPool, memSize);
}

void * HeapAlloc   ( int    size )
{
    if(size < 1)
        return NULL;

    return MemManager::instance().Alloc(size);
}

bool   HeapFree    ( void * blk )
{
    if(blk == NULL)
        return false;

    return MemManager::instance().Free(blk);
}
void   HeapDone    ( int  * pendingBlk )
{
    (*pendingBlk) = MemManager::instance().getCountDone();
}

#ifndef __PROGTEST__
int main ( void )
{
    uint8_t       * p0, *p1, *p2, *p3, *p4;
    int             pendingBlk;
    static uint8_t  memPool[3 * 1048576];
//    uint8_t * memPool = (uint8_t *) calloc (2097152, sizeof(uint8_t));
//    static uint8_t  memPool[3 * 1048576];

    HeapInit ( memPool, 2097152 );
    p0 = (uint8_t*) HeapAlloc ( (2097152/2)-32 );
    memset ( p0, 1, (2097152/2)-32 );
    p1 = (uint8_t*) HeapAlloc ( (2097152/2)-32 );
    memset ( p1, 1, (2097152/2)-32 );
//    printf("%i %i", p0, p1);
    HeapFree ( p0 );
    HeapFree ( p1 );
    p0 = (uint8_t*) HeapAlloc ( 2097152-32 );
    memset ( p0, 1, 2097152-32 );
    HeapFree ( p0 );
    p0 = (uint8_t*) HeapAlloc ( 1 );
    memset ( p0, 1, 1 );
    HeapFree ( p0 );


    HeapInit ( memPool, 1000);
    p0 = (uint8_t*) HeapAlloc ( 100 );
//    printf("%i %i", p0, p0);
    memset ( p0, 1, 100 );
    p1 = (uint8_t*) HeapAlloc ( 250 );
    memset ( p1, 1, 250 );
    p2 = (uint8_t*) HeapAlloc ( 450 );
    memset ( p2, 1, 451 );
    p3 = (uint8_t*) HeapAlloc ( 71 );
    memset ( p3, 1, 71 );

    /*for(int i=0; i < 1020; i++)
        printf("mm   %i    %i\n", i, memPool[i]);*//*
//    printf("%i %i", p0, p3);
    HeapFree ( p1 );
    HeapFree ( p2 );
    p2 = (uint8_t*) HeapAlloc ( 732 );
    memset ( p2, 0, 732 );
//    printf("%i %i", p3, p2);
    HeapFree ( p0 );
    p0 = (uint8_t*) HeapAlloc ( 1 );
    memset ( p0, 0, 2 );
    HeapFree ( p0 );*/



    HeapInit ( memPool, 2097152 );
    assert ( ( p0 = (uint8_t*) HeapAlloc ( 512000 ) ) != NULL );
    memset ( p0, 1, 512000 );
    assert ( ( p1 = (uint8_t*) HeapAlloc ( 511000 ) ) != NULL );
    memset ( p1, 1, 511000 );
    assert ( ( p2 = (uint8_t*) HeapAlloc ( 26000 ) ) != NULL );
    memset ( p2, 1, 26000 );
    HeapDone ( &pendingBlk );
    assert ( pendingBlk == 3 );


    HeapInit ( memPool, 2097152 );
    assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
    memset ( p0, 1, 1000000 );
    assert ( ( p1 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
    memset ( p1, 1, 250000 );
    assert ( ( p2 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
    memset ( p2, 1, 250000 );
    assert ( ( p3 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
    memset ( p3, 1, 250000 );
    assert ( ( p4 = (uint8_t*) HeapAlloc ( 50000 ) ) != NULL );
    memset ( p4, 1, 50000 );
    assert ( HeapFree ( p2 ) );
    assert ( HeapFree ( p4 ) );
    assert ( HeapFree ( p3 ) );
    assert ( HeapFree ( p1 ) );
    assert ( ( p1 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
    memset ( p1, 1, 500000 );
    assert ( HeapFree ( p0 ) );
    assert ( HeapFree ( p1 ) );
    HeapDone ( &pendingBlk );
    assert ( pendingBlk == 0 );
    //////


    HeapInit ( memPool, 2359296 );
    assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
    memset ( p0, 0, 1000000 );
    assert ( ( p1 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
    memset ( p1, 0, 500000 );
    assert ( ( p2 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
    memset ( p2, 0, 500000 );
    assert ( ( p3 = (uint8_t*) HeapAlloc ( 500000 ) ) == NULL );
    assert ( HeapFree ( p2 ) );
    assert ( ( p2 = (uint8_t*) HeapAlloc ( 300000 ) ) != NULL );
    memset ( p2, 0, 300000 );
    assert ( HeapFree ( p0 ) );
    assert ( HeapFree ( p1 ) );
    HeapDone ( &pendingBlk );
    assert ( pendingBlk == 1 );


    HeapInit ( memPool, 2359296 );
    assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
    memset ( p0, 0, 1000000 );
    assert ( ! HeapFree ( p0 + 1000 ) );
    HeapDone ( &pendingBlk );
    assert ( pendingBlk == 1 );
//    free(memPool);
    return 0;
}
#endif /* __PROGTEST__ */

