#ifndef _TMEXSRVR_HXX_
#define _TMEXSRVR_HXX_

#include "time.h"
#include "nwndef.h"
#include "device.hxx"
#include "access.hxx"
#include "rqread.hxx"
#include "rqprfrpt.hxx"

typedef int ThrdId;
void logger(ThrdId id,int level,char *format,...);

enum conn_state { ST_DISCONNECTED = 0, ST_CONNECTED, ST_LOGGEDIN };


// structure for thread info
struct _thrd_info {
	char			ti_log_name[FILENAME_MAX];	// logname for this thread
	FILE			*ti_log_fp;			// fp for log
	enum conn_state		ti_conn_state;			// connection state
	time_t			ti_last_conn_time;		// connection timestamp
	long			ti_conn_count;			// connection count
	long			ti_login_count;			// login count
	};
#endif
