// ptl.hxx
// "Paul's thread library"
// An extremely minimal attempt at abstracting threads
// (01/27/98) Initial version

#ifndef _PTL_HXX_
#define _PTL_HXX_

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

#define PTL_MAXARGS		10
#define PTL_MAXTHREADS		128

#define PTL_ERRMAXTHREADS	-1
#define PTL_ERRNOSTATUS		-2
#define PTL_ERRSTATFAIL		-3
#define PTL_ERRNOCANDO		-4

#define PTL_STACKSIZE		256000

enum states { ST_FINISHED, ST_RUNNING };

class Ptl;
#ifdef WIN32
typedef void (*fn_ptr_t)(void *);
struct ptl_struc {
	Ptl		*ptl_ptr;
	};
#else
typedef void *(*fn_ptr_t)(void *);
struct ptl_struc {
	Ptl		*ptl_ptr;
	pthread_t	ptl_thread;
	};
#endif

class Ptl {
public:
#ifdef WIN32
	Ptl(void (*fn_ptr)(Ptl *), int rqinstance = 0);
#else
	Ptl(void *(*fn_ptr)(Ptl *), int rqinstance = 0);
#endif
	~Ptl();

	int StartThread(void);
	void EndThread(int);
	int PutArg(void *);
	void *GetArg(void);
	int GetInstance(void) { return instance; }
	void PutStackSize(size_t size) { stack_size = size; }
	size_t GetStackSize(void) { return stack_size; }
	static int PutMaxThreads(int thrd_count);
	static int GetMaxThreads(void);
	static int GetStatus(int *);

private:
	fn_ptr_t pfn_ptr;
	void *arg_array[PTL_MAXARGS];
	int put_index;
	int get_index;
	int instance;
	int state;
	size_t stack_size;
#ifdef WIN32
	int status;
#endif

	static int GetNextInstance(void);

	static struct ptl_struc	ptl_array[PTL_MAXTHREADS];
	static int status_count;
	static int max_threads;
	};

#ifdef _PTL_CXX_
struct ptl_struc Ptl::ptl_array[PTL_MAXTHREADS];
int Ptl::status_count = 0;
int Ptl::max_threads = 0;
#endif

#endif
