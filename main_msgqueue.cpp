#include "msgqueue.h"

#include <cstddef>
#include <thread>

#include<sys/time.h>
#include <iostream>
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)
using namespace std;

struct Count {
    Count(int _v) : v(_v), next(nullptr) {}
    int v;
    Count *next;
};

int main() {
    struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);

// linkoff  Count 偏移多少个字节就是用于链接下一个节点的next指针
    msgqueue_t* queue = msgqueue_create(1024, sizeof(int));

    msgqueue_set_nonblock(queue);  //设置为非阻塞

    std::thread pd1([&]() {
        for(int i=0;i<100000;i++){
            msgqueue_put(new Count(i), queue);
            //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    std::thread pd2([&]() {
        for(int i=0;i<100000;i++){
            msgqueue_put(new Count(i), queue);
            //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    std::thread cs1([&]() {
        Count *cnt;
        while((cnt = (Count *)msgqueue_get(queue)) != NULL) {
            //std::cout << std::this_thread::get_id() << " : pop " << cnt->v << std::endl;
            delete cnt;
        }
    });
    std::thread cs2([&]() {
        Count *cnt;
        while((cnt = (Count *)msgqueue_get(queue)) != NULL) {
            //std::cout << std::this_thread::get_id() << " : pop " << cnt->v << std::endl;
            delete cnt;
        }
    });

    pd1.join();
    pd2.join();
    cs1.join();
    cs2.join();

    msgqueue_destroy(queue);

    struct timeval tv_end;
	gettimeofday(&tv_end, NULL);

	int time_used = TIME_SUB_MS(tv_end, tv_begin);
    std::cout<<time_used<<std::endl;
    
    return 0;
}
