#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "bctrace.h"

BC_Trace *BC_Trace::global_trace = 0;
int trace_memory = 0;
int trace_locks = 1;

BC_Trace::BC_Trace()
{
}
BC_Trace::~BC_Trace()
{
}

bc_trace_mutex execution_table;
bc_trace_mutex memory_table;
bc_trace_mutex lock_table;
bc_trace_list lock_free;
bc_trace_mutex file_table;

// incase lock set after task ends
static pthread_t last_lock_thread = 0;
static const char *last_lock_title = 0;
static const char *last_lock_location = 0;

trace_item::trace_item(bc_trace_t &t) : table(t) { ++table.size; }
trace_item::~trace_item() { --table.size; } 

extern "C" void dump()
{
	BC_Trace::dump_traces();
	BC_Trace::dump_locks();
	BC_Trace::dump_buffers();
}

#define TOTAL_TRACES 16

void BC_Trace::new_trace(const char *text)
{
	if(!global_trace) return;
	execution_table.lock();
	execution_item *it;
	if( execution_table.size >= TOTAL_TRACES ) {
		it = (execution_item *)execution_table.first;
		execution_table.remove_pointer(it);
	}
	else
		it = new execution_item();
	it->set(text);
	execution_table.append(it);
	execution_table.unlock();
}

void BC_Trace::new_trace(const char *file, const char *function, int line)
{
	char string[BCTEXTLEN];
	snprintf(string, BCTEXTLEN, "%s: %s: %d", file, function, line);
	new_trace(string);
}

void BC_Trace::delete_traces()
{
	if(!global_trace) return;
	execution_table.lock();
	execution_table.clear();
	execution_table.unlock();
}

void BC_Trace::enable_locks()
{
	lock_table.lock();
	trace_locks = 1;
	lock_table.unlock();
}

void BC_Trace::disable_locks()
{
	lock_table.lock();
	trace_locks = 0;
	while( lock_table.last ) {
		lock_item *p = (lock_item*)lock_table.last;
		p->info->trace = 0;
		lock_table.remove_pointer(p);  lock_free.append(p);
	}
	lock_free.clear();
	lock_table.unlock();
}

// no canceling with lock held
void BC_Trace::lock_locks(const char *s)
{
	lock_table.lock();
	last_lock_thread = pthread_self();
	last_lock_title = s;
	last_lock_location = 0;
}

void BC_Trace::unlock_locks()
{
	lock_table.unlock();
}

#define TOTAL_LOCKS 256

int BC_Trace::set_lock(const char *title, const char *loc, trace_info *info)
{
	if( !global_trace || !trace_locks ) return 0;
	lock_table.lock();
	last_lock_thread = pthread_self();
	last_lock_title = title;
	last_lock_location = loc;
	lock_item *it;
	if( lock_table.size >= TOTAL_LOCKS ) {
		it = (lock_item*)lock_table.first;
		lock_table.remove_pointer(it);
	}
	else if( (it=(lock_item*)lock_free.first) != 0 )
		lock_free.remove_pointer(it);
	else
		it = new lock_item();
	it->set(info, title, loc);
	lock_table.append(it);
	info->trace = (void *)it;
	lock_table.unlock();
	return it->id;
}

void BC_Trace::set_lock2(int table_id, trace_info *info)
{
	if( !global_trace || !trace_locks ) return;
	lock_table.lock();
	lock_item *p = (lock_item *)info->trace;
	if( !p || p->id != table_id ) {
		p = (lock_item*)lock_table.last;
		while( p && p->id != table_id ) p = (lock_item*)p->previous;
	}
	if( p ) {
		info->trace = (void *)p;
		p->is_owner = 1;
		p->tid = pthread_self();
	}
	lock_table.unlock();
}

void BC_Trace::unset_lock2(int table_id, trace_info *info)
{
	if( !global_trace || !trace_locks ) return;
	lock_table.lock();
	lock_item *p = (lock_item *)info->trace;
	if( !p || p->id != table_id ) {
		p = (lock_item*)lock_table.last;
		while( p && p->id != table_id ) p = (lock_item*)p->previous;
	}
	else
		info->trace = 0;
	if( p ) { lock_table.remove_pointer(p);  lock_free.append(p); }
	lock_table.unlock();
}

void BC_Trace::unset_lock(trace_info *info)
{
	if( !global_trace || !trace_locks ) return;
	lock_table.lock();
	lock_item *p = (lock_item *)info->trace;
	if( !p || p->info!=info || !p->is_owner ) {
		p = (lock_item*)lock_table.last;
		while( p && ( p->info!=info || !p->is_owner ) ) p = (lock_item*)p->previous;
	}
	else
		info->trace = 0;
	if( p ) { lock_table.remove_pointer(p);  lock_free.append(p); }
	lock_table.unlock();
}


void BC_Trace::unset_all_locks(trace_info *info)
{
	if( !global_trace || !trace_locks ) return;
	lock_table.lock();
	lock_item *p = (lock_item*)lock_table.first;
	while( p ) {
		lock_item *lp = p;  p = (lock_item*)p->next;
		if( lp->info != info ) continue;
		lock_table.remove_pointer(p);  lock_free.append(p);
	}
	lock_table.unlock();
}

void BC_Trace::clear_locks_tid(pthread_t tid)
{
	if( !global_trace || !trace_locks ) return;
	lock_table.lock();
	lock_item *p = (lock_item*)lock_table.first;
	while( p ) {
		lock_item *lp = p;  p = (lock_item*)p->next;
		if( lp->tid != tid ) continue;
		lock_table.remove_pointer(p);  lock_free.append(p);
	}
	lock_table.unlock();
}


