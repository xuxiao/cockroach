#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
using namespace std;

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "mapped_lib_manager.h"
#include "utils.h"

#define NUM_SHARED_LIB_MAP_ARGS      6
#define IDX_SHARED_LIB_MAP_ADDR_RAGE 0
#define IDX_SHARED_LIB_MAP_PERM      1
#define IDX_SHARED_LIB_MAP_PATH      5

#define LEN_SHARED_LIB_MAP_PERM      4
#define IDX_PERM_EXEC                2

// --------------------------------------------------------------------------
// private functions
// --------------------------------------------------------------------------
void mapped_lib_manager::parse_mapped_lib_line(const char *line)
{
	// strip head spaces
	vector<string> tokens = utils::split(line);
	if (tokens.size() != NUM_SHARED_LIB_MAP_ARGS)
		return;

	// select path that starts from '/'
	string &path = tokens[IDX_SHARED_LIB_MAP_PATH];
	if (path.at(0) != '/')
		return;

	// select permission that contains execution
	string &perm = tokens[IDX_SHARED_LIB_MAP_PERM];
	if (perm.size() < LEN_SHARED_LIB_MAP_PERM)
		return;
	if (perm[IDX_PERM_EXEC] != 'x')
		return;

	// parse start address
	unsigned long start_addr, end_addr;
	string &addr_range = tokens[0];
	int ret = sscanf(addr_range.c_str(), "%lx-%lx", &start_addr, &end_addr);
	if (ret != 2)
		return;

	unsigned long length = end_addr - start_addr;

	// add to the maps
	bool is_exe = (path == m_exe_path);
	mapped_lib_info *lib_info
	  = new mapped_lib_info(path.c_str(), start_addr, length, is_exe);

	m_lib_info_path_map[lib_info->get_path()] = lib_info;
	m_lib_info_filename_map[lib_info->get_filename()] = lib_info;
}

void mapped_lib_manager::_parse_mapped_lib_line(const char *line, void *arg)
{
	mapped_lib_manager *obj = static_cast<mapped_lib_manager *>(arg);
	obj->parse_mapped_lib_line(line);
}

// --------------------------------------------------------------------------
// public functions
// --------------------------------------------------------------------------
mapped_lib_manager::mapped_lib_manager()
{
	// get the executable path
	const static int BUF_LEN = PATH_MAX;
	char buf[BUF_LEN];
	ssize_t len = readlink("/proc/self/exe", buf, BUF_LEN);
	if (len == -1) {
		ROACH_ERR("Failed to readlink(\"/proc/self/exe\"): %d\n",
		          errno);
		ROACH_ABORT();
	}
	m_exe_path = string(buf, len);

	// lookup mapped files
	const char *map_file_path = "/proc/self/maps";
	bool ret = utils::read_one_line_loop
	                   (map_file_path,
	                    mapped_lib_manager::_parse_mapped_lib_line, this);
	if (!ret) {
		printf("Failed to parse recipe file: %s (%d)\n", map_file_path,
		       errno);
		exit(EXIT_FAILURE);
	}
}

const mapped_lib_info *mapped_lib_manager::get_lib_info(const char *name)
{
	mapped_lib_info_map_itr it;
	if (utils::is_absolute_path(name)) {
		it = m_lib_info_path_map.find(name);
		if (it == m_lib_info_path_map.end())
			return NULL;
	} else {
		it = m_lib_info_filename_map.find(name);
		if (it == m_lib_info_filename_map.end())
			return NULL;
	}
	return it->second;
}

void mapped_lib_manager::update(void)
{
	ROACH_BUG("Not implemented\n");
}
