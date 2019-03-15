#include <memory.h>
#include <stdio.h>
#include "MemoryPool.h"
/************************************************************************/
/* 内存池起始地址对齐到ADDR_ALIGN字节
 */
/************************************************************************/
size_t check_align_addr(void *&pBuf)
{
    size_t align = 0;
    // void * -> int * -> int
    // 获得指针当前 内存地址的值
    size_t addr = *((int *)(pBuf));
    align = (ADDR_ALIGN - addr % ADDR_ALIGN) % ADDR_ALIGN;
    // 计算内存对齐偏移大小
    pBuf = (char *)pBuf + align;
    return align;
}
/************************************************************************/
// 内存block大小对齐到MINUNITSIZE字节 
/************************************************************************/
size_t check_align_block(size_t size)
{
    size_t align = size % MINUNITSIZE;
    return size - align;
}
/************************************************************************/
//  分配内存大小对齐到SIZE_ALIGN字节 
/************************************************************************/
size_t check_align_size(size_t size)
{
    size = (size + SIZE_ALIGN - 1) / SIZE_ALIGN * SIZE_ALIGN;
    return size;
}
/************************************************************************/
/** 以下是链表相关操作 
 * 从 Pool 地址开始，创建 count 个 memory_chunk 的双链表
 */
/************************************************************************/
memory_chunk *create_list(memory_chunk *pool, size_t count)
{
    if (!pool)
    {
        return NULL;
    }
    // 第一个 head is null
    memory_chunk *head = NULL;
    for (size_t i = 0; i < count; i++)
    {
        pool->pre = NULL;
        pool->next = head;
        if (head != NULL)
        {
            head->pre = pool;
        }
        head = pool;
        pool++;
    }

    // node - node - node - ... - head - pool
    // 返回head
    return head;
}

memory_chunk *front_pop(memory_chunk *&pool)
{
    if (!pool)
    {
        return NULL;
    }
    memory_chunk *tmp = pool;
    pool = tmp->next;
    // pool = pool->next;
    pool->pre = NULL;
    //       ->       ->      ->     ##
    //  ndoe    node    node    pool    tmp
    //      <-       <-      <-      <-
    return tmp;
}

