#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cmath>

using namespace std;
#endif /* __PROGTEST__ */

struct NewHeapMemory{
    int size;
    uint8_t * next;
    uint8_t * prev;
    uint8_t * nextFree;
    uint8_t * prevFree;
    int nextSize;
    int prevSize;
};

class MemManager {

    uint8_t * localMemPool;
    uint8_t * firstPoint;           //////IF FREE ALL BLOCKS POINTS ON LOCALMEMPOOL
    uint8_t structSize;
    uint64_t countDone=0;
    NewHeapMemory* tmp;
    NewHeapMemory* toAdd;
    int poolSize;
    static MemManager* instanceVar;

    bool freePool = false;

    NewHeapMemory tmpOb;

    MemManager(){}

    void        collectFree         ( NewHeapMemory* fromAdd,  NewHeapMemory* toAdd, bool nextB);
    void        makeNull            ( NewHeapMemory* m);
public:

    static  MemManager  * instance ( void * memPool = 0 )   {
        if(instanceVar == 0)
            instanceVar = (MemManager*) memPool;

        return instanceVar;
    }

    void *      Alloc           ( int    size );
    void        Init            ( void * memPool, int memSize);
    bool        Free            ( void * blk );

    size_t      getCountDone    (  )       { return countDone;}

};
MemManager* MemManager::instanceVar=0;

void MemManager::Init(void * memPool, int memSize){
    localMemPool =(uint8_t *) memPool + sizeof(MemManager);
    firstPoint = localMemPool;
    freePool = true;
    countDone = 0;
    structSize = sizeof(NewHeapMemory);
    poolSize = memSize - sizeof(MemManager);
}

void * MemManager::Alloc ( int size ){
    ////If first allocated block is not equal to start of pool
    tmp = (NewHeapMemory*)firstPoint;

    if(freePool){
        if(poolSize - structSize >= size){
            freePool = false;

            tmpOb.next = NULL;
            tmpOb.size = size;
            tmpOb.prevFree = NULL;
            tmpOb.nextSize = poolSize - structSize - size;
            tmpOb.nextFree = localMemPool+structSize+size;
            tmpOb.prevSize = 0;
            tmpOb.prev = NULL;
            memcpy(localMemPool,(uint8_t*) &tmpOb, structSize);
            ++countDone;
            return localMemPool+structSize;
        }
        else
            return NULL;
    }
    else if(firstPoint != localMemPool && tmp->prevSize - structSize >= size){
        tmpOb.next = (uint8_t*) tmp;
        tmpOb.prev = NULL;

        tmpOb.size = size;
        tmpOb.prevFree = NULL;
        tmpOb.prevSize = 0;

        tmpOb.nextSize = 0;
        tmpOb.nextFree = NULL;
        if(tmp->prevSize - structSize - size > 0) {
            tmpOb.nextSize = tmp->prevSize - structSize - size;
            tmpOb.nextFree = tmp->prevFree + structSize + size;
        }

        tmp->prev = localMemPool;    ///////
        tmp->prevFree = tmpOb.nextFree;
        tmp->prevSize = tmpOb.nextSize;
        memcpy(localMemPool,(uint8_t*) &tmpOb, structSize);
        firstPoint = localMemPool;

        ++countDone;
        return tmp->prevFree+structSize;
    }

    while ( true ){
        if(tmp->nextSize - structSize >= size){
            tmpOb.next = tmp->next;
            tmpOb.size = size;
            tmpOb.prevFree = NULL;
            tmpOb.prevSize = 0;

            tmpOb.nextSize = 0;
            tmpOb.nextFree = NULL;
            if(tmp->nextSize - structSize - size > 0) {
                tmpOb.nextSize = tmp->nextSize - structSize - size;
                tmpOb.nextFree = tmp->nextFree + structSize + size;
            }

            tmpOb.prev = (uint8_t*)tmp;

            tmp->next = tmp->nextFree;
            tmp->nextFree = NULL;
            tmp->nextSize = 0;

            memcpy(tmp->next,(uint8_t*) &tmpOb, structSize);
            ++countDone;
            return tmp->next+structSize;

        }
        if(tmp->next != NULL)
            tmp = (NewHeapMemory*)tmp->next;
        else
            return NULL;
    }
}

bool    MemManager::Free    ( void * blk ){
    tmp = (NewHeapMemory*)blk;
    tmp--;
    bool nextB=false;

    if(freePool || (tmp->nextFree == NULL && tmp->next == NULL) )
        return false;

    if(tmp->next != NULL) {
        toAdd = (NewHeapMemory *) tmp->next;
        nextB = true;
    }
    else if(tmp->prev != NULL)
        toAdd = (NewHeapMemory*)tmp->prev;
    else {
        freePool = true;
        firstPoint = localMemPool;
        makeNull(tmp);
        --countDone;
        return true;                    ///////check if correct
    }

    collectFree(tmp, toAdd, nextB);
    makeNull(tmp);

    --countDone;
    return true;
}

