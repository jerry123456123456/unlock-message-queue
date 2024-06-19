#include"LockedQueue.h"
#include<chrono>
#include<thread>
#include<iostream>

int main(){
    LockedQueue<int> queue;
    //存
    std::thread pd1([&]{
        queue.add(1);
        queue.add(2);
        queue.add(3);
        queue.add(4);
    });

    std::thread pd2([&]() {
        queue.add(5);
        queue.add(6);
        queue.add(7);
        queue.add(8);
    });
    //取
    std::thread cs1([&]() {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        int ele;
        while(queue.next(ele)) {
            std::cout << std::this_thread::get_id() << " : pop " << ele << std::endl;
        }
    });
    
    std::thread cs2([&]() {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        int ele;
        while(queue.next(ele)) {
            std::cout << std::this_thread::get_id() << " : pop " << ele << std::endl;
        }
    });

    pd1.join();
    pd2.join();
    cs1.join();
    cs2.join();

    return 0;
}