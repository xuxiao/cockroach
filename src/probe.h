#ifndef probe_h
#define probe_h

#include <string>
using namespace std;

#include <stdint.h>
#include "cockroach-probe.h"
#include "mapped_lib_info.h"
#include "opecode.h"

typedef void (*label_func_t)(void);

#if defined(__x86_64__) || defined(__i386__)
// push %rax (1); mov $adrr,%rax (10); push *%rax (2);
#define OPCODES_LEN_OVERWRITE_JUMP 13
#define LEN_OPCODE_JMP_REL32 5

#define OPCODE_NOP        0x90
#define OPCODE_RET        0xc3
#define OPCODE_PUSH_RAX   0x50
#define OPCODE_MOVQ_0     0x48
#define OPCODE_MOVQ_1     0xb8
#define OPCODE_JMP_ABS_RAX_0 0xff
#define OPCODE_JMP_ABS_RAX_1 0xe0
#define OPCODE_JMP_REL    0xe9
#endif // defined(__x86_64__) || define(__i386__)

enum probe_type_t {
	PROBE_TYPE_UNKNOWN,
	PROBE_TYPE_BUILT_IN_TIME_MEASURE,
	PROBE_TYPE_USER,
};

enum install_type_t {
	INSTALL_TYPE_UNKNOWN,
	INSTALL_TYPE_OVERWRITE_ABS64_JUMP,
	INSTALL_TYPE_OVERWRITE_REL32_JUMP,
	INSTALL_TYPE_REPLACE_JUMP_ADDR,
};

class probe {
	probe_type_t m_probe_type;
	install_type_t m_install_type;
	string m_target_lib_path;
	string m_symbol_name;
	unsigned long m_offset_addr;
	int m_overwrite_length;
	bool m_overwrite_length_auto_detect;

	string            m_probe_lib_path;
	probe_init_func_t m_probe_init;
	probe_func_t      m_probe;
	void             *m_probe_priv_data;

	// methods
	label_func_t get_bridge_begin_addr();
	void change_page_permission(void *addr);
	void change_page_permission_all(void *addr, int len);
	void overwrite_jump_code(void *intrude_addr, void *jump_abs_addr,
	                         int copy_code_size);
	uint8_t *overwrite_jump_abs64(uint8_t *code, void *jump_abs_addr,
	                              int copy_code_size);
	uint8_t *overwrite_jump_rel32(uint8_t *code, void *jump_abs_addr,
	                              int copy_code_size);
	int get_minimum_overwrite_length(void);
	int get_overwrite_code_length(void);
	void install_core(unsigned long target_addr);
	bool is_opecode_ret(const opecode *ope) const;
	int32_t get_rel_addr32_for_jump(void *curr, void *dest);

public:
	probe(probe_type_t probe_type, install_type_t install_type);
	void set_target_address(const char *target_lib_path, unsigned long addr,
	                        int overwrite_length = 0);
	void set_probe(const char *probe_lib_path, probe_func_t probe,
	               probe_init_func_t probe_init = NULL);

	const char *get_target_lib_path(void);
	void install(const mapped_lib_info *lib_info);
	void install(void *mapped_addr = NULL);
};

#endif


