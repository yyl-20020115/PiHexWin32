#include <string.h>
#include <stdio.h>
#include <direct.h>
#include <windows.h>
#include <float.h>
#include "pihex.h"
#include "main.h"

#ifdef Sample
char Samplefn[] = "Pihexsmp.txt";
#endif
#ifdef times
char timesfn[] = "pitimes.txt";
#endif
#ifndef QWORD
#define QWORD unsigned long long
#endif
#ifndef calc_progress
#define calc_progress CALC_PROGRESS
#endif

extern void PASCAL calc_progress(struct threaddat* dat);

extern DWORD _overheat_err = 0;

extern QWORD _where = 0;
extern QWORD _m_start_pos = 0;
extern QWORD _progress = 0;
extern QWORD _m_end_pos = 0;
extern DWORD _fracdone = 0;
extern DWORD _numclocks = 0;

extern DWORD _CPUVENDOR = 0;
extern BYTE _CPUTYPE = 0;

extern char IniFileName[];

char iniName[] = sIniName;
char otheriniName[] = sotherIniName;

extern long communicating;
extern long cont;

extern HWND mainHwnd;
extern void  PASCAL OutputText(
	HWND    hWnd,
	LPSTR   str);


unsigned long where[2] = { 0 };      //these are for the range currently being divided between
unsigned long m_start_pos[2] = { 0 };//the threads.
unsigned long progress[2] = { 0 };
unsigned long m_current_pos[2] = { 0 };
unsigned long m_end_pos[2] = { 0 };

long overheat_err = 0;
long numclocks = 0;
float fracdone = 0.0f;              //used by calc_main_status -- if a new range is just started and
//the last one isn't done, (i.e. with a multi-processor system)
//this can be negative.


extern long lastcommunicated;

long rangeswaiting = 0;

typedef struct {
	long d[2];
	long dinc;
	float amul;
	long w[2];
	long winc;
	float xmul;
	long power[2];
	long powerinc;
	long termcount;
	long internal[6];
} polylogdat;

struct threaddat {
	long threadnum;
	long junk;
	unsigned long where[2];
	unsigned long start_pos[2];                         //start of this session's execution -- not start of subrange
	unsigned long current_pos[2];
	unsigned long end_pos[2];
	double pisum[4];
	long internal[8];
	polylogdat pldat;
};


extern void PASCAL POWERINIT(void);
extern void PASCAL POWERFUNC(polylogdat* dat);
extern void PASCAL POLYLOGCONVOUT(polylogdat* dat);

long error = 0;
char ErrMsg[256] = { 0 };

CRITICAL_SECTION not_calculating = { 0 };  //just use one criticalsection for all special purposes
// -- under 0.01% of time should be spent in them in total
//anyway.                                                                                

void pihex_dropout(void)
{
	char cbuf[256] = { 0 };
	char buf[256] = { 0 };
	long thrd = 0;

	WritePrivateProfileString("Main", "where", "0000000000000000", IniFileName);
	WritePrivateProfileString("Main", "srt_pos", "0000000000000000", IniFileName);
	WritePrivateProfileString("Main", "cur_pos", "0000000000000000", IniFileName);
	WritePrivateProfileString("Main", "end_pos", "0000000000000000", IniFileName);

	WritePrivateProfileString("Main", "rangesw", "0", IniFileName);

	for (thrd = 0;; thrd++) {
		sprintf(cbuf, "Thread%d", thrd);
		if (GetPrivateProfileSection(cbuf, buf, 256, IniFileName) == 0) break;
		buf[0] = 0; buf[1] = 0;
		WritePrivateProfileSection(cbuf, buf, IniFileName);
	};

	remove("range0.ini");
	remove("range1.ini");
}

