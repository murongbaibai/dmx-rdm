#ifndef DR_TASK_H
#define DR_TASK_H 

#define DR_SEMAPHORE_GIVE(x) (x) = 1
#define DR_SEMAPHORE_TAKE(x) ((x) ? ((x)=0, 1) : 0)
#define DR_SEMAPHORE_READ(x) (x)

#endif

