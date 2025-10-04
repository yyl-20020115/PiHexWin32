#pragma once
#include "RESOURCES.h"
#define _EXPORT __export

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <direct.h>
#include <malloc.h>
#include <process.h>
#include <io.h>
#include <winsock.h>
#include <ras.h>
#ifndef __pascal
#define __pascal __stdcall
#endif 
#ifndef __export
#define __export
#endif

long __pascal calc_main_init(void);
// This is called whenever Pihex/start calculating is selected from the
// menu, and also upon startup if auto-continue is selected.
// It should preform non-thread-specific initialization.
// Returns: non-zero if it supports multiple calculating threads
//          zero otherwise.

void __pascal calc_main_status(void);
extern char calc_Status[];
extern char calc_Tip[];
// calc_main_status is called approximately once every second.
// It should write into calc_Status and calc_Tip.

extern long calc_error;
// When calc_error is 1, the tray icon (if it exists) and the title bar
// will flash -- this gets the user's attention and gets them to deal with
// a problem.

extern void __pascal calc_main_done(void);
// This is called whenever either Pihex/stop calulating is selected from
// the menu or all of the calculating threads finish.
// calc_main_done should perform non-thread-specific shutdown.

void __pascal calc_thread_init(LPVOID dat);

void __pascal calc_thread_done(LPVOID dat);

#ifndef CALC_THREAD_DOWORK
#define calc_thread_dowork CALC_THREAD_DOWORK
#endif
void __pascal calc_thread_dowork(LPVOID dat);

// These functions get passed the location of a region of memory of size
// ThreadDatSize. The first doubleword serves two purposes: first, it
// contains the number of the thread, starting with 0. Second, if it is
// ever set to -1 (FFFFFFFF) when calc_thread_init or calc_thread_dowork
// return, calc_thread_done is called, and then the thread is halted.
// The only other circumstance under which the loop which calls
// calc_thread_dowork will end is when Pihex/stop calculating is selected
// from the menu.

extern void pihex_dropout(void);

extern void spoolmsg(char* dat);
// Provided by main.c;  writes a message to the spool file.

extern void request_communication(long startcomputing, long forceattempt);
// If this function is called, main.c will communicate ASAP.

#ifdef Sample
extern char Samplefn[];
#endif
// If the code is to be sampled, Samplefn must be defined.
// This will only sample the first thread (thread 0), although it shouldn't
// matter, since all the calculating threads are identical.

#ifdef times
extern char timesfn[];
#endif

extern char iniName[];
//The name of the .ini file.

extern char otheriniName[];

