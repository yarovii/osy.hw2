#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <cmath>

/*#include <time.h>
#include <vector>
#include <set>*/
using namespace std;
#endif /* __PROGTEST__ */

struct HeapMemory{
    uint64_t index;   ////store index of free heap
    int size;
    int nextSize;
    bool isFree;
    HeapMemory * prev;
    uint8_t * next;
    bool nextNull;

    HeapMemory(size_t index, int size, bool isFree, HeapMemory * prev, uint8_t * next) :
            index(index), size(size), isFree(isFree), prev(prev), next(next) {}

    HeapMemory(){}
};

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
    uint8_t * prevAddress;
    uint8_t * firstPoint;           //////IF FREE ALL BLOCKS POINTS ON LOCALMEMPOOL
    uint8_t structSize;
    uint64_t countDone=0;
    NewHeapMemory* tmp;
    NewHeapMemory* toAdd;
    int nextSize;
    int poolSize;
    static MemManager* instanceVar;

    bool freePool = false;

    NewHeapMemory tmpOb;

    MemManager(){}

    /*void        collectFree         ( HeapMemory* fromAdd,  HeapMemory* toAdd);
    void        collectFreeAll      ( HeapMemory* prev,  HeapMemory* now, HeapMemory* next);
    void        makeNull            ( HeapMemory* m);*/
    void        collectFree         ( NewHeapMemory* fromAdd,  NewHeapMemory* toAdd, bool nextB);
//    void        collectFreeAll      ( NewHeapMemory* prev,  NewHeapMemory* now, NewHeapMemory* next);
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
//    printf("%i    memmene", sizeof(MemManager));
    firstPoint = localMemPool;
    freePool = true;
    countDone = 0;
    structSize = sizeof(NewHeapMemory);
    poolSize = memSize - sizeof(MemManager);
}