void BC_Trace::enable_memory()
{
	trace_memory = 1;
}

void BC_Trace::disable_memory()
{
	trace_memory = 0;
}


void BC_Trace::set_buffer(int size, void *ptr, const char* loc)
{
	if(!global_trace) return;
	if(!trace_memory) return;
	memory_table.lock();
//printf("BC_Trace::set_buffer %p %s\n", ptr, loc);
	memory_table.append(new memory_item(size, ptr, loc));
	memory_table.unlock();
}

int BC_Trace::unset_buffer(void *ptr)
{
	if(!global_trace) return 0;
	if(!trace_memory) return 0;
	memory_table.lock();
	memory_item *p = (memory_item*)memory_table.first;
	for( ; p!=0 && p->ptr!=ptr; p=(memory_item*)p->next );
	if( p ) delete p;
	memory_table.unlock();
	return !p ? 1 : 0;
}

#ifdef TRACE_MEMORY

void* operator new(size_t size)
{
	void *result = malloc(size);
	BUFFER(size, result, "new");
	return result;
}

void* operator new[](size_t size)
{
	void *result = malloc(size);
	BUFFER(size, result, "new []");
	return result;
}

void operator delete(void *ptr)
{
	UNBUFFER(ptr);
	free(ptr);
}

void operator delete[](void *ptr)
{
	UNBUFFER(ptr);
	free(ptr);
}

#endif

void BC_Trace::dump_traces(FILE *fp)
{
// Dump trace table
	for( trace_item *tp=execution_table.first; tp!=0; tp=tp->next ) {
		execution_item *p=(execution_item*)tp;
		fprintf(fp,"    %s\n", (char*)p->value);
	}
}

void BC_Trace::dump_locks(FILE *fp)
{
// Dump lock table
#ifdef TRACE_LOCKS
	fprintf(fp,"signal_entry: lock table size=%d\n", lock_table.size);
	for( trace_item *tp=lock_table.first; tp!=0; tp=tp->next ) {
		lock_item *p=(lock_item*)tp;
		fprintf(fp,"    %p %s %s %p%s\n", p->info, p->title,
			p->loc, (void*)p->tid, p->is_owner ? " *" : "");
	}
#endif
}

void BC_Trace::dump_buffers(FILE *fp)
{
#ifdef TRACE_MEMORY
	memory_table.lock();
// Dump buffer table
	fprintf(fp,"BC_Trace::dump_buffers: buffer table size=%d\n", memory_table.size);
	for( trace_item *tp=memory_table.first; tp!=0; tp=tp->next ) {
		memory_item *p=(memory_item*)tp;
		fprintf(fp,"    %d %p %s\n", p->size, p->ptr, p->loc);
	}
	memory_table.unlock();
#endif
}

void BC_Trace::delete_temps()
{
	file_table.lock();
	if( file_table.size )
		printf("BC_Trace::delete_temps: deleting %d temp files\n", file_table.size);
	while( file_table.first ) {
		file_item *p = (file_item*)file_table.first;
		printf("    %s\n", p->value);
		::remove(p->value);
		delete p;
	}
	file_table.unlock();
}

void BC_Trace::reset_locks()
{
	lock_table.unlock();
}

void BC_Trace::set_temp(char *string)
{
	file_table.lock();
	file_item *it = new file_item();
	it->set(string);
	file_table.append(it);
	file_table.unlock();
}

void BC_Trace::unset_temp(char *string)
{
	file_table.lock();
	file_item *p = (file_item *)file_table.last;
	for( ; p!=0 && strcmp(p->value,string); p=(file_item*)p->previous );
	if( p ) delete p;
	file_table.unlock();
}


#ifdef TRACE_THREADS

TheLock TheLocker::the_lock;
TheList TheList::the_list;
TheChk TheChk::the_chk;

int lock_item::table_id = 0;

void TheList::dbg_add(pthread_t tid, pthread_t owner, const char *nm)
{
	TheLocker the_locker;
	int i = the_list.size();
	while( --i >= 0 && the_list[i]->tid != tid );
	if( i >= 0 ) {
		printf("dbg_add, dup %016lx %s %s\n",
			(unsigned long)tid, nm, the_list[i]->name);
		return;
	}
	the_list.append(new TheDbg(tid, owner, nm));
}

void TheList::dbg_del(pthread_t tid)
{
	TheLocker the_locker;
	int i = the_list.size();
	while( --i >= 0 && the_list[i]->tid != tid );
	if( i < 0 ) {
		printf("dbg_del, mis %016lx\n",(unsigned long)tid);
		return;
	}
	the_list.remove_object_number(i);
}


void TheList::dump_threads(FILE *fp)
{
	int i = the_list.size();
	while( --i >= 0 ) {
		fprintf(fp, "thread 0x%012lx, owner 0x%012lx, %s\n",
			(unsigned long)the_list[i]->tid, (unsigned long)the_list[i]->owner,
			the_list[i]->name);
	}
}

#else
#define dbg_add(t, o, nm) do {} while(0)
#define dbg_del(t) do {} while(0)
void TheList::dump_threads(FILE *fp)
{
}
#endif

void BC_Trace::dump_threads(FILE *fp)
{
	TheList::dump_threads(fp);
}


