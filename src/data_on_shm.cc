#include <pthread.h>
#include <sys/mman.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h> 
#include <sys/types.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "cockroach-probe.h"
#include "utils.h"

#define COCKROACH_DATA_SHM_NAME "cockroach_data"

#define SHM_ADD_UNIT_SIZE (1024*1024) // 1MiB
#define SHM_WINDOW_SIZE   SHM_ADD_UNIT_SIZE

struct primary_header_t {
	sem_t sem;
	uint64_t size;
	uint64_t head_index;
	uint64_t count;
};

struct item_header_t {
	uint32_t id;
	size_t size;
};

struct map_info_t {
	int used_count;
	uint64_t offset;
	uint64_t size;
};

static int               g_shm_fd = 0;
static pthread_mutex_t   g_shm_mutex = PTHREAD_MUTEX_INITIALIZER;
static primary_header_t *g_primary_header = NULL;
static pthread_mutex_t   g_map_info_mutex = PTHREAD_MUTEX_INITIALIZER;

static int lock_shm(primary_header_t *header)
{
top:
	int ret = sem_wait(&header->sem);
	if (ret == 0)
		return 0;
	if (errno == EINTR)
		goto top;
	return -1;
}

static int unlock_shm(primary_header_t *header)
{
	int ret = sem_post(&header->sem); if (ret == 0)
		return 0;
	return -1;
}

static void lock_shm(void)
{
	if (lock_shm(g_primary_header) == -1) {
		ROACH_ERR("Failed: cockroach_lock_shm: %d\n", errno);
		ROACH_ABORT();
	}
}

static void unlock_shm(void)
{
	if (unlock_shm(g_primary_header) == -1) {
		ROACH_ERR("Failed: cockroach_unlock_shm: %d\n", errno);
		ROACH_ABORT();
	}
}

static void open_shm_if_needed(void)
{
	if (g_primary_header)
		return;

	pthread_mutex_lock(&g_shm_mutex);
	// check again because other thread might open the shm while
	// in pthread_mutex_lock().
	if (g_primary_header) {
		pthread_mutex_unlock(&g_shm_mutex);
		return;
	}

	size_t shm_size = sizeof(primary_header_t);
	g_shm_fd = shm_open(COCKROACH_DATA_SHM_NAME, O_RDWR, 0644);
	if (g_shm_fd == -1) {
		if (errno == ENOENT) {
			g_shm_fd = shm_open(COCKROACH_DATA_SHM_NAME,
			                    O_RDWR|O_CREAT, 0644);
		}
		if (g_shm_fd == -1) {
			ROACH_ERR("Failed to open shm: %d\n", errno);
			ROACH_ABORT();
		}
		if (ftruncate(g_shm_fd, shm_size) == -1) {
			ROACH_ERR("Failed: ftrunc: %d\n", errno);
			ROACH_ABORT();
		}
		ROACH_INFO("created: SHM for data\n");
	}

	void *ptr = mmap(NULL, shm_size,
	                 PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd, 0);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to map shm: %d\n", errno);
		ROACH_ABORT();
	}
	g_primary_header = (primary_header_t *)ptr;
	int shared = 1;
	unsigned int sem_count = 1;
	sem_init(&g_primary_header->sem, shared, sem_count);
	g_primary_header->size = shm_size;
	g_primary_header->head_index = shm_size;
	g_primary_header->count = 0;
	pthread_mutex_unlock(&g_shm_mutex);
}

static
map_info_t *find_map_info_and_inc_used_count(uint64_t offset, size_t size)
{
	pthread_mutex_lock(&g_map_info_mutex);
	pthread_mutex_unlock(&g_map_info_mutex);
	return NULL;
}

static item_header_t *get_shm_region(size_t size)
{
	open_shm_if_needed();

	lock_shm();
	//uint64_t size = g_primary_header->size;
	uint64_t shm_size = g_primary_header->size;
	uint64_t offset = g_primary_header->head_index;

	// truncate shm if its size is small.

	// just reserve region. Writing data is performed later w/o the lock.
	g_primary_header->head_index += (sizeof(item_header_t) + size);
	g_primary_header->count++;
	unlock_shm();

	// map window if needed
	map_info_t *map_info = find_map_info_and_inc_used_count(offset, size);
	if (map_info) {
		return NULL;
	}

	// make a new mapped region
	int page_size = utils::get_page_size();
	uint64_t offset_page_aligned = (offset + page_size - 1) / page_size;
	offset_page_aligned *= page_size;
	size_t length = SHM_WINDOW_SIZE;
	void *ptr = mmap(NULL, length,
	                 PROT_READ | PROT_WRITE, MAP_SHARED, g_shm_fd,
	                 offset_page_aligned);
	if (ptr == MAP_FAILED) {
		ROACH_ERR("Failed to map shm: %d: offset: %"PRIu64"\n",
		          errno, offset);
		ROACH_ABORT();
	}

	//return (item_header_t *)ptr;
	return NULL;
}

static uint8_t *get_shm_data_area_ptr(item_header_t *item_header)
{
	return (uint8_t *)(item_header + 1);
}

void cockroach_record_data_on_shm(uint32_t id, size_t size,
                                  record_data_func_t record_data_func)
{
	if (id >= COCKROACH_RECORD_ID_RESERVED_BEGIN) {
		ROACH_ERR("id: %d is RESERVED. IGNORED.\n", id);
		return;
	}

	item_header_t *item_header = get_shm_region(size);
	uint8_t *data_area = get_shm_data_area_ptr(item_header);
	item_header->id = id;
	item_header->size = size;
	(*record_data_func)(size, data_area);
	// TODO: decrement map used count
}
