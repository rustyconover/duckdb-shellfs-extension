#include "shell_file_system.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/file_opener.hpp"
#include "duckdb/common/file_system.hpp"
#include "duckdb/common/helper.hpp"
#include "duckdb/common/limits.hpp"


#ifndef _WIN32
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cstdlib>

#else
#include "duckdb/common/windows_util.hpp"

#include <io.h>
#include <string>
#include <stdio.h>

#endif


namespace duckdb {

struct ShellFileHandle : public FileHandle {
public:
	ShellFileHandle(FileSystem &file_system, string path, FILE *pipe) : FileHandle(file_system, std::move(path)), pipe(pipe) {
	}
	~ShellFileHandle() override {
		ShellFileHandle::Close();
	}

	FILE *pipe;

public:
	void Close() override {
		if(!pipe) {
			return;
		}
		int result;

#ifndef _WIN32
		result = pclose(pipe);
#else
		result = _pclose(pipe);
#endif

		if(result == -1) {
			throw IOException("Could not close pipe \"%s\": %s", {{"errno", std::to_string(errno)}}, path,
												strerror(errno));
		}
		pipe = NULL;
	};
};

void ShellFileSystem::Reset(FileHandle &handle) {
	throw InternalException("Cannot reset shell file system");
}


int64_t ShellFileSystem::Read(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	FILE *pipe = handle.Cast<ShellFileHandle>().pipe;
	int64_t bytes_read = fread(buffer, 1, nr_bytes, pipe);
	if (bytes_read == -1)
	{
		throw IOException("Could not read from pipe \"%s\": %s", {{"errno", std::to_string(errno)}}, handle.path,
		                  strerror(errno));
	}
	return bytes_read;
}


int64_t ShellFileSystem::Write(FileHandle &handle, void *buffer, int64_t nr_bytes) {
	FILE *pipe = handle.Cast<ShellFileHandle>().pipe;
	int64_t bytes_written = 0;

	while (nr_bytes > 0)
	{
		auto bytes_to_write = MinValue<idx_t>(idx_t(NumericLimits<int32_t>::Maximum()), idx_t(nr_bytes));
		int64_t current_bytes_written = fwrite(buffer, 1, bytes_to_write, pipe);
		if (current_bytes_written <= 0) {
			throw IOException("Could not write to pipe \"%s\": %s", {{"errno", std::to_string(errno)}}, handle.path,
												strerror(errno));
		}
		bytes_written += current_bytes_written;
		buffer = (void *)(data_ptr_cast(buffer) + current_bytes_written);
		nr_bytes -= current_bytes_written;
	}

	return bytes_written;
}

int64_t ShellFileSystem::GetFileSize(FileHandle &handle) {
	// You can't know the size of the data that will come over a pipe
	// some code uses the size to allocate buffers, so don't return
	// a very large number.
	return 0;
}

unique_ptr<FileHandle> ShellFileSystem::OpenFile(const string &path, FileOpenFlags flags,
                                                optional_ptr<FileOpener> opener) {
	FILE *pipe;
	if (path.front() == '|')
	{
		// We want to write to the pipe.
#ifndef _WIN32
		pipe = popen(path.substr(1, path.size()).c_str(), "w");
#else
		pipe = _popen(path.substr(1, path.size()).c_str(), "w");
#endif
	}
	else
	{
		// We want to read from the pipe
#ifndef _WIN32
		pipe = popen(path.substr(0, path.size()-1).c_str(), "r");
#else
		pipe = _popen(path.substr(0, path.size()-1).c_str(), "r");
#endif
	}

#ifndef _WIN32
	Value value;
	bool ignore_sigpipe = false;
	if (FileOpener::TryGetCurrentSetting(opener, "ignore_sigpipe", value))
	{
		ignore_sigpipe = value.GetValue<bool>();
	}

	if(ignore_sigpipe) {
		signal(SIGPIPE, SIG_IGN);
	}
#endif

	return make_uniq<ShellFileHandle>(*this, path, pipe);
}

bool ShellFileSystem::CanHandleFile(const string &fpath) {
	if (fpath.empty()) {
		return false;
	}
	// If the filename ends with | or starts with |
	// it can be handled by this file system.
	return fpath.back() == '|' || fpath.front() == '|';
}

} // namespace duckdb