// 将 element 添加到 head 双链表尾部
void push_back(memory_chunk *&head, memory_chunk *element)
{
    if (head == NULL)
    {
        head = element;
        head->pre = element;
        head->next = element;
        return;
    }
    head->pre->next = element;
    element->pre = head->pre;
    head->pre = element;
    element->next = head;
}
void push_front(memory_chunk *&head, memory_chunk *element)
{
    element->pre = NULL;
    element->next = head;
    if (head != NULL)
    {
        head->pre = element;
    }
    head = element;
}
void delete_chunk(memory_chunk *&head, memory_chunk *element)
{
    // 在双循环链表中删除元素
    if (element == NULL)
    {
        return;
    }
    // element为链表头
    else if (element == head)
    {
        // 链表只有一个元素
        if (head->pre == head)
        {
            head = NULL;
        }
        else
        {
            // element
            // head     -   node    -   node    -   node
            head = element->next;
            // element  -   head    -   node    -   node - node
            head->pre = element->pre;
            head->pre->next = head;
        }
    }
    // element为链表尾
    else if (element->next == head)
    {
        head->pre = element->pre;
        element->pre->next = head;
    }
    else
    {
        // element 是双链表中元素
        element->pre->next = element->next;
        element->next->pre = element->pre;
    }

    // 清除 element 和 双链表的关系
    element->pre = NULL;
    element->next = NULL;
}
/************************************************************************/
// 内存映射表中的索引转化为内存起始地址                                                                     
/************************************************************************/
void *index2addr(PMEMORYPOOL mem_pool, size_t index)
{
    char *p = (char *)(mem_pool->memory);
    // 从 memory 地址开始，偏移 index 个 MINUNITSIZE 长度
    void *ret = (void *)(p + index * MINUNITSIZE);

    return ret;
}
/************************************************************************/
//  内存起始地址转化为内存映射表中的索引                                                                     
/************************************************************************/
size_t addr2index(PMEMORYPOOL mem_pool, void *addr)
{
    char *start = (char *)(mem_pool->memory);
    char *p = (char *)addr;
    size_t index = (p - start) / MINUNITSIZE;
    return index;
}
/************************************************************************/
/* 生成内存池 ， 使用一块分配好的内存来创建内存池
* pBuf: 给定的内存buffer起始地址 
* sBufSize: 给定的内存buffer大小 
* 返回生成的内存池指针
* 根据给定的内存大小，划分 4个部分 
*/
/************************************************************************/
PMEMORYPOOL CreateMemoryPool(void *pBuf, size_t sBufSize)
{
    // 初始化该内存块
    memset(pBuf, 0, sBufSize);
    // void * -> memoryPool *
    PMEMORYPOOL mem_pool = (PMEMORYPOOL)pBuf;
    // 计算需要多少memory map单元格
    size_t mem_pool_struct_size = sizeof(MEMORYPOOL);
    // + MINUNITSIZE - 1 的意思是，为剩余不足 MINUNITSIZE 的内存大小也算作一个 count
    mem_pool->mem_map_pool_count = (sBufSize - mem_pool_struct_size + MINUNITSIZE - 1) / MINUNITSIZE;
    // 单元个数
    mem_pool->mem_map_unit_count = (sBufSize - mem_pool_struct_size + MINUNITSIZE - 1) / MINUNITSIZE;
    // 偏移   mem_pool_struct_size 长度后 即 memory_block,  第二部分
    mem_pool->pmem_map = (memory_block *)((char *)pBuf + mem_pool_struct_size);
    // 空闲队列指针，   预留 mem_map_unit_count 个 memory_block 的大小
    mem_pool->pfree_mem_chunk_pool = (memory_chunk *)((char *)pBuf + mem_pool_struct_size + sizeof(memory_block) * mem_pool->mem_map_unit_count);
    // memory
    mem_pool->memory = (char *)pBuf + mem_pool_struct_size + sizeof(memory_block) * mem_pool->mem_map_unit_count + sizeof(memory_chunk) * mem_pool->mem_map_pool_count;

    // size 表示剩余可用的内存大小
    mem_pool->size = sBufSize - mem_pool_struct_size - sizeof(memory_block) * mem_pool->mem_map_unit_count - sizeof(memory_chunk) * mem_pool->mem_map_pool_count;
    // 内存对齐 memory  8 的倍数
    size_t align = check_align_addr(mem_pool->memory);
    mem_pool->size -= align;
    // 对齐 size  64 的倍数
    mem_pool->size = check_align_block(mem_pool->size);

    // 剩余的 内存可以分配count个64字节
    mem_pool->mem_block_count = mem_pool->size / MINUNITSIZE;
    // 链表化 pfree_mem_chunk_pool 指向第三第三区域的末尾
    mem_pool->pfree_mem_chunk_pool = create_list(mem_pool->pfree_mem_chunk_pool, mem_pool->mem_map_pool_count);
    // 初始化 pfree_mem_chunk，获得双向列表的 最前面node
    // node - node - node - node - ... - node - head
    memory_chunk *tmp = front_pop(mem_pool->pfree_mem_chunk_pool);
    tmp->pre = tmp;
    tmp->next = tmp; // 循环节点
    tmp->pfree_mem_addr = NULL;
    // 可使用的 内存映射量减一
    mem_pool->mem_map_pool_count--;

    // 初始化 pmem_map  , count 表示拥有的 block 数量
    mem_pool->pmem_map[0].count = mem_pool->mem_block_count;

    // ----||[0]----------||----------------tmp||---------------------------------end
    mem_pool->pmem_map[0].pmem_chunk = tmp;
    // 内存映射数组的最后一个元素 指定 开始位置idx 为 0
    mem_pool->pmem_map[mem_pool->mem_block_count - 1].start = 0;

    // tmp 绑定的 block 是 pmem_map[0]
    tmp->pfree_mem_addr = mem_pool->pmem_map;
    // 将 tmp 加入到 pfree_mem_chunk 双链表尾部
    push_back(mem_pool->pfree_mem_chunk, tmp);
    // 可用内存块 数量为 1
    mem_pool->free_mem_chunk_count = 1;
    mem_pool->mem_used_size = 0;
    return mem_pool;
}
/************************************************************************/
//  暂时没用 
/************************************************************************/
void ReleaseMemoryPool(PMEMORYPOOL *pool)
{
}
/************************************************************************/
/* 从内存池中分配指定大小的内存  
* pool: 内存池 指针 
* sMemorySize: 要分配的内存大小 
* 成功时返回分配的内存起始地址，失败返回NULL 
*/
/************************************************************************/
void *GetMemory(size_t sMemorySize, PMEMORYPOOL pool)
{
    // 内存大小对齐
    sMemorySize = check_align_size(sMemorySize);
    size_t index = 0;
    // 遍历 内存池 pfree_mem_chunk ， 找到可以分配的 chunk
    memory_chunk *tmp = pool->pfree_mem_chunk;
    for (index = 0; index < pool->free_mem_chunk_count; index++)
    {
        // chunk 对应的 block 的大小 * 64b 的内存大小符合分配条件
        if (tmp->pfree_mem_addr->count * MINUNITSIZE >= sMemorySize)
        {
            break;
        }
        tmp = tmp->next;
    }

    if (index == pool->free_mem_chunk_count)
    {
        // 遍历没有找到合适的大小
        return NULL;
    }
    // 内存池已经使用的大小 +sMemorySize
    pool->mem_used_size += sMemorySize;
    // tmp 是即将分配内存的 chunk

    // 分配的大小 和 chunk 大小相同
    if (tmp->pfree_mem_addr->count * MINUNITSIZE == sMemorySize)
    {
        // 当要分配的内存大小与当前chunk中的内存大小相同时，从pfree_mem_chunk链表中删除此chunk
        size_t current_index = (tmp->pfree_mem_addr - pool->pmem_map);
        // 从可用 内存 chunk 双链表中删除 tmp
        delete_chunk(pool->pfree_mem_chunk, tmp);
        // tmp 关联的 block 设置为 NULL
        tmp->pfree_mem_addr->pmem_chunk = NULL;

        // 将 tmp 放入 pfree_mem_chunk_pool 内存chunk池 头部
        push_front(pool->pfree_mem_chunk_pool, tmp);
        pool->free_mem_chunk_count--;
        pool->mem_map_pool_count++;

        // 将 index 转换成 address 地址并返回
        return index2addr(pool, current_index);
    }
    else
    {
        // 当要分配的内存小于当前chunk中的内存时，更改pfree_mem_chunk中相应chunk的pfree_mem_addr
        // 复制当前mem_map_unit
        memory_block copy;
        copy.count = tmp->pfree_mem_addr->count;
        // block 属于 chunk
        copy.pmem_chunk = tmp;
        // 记录该block的起始和结束索引
        memory_block *current_block = tmp->pfree_mem_addr;
        current_block->count = sMemorySize / MINUNITSIZE;
        size_t current_index = (current_block - pool->pmem_map);
        // block end 设置 start 为 current_index
        pool->pmem_map[current_index + current_block->count - 1].start = current_index;
        current_block->pmem_chunk = NULL; // NULL表示当前内存块已被分配
        // ---current_block--------------count || modify_block-------------------------end_index
        size_t modify_block = current_index + current_block->count;
        pool->pmem_map[modify_block].count = copy.count - current_block->count;
        pool->pmem_map[modify_block].pmem_chunk = copy.pmem_chunk;
        // 更新原来的pfree_mem_addr
        tmp->pfree_mem_addr = &(pool->pmem_map[modify_block]);

        size_t end_index = current_index + copy.count - 1;
        pool->pmem_map[end_index].start = modify_block;
        return index2addr(pool, current_index);
    }
}
/************************************************************************/
/* 从内存池中释放申请到的内存 
* pool：内存池指针 
* ptrMemoryBlock：申请到的内存起始地址 
*/
/************************************************************************/
void FreeMemory(void *ptrMemoryBlock, PMEMORYPOOL pool)
{
    // 根据内存地址 获得 内存块在 内存映射表上的 idx
    size_t current_index = addr2index(pool, ptrMemoryBlock);
    // 获得 内存块的大小
    size_t size = pool->pmem_map[current_index].count * MINUNITSIZE;
    // 判断与当前释放的内存块相邻的内存块是否可以与当前释放的内存块合并
    memory_block *pre_block = NULL;
    memory_block *next_block = NULL;
    memory_block *current_block = &(pool->pmem_map[current_index]);
    // 第一个
    if (current_index == 0)
    {
        // 内存块的大小 小于 内存池的mem_block_count
        if (current_block->count < pool->mem_block_count)
        {
            // mem_map
            // current_block----------------end_block || next_block---------------------
            next_block = &(pool->pmem_map[current_index + current_block->count]);
            // 如果后一个内存块是空闲的，合并
            if (next_block->pmem_chunk != NULL)
            {
                // next_block 绑定的 chunk 设置绑定的 block 从 next_block 变成 current_block
                next_block->pmem_chunk->pfree_mem_addr = current_block;
                pool->pmem_map[current_index + current_block->count + next_block->count - 1].start = current_index;
                current_block->count += next_block->count;
                current_block->pmem_chunk = next_block->pmem_chunk;
                next_block->pmem_chunk = NULL;
            }
            // 如果后一块内存不是空闲的，在pfree_mem_chunk中增加一个chunk
            else
            {
                memory_chunk *new_chunk = front_pop(pool->pfree_mem_chunk_pool);
                new_chunk->pfree_mem_addr = current_block;
                current_block->pmem_chunk = new_chunk;
                push_back(pool->pfree_mem_chunk, new_chunk);
                pool->mem_map_pool_count--;
                pool->free_mem_chunk_count++;
            }
        }
        else
        {
            memory_chunk *new_chunk = front_pop(pool->pfree_mem_chunk_pool);
            new_chunk->pfree_mem_addr = current_block;
            current_block->pmem_chunk = new_chunk;
            push_back(pool->pfree_mem_chunk, new_chunk);
            pool->mem_map_pool_count--;
            pool->free_mem_chunk_count++;
        }
    }
    // 最后一个
    else if (current_index == pool->mem_block_count - 1)
    {
        if (current_block->count < pool->mem_block_count)
        {
            pre_block = &(pool->pmem_map[current_index - 1]);
            size_t index = pre_block->count;
            pre_block = &(pool->pmem_map[index]);

            // 如果前一个内存块是空闲的，合并
            if (pre_block->pmem_chunk != NULL)
            {
                pool->pmem_map[current_index + current_block->count - 1].start = current_index - pre_block->count;
                pre_block->count += current_block->count;
                current_block->pmem_chunk = NULL;
            }
            // 如果前一块内存不是空闲的，在pfree_mem_chunk中增加一个chunk
            else
            {
                memory_chunk *new_chunk = front_pop(pool->pfree_mem_chunk_pool);
                new_chunk->pfree_mem_addr = current_block;
                current_block->pmem_chunk = new_chunk;
                push_back(pool->pfree_mem_chunk, new_chunk);
                pool->mem_map_pool_count--;
                pool->free_mem_chunk_count++;
            }
        }
        else
        {
            memory_chunk *new_chunk = front_pop(pool->pfree_mem_chunk_pool);
            new_chunk->pfree_mem_addr = current_block;
            current_block->pmem_chunk = new_chunk;
            push_back(pool->pfree_mem_chunk, new_chunk);
            pool->mem_map_pool_count--;
            pool->free_mem_chunk_count++;
        }
    }
    else
    {
        // 需要释放的 内存块在 内存映射表的中间位置
        // -----index-------------pre_block || current_block----------------end_block || next_block---------------------
        next_block = &(pool->pmem_map[current_index + current_block->count]);
        pre_block = &(pool->pmem_map[current_index - 1]);
        size_t index = pre_block->start;
        // -----pre_block------------------ || current_block----------------end_block || next_block---------------------        
        pre_block = &(pool->pmem_map[index]);
        bool is_back_merge = false;

        // 四种情况，全部被覆盖使用
        // 1. 前后都被使用
        // 2. 前未被使用
        // 3. 后未被使用
        // 4. 前后都未被使用

        // chunk is null 表示已经被使用
        if (next_block->pmem_chunk == NULL && pre_block->pmem_chunk == NULL)
        {
            // 新建一个 chunk 用于记录当前需要被释放的 内存块
            memory_chunk *new_chunk = front_pop(pool->pfree_mem_chunk_pool);
            // chunk - block 互相关联
            new_chunk->pfree_mem_addr = current_block;
            current_block->pmem_chunk = new_chunk;
            // new_chunk 存入 memory chunk set 中，表示可用的一个 chunk
            push_back(pool->pfree_mem_chunk, new_chunk);
            pool->mem_map_pool_count--;
            pool->free_mem_chunk_count++;
        }
        // 后一个内存块 没有被使用
        if (next_block->pmem_chunk != NULL)
        {
            next_block->pmem_chunk->pfree_mem_addr = current_block;
            pool->pmem_map[current_index + current_block->count + next_block->count - 1].start = current_index;
            current_block->count += next_block->count;
            current_block->pmem_chunk = next_block->pmem_chunk;
            next_block->pmem_chunk = NULL;
            // 标示向后 融合
            is_back_merge = true;
        }
        // 前一个内存块
        if (pre_block->pmem_chunk != NULL)
        {
            pool->pmem_map[current_index + current_block->count - 1].start = current_index - pre_block->count;
            pre_block->count += current_block->count;
            if (is_back_merge)
            {
                // pfree_mem_chunk -> current_chunk -> pfree_mem_chunk_pool
                // 前后同时融合的话，需要删除一个多余的 chunk，即删除next_chunk
                delete_chunk(pool->pfree_mem_chunk, current_block->pmem_chunk);
                push_front(pool->pfree_mem_chunk_pool, current_block->pmem_chunk);
                pool->free_mem_chunk_count--;
                pool->mem_map_pool_count++;
            }
            current_block->pmem_chunk = NULL;
        }
    }
    pool->mem_used_size -= size;
}