/**
 * Author: Jingyue
 */

#include <errno.h>
#include <pthread.h>

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
using namespace std;

static pthread_mutex_t trace_mutex = PTHREAD_MUTEX_INITIALIZER;

static void __append_to_trace(const string &record) {
	ofstream fout("/tmp/fp", ios::binary | ios::app);
	fout << pthread_self() << " " << record << "\n";
}

static void append_to_trace(const string &record) {
	pthread_mutex_lock(&trace_mutex);
	__append_to_trace(record);
	pthread_mutex_unlock(&trace_mutex);
}

extern "C" void trace_init() {
	pthread_mutex_lock(&trace_mutex);
	// There might be multiple processes. Thus we use fulltrace*. 
	system("rm -f /tmp/fp");
	pthread_mutex_unlock(&trace_mutex);
}

/*
 * Injected to the traced program
 * Need restore <errno> at the end. 
 */
extern "C" void trace_source(unsigned call_ins_id) {
	int saved_errno = errno;
	ostringstream oss;
	oss << call_ins_id;
	string tmp_str = oss.str();
	append_to_trace(tmp_str);
	errno = saved_errno;
}

/*
 * Injected to the traced program
 * Need restore <errno> at the end. 
 */
extern "C" void trace_dest(const char *f) {
	int saved_errno = errno;
	string tmp_str = f;
	append_to_trace(tmp_str);
	errno = saved_errno;
}
