#ifndef LOGGING_H
#define LOGGING_H

#include <cstdio>
#include <cstdarg>

namespace Logging {

	enum Severity {
		FATAL,
		ERROR,
		WARNING,
		INFO,
		VERBOSE,
		DEBUG,
	};

	void init();
	void cleanup();

	void add_destination(Severity severity, FILE* dst);
	void add_destination(Severity severity, const char* filename);

	void __FORMAT__(printf, 2,3) message(Severity severity, const char* fmt, ...);
	void vmessage(Severity severity, const char* fmt, va_list ap);

	void __FORMAT__(printf, 1,2) fatal(const char* fmt, ...);
	void __FORMAT__(printf, 1,2) error(const char* fmt, ...);
	void __FORMAT__(printf, 1,2) warning(const char* fmt, ...);
	void __FORMAT__(printf, 1,2) info(const char* fmt, ...);
	void __FORMAT__(printf, 1,2) verbose(const char* fmt, ...);
	void __FORMAT__(printf, 1,2) debug(const char* fmt, ...);
}

#endif /* LOGGING_H */