long main_newrange(void)
{
	char rangeini[256] = { 0 };
	char cBuf[32] = { 0 };
	long temp = 0;
	HANDLE hFind = NULL;
	WIN32_FIND_DATA find_data = { 0 };

	getcwd(rangeini, sizeof(rangeini));
	strcat(rangeini, "\\range*.ini");

	hFind = FindFirstFile((LPCTSTR)&rangeini, &find_data);
	if (hFind == INVALID_HANDLE_VALUE) {
		request_communication(1, 0);
		error = 1;
		return(0);
	};

	// The file exists now
	getcwd(rangeini, sizeof(rangeini));
	strcat(rangeini, "\\");
	strcat(rangeini, find_data.cFileName);
	FindClose(hFind);

	GetPrivateProfileString("Main", "Where", "0000000000000000", cBuf, 20, rangeini);
	if (strncmp(cBuf, "0000000000000000", 16) == 0) // another thread read the file first
		return(1);

	sscanf(cBuf, "%08lX", &where[1]);
	sscanf(&cBuf[8], "%08lX", &where[0]);

	temp = progress[0];
	progress[0] -= m_end_pos[0];
	if (temp < progress[0]) progress[1] = progress[1] - m_end_pos[1] - 1;               //borrow
	else progress[1] = progress[1] - m_end_pos[1];
	//progress is -(the amount left to do in old range)

	GetPrivateProfileString("Main", "srt_pos", "0000000000000000", cBuf, 20, rangeini);
	sscanf(cBuf, "%08lX", &m_start_pos[1]);
	sscanf(&cBuf[8], "%08lX", &m_start_pos[0]);

	m_current_pos[0] = m_start_pos[0];
	m_current_pos[1] = m_start_pos[1];

	GetPrivateProfileString("Main", "end_pos", "0000000000000000", cBuf, 20, rangeini);
	sscanf(cBuf, "%08lX", &m_end_pos[1]);
	sscanf(&cBuf[8], "%08lX", &m_end_pos[0]);

	temp = progress[0];
	progress[0] += m_start_pos[0];
	if (temp > progress[0]) progress[1] = progress[1] + m_start_pos[1] + 1;               //carry
	else progress[1] = progress[1] + m_start_pos[1];

	sprintf(cBuf, "%08lX%08lX", where[1], where[0]);
	WritePrivateProfileString("Main", "where", cBuf, IniFileName);

	sprintf(cBuf, "%08lX%08lX", m_start_pos[1], m_start_pos[0]);
	WritePrivateProfileString("Main", "srt_pos", cBuf, IniFileName);

	sprintf(cBuf, "%08lX%08lX", m_current_pos[1], m_current_pos[0]);
	WritePrivateProfileString("Main", "cur_pos", cBuf, IniFileName);

	sprintf(cBuf, "%08lX%08lX", m_end_pos[1], m_end_pos[0]);
	WritePrivateProfileString("Main", "end_pos", cBuf, IniFileName);  //write range info into PiHex.ini

	GetPrivateProfileString("Main", "rnum", "0", cBuf, 20, rangeini);
	WritePrivateProfileString("Main", "rnum", cBuf, IniFileName);

	remove(rangeini);                       //delete Range.ini, so we don't start this range again.

	return(1);
};

long PASCAL calc_main_init(void)
{
	char cBuf[256] = { 0 };

	InitializeCriticalSection(&not_calculating);

	POWERINIT();

	error = 0;
	overheat_err = 0;

	GetPrivateProfileString("Main", "where", "0000000000000000", cBuf, 20, IniFileName);
	sscanf(cBuf, "%08lX", &where[1]);
	sscanf(&cBuf[8], "%08lX", &where[0]);

	GetPrivateProfileString("Main", "srt_pos", "0000000000000000", cBuf, 20, IniFileName);
	sscanf(cBuf, "%08lX", &m_start_pos[1]);
	sscanf(&cBuf[8], "%08lX", &m_start_pos[0]);

	GetPrivateProfileString("Main", "cur_pos", "0000000000000000", cBuf, 20, IniFileName);
	sscanf(cBuf, "%08lX", &m_current_pos[1]);
	sscanf(&cBuf[8], "%08lX", &m_current_pos[0]);

	progress[0] = m_current_pos[0];
	progress[1] = m_current_pos[1];

	GetPrivateProfileString("Main", "end_pos", "0000000000000000", cBuf, 20, IniFileName);
	sscanf(cBuf, "%08lX", &m_end_pos[1]);
	sscanf(&cBuf[8], "%08lX", &m_end_pos[0]);

	return(1);
};

extern DWORD get_high_time(void);
extern void update_hours(float cav);
extern void tell_server_hours(void);

