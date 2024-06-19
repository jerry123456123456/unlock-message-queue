//生产者消费者队列

#ifndef _MARK_PC_QUEUE_H
#define _MARK_PC_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <atomic>
#include <type_traits>   //是 C++ 标准库中的头文件，其中包含了许多用于类型特性检查和转换的模板类

template <typename T>
class ProducerConsumerQueue{
private:
    std::mutex _queueLock;
    std::queue<T> _queue;
    std::condition_variable _condition;
    std::atomic<boo> _shutdown;

public:
    ProducerConsumerQueue<T>() : _shutdown(false){}

    void Push(const T& value){
        std::lock_guard<std::mutex> lock(_queueLock);
        _queue.push(std::move(value));

        _condition.notify_one();
    }

    bool Empty(){
        std::lock_guard<std::mutex> lock(_queueLock);

        return _queue.empty();
    }

    size_t Size() const
    {
        return _queue.size();
    }

    bool Pop(T& value)
    {
        std::lock_guard<std::mutex> lock(_queueLock);

        if (_queue.empty() || _shutdown)
            return false;

        value = _queue.front();

        _queue.pop();

        return true;
    }

    //没产品就阻塞，有产品被唤醒
    void WaitAndPop(T& value)
    {
        std::unique_lock<std::mutex> lock(_queueLock);

        while (_queue.empty() && !_shutdown)
            _condition.wait(lock);

        if (_queue.empty() || _shutdown)
            return;

        value = _queue.front();

        _queue.pop();
    }

    void Cancel()
    {
        std::unique_lock<std::mutex> lock(_queueLock);

        while (!_queue.empty())
        {
            T& value = _queue.front();

            DeleteQueuedObject(value);

            _queue.pop();
        }

        _shutdown = true;

        _condition.notify_all();
    }
private:
    template<typename E = T>
    typename std::enable_if<std::is_pointer<E>::value>::type DeleteQueuedObject(E& obj) { delete obj; }

    template<typename E = T>
    typename std::enable_if<!std::is_pointer<E>::value>::type DeleteQueuedObject(E const& /*packet*/) { }

};

#endif