#ifndef MARK_LOCKEDQUEUE_H
#define MARK_LOCKEDQUEUE_H

#include<deque>
#include<mutex>

template<class T,typename StorageType=std::deque<T>>
class LockedQueue{
    std::mutex _lock;
    StorageType _queue;
    volatile bool _canceled;   //该变量可能会在程序的控制之外被修改，告诉编译器不要对这个变量的读取和写入进行优化

public:
    LockedQueue():_canceled(false)
    {    
    }
    virtual ~LockedQueue()
    {       
    }

    void add(const T& item){   //引用方式传递，避免调用拷贝构造函数，提高性能
        lock();
        _queue.push_back(item);
        unlock();
    }

    //将范围内的元素添回队列的前端
    template<class Iterator>
    void readd(Iterator begin, Iterator end)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _queue.insert(_queue.begin(), begin, end);
    }

    //拿出队列中的第一个元素
    bool next(T& result){
        std::lock_guard<std::mutex> lock(_lock);
        if(_queue.empty()){
            return false;
        }

        result=_queue.front();
        _queue.pop_front();

        return true;
    }

    //函数重载
    template<class Checker>
    bool next(T& result,Checker& check){
        std::lock_guard<std::mutex> lock(_lock);
        if(_queue.empty()){
            return false;
        }

        result=_queue.front();
        if(!check.Process(result)){
            return false;
        }
        _queue.pop_front();
        return true;
    }

    //检查一下，查第一个元素但是不删除(如果不提供参数，默认不解锁)
    T& peek(bool autoUnlock=false){
        lock();

        T& result=_queue.front();
        if(autoUnlock){
            unlock();
        }
        return result;
    }

    //! Cancels the queue.
    void cancel()
    {
        std::lock_guard<std::mutex> lock(_lock);

        _canceled = true;
    }

    //! Checks if the queue is cancelled.
    bool cancelled()
    {
        std::lock_guard<std::mutex> lock(_lock);
        return _canceled;
    }

    //虽然系统员先提供的这些函数已经是线程安全的，但是重新实现一遍确保安全性

    //! Locks the queue for access.
    void lock()
    {
        this->_lock.lock();
    }

    //! Unlocks the queue.
    void unlock()
    {
        this->_lock.unlock();
    }

    ///! Calls pop_front of the queue
    void pop_front()
    {
        std::lock_guard<std::mutex> lock(_lock);
        _queue.pop_front();
    }

    ///! Checks if we're empty or not with locks held
    bool empty()
    {
        std::lock_guard<std::mutex> lock(_lock);
        return _queue.empty();
    }

};


#endif