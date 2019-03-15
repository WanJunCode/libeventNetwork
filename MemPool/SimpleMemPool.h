#ifndef _WJ_SIM_MEM_POOL_H
#define _WJ_SIM_MEM_POOL_H

template <typename T, int BaseSize = 32>
class SimpleMemPool
{
  private:
#pragma pack(push, 1)
    union ObjectChunk {
        ObjectChunk *next;
        char buf[sizeof(T)];
    };
    struct MemBlock
    {
        MemBlock *next;
        ObjectChunk chunks[BaseSize];
    };
#pragma pack(pop)

    // 复制构造函数
    SimpleMemPool(const SimpleMemPool<T, BaseSize> &);

    MemBlock *head;
    ObjectChunk *freeChunk;

  public:
    SimpleMemPool();
    ~SimpleMemPool()
    {
        while (head)
        {
            MemBlock *t = head;
            head = head->next;
            delete t;
        }
    }

    T *New()
    {
        if (!freeChunk)
        {
            MemBlock *t = new MemBlock;
            t->next = head;
            head = t;
            for (unsigned int i = 0; i < BaseSize - 1; ++i)
            {
                head->chunks[i].next = &(head->chunks[i + 1]);
            }
            head->chunks[BaseSize - 1].next = 0;
            freeChunk = &(head->chunks[0]);
        }

        ObjectChunk *t = freeChunk;
        freeChunk = freeChunk->next;
        return reinterpret_cast<T *>(t);
    }

    void Delete(T *t)
    {
        ObjectChunk *chunk = reinterpret_cast<ObjectChunk *>(t);
        chunk->next = freeChunk;
        freeChunk = chunk;
    }
};

#endif