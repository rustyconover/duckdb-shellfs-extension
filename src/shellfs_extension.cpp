#define DUCKDB_EXTENSION_MAIN

#include "shellfs_extension.hpp"
#include "shell_file_system.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"

namespace duckdb {

static void LoadInternal(DatabaseInstance &instance) {
    // Register a scalar function
	auto &fs = instance.GetFileSystem();

	fs.RegisterSubSystem(make_uniq<ShellFileSystem>());

	auto &config = DBConfig::GetConfig(instance);

    // When writing to a PIPE ignore the SIGPIPE error and consider that the write succeeded.
	config.AddExtensionOption("ignore_sigpipe", "Ignore SIGPIPE", LogicalType::BOOLEAN, Value(false));

}
void ShellfsExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string ShellfsExtension::Name() {
	return "shellfs";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void shellfs_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::ShellfsExtension>();
}

DUCKDB_EXTENSION_API const char *shellfs_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
