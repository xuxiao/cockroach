#ifndef testutil_h
#define testutil_h

#include <string>
#include <list>
#include <map>
using namespace std;

#include <glib.h>

struct record_data_tool_output {
	uint32_t id;
	size_t size;
	uint8_t *data;

	record_data_tool_output(void);
	virtual ~record_data_tool_output();
};

struct exec_command_info {
	// intput
	bool save_stdout;
	bool save_stderr;
	const gchar **argv;
	const gchar **envp;
	const gchar *working_dir;
	GSpawnChildSetupFunc child_setup;
	gpointer user_data;

	// output (automatically freed)
	GPid child_pid;
	string stdout_str;
	string stderr_str;
	GError *error;
	int status;

	// constructor
	exec_command_info(const char *argv[] = NULL);
	virtual ~exec_command_info();
};

class target_probe_info {
	pid_t m_pid;
	const char *m_recipe_file;
	const char *m_target_func;

public:
	target_probe_info(pid_t pid, const char *recipe_file,
	                  const char *target_func);
	unsigned long get_target_addr(void);
	pid_t get_pid(void);
};

class testutil {
	static map<int, string> signal_name_map;
	static void add_test_libs_dir_to_ld_library_path_if_needed(void);

public:
	static long get_page_size(void);
	static void exec_command(exec_command_info *arg);
	static void exec_time_measure_tool(const char *arg,
	                                   exec_command_info *exec_info);
	static void reset_time_list(void);
	static void assert_measured_time(int expected_num_line,
	                                 target_probe_info *probe_info);
	static void assert_measured_time_lines(int expected_num_line,
	                                       string &lines,
	                                       target_probe_info *probe_info);
	static void assert_measured_time_format(string &line,
	                                        target_probe_info *probe_info);
	static void run_target_exe(const char *recipe_path, string arg_str,
	                           exec_command_info *exec_info);
	static void assert_get_record_data(list<record_data_tool_output> &tool_out);
	static void assert_parse_record_data_output
	              (string *line,
	               list<record_data_tool_output> &tool_output_list);
	static void exec_record_data_tool(const char *arg,
	                                  exec_command_info *exec_info);
	static void reset_record_data(void);
	static string get_exit_info(int status);
	static const string &get_signal_name(int signo);
};

#endif
