#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "logging.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <csignal>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

namespace Logging {

	class Destination {
	public:
		Destination(Severity severity, FILE* dst, bool autoclose)
			: severity(severity)
			, dst(dst)
			, autoclose(autoclose) {

		}

		~Destination(){
			if ( autoclose ){
				fclose(dst);
			}
		}

		void write(Severity severity, const char* message){
			if ( severity > this->severity ) return;
			fputs(message, dst);
		}

		Severity severity;
		FILE* dst;
		bool autoclose;
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

	static void add_destination(Severity severity, FILE* dst, bool autoclose){
		Destination* o = new Destination(severity, dst, false);
		output.push_back(o);

		if ( dst == stdout || dst == stderr ){
			return;
		}

#ifdef HAVE_GETTIMEOFDAY
		struct timeval tv;
		gettimeofday(&tv, nullptr);
		struct tm* local = localtime(&tv.tv_sec);
#else
		__time64_t tv;
		struct tm tm, *local = &tm;
        _time64(&tv);
        _localtime64_s(local, &tv);
#endif

		char buf[64] = {0,};
		strftime(buf, sizeof(buf), "\nLogging started at %Y-%m-%d %H.%M.%S\n", local);
		o->write(INFO, buf);
	}

	void add_destination(Severity severity, FILE* dst){
		add_destination(severity, dst, false);
	}

	void add_destination(Severity severity, const char* filename){
		FILE* fp = fopen(filename, "a");
		if ( !fp ){
			error("failed to open logging destination `%s'.", filename);
			return;
		}
		add_destination(severity, fp, true);
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
		free(message);
	}

	void fatal(const char* fmt, ...){
		va_list ap;
		va_start(ap, fmt);
		vmessage(FATAL, fmt, ap);
		va_end(ap);

#ifdef _MSC_VER
		__debugbreak();
#else
		raise(SIGTRAP);
#endif
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
