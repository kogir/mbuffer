#ifndef MBUFFER_MAC_SHIMS_H
#define MBUFFER_MAC_SHIMS_H

/* OS X is of course slightly different from everything else.
 * Work around that with shims.
 */

/* POSIX Semaphore Shims */
#include <assert.h>
#include <pthread.h>

typedef struct
{
    volatile int count;
    pthread_mutex_t m_count, m_wait;
    pthread_cond_t c_wait;
} sem_t;

int sem_getvalue(sem_t *sem, int *sval)
{
	*sval = sem->count;
	return 0;
}

int sem_init(sem_t *sem, int pshared, unsigned int value)
{
	int err;

	if (pshared)
		return -1;
	sem->count = (int)value;
	err = pthread_mutex_init(&(sem->m_count), NULL);
	assert(err == 0);
	err = pthread_mutex_init(&(sem->m_wait), NULL);
	assert(err == 0);
	err = pthread_cond_init(&(sem->c_wait), NULL);
	assert(err == 0);
	return 0;
}

int sem_post(sem_t *sem)
{
	int err;

	err = pthread_mutex_lock(&(sem->m_count));
	assert(err == 0);
	err = pthread_mutex_lock(&(sem->m_wait));
	assert(err == 0);
	// OK to signal before edit because woken thread waits on m_count
	err = pthread_cond_signal(&(sem->c_wait));
	assert(err == 0);
	err = pthread_mutex_unlock(&(sem->m_wait));
	assert(err == 0);
	++(sem->count);
	err = pthread_mutex_unlock(&(sem->m_count));
	assert(err == 0);
	return 0;
}

int sem_wait(sem_t *sem)
{
	int err = 0, acquired = 0;

	while(1)
	{
		err = pthread_mutex_lock(&(sem->m_count));
		assert(err == 0);
		if (sem->count > 0)
		{
			--(sem->count);
			acquired = 1;
		}
		err = pthread_mutex_unlock(&(sem->m_count));
		assert(err == 0);
		if(acquired)
			return 0;
		err = pthread_mutex_lock(&(sem->m_wait));
		assert(err == 0);
		err = pthread_cond_wait(&(sem->c_wait), &(sem->m_wait));
		assert(err == 0);
		err = pthread_mutex_unlock(&(sem->m_wait));
		assert(err == 0);
	}
}


/* clock_gettime Shim */
#include <mach/mach_time.h>
#include <sys/time.h>

#define CLOCK_REALTIME 0
typedef unsigned long clockid_t;

static inline int clock_gettime(unsigned long int id, struct timespec *ts)
{
	uint64_t now;
	
	now = mach_absolute_time();
    ts->tv_sec = now / 1000000;
    ts->tv_nsec = now % 1000000;
    return 0;
}

#endif