void newrange(struct threaddat* dat)
{
	char Thname[256] = { 0 };
	char cBuf[256] = { 0 };
	unsigned long mcp = 0;
	unsigned long pisum1[4] = { 0 };
	char outstr[256] = { 0 };
	FILE* f = NULL;

	long starttime = 0;
	long endtime = 0;
	float idealtime = 0;
	float cav = 0.0f;

	POLYLOGCONVOUT(&dat->pldat);

	pisum1[3] = dat->pisum[0];                    //convert from double to long
	pisum1[2] = dat->pisum[1];
	pisum1[1] = dat->pisum[2];
	pisum1[0] = dat->pisum[3];

	if ((where[1] | where[0]) != 0)
	{
		sprintf(outstr, "output,%08lX%08lX,%08lX%08lX,%08lX%08lX,%08lX%08lX%08lX%08lX\n"
			, dat->where[1], dat->where[0], dat->start_pos[1], dat->start_pos[0]
			, dat->current_pos[1], dat->current_pos[0]
			, pisum1[3], pisum1[2], pisum1[1], pisum1[0]);

		spool_message(outstr);
	};

	sprintf(Thname, "Thread%d", dat->threadnum);

	endtime = get_high_time();
	starttime = GetPrivateProfileInt(Thname, "stime", 0, IniFileName);
	sprintf(cBuf, "%d", endtime);
	WritePrivateProfileString(Thname, "stime", cBuf, IniFileName);
	if (starttime > 0) {
		cav = 24. / (endtime - starttime);
		GetPrivateProfileString("Main", "rtime", "0", cBuf, 20, IniFileName);
		sscanf(cBuf, "%f", &idealtime);
		cav = cav * idealtime;
		if (idealtime > 0) update_hours(cav);
	};

	mcp = GetPrivateProfileInt(Thname, "lastr", 0, IniFileName);
	if (mcp != 0) {
		sprintf(outstr, "donerange,%d\n", mcp);
		tell_server_hours();
		spool_message(outstr);
		spool_message("getrange\n");
		WritePrivateProfileString(Thname, "lastr", "0", IniFileName);

		rangeswaiting--;
		sprintf(cBuf, "%d", rangeswaiting);
		WritePrivateProfileString("Main", "rangesw", cBuf, IniFileName);

		lastcommunicated = 0;
		WritePrivateProfileString("Main", "lastcommunicated", "0", IniFileName);
	};

	dat->pisum[0] = 0;
	dat->pisum[1] = 0;
	dat->pisum[2] = 0;
	dat->pisum[3] = 0;

	if (m_current_pos[1] == m_end_pos[1] && m_current_pos[0] == m_end_pos[0]) {
		if (main_newrange() == 0) {
			sprintf(Thname, "Thread%d", dat->threadnum);
			sprintf(cBuf, "%08lX%08lX", dat->current_pos[1], dat->current_pos[0]);
			WritePrivateProfileString(Thname, "cur_pos", cBuf, IniFileName);
			dat->threadnum = -1;
			return;
		};
	};

	dat->where[0] = where[0];
	dat->where[1] = where[1];
	dat->current_pos[0] = m_current_pos[0];
	dat->current_pos[1] = m_current_pos[1];
	dat->start_pos[0] = m_current_pos[0];
	dat->start_pos[1] = m_current_pos[1];

	mcp = m_current_pos[0];
	m_current_pos[0] = (m_current_pos[0] & (0xF0000000)) + (1 << 28);
	if (m_current_pos[0] < mcp) m_current_pos[1]++;

	if ((m_current_pos[1] > m_end_pos[1]) || ((m_current_pos[1] == m_end_pos[1]) &&
		m_current_pos[0] > m_end_pos[0]))
	{
		m_current_pos[0] = m_end_pos[0]; m_current_pos[1] = m_end_pos[1];
	};

	dat->end_pos[0] = m_current_pos[0];
	dat->end_pos[1] = m_current_pos[1];

	sprintf(Thname, "Thread%d", dat->threadnum);

	if ((m_current_pos[1] == m_end_pos[1]) && (m_current_pos[0] == m_end_pos[0]))
		GetPrivateProfileString("Main", "rnum", "0", cBuf, 20, IniFileName);
	else
		sprintf(cBuf, "0");

	WritePrivateProfileString(Thname, "lastr", cBuf, IniFileName);

	sprintf(cBuf, "%08lX%08lX", m_current_pos[1], m_current_pos[0]);
	WritePrivateProfileString("Main", "cur_pos", cBuf, IniFileName);

	sprintf(cBuf, "%08lX%08lX", dat->where[1], dat->where[0]);
	WritePrivateProfileString(Thname, "where", cBuf, IniFileName);

	sprintf(cBuf, "%08lX%08lX", dat->current_pos[1], dat->current_pos[0]);
	WritePrivateProfileString(Thname, "cur_pos", cBuf, IniFileName);

	sprintf(cBuf, "%08lX%08lX", dat->end_pos[1], dat->end_pos[0]);
	WritePrivateProfileString(Thname, "end_pos", cBuf, IniFileName);
}

void PASCAL calc_progress(struct threaddat* dat)
{
	unsigned long t = 0;
	EnterCriticalSection(&not_calculating);
	if (overheat_err) {
		sprintf(ErrMsg, "Error: CPU appears to be overheating."); error = 1;
		dat->threadnum = -1;
	};

	if (rangeswaiting < 1)
	{
		request_communication(0, 0);
	}
	t = progress[0];
	progress[0] += 16;
	if (progress[0] < t) progress[1]++;         //carry
	if (dat->current_pos[0] == dat->end_pos[0]
		&& dat->current_pos[1] == dat->end_pos[1])     //if we've finished this sub-range...
		newrange(dat);                                                //get another.
	LeaveCriticalSection(&not_calculating);
}

