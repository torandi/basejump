#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "data.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static std::set<std::string> search_path;

static long file_size(FILE* fp){
	const long cur = ftell(fp);
	fseek (fp , 0 , SEEK_END);
	const long bytes = ftell(fp);
	fseek(fp, cur, SEEK_SET);

	if ( bytes == -1 ){
		Logging::fatal("ftell(%d) failed: %s\n", fileno(fp), strerror(errno));
	}

	return bytes;
}

static bool real_file_exists(const std::string& fullpath){
#ifdef HAVE_ACCESS
	return access(fullpath.c_str(), R_OK) == 0;
#elif defined(WIN32)
	const DWORD dwAttrib = GetFileAttributes(fullpath.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
	        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
#error file_exists is not defined for this platform.
#endif
}

Data::file_load_func * Data::load_file = &Data::load_from_file;

Data * Data::open(const std::string &filename) {
	return open(filename.c_str());
}

Data * Data::open(const char * filename) {
	const std::string real_path = expand_path(filename);
	if ( real_path == "" ){
		return nullptr;
	}

	size_t size;
	void * data = load_file(real_path.c_str(), size);
	if(data == nullptr){
		return nullptr;
	}

	return new Data(data, size);
}

static std::string path_cleanup(std::string path){
	/* if the path is empty it is already fine */
	if ( path == "" ) return path;

	/* make sure path has trailing slash */
	const char last = path[path.length()-1];
	if ( last != '/' ){
		path += '/';
	}

	/* windows dislikes ./ */
	if ( path == "./" ){
		return "";
	}

	return path;
}

void Data::add_search_path(std::string path){
	search_path.insert(path_cleanup(path));
}

std::vector<std::string> Data::get_search_path(){
	return std::vector<std::string>(search_path.begin(), search_path.end());
}

void Data::remove_search_paths(){
	search_path.clear();
}

std::string Data::expand_path(std::string filename){
	if ( filename.length() == 0 ){
		return "";
	}

	/**
	 * It might seem a little bit stupid to enforce a leading slash then just
	 * strip it off but it makes sense to always put a leading slash in the
	 * filename they are distinguished from a plain relative path. Maybe think
	 * of the paths as a chroot.
	 */
	if ( filename[0] != '/' ){
		Logging::warning("Resource `%s' lacks leading slash, fixed.\n", filename.c_str());
	} else {
		filename = filename.substr(1);
	}

	for ( auto path : search_path ){
		const std::string fullpath = path + filename;
		if ( real_file_exists(fullpath) ){
			return fullpath;
		}
	}
	return "";
}

bool Data::file_exists(const std::string& filename){
	return expand_path(filename) != "";
}

void * Data::load_from_file(const char * filename, size_t &size) {
	FILE * file = fopen(filename, "rb");
	if(file == nullptr) {
		Logging::error("[Data] Couldn't open file `%s'\n", filename);
		return nullptr;
	}

	size = (size_t)file_size(file);

	void* data = malloc(size);
	const size_t res = fread(data, 1, size, file);
	if ( res != size ) {
		if ( ferror(file) ){
			Logging::fatal("Error when reading file `%s': %s\n", filename, strerror(errno));
		} else {
			Logging::fatal("Error when reading file `%s': read size was not the expected size (read %lu bytes, expected %lu)\n", filename, res, size);
		}
	}
	fclose(file);

	return data;
}

bool Data::eof() const {
	return ((char*)_pos >= ((char*)_data+_size));
}

const void * Data::data() const {
	return _data;
}

const size_t &Data::size() const {
	return _size;
}

ssize_t Data::read(void * ptr, size_t size, size_t count) const {
	ssize_t to_read = count;
	if(((char*)_pos + size*count) > ((char*)_data + _size)) {
		to_read = (_size - ((char*)_pos-(char*)_data)) % size;
	}
	memcpy(ptr, _pos, size*to_read);
	_pos = (void*)((char*)_pos + size*to_read);
	return to_read;
}

int Data::seek(long int offset, int origin) const {
	void * newpos;
	switch(origin) {
		case SEEK_SET:
			newpos = (void*) ((char*)_data + offset);
			break;
		case SEEK_CUR:
			newpos = (void*)((char*)_pos + offset);
			break;
		case SEEK_END:
			newpos = (void*)((char*)_data + _size + offset);
			break;
		default:
			errno = EINVAL;
			Logging::warning("Warning unknown origin to Data::seek\n");
			return -1;
	}
	if(newpos > ((char*)_data + _size)) {
		return -1;
	} else {
		_pos = newpos;
		return 0;
	}
}

long int Data::tell() const {
	return (long int)((char*)_pos - (char*)_data);
}

ssize_t Data::getline(char **lineptr, size_t *n) const{
	if(n == nullptr || lineptr == nullptr) {
		errno = EINVAL;
		return -1;
	}

	if(eof())
		return -1;

	//Find the next newline (or eof)
	size_t next = 0;
	size_t max = _size - ((char*)_pos-(char*)_data); //Bytes left until eof
	while( next < max && *((char*)_pos+next) != '\n' ) { ++next; }
	++next; //Include the newline

	if(*lineptr == nullptr) {
		*lineptr = (char*)malloc(sizeof(char) * (next + 1) );
		*n = (next + 1);
	} else if(*n < (next + 1 )) {
		*lineptr = (char*)realloc(*lineptr, (next + 1));
		*n = (next + 1);
	}

	return read((void*)*lineptr, sizeof(char), next);
}

Data::Data(void * data, const size_t &size) :
		_data(data)
	,	_size(size)
	,	_pos(data) {
}

Data::~Data() {
	free(_data);
}

std::ostream& operator<< (std::ostream& out, const Data &data) {
	out.write((const char*)data.data(), (data.size()*sizeof(char)));
	return out;
}

std::ostream& operator<< (std::ostream& out, const Data * data) {
	return (out << *data);
}
