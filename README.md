前两个是有锁队列，msg是基于普通lockqueue，减少生产者消费者线程间的碰撞提高性能，mpsc是基于多生产者单消费者的无锁消息队列