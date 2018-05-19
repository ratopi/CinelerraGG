
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#ifndef __FORKBASE_H__
#define __FORKBASE_H__

#include "forkbase.inc"
#include "mutex.h"
#include "thread.h"

// Utility functions for all the forking classes

#include <stdint.h>

class ForkBase
{
public:
	enum { EXIT_CODE=0x7fff };
	typedef struct { int64_t token; int bytes; } token_bfr_t;

	ForkBase();
	virtual ~ForkBase();

	int is_running();
	int read_timeout(int ms, int fd, void *data, int bytes);
	int read_parent(int ms);
	int send_parent(int64_t value, const void *data, int bytes);
	int read_child(int ms);
	int send_child(int64_t value, const void *data, int bytes);

	int done, pid;
	ForkChild *child;

	int child_fd;
	int64_t child_token;
	int child_bytes;
	int child_allocated;
	uint8_t *child_data;

	int parent_fd;
	int64_t parent_token;
	int parent_bytes;
	int parent_allocated;
	uint8_t *parent_data;
};

class ForkChild : public ForkBase
{
public:
	ForkChild();
	virtual ~ForkChild();
	virtual void run() = 0;
	virtual int handle_child();
	int child_iteration();
};

class ForkParent : public Thread, public ForkBase
{
public:
	ForkParent();
	virtual ~ForkParent();
	virtual int handle_parent();
	virtual ForkChild *new_fork() = 0;

	void start_child();
	void start();
	void stop();
	void run();
	int parent_iteration();

	Mutex *lock;
};

#endif
