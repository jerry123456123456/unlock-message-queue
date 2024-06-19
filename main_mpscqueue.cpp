#include "MPSCQueue.h"

#include <thread>
#include<sys/time.h>
#include <iostream>
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)


struct Count {
    Count(int _v) : v(_v){}
    int v;
};

int main() {
    struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);

    MPSCQueue<Count> queue;
    std::thread pd1([&]() {
        for(int i=0;i<1000000;i++){
            queue.Enqueue(new Count(i));
        }
    });

    std::thread pd2([&]() {
        for(int i=0;i<1000000;i++){
            queue.Enqueue(new Count(i));
        }
    });

    std::thread cs1([&]() {
        Count* ele;
        while(queue.Dequeue(ele)) {
            //std::cout << std::this_thread::get_id() << " : pop " << ele->v << std::endl;
            delete ele;
        }
    });

    pd1.join();
    pd2.join();
    cs1.join();

    struct timeval tv_end;
	gettimeofday(&tv_end, NULL);

	int time_used = TIME_SUB_MS(tv_end, tv_begin);
    std::cout<<time_used<<std::endl;
    
    return 0;
}