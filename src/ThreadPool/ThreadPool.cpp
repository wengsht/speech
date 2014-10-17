#include "ThreadPool.h"
#include "assert.h"
#include "configure.h"

ThreadPool::ThreadPool() : idx(0), threadNum(DEFAULT_THREAD_NUM) {
    threadTasks.resize(threadNum);
}
ThreadPool::ThreadPool(int threadNum) : idx(0), threadNum(threadNum) {
    threadTasks.resize(threadNum);
}
ThreadPool::~ThreadPool() {
}
void ThreadPool::addTask(struct sp_task & task) {
    assert(idx < threadTasks.size());

    threadTasks[idx].tasks.push_back(task);

    increament();
}

void ThreadPool::run() {
    for(int i = 0;i < threadNum;i++) {
        pthread_create(&threadTasks[i].tid, NULL, ThreadPool::processThread, (void *)(&threadTasks[i]));
    }
    for(int i = 0;i < threadNum;i++) {
        pthread_join(threadTasks[i].tid, NULL);
    }
}

inline 
void *ThreadPool::processThread(void *param) {
    thread_info *info = (thread_info *) param;

    std::vector<sp_task> &tasks = info->tasks;
    for(int i = 0;i < tasks.size(); i++) {
        tasks[i].func(tasks[i].in);
    }

    return NULL;
}
