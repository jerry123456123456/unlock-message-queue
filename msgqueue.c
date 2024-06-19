#include<error.h>
#include<stdlib.h>
#include<pthread.h>
#include<stdio.h>
#include"msgqueue.h"

//这个代码也是有锁队列，但是为了减少生产者线程和消费者线程之间的碰撞进行优化

struct __msgqueue{
    size_t msg_max;   //消息队列里的最大数量
    size_t msg_cnt;   //表示当前消息队列中的消息数量
    int linkoff;      //链接偏移
    int nonblock;     //非阻塞标志
    void *head1;      //put
    void *head2;      //get
    void **get_head;  //put
    void **put_head;  //put
    void **put_tail;  //get
    pthread_mutex_t get_mutex;
    pthread_mutex_t put_mutex;
    pthread_cond_t get_cond;
    pthread_cond_t put_cond;
};

void msgqueue_set_nonblock(msgqueue_t *queue)
{
	queue->nonblock = 1;
	pthread_mutex_lock(&queue->put_mutex);
	pthread_cond_signal(&queue->get_cond);
	pthread_cond_broadcast(&queue->put_cond);
	pthread_mutex_unlock(&queue->put_mutex);
}

void msgqueue_set_block(msgqueue_t *queue)
{
	queue->nonblock = 0;
}

static size_t __msgqueue_swap(msgqueue_t *queue)
{
	void **get_head = queue->get_head;
	size_t cnt;

	queue->get_head = queue->put_head;
	pthread_mutex_lock(&queue->put_mutex);
	while (queue->msg_cnt == 0 && !queue->nonblock){
		pthread_cond_wait(&queue->get_cond, &queue->put_mutex);
	}

	cnt = queue->msg_cnt;
	if (cnt > queue->msg_max - 1)
		pthread_cond_broadcast(&queue->put_cond);

	queue->put_head = get_head;
	queue->put_tail = get_head;
	queue->msg_cnt = 0;
	pthread_mutex_unlock(&queue->put_mutex);
	return cnt;
}



void msgqueue_put(void *msg,msgqueue_t *queue){
    void **link=(void**)((char*)msg+queue->linkoff);  //*link的地址是消息的起始地址加偏移，结合main刻制是Count->next

    *link=NULL;    //这个地址对应的指针（也就是next）被赋值为NULL
    pthread_mutex_lock(&queue->put_mutex);
    while(queue->msg_cnt>queue->msg_max-1&&!queue->nonblock){
        pthread_cond_wait(&queue->put_cond,&queue->put_mutex);
    }

    *queue->put_tail=link;   //里面的值赋值给尾指针
    queue->put_tail=link;    //更新尾部指针指向link
    queue->msg_cnt++;
    pthread_mutex_unlock(&queue->put_mutex);
    pthread_cond_signal(&queue->get_cond);
}

void *msgqueue_get(msgqueue_t *queue){
    void *msg;

    pthread_mutex_lock(&queue->get_mutex);
    if(*queue->get_head || __msgqueue_swap(queue)>0){
        msg=(char*)*queue->get_head-queue->linkoff;
        *queue->get_head=*(void**)*queue->get_head;
    }else{
        msg=NULL;
    }
    pthread_mutex_unlock(&queue->get_mutex);
    return msg;
}

msgqueue_t *msgqueue_create(size_t maxlen, int linkoff)
{
	msgqueue_t *queue = (msgqueue_t *)malloc(sizeof (msgqueue_t));
	int ret;

	if (!queue)
		return NULL;

	ret = pthread_mutex_init(&queue->get_mutex, NULL);
	if (ret == 0)
	{
		ret = pthread_mutex_init(&queue->put_mutex, NULL);
		if (ret == 0)
		{
			ret = pthread_cond_init(&queue->get_cond, NULL);
			if (ret == 0)
			{
				ret = pthread_cond_init(&queue->put_cond, NULL);
				if (ret == 0)
				{
					queue->msg_max = maxlen;
					queue->linkoff = linkoff;
					queue->head1 = NULL;
					queue->head2 = NULL;
					queue->get_head = &queue->head1;
					queue->put_head = &queue->head2;
					queue->put_tail = &queue->head2;
					queue->msg_cnt = 0;
					queue->nonblock = 0;
					return queue;
				}

				pthread_cond_destroy(&queue->get_cond);
			}

			pthread_mutex_destroy(&queue->put_mutex);
		}

		pthread_mutex_destroy(&queue->get_mutex);
	}

	//errno = ret;
	free(queue);
	return NULL;
}

void msgqueue_destroy(msgqueue_t *queue)
{
	pthread_cond_destroy(&queue->put_cond);
	pthread_cond_destroy(&queue->get_cond);
	pthread_mutex_destroy(&queue->put_mutex);
	pthread_mutex_destroy(&queue->get_mutex);
	free(queue);
}
