#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logging.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>

#ifndef SYS_TIME_H
#include <sys/time.h>
#endif

namespace Logging {

	class Destination {
	public:
		Destination(Severity severity, FILE* dst)
			: severity(severity)
			, dst(dst) {

		}

		~Destination(){
			fclose(dst);
		}

		void write(Severity severity, const char* message){
			if ( severity > this->severity ) return;
			fputs(message, dst);
		}

		Severity severity;
		FILE* dst;
	};

	static std::vector<Destination*> output;

	void init(){

	}

	void cleanup(){
		for ( auto dst : output ){
			delete dst;
		}
		output.clear();
	}

	void add_destination(Severity severity, FILE* dst){
		Destination* o = new Destination(severity, dst);
		output.push_back(o);

		if ( dst == stdout || dst == stderr ){
			return;
		}

		struct timeval tv;
		gettimeofday(&tv, nullptr);

		struct tm* local = localtime(&tv.tv_sec);

		char buf[64] = {0,};
		strftime(buf, sizeof(buf), "\nLogging started at %Y-%m-%d %H.%M.%S\n", local);
		o->write(INFO, buf);
	}

	void add_destination(Severity severity, const char* filename){
		FILE* fp = fopen(filename, "a");
		if ( !fp ){
			error("failed to open logging destination `%s'.", filename);
			return;
		}
		add_destination(severity, fp);
	}

	void message(Severity severity, const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(severity, fmt, ap);
		va_end(ap);
	}

	void vmessage(Severity severity, const char* fmt, va_list ap){
		char* message = nullptr;
		vasprintf(&message, fmt, ap);
		for ( auto dst : output ){
			dst->write(severity, message);
		}
	}

	void fatal(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(FATAL, fmt, ap);
		va_end(ap);
		abort();
	}

	void error(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(ERROR, fmt, ap);
		va_end(ap);
	}

	void warning(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(WARNING, fmt, ap);
		va_end(ap);
	}

	void info(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(INFO, fmt, ap);
		va_end(ap);
	}

	void verbose(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(VERBOSE, fmt, ap);
		va_end(ap);
	}

	void debug(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(DEBUG, fmt, ap);
		va_end(ap);
	}
}