void * MemManager::Alloc ( int size ){
//    tmp = (HeapMemory*)localMemPool;
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
            tmpOb.prevSize = NULL;
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
        tmpOb.prevSize = NULL;

        tmpOb.nextSize = NULL;
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
            tmpOb.prevSize = NULL;

            tmpOb.nextSize = NULL;
            tmpOb.nextFree = NULL;
            if(tmp->nextSize - structSize - size > 0) {
                tmpOb.nextSize = tmp->nextSize - structSize - size;
                tmpOb.nextFree = tmp->nextFree + structSize + size;
            }

            tmpOb.prev = (uint8_t*)tmp;

            tmp->next = tmp->nextFree;
            tmp->nextFree = NULL;
            tmp->nextSize = NULL;

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
    m->prevSize = NULL;
    m->next = NULL;
    m->size = NULL;
    m->prev = NULL;
    m->prevFree = NULL;
    m->nextFree = NULL;
    m->nextSize = NULL;
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
/*
void test(void * memPool, int  pendingBlk){
    srand (time(NULL));
    int r=0, s=1000, size = sizeof(uint8_t*)*1000;

//    uint8_t ** arr = (uint8_t**) malloc(size);
    vector<uint8_t* > arr ;

    vector<int> output;
    int ss=1048576;
    int demoss=0;

    HeapInit ( memPool, ss );

    int a=0, f=0, o=0, er=0;

    for(int i =0; i < s; i++){
        r = rand() % 3;
        switch(r){
            case 0:
                if(size>0) {
                    r = rand() % ss;
                    uint8_t *hm = (uint8_t *) HeapAlloc(r);
//                    arr[a] = (uint8_t *) HeapAlloc(r);
                    if ( hm == NULL );
//                        printf("NULL ALLOC    %i\n", r);
//                        o=0;
                    else {
                        printf("ALLOC    %i      %i\n", r, a);
                        arr.push_back(hm);
//                        if ( arr[a] != NULL ){
                        a++;
                        output.push_back(r);
                        ++o;
                        demoss += r;
                        if(demoss > ss)
                            printf("OPA TUT NE NADO TAK\n");
                    }
                }
                break;
            case 1:
                if(a > 0) {
                    f = rand() % a;
                    uint8_t * hm = arr.at(f);
                    if(HeapFree(hm)){
                        demoss -= output.at(f);
                        output.at(f) = 0;
                        printf("freee    %i\n", f);
                        --o;
                    }
                }
                break;
            case 2:
                HeapDone ( &pendingBlk );
                if(pendingBlk != o)
                    printf("PROBLEM   %i    %i\n", pendingBlk, o, er++);
                break;

            default: break;

        }
    }
    int all=0;
    for(int i=0; i < output.size(); i ++){
        if(all != all+ output.at(i)) {
            all += output.at(i);
            printf("%i   %i    %i\n", output.at(i), all, i);
        }
    }
    if(all >= ss)
        printf("TREVOGAAAA \n");
    HeapDone ( &pendingBlk );
        printf("SSSS %i      %i      %i   %i      %i\n", all, ss, ss-all, er, pendingBlk);
}*/

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

    /*uint8_t * m = (uint8_t*) malloc(1);
    memset ( m, 0, 1 );
    printf("%i  %i", (*m), m);
    uint8_t * l = (uint8_t*) malloc(1);
    memset ( l, 1, 1 );
    printf("\n%i  %i", (*l), l);*/


   /* HeapInit ( memPool, 250 );
    p0 = (uint8_t*) HeapAlloc ( 121 );
    printf("mmm   %i\n", p0);
    HeapDone ( &pendingBlk );
    printf("done   %i \n", pendingBlk);
    p1 = (uint8_t*) HeapAlloc ( 1 );
    printf("mmm   %i\n", p1);
    HeapFree ( p1 );
    p2 = (uint8_t*) HeapAlloc ( 1);
    printf("mmm   %i\n", p2);
    HeapFree ( p0 );
    HeapFree ( p2 );
    HeapDone ( &pendingBlk );
    printf("done   %i \n", pendingBlk);*/

    /* HeapFree ( p0 );
     HeapDone ( &pendingBlk );
     printf("done   %i \n", pendingBlk);
 //    memset ( p1, 10, 10 );
     p2 = (uint8_t*) HeapAlloc ( 1048576-32-64 );
     printf("mmm   %i     hhh 3\n", p2);
     HeapDone ( &pendingBlk );
     printf("done   %i \n", pendingBlk);*/
//    memset ( p2, 11, 11 );

    /*HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 3\n", pendingBlk);
    p2 = (uint8_t*) HeapAlloc ( 10 );
    printf("mmm   %i    p222\n", p2);
    p3 = (uint8_t*) HeapAlloc ( 11 );
    printf("mmm   %i    p333\n", p3);

    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 3\n", pendingBlk);*/

    /*p3 = (uint8_t*) HeapAlloc ( 39 );
    memset ( p3, 39, 39 );
    HeapFree ( p3 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 3\n", pendingBlk);
    p3 = (uint8_t*) HeapAlloc ( 40 );
    memset ( p3, 4, 40 );
    HeapFree ( p0 );
    HeapFree ( p2 );
    HeapFree ( p1 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 1\n", pendingBlk);

    p0 = (uint8_t*) HeapAlloc ( 5 );
    memset ( p0, 5, 5 );
//    HeapFree ( p0 );
    p1 = (uint8_t*) HeapAlloc ( 10 );
    memset ( p1, 10, 10 );
    p2 = (uint8_t*) HeapAlloc ( 11 );
    memset ( p2, 11, 11 );
//    HeapFree ( p0 );
    HeapFree ( p2 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 3\n", pendingBlk);
    p2 = (uint8_t*) HeapAlloc ( 11 );
    memset ( p2, 11, 11 );
    HeapFree ( p0 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 3\n", pendingBlk);
    p0 = (uint8_t*) HeapAlloc ( 5 );
    memset ( p0, 5, 5 );
    HeapFree ( p0 );
    HeapFree ( p2 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 2\n", pendingBlk);
    p2 = (uint8_t*) HeapAlloc ( 11 );
    memset ( p2, 11, 11 );
    HeapFree ( p0 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 2\n", pendingBlk);
    p0 = (uint8_t*) HeapAlloc ( 5 );
    memset ( p0, 5, 5 );
    HeapFree ( p1 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 3\n", pendingBlk);
    p1 = (uint8_t*) HeapAlloc ( 10 );
    memset ( p1, 10, 10 );
    HeapFree ( p0 );
    HeapFree ( p1 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 2\n", pendingBlk);
    p1 = (uint8_t*) HeapAlloc ( 10 );
    memset ( p1, 10, 10 );
    p0 = (uint8_t*) HeapAlloc ( 5 );
    memset ( p0, 5, 5 );
    HeapDone ( &pendingBlk );
    printf("mmm   %i     hhh 4\n", pendingBlk);*/

//    for(int i=0; i < 251; i++)
//        printf("mmm   %i     %i\n", i, memPool[i]);


    /*HeapInit ( memPool, 1000);
    p0 = (uint8_t*) HeapAlloc ( 100 );
//    printf("%i %i", p0, p0);
    memset ( p0, 1, 100 );
    p1 = (uint8_t*) HeapAlloc ( 250 );
    memset ( p1, 1, 250 );
    p2 = (uint8_t*) HeapAlloc ( 450 );
    memset ( p2, 1, 450 );
    p3 = (uint8_t*) HeapAlloc ( 71 );
//    memset ( p3, 1, 71 );*/

//    for(int i=0; i < 1020; i++)
//        printf("mm   %i    %i\n", i, memPool[i]);
//    printf("%i %i", p0, p3);
    /*HeapFree ( p1 );
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

