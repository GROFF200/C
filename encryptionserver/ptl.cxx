// ptl.cxx

#define _PTL_CXX_
#include <stdio.h>
#include "ptl.hxx"

#ifdef WIN32
Ptl::Ptl(void (*fn_ptr)(Ptl *), int rqinstance)
#else
Ptl::Ptl(void *(*fn_ptr)(Ptl *), int rqinstance)
#endif
{
put_index = get_index = 0;
pfn_ptr = (fn_ptr_t)fn_ptr;
stack_size = PTL_STACKSIZE;
GetMaxThreads();	// set max_threads if not done yet

if(rqinstance)
	{
	if((rqinstance < 0) || (rqinstance >= max_threads))
		{
		instance = PTL_ERRMAXTHREADS;
		return;
		}

	if(ptl_array[rqinstance].ptl_ptr)
		instance = PTL_ERRMAXTHREADS;
	else
		{
		ptl_array[rqinstance].ptl_ptr = this;
		instance = rqinstance;
		}
	}
else
	{
	instance = GetNextInstance();
	if(instance >= 0)
		ptl_array[instance].ptl_ptr = this;
	}
}

Ptl::~Ptl()
{
}

int Ptl::GetMaxThreads(void)
{
if(!max_threads)
	max_threads = PTL_MAXTHREADS;

return max_threads;
}

int Ptl::PutMaxThreads(int thrd_count)
{
if(max_threads)		// can only set once
	return PTL_ERRNOCANDO;

if((thrd_count < 1) || (thrd_count > PTL_MAXTHREADS))
	{
	max_threads = PTL_MAXTHREADS;
	return PTL_ERRNOCANDO;
	}
else
	{
	max_threads = thrd_count;
	return 0;
	}
}

int Ptl::StartThread(void)
{
// A lot is assumed here. By default, the scheduling algorithm for DECthreads is
// functionally the same as it is under NT - 1:1 model (one virtual machine per
// user thread).
if(instance < 0)
	return PTL_ERRMAXTHREADS;

state = ST_RUNNING;

#ifdef WIN32
return _beginthread(pfn_ptr,stack_size,this);
#else
int		rc;
pthread_attr_t	attr;

if((rc = pthread_attr_init(&attr)))
	return rc;
if((rc = pthread_attr_setstacksize(&attr,stack_size)))
	return rc;
return pthread_create(&ptl_array[instance].ptl_thread,&attr,pfn_ptr,this);
#endif
}

void Ptl::EndThread(int stat)
{
if(instance >= 0)
	{
#ifdef WIN32
	status = stat;
#endif
	state = ST_FINISHED;
	}
}

int Ptl::PutArg(void *pParm)
{
if(put_index < PTL_MAXARGS)
	{
	arg_array[put_index++] = pParm;
	return 0;
	}
return -1;
}

void *Ptl::GetArg(void)
{
if(get_index < put_index)
	return arg_array[get_index++];

get_index = 0;
return NULL;
}

int Ptl::GetNextInstance(void)
{
int count;

// get lowest available thread instance number
for(count = 0;count < max_threads;++count)
	{
	if(!(ptl_array[count].ptl_ptr))
		return count;
	}

return PTL_ERRMAXTHREADS;
}

int Ptl::GetStatus(int *pstatus)
{
// pthread_addr_t pt;
int pt;
int rc = PTL_ERRNOSTATUS;
int rval;

for(;status_count < max_threads;++status_count)
	{
	if(!(ptl_array[status_count].ptl_ptr))
		continue;

	if(ptl_array[status_count].ptl_ptr->state == ST_RUNNING)
		continue;

#ifdef WIN32
#else
	if((rval = pthread_join(ptl_array[status_count].ptl_thread,(void **)&pt)) == 0)
		{
		*pstatus = (int)pt;
		rc = status_count;
		}
	else
		{
		*pstatus = rc;
		rc = PTL_ERRSTATFAIL;
		}

	// pthread_detach(ptl_array[status_count].ptl_thread);
#endif

	delete ptl_array[status_count].ptl_ptr;
	ptl_array[status_count].ptl_ptr = (Ptl *)NULL;

	++status_count;
	return rc;
	}

status_count = 0;
return rc;
}

