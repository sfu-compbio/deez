#ifndef Logger_H
#define Logger_H

/*class Logger {
private:
	static FILE	*file;
	static int	level;

private:
	Logger (void) {}
	Logger (Logger const&);
	void operator= (Logger const&); 

public:
	static void initialize (FILE *f, int l = 0) {
		if (file != 0 && level != -1) {
			Logger::level = l;
			Logger::file  = f;
		}
	}

	static void log (const char* format, ...) {
		if (Logger::level < 2) {
			fprintf(Logger::file, "[L] ");
			va_list args;
			va_start(args, format);
			vfprintf(Logger::file, format, args);
			va_end(args);
			fprintf(Logger::file, "\n");
		}
	}

	void debug (const char* format, ...) {
		if (Logger::level < 1) {
			fprintf(Logger::file, "[D] ");
			va_list args;
			va_start(args, format);
			vfprintf(Logger::file, format, args);
			va_end(args);
			fprintf(Logger::file, "\n");
		}
	}

	static void err (const char* format, ...) {
		fprintf(Logger::file, "[E] ");
		va_list args;
		va_start(args, format);
		vfprintf(Logger::file, format, args);
		va_end(args);
		fprintf(Logger::file, "\n");
	}
};*/

#ifdef _MSC_VER
#define FILE_NAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define FILE_NAME __FILE__
#endif

#define LOG(c,...)\
	fprintf(stderr, "L.%20s\t"c"\n", FILE_NAME, ##__VA_ARGS__)
#define DEBUG(c,...)\
	fprintf(stderr, "D.%20s\t"c"\n", FILE_NAME, ##__VA_ARGS__)

#endif