void    MemManager::collectFree(NewHeapMemory* fromAdd,  NewHeapMemory* toAdd, bool nextB){
    if(nextB) {
        toAdd->prevSize += fromAdd->size + structSize + fromAdd->prevSize;
        toAdd->prev = fromAdd->prev;
        if(fromAdd->prevFree != NULL)
            toAdd->prevFree = fromAdd->prevFree;
        else
            toAdd->prevFree = (uint8_t*)fromAdd;
    }
    else{
        toAdd->nextSize += fromAdd->size + structSize + fromAdd->nextSize;
        toAdd->next = fromAdd->next;
        if(toAdd->nextFree == NULL)
            toAdd->nextFree = (uint8_t*)fromAdd;
    }
    if(firstPoint == (uint8_t*)fromAdd)
        firstPoint = (uint8_t*) toAdd;

}

void    MemManager::makeNull      ( NewHeapMemory * m){
    m->prevSize = 0;
    m->next = NULL;
    m->size = 0;
    m->prev = NULL;
    m->prevFree = NULL;
    m->nextFree = NULL;
    m->nextSize = 0;
}

void   HeapInit    ( void * memPool, int memSize )
{
    MemManager::instance(memPool)->Init(memPool, memSize);
}

void * HeapAlloc   ( int    size )
{
    return MemManager::instance()->Alloc(size);
}

bool   HeapFree    ( void * blk )
{
    if(blk == NULL)
        return false;

    return MemManager::instance()->Free(blk);
}
void   HeapDone    ( int  * pendingBlk )
{
    (*pendingBlk) = MemManager::instance()->getCountDone();
}

#ifndef __PROGTEST__

int main ( void )
{
    uint8_t       * p0, *p1, *p2, *p3, *p4;
    int             pendingBlk;
    static uint8_t  memPool[3 * 1048576];



    /*HeapInit ( memPool, sizeof(MemManager)+48 );
    p0 = (uint8_t*) HeapAlloc ( 0 );
//    memset ( p0, 11, 11 );
    HeapFree ( p0 );
    printf("%i\n", p0);
    p0 = (uint8_t*) HeapAlloc ( 0 );
    printf("%i\n", p0);
    HeapFree ( p0 );
    printf("%i\n", p0);
    p0 = (uint8_t*) HeapAlloc ( 0 );
    printf("%i", p0);*/
    /*HeapInit ( memPool, sizeof(MemManager)+32 );
    p0 = (uint8_t*) HeapAlloc ( 0);
    printf("mmm   %i\n", p0);*/

    HeapInit ( memPool, 2097152 );
    assert ( ( p0 = (uint8_t*) HeapAlloc ( 512000 ) ) != NULL );
    memset ( p0, 0, 512000 );
    assert ( ( p1 = (uint8_t*) HeapAlloc ( 511000 ) ) != NULL );
    memset ( p1, 0, 511000 );
    assert ( ( p2 = (uint8_t*) HeapAlloc ( 26000 ) ) != NULL );
    memset ( p2, 0, 26000 );
    HeapDone ( &pendingBlk );
//    printf("\n%i\n",pendingBlk);
    assert ( pendingBlk == 3 );


    HeapInit ( memPool, 2097152 );
    assert ( ( p0 = (uint8_t*) HeapAlloc ( 1000000 ) ) != NULL );
    memset ( p0, 0, 1000000 );
    assert ( ( p1 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
    memset ( p1, 0, 250000 );
    assert ( ( p2 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
    memset ( p2, 0, 250000 );
    assert ( ( p3 = (uint8_t*) HeapAlloc ( 250000 ) ) != NULL );
    memset ( p3, 0, 250000 );
    assert ( ( p4 = (uint8_t*) HeapAlloc ( 50000 ) ) != NULL );
    memset ( p4, 0, 50000 );
    assert ( HeapFree ( p2 ) );
    assert ( HeapFree ( p4 ) );
    assert ( HeapFree ( p3 ) );
    assert ( HeapFree ( p1 ) );
    assert ( ( p1 = (uint8_t*) HeapAlloc ( 500000 ) ) != NULL );
    memset ( p1, 0, 500000 );
    assert ( HeapFree ( p0 ) );
    assert ( HeapFree ( p1 ) );
    HeapDone ( &pendingBlk );
    assert ( pendingBlk == 0 );


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


    return 0;
}

#endif /* __PROGTEST__ */

