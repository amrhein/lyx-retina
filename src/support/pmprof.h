// -*- C++ -*-
/* \file pmprof.h
 * This file is part of LyX, the document processor.
 * Licence details can be found in the file COPYING.
 *
 * \author Jean-Marc Lasgouttes
 *
 * Full author contact details are available in file CREDITS.
 */

/** How to use this trivial profiler:
 *
 * * at the beginning of the interesting block, just add:
 *   PROFILE_THIS_BLOCK(some_identifier)
 *
 *   A trailing semicolon can be added at your discretion.
 *
 * * when the program ends, statistics will be sent to standard error, like:
 *
 *   ##### some_identifier: 6.48475usec, count=25405
 *
 * The code measured by the profiler corresponds to the lifetime of a
 * local variable declared by the PROFILE_THIS_BLOCK macro.
 *
 * Some examples of profiling scope: In the snippets below, c1, c2...
 * designate code chunks, and the identifiers of profiling blocks are
 * chosen to reflect what they count.
 *
 * {
 *   c1
 *   PROFILE_THIS_BLOCK(c2)
 *   c2
 * }
 *
 *
 * {
 *   PROFILE_THIS_BLOCK(c1_c2)
 *   c1
 *   PROFILE_THIS_BLOCK(c2)
 *   c2
 * }
 *
 *
 * {
 *   {
 *     PROFILE_THIS_BLOCK(c1)
 *     c1
 *   }
 *   PROFILE_THIS_BLOCK(c2)
 *   c2
 * }
 *
 *
 * {
 *   PROFILE_THIS_BLOCK(c1_c2_c3)
 *   c1
 *   {
 *     PROFILE_THIS_BLOCK(c2)
 *     c2
 *   }
 *   c3
 * }
 *
 * Influence of identifier names: they are mainly used for display
 * purpose, but the same name should not be used twice in the same
 * scope.
 *
 * {
 *   PROFILE_THIS_BLOCK(foo)
 *   c1
 *   PROFILE_THIS_BLOCK(foo) // error: identifier clash
 *   c2
 * }
 *
 * In the example below, c1+c2 and c2 are counted separately, but in
 * the output, both are confusingly labelled `foo'.
 *
 * {
 *   PROFILE_THIS_BLOCK(foo)
 *   c1
 *   {
 *     PROFILE_THIS_BLOCK(foo) // error: identifier clash
 *     c2
 *   }
 * }

 */

#ifndef PMPROF_H
#define PMPROF_H

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include <iomanip>
#include <iostream>


#if defined(__GNUG__) && defined(_GLIBCXX_DEBUG)
#error Profiling is not usable when run-time debugging is in effect
#endif

#ifdef _WIN32
/* This function does not really returns the "time of day",
 * but it will suffice to evaluate elapsed times.
 */
int gettimeofday(struct timeval * tv, struct timezone * /*tz*/)
{
	LARGE_INTEGER frequency, t;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&t);

	tv->tv_sec = long(t.QuadPart / frequency.QuadPart);
	tv->tv_usec = long((1000000.0 * (t.QuadPart % frequency.QuadPart)) / frequency.QuadPart);
	return 0;
}

#endif // _WIN32

/* Helper class for gathering data. Instantiate this as a static
 * variable, so that its destructor will be executed when the program
 * ends.
 */

class PMProfStat {
public:
	PMProfStat(char const * name)
	  : name_(name), sec_(0), usec_(0), count_(0) {}

	~PMProfStat() {
		if (count_>0) {
			double total = 0.001 * (sec_ * 1000000 + usec_);
			std::cerr << std::fixed << std::setprecision(2) 
				<< "#pmprof# " << name_ << ": "
				<< total / count_
				<< "msec, count=" << count_
				<< ", total=" << total << "msec"
				<< std::endl;
		}
	}

	void add(const long long s, const long long u) {
		sec_ += s;
		usec_ += u;
		count_++;
	}

private:
	char const * name_;
	long long sec_, usec_;
	unsigned long long count_;
};


/* Helper class which gathers data at the end of the scope. One
 * instance of this one should be created at each execution of the
 * block. At the end of the block, it sends statistics to the static
 * PMProfStat object.
 */
class PMProfInstance {
public:
	PMProfInstance(PMProfStat * stat) : stat_(stat)
	{
		gettimeofday(&before_, 0);
	}

	~PMProfInstance() {
		gettimeofday(&after_, 0);
		stat_->add(after_.tv_sec - before_.tv_sec,
			   after_.tv_usec - before_.tv_usec);
	}

private:
	timeval before_, after_;
	PMProfStat * stat_;
};


#define PROFILE_THIS_BLOCK(a) \
	static PMProfStat PMPS_##a(#a);\
	PMProfInstance PMPI_##a(&PMPS_##a);


#endif
