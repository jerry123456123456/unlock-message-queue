// 头文件保护，防止头文件被重复包含
#ifndef _MARK_MPSC_QUEUE_H
#define _MARK_MPSC_QUEUE_H

#include <atomic>  // 包含原子操作相关的头文件，用于实现线程安全的队列操作
#include <utility> // 包含一些常用的工具函数和类型

// 非侵入式多生产者单消费者队列模板类
template<typename T>
class MPSCQueueNonIntrusive
{
public:
    // 构造函数
    MPSCQueueNonIntrusive() 
        // 初始化头指针，创建一个新的节点作为初始头节点
        : _head(new Node()), 
        // 初始化尾指针，指向头节点
        _tail(_head.load(std::memory_order_relaxed)) 
    {
        // 获取头节点
        Node* front = _head.load(std::memory_order_relaxed);
        // 将头节点的下一个节点指针初始化为 nullptr
        front->Next.store(nullptr, std::memory_order_relaxed);
    }

    // 析构函数
    ~MPSCQueueNonIntrusive()
    {
        T* output;
        // 循环从队列中出队元素，直到队列为空
        while (Dequeue(output))
            // 释放出队元素的内存
            delete output;

        // 获取头节点
        Node* front = _head.load(std::memory_order_relaxed);
        // 释放头节点的内存
        delete front;
    }

    // 入队操作，是无等待的（wait-free）
    void Enqueue(T* input)
    {
        // 创建一个新的节点，存储输入元素
        Node* node = new Node(input);
        // 原子地交换头指针，返回旧的头指针
        Node* prevHead = _head.exchange(node, std::memory_order_acq_rel);
        // 将旧头节点的下一个节点指针指向新节点
        prevHead->Next.store(node, std::memory_order_release);
    }

    // 出队操作
    bool Dequeue(T*& result)
    {
        // 获取尾节点
        Node* tail = _tail.load(std::memory_order_relaxed);
        // 获取尾节点的下一个节点
        Node* next = tail->Next.load(std::memory_order_acquire);
        // 如果尾节点的下一个节点为空，说明队列为空，返回 false
        if (!next)
            return false;

        // 将出队元素赋值给结果指针
        result = next->Data;
        // 更新尾指针为下一个节点
        _tail.store(next, std::memory_order_release);
        // 释放旧尾节点的内存
        delete tail;
        return true;
    }

private:
    // 队列节点结构体
    struct Node
    {
        // 默认构造函数
        Node() = default;
        // 带参数的构造函数，初始化数据指针
        explicit Node(T* data) : Data(data)
        {
            // 将节点的下一个节点指针初始化为 nullptr
            Next.store(nullptr, std::memory_order_relaxed);
        }

        T* Data;  // 节点存储的数据指针
        std::atomic<Node*> Next; // 指向下一个节点的原子指针
    };

    std::atomic<Node*> _head; // 队列的头指针，原子类型
    std::atomic<Node*> _tail; // 队列的尾指针，原子类型

    // 禁用拷贝构造函数
    MPSCQueueNonIntrusive(MPSCQueueNonIntrusive const&) = delete;
    // 禁用赋值运算符
    MPSCQueueNonIntrusive& operator=(MPSCQueueNonIntrusive const&) = delete;
};

// 侵入式多生产者单消费者队列模板类
template<typename T, std::atomic<T*> T::* IntrusiveLink>
class MPSCQueueIntrusive
{
public:
    // 构造函数
    MPSCQueueIntrusive() 
        // 初始化 dummy 指针，指向 dummy 对象
        : _dummyPtr(reinterpret_cast<T*>(std::addressof(_dummy))), 
        // 初始化头指针和尾指针，都指向 dummy 对象
        _head(_dummyPtr), _tail(_dummyPtr)
    {
        // dummy 对象是通过 aligned_storage 构造的，可能没有默认构造函数，所以这里只初始化其侵入式链接
        std::atomic<T*>* dummyNext = new (&(_dummyPtr->*IntrusiveLink)) std::atomic<T*>();
        // 将 dummy 对象的下一个节点指针初始化为 nullptr
        dummyNext->store(nullptr, std::memory_order_relaxed);
    }

    // 析构函数
    ~MPSCQueueIntrusive()
    {
        T* output;
        // 循环从队列中出队元素，直到队列为空
        while (Dequeue(output))
            // 释放出队元素的内存
            delete output;
    }

    // 入队操作
    void Enqueue(T* input)
    {
        // 将输入元素的侵入式链接指针初始化为 nullptr
        (input->*IntrusiveLink).store(nullptr, std::memory_order_release);
        // 原子地交换头指针，返回旧的头指针
        T* prevHead = _head.exchange(input, std::memory_order_acq_rel);
        // 将旧头节点的侵入式链接指针指向输入元素
        (prevHead->*IntrusiveLink).store(input, std::memory_order_release);
    }

    // 出队操作
    bool Dequeue(T*& result)
    {
        // 获取尾节点
        T* tail = _tail.load(std::memory_order_relaxed);
        // 获取尾节点的下一个节点
        T* next = (tail->*IntrusiveLink).load(std::memory_order_acquire);
        // 如果尾节点是 dummy 节点
        if (tail == _dummyPtr)
        {
            // 如果下一个节点为空，说明队列为空，返回 false
            if (!next)
                return false;

            // 更新尾指针为下一个节点
            _tail.store(next, std::memory_order_release);
            tail = next;
            // 再次获取下一个节点
            next = (next->*IntrusiveLink).load(std::memory_order_acquire);
        }

        // 如果下一个节点不为空
        if (next)
        {
            // 更新尾指针为下一个节点
            _tail.store(next, std::memory_order_release);
            // 将出队元素赋值给结果指针
            result = tail;
            return true;
        }

        // 获取头节点
        T* head = _head.load(std::memory_order_acquire);
        // 如果尾节点和头节点相同，说明队列为空
        if (tail != head)
            return false;

        // 将 dummy 节点入队
        Enqueue(_dummyPtr);
        // 再次获取尾节点的下一个节点
        next = (tail->*IntrusiveLink).load(std::memory_order_acquire);
        // 如果下一个节点不为空
        if (next)
        {
            // 更新尾指针为下一个节点
            _tail.store(next, std::memory_order_release);
            // 将出队元素赋值给结果指针
            result = tail;
            return true;
        }
        return false;
    }

private:
    std::aligned_storage_t<sizeof(T), alignof(T)> _dummy; // 用于占位的 dummy 对象
    T* _dummyPtr; // 指向 dummy 对象的指针
    std::atomic<T*> _head; // 队列的头指针，原子类型
    std::atomic<T*> _tail; // 队列的尾指针，原子类型

    // 禁用拷贝构造函数
    MPSCQueueIntrusive(MPSCQueueIntrusive const&) = delete;
    // 禁用赋值运算符
    MPSCQueueIntrusive& operator=(MPSCQueueIntrusive const&) = delete;
};

// 根据传入的侵入式链接指针是否为空，选择使用非侵入式或侵入式队列
template<typename T, std::atomic<T*> T::* IntrusiveLink = nullptr>
using MPSCQueue = std::conditional_t<IntrusiveLink != nullptr, MPSCQueueIntrusive<T, IntrusiveLink>, MPSCQueueNonIntrusive<T>>;

#endif // MPSCQueue_h__
