//  GPU.h
//
//	Avoiding bank conflicts in GPU

#define LOG_NUM_BANKS 4
#define NUM_BANKS (1<<(LOG_NUM_BANKS))
#define CONFLICT_FREE_OFFSET(z) ((z) >> (LOG_NUM_BANKS))
