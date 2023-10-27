
#ifndef DEBUG_PROFILE_H_
#define DEBUG_PROFILE_H_

#include <stddef.h>
#include <stdint.h>

#define THREAD_PROFILE_DATA_LEN 13

typedef struct thread_profile_data {
	uintptr_t address;
	size_t count;
	struct thread_profile_data *next;
	struct thread_profile_data *child[THREAD_PROFILE_DATA_LEN];
} thread_profile_data_t;

void debug_profile_init(void);
void debug_profile_start(void);
void debug_profile_stop(void);
void debug_profile_gather(void);

#endif  /* DEBUG_PROFILE_H */
