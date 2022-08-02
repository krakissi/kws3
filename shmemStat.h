/*
	shmemStat.h
	mperron (2022)

	Intended to be included inside a class definition.
*/

// Write debug stat data to the provided stream.
static void DumpDebugStats(std::stringstream &ss);

// Manage shared memory for the stat table.
static void InitStats();
static void UninitStats();

#ifndef KWS3_SHMEM_STAT_INIT
#define KWS3_SHMEM_STAT_INIT(CLS) \
	void CLS::InitStats(){        \
		s_debugStats = (DebugStats*) mmap(NULL, sizeof(DebugStats), (PROT_READ | PROT_WRITE), (MAP_SHARED | MAP_ANONYMOUS), -1, 0); \
	}
#endif

#ifndef KWS3_SHMEM_STAT_UNINIT
#define KWS3_SHMEM_STAT_UNINIT(CLS)                    \
	void CLS::UninitStats(){                           \
		if(s_debugStats)                               \
			munmap(s_debugStats, sizeof(DebugStats));  \
                                                       \
		s_debugStats = nullptr;                        \
	}
#endif