void PASCAL calc_thread_init(LPVOID p)
{
	struct threaddat* dat = (struct threaddat*)p;
	char Thname[32] = { 0 };
	char cBuf[32] = { 0 };
	unsigned long t;
	EnterCriticalSection(&not_calculating);
	_control87(_PC_64 | _RC_DOWN, _MCW_PC | _MCW_RC);        //set precision to 64 bits, rounding to down.
	sprintf(Thname, "Thread%d", dat->threadnum);

	GetPrivateProfileString(Thname, "where", "0000000000000000", cBuf, 20, IniFileName);
	sscanf(cBuf, "%08lX", &dat->where[1]);
	sscanf(&cBuf[8], "%08lX", &dat->where[0]);

	GetPrivateProfileString(Thname, "cur_pos", "0000000000000000", cBuf, 20, IniFileName);
	sscanf(cBuf, "%08lX", &dat->current_pos[1]);
	sscanf(&cBuf[8], "%08lX", &dat->current_pos[0]);

	GetPrivateProfileString(Thname, "end_pos", "0000000000000000", cBuf, 20, IniFileName);
	sscanf(cBuf, "%08lX", &dat->end_pos[1]);
	sscanf(&cBuf[8], "%08lX", &dat->end_pos[0]);

	dat->pisum[0] = 0;
	dat->pisum[1] = 0;
	dat->pisum[2] = 0;
	dat->pisum[3] = 0;
	dat->start_pos[0] = dat->current_pos[0];
	dat->start_pos[1] = dat->current_pos[1];

	t = progress[0];                     //adjust progress to account for the fact that
	progress[0] -= dat->end_pos[0];         //this sub-range isn't finished yet.
	if (t < progress[0]) progress[1]--;   //borrow
	progress[1] -= dat->end_pos[1];

	t = progress[0];
	progress[0] += dat->current_pos[0];
	if (t > progress[0]) progress[1]++;   //carry
	progress[1] += dat->current_pos[1];

	LeaveCriticalSection(&not_calculating);
}

long calc_error = 0;
char calc_status[256] = { 0 };
char calc_Tip[256] = { 0 };
long oldprogress = 0;
long oldtcount = 0;
extern void calcfracdone(void);
void PASCAL calc_main_status(void)
{
	WIN32_FIND_DATA find_data = { 0 };
	HANDLE hFind = NULL;
	char outputname[32] = { 0 };
	char cBuf[32] = { 0 };
	float tf = 0.0f;
	unsigned long long tl = 0;

	calcfracdone();

	tl = GetTickCount64();
	if (oldprogress != 0)
	{
		tf = progress[0] - oldprogress;
		tf = tf * numclocks / 1000. / (tl - oldtcount);
		sprintf(calc_status, "%f%% done,%fMHz", fracdone, tf);
	}
	else
	{
		sprintf(calc_status, "%f%% done", fracdone);
	};
	oldtcount = tl;
	oldprogress = progress[0];
	sprintf(calc_Tip, "PiHex %f%% done", fracdone);

	if (error) { sprintf(calc_status, "%s", ErrMsg); calc_error = 1; return; };
	calc_error = 0;
};

void PASCAL calc_thread_done(LPVOID p)
{
	struct threaddat* dat = (struct threaddat*)p;
	char Thname[32] = { 0 };
	char cBuf[256] = { 0 };
	unsigned long pisum1[4] = { 0 };
	FILE* f = NULL;
	char outstr[256] = { 0 };

	if (dat->threadnum == -1) return;

	EnterCriticalSection(&not_calculating);

	POLYLOGCONVOUT(&dat->pldat);

	pisum1[3] = dat->pisum[0];
	pisum1[2] = dat->pisum[1];
	pisum1[1] = dat->pisum[2];
	pisum1[0] = dat->pisum[3];


	if ((where[1] | where[0]) != 0)
	{
		sprintf(outstr, "output,%08lX%08lX,%08lX%08lX,%08lX%08lX,%08lX%08lX%08lX%08lX\n"
			, dat->where[1], dat->where[0], dat->start_pos[1], dat->start_pos[0]
			, dat->current_pos[1], dat->current_pos[0]
			, pisum1[3], pisum1[2], pisum1[1], pisum1[0]);

		spool_message(outstr);
	};

	sprintf(Thname, "Thread%d", dat->threadnum);

	sprintf(cBuf, "%08lX%08lX", dat->current_pos[1], dat->current_pos[0]);
	WritePrivateProfileString(Thname, "cur_pos", cBuf, IniFileName);

	LeaveCriticalSection(&not_calculating);
};

void PASCAL calc_main_done(void)
{
	char cBuf[32] = { 0 };

	sprintf(cBuf, "%d", rangeswaiting);
	WritePrivateProfileString("Main", "rangesw", cBuf, IniFileName);

	DeleteCriticalSection(&not_calculating);
}

