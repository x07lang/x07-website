#define X07_FREESTANDING 1

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef X07_FREESTANDING
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <poll.h>
#include <spawn.h>
#include <sys/mman.h>
#include <sys/wait.h>
#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif
#else
#include <stddef.h>
#include <stdint.h>

void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
void* memset(void* dst, int c, size_t n);
int memcmp(const void* a, const void* b, size_t n);
char* getenv(const char* name);
int snprintf(char* s, size_t n, const char* fmt, ...);
#endif

#ifndef X07_MEM_CAP
#define X07_MEM_CAP (64u * 1024u * 1024u)
#endif

#ifndef X07_FUEL_INIT
#define X07_FUEL_INIT 50000000ULL
#endif

#ifndef X07_ENABLE_FS
#define X07_ENABLE_FS 0
#endif

#ifndef X07_ENABLE_RR
#define X07_ENABLE_RR 0
#endif

#ifndef X07_ENABLE_KV
#define X07_ENABLE_KV 0
#endif

#define X07_ENABLE_STREAMING_FILE_IO (X07_ENABLE_FS || X07_ENABLE_RR || X07_ENABLE_KV)

#ifdef X07_FREESTANDING
#if X07_ENABLE_FS || X07_ENABLE_RR || X07_ENABLE_KV
#error "X07_FREESTANDING requires X07_ENABLE_FS/RR/KV=0"
#endif
#endif

#ifdef X07_DEBUG_BORROW
#ifndef X07_DBG_ALLOC_CAP
#define X07_DBG_ALLOC_CAP 65536u
#endif
#ifndef X07_DBG_BORROW_CAP
#define X07_DBG_BORROW_CAP 65536u
#endif
#endif

typedef struct {
  uint8_t* ptr;
  uint32_t len;
} bytes_t;

typedef struct {
  uint8_t* ptr;
  uint32_t len;
#ifdef X07_DEBUG_BORROW
  uint64_t aid;
  uint64_t bid;
  uint32_t off_bytes;
#endif
} bytes_view_t;

typedef struct {
  uint32_t tag;
  uint32_t payload;
} option_i32_t;

typedef struct {
  uint32_t tag;
  bytes_t payload;
} option_bytes_t;

typedef struct {
  uint32_t tag;
  bytes_view_t payload;
} option_bytes_view_t;

typedef struct {
  uint32_t tag;
  union {
    uint32_t ok;
    uint32_t err;
  } payload;
} result_i32_t;

typedef struct {
  uint32_t tag;
  union {
    bytes_t ok;
    uint32_t err;
  } payload;
} result_bytes_t;

typedef struct {
  uint32_t tag;
  union {
    bytes_view_t ok;
    uint32_t err;
  } payload;
} result_bytes_view_t;

typedef struct {
  uint32_t tag;
  union {
    result_bytes_t ok;
    uint32_t err;
  } payload;
} result_result_bytes_t;

#define RT_TASK_OUT_KIND_BYTES UINT32_C(1)
#define RT_TASK_OUT_KIND_RESULT_BYTES UINT32_C(2)

typedef struct {
  uint32_t kind;
  union {
    bytes_t bytes;
    result_bytes_t result_bytes;
  } payload;
} rt_task_out_t;

typedef struct {
  uint32_t data;
  uint32_t vtable;
} iface_t;

static __attribute__((noreturn)) void rt_trap(const char* msg);
static __attribute__((noreturn)) void rt_trap_path(const char* msg, const char* path);

#define RT_IFACE_VTABLE_IO_READER UINT32_C(1)
#define RT_IFACE_VTABLE_EXT_IO_READER_MIN UINT32_C(2)
#define RT_IFACE_VTABLE_EXT_IO_READER_MAX UINT32_C(64)

typedef uint32_t (*rt_ext_io_reader_read_fn_t)(uint32_t data, uint8_t* dst, uint32_t cap);
typedef void (*rt_ext_io_reader_drop_fn_t)(uint32_t data);

typedef struct {
  rt_ext_io_reader_read_fn_t read;
  rt_ext_io_reader_drop_fn_t drop;
} rt_ext_io_reader_vtable_t;

static rt_ext_io_reader_vtable_t rt_ext_io_reader_vtables[
  RT_IFACE_VTABLE_EXT_IO_READER_MAX - RT_IFACE_VTABLE_EXT_IO_READER_MIN + 1
];
static uint32_t rt_ext_io_reader_vtables_len = 0;

// External packages register IO reader vtables at runtime to enable `iface` streaming
// through `io.read` / `bufread.*` without adding new builtins.
uint32_t x07_rt_register_io_reader_vtable_v1(
  rt_ext_io_reader_read_fn_t read,
  rt_ext_io_reader_drop_fn_t drop
) {
  if (!read) return 0;

  for (uint32_t i = 0; i < rt_ext_io_reader_vtables_len; i++) {
    rt_ext_io_reader_vtable_t* vt = &rt_ext_io_reader_vtables[i];
    if (vt->read == read && vt->drop == drop) {
      return RT_IFACE_VTABLE_EXT_IO_READER_MIN + i;
    }
  }

  uint32_t cap = (uint32_t)(sizeof(rt_ext_io_reader_vtables) / sizeof(rt_ext_io_reader_vtables[0]));
  if (rt_ext_io_reader_vtables_len >= cap) return 0;

  rt_ext_io_reader_vtable_t* vt = &rt_ext_io_reader_vtables[rt_ext_io_reader_vtables_len];
  vt->read = read;
  vt->drop = drop;

  uint32_t id = RT_IFACE_VTABLE_EXT_IO_READER_MIN + rt_ext_io_reader_vtables_len;
  rt_ext_io_reader_vtables_len += 1;
  return id;
}

static uint32_t rt_ext_io_reader_read_into(uint32_t vtable, uint32_t data, uint8_t* dst, uint32_t cap) {
  if (vtable < RT_IFACE_VTABLE_EXT_IO_READER_MIN || vtable > RT_IFACE_VTABLE_EXT_IO_READER_MAX) {
    rt_trap("ext io reader invalid vtable");
  }
  uint32_t idx = vtable - RT_IFACE_VTABLE_EXT_IO_READER_MIN;
  if (idx >= rt_ext_io_reader_vtables_len) {
    rt_trap("ext io reader unregistered vtable");
  }
  rt_ext_io_reader_vtable_t* vt = &rt_ext_io_reader_vtables[idx];
  if (!vt->read) {
    rt_trap("ext io reader missing read fn");
  }
  uint32_t got = vt->read(data, dst, cap);
  if (got > cap) {
    rt_trap("ext io reader returned too many bytes");
  }
  return got;
}

typedef struct {
  bytes_t key;
  bytes_t val;
} kv_entry_t;

typedef struct {
  bytes_t path;
  uint32_t ticks;
} fs_latency_entry_t;

typedef struct {
  uint32_t used;

  uint32_t latency_ticks;

  uint32_t kind_off;
  uint32_t kind_len;

  uint32_t op_off;
  uint32_t op_len;

  uint32_t key_off;
  uint32_t key_len;

  uint32_t payload_off;
  uint32_t payload_len;
} rr_entry_desc_t;

typedef struct {
  bytes_t path;
  bytes_t blob;
  rr_entry_desc_t* entries;
  uint32_t entries_len;
  uint32_t entries_cap;

  uint64_t file_bytes;
  void* append_f;
} rr_cassette_t;

typedef struct {
  uint32_t alive;
  uint8_t mode;
  uint8_t match_mode;
  uint16_t reserved;

  uint64_t max_cassette_bytes;
  uint32_t max_entries;
  uint32_t max_req_bytes;
  uint32_t max_resp_bytes;
  uint32_t max_key_bytes;

  uint32_t transcript_cassette;
  uint32_t transcript_idx;

  rr_cassette_t* cassettes;
  uint32_t cassettes_len;
  uint32_t cassettes_cap;
} rr_handle_t;

typedef struct {
  bytes_t key;
  uint32_t ticks;
} kv_latency_entry_t;

typedef struct {
  uint8_t* mem;
  uint32_t cap;
  uint32_t free_head;
} heap_t;

// Heap allocator is fixed-capacity and deterministic.
// It uses a singly-linked free list of blocks (sorted by address) with coalescing.
#define RT_HEAP_ALIGN UINT32_C(16)
#define RT_HEAP_MAGIC_FREE UINT32_C(0x45564f46) // "X07F"
#define RT_HEAP_MAGIC_USED UINT32_C(0x45564f55) // "X07U"
#define RT_HEAP_NULL_OFF UINT32_MAX

typedef struct {
  uint32_t size;     // total block size including header, multiple of RT_HEAP_ALIGN
  uint32_t next_off; // free: free-list next (offset from heap base), used: allocation epoch id
  uint32_t magic;    // RT_HEAP_MAGIC_FREE / RT_HEAP_MAGIC_USED
  uint32_t req_size; // requested payload size in bytes
} heap_hdr_t;

static uint32_t rt_heap_align_u32(uint32_t x) {
  return (x + (RT_HEAP_ALIGN - 1u)) & ~(RT_HEAP_ALIGN - 1u);
}

typedef struct {
  uint64_t alloc_calls;
  uint64_t realloc_calls;
  uint64_t free_calls;

  uint64_t bytes_alloc_total;
  uint64_t bytes_freed_total;

  uint64_t live_bytes;
  uint64_t peak_live_bytes;

  uint64_t live_allocs;
  uint64_t peak_live_allocs;

  uint64_t memcpy_bytes;
} mem_stats_t;

typedef struct {
  void* ctx;
  void* (*alloc)(void* ctx, uint32_t size, uint32_t align);
  void* (*realloc)(void* ctx, void* ptr, uint32_t old_size, uint32_t new_size, uint32_t align);
  void (*free)(void* ctx, void* ptr, uint32_t size, uint32_t align);
} allocator_v1_t;

__attribute__((weak)) allocator_v1_t x07_custom_allocator(void) {
  return (allocator_v1_t){0};
}

#ifdef X07_DEBUG_BORROW
typedef struct {
  uint8_t* base_ptr;
  uint32_t size_bytes;
  uint32_t alive;
  uint64_t borrow_id;
} dbg_alloc_rec_t;

typedef struct {
  uint64_t alloc_id;
  uint32_t off_bytes;
  uint32_t len_bytes;
  uint32_t active;
} dbg_borrow_rec_t;
#endif

typedef struct {
  uint64_t tasks_spawned;
  uint64_t spawn_calls;
  uint64_t join_calls;
  uint64_t yield_calls;
  uint64_t sleep_calls;
  uint64_t chan_send_calls;
  uint64_t chan_recv_calls;
  uint64_t ctx_switches;
  uint64_t wake_events;
  uint64_t blocked_waits;
  uint64_t virtual_time_end;
  uint64_t sched_trace_hash;
} sched_stats_t;

typedef struct rt_task_s rt_task_t;
typedef struct rt_timer_ev_s rt_timer_ev_t;
typedef struct rt_chan_bytes_s rt_chan_bytes_t;
typedef struct rt_select_evt_s rt_select_evt_t;
typedef struct rt_io_reader_s rt_io_reader_t;
typedef struct rt_bufread_s rt_bufread_t;
typedef struct rt_scratch_u8_fixed_s rt_scratch_u8_fixed_t;
typedef struct rt_os_proc_s rt_os_proc_t;

#define X07_ASSERT_BYTES_EQ_PREFIX_MAX 64

typedef struct {
  uint64_t fuel_init;
  uint64_t fuel;
  int32_t exit_code;
  uint32_t budget_fuel_depth;
  heap_t heap;
  allocator_v1_t allocator;
  uint32_t allocator_is_custom;

  // Total heap usage counters (all allocations, including input/runtime metadata).
  uint64_t heap_live_bytes;
  uint64_t heap_peak_live_bytes;
  uint64_t heap_live_allocs;
  uint64_t heap_peak_live_allocs;

  // Epoch id for mem_stats tracking (0 means "not started yet").
  uint32_t mem_epoch_id;
  mem_stats_t mem_stats;
#ifdef X07_DEBUG_BORROW
  dbg_alloc_rec_t* dbg_allocs;
  uint32_t dbg_allocs_len;
  uint32_t dbg_allocs_cap;

  dbg_borrow_rec_t* dbg_borrows;
  uint32_t dbg_borrows_len;
  uint32_t dbg_borrows_cap;

  uint64_t dbg_borrow_violations;
#endif
  uint64_t fs_read_file_calls;
  uint64_t fs_list_dir_calls;
  uint64_t rr_open_calls;
  uint64_t rr_close_calls;
  uint64_t rr_stats_calls;
  uint64_t rr_next_calls;
  uint64_t rr_next_miss_calls;
  uint64_t rr_append_calls;

  int32_t rr_current;

  rr_handle_t* rr_handles;
  uint32_t rr_handles_len;
  uint32_t rr_handles_cap;
  uint64_t kv_get_calls;
  uint64_t kv_set_calls;

  // Phase G2 fixture-backed latency indices (loaded lazily).
  uint32_t fs_latency_loaded;
  uint32_t fs_latency_default_ticks;
  fs_latency_entry_t* fs_latency_entries;
  uint32_t fs_latency_len;
  bytes_t fs_latency_blob;

  uint32_t kv_latency_loaded;
  uint32_t kv_latency_default_ticks;
  kv_latency_entry_t* kv_latency_entries;
  uint32_t kv_latency_len;
  bytes_t kv_latency_blob;

  kv_entry_t* kv_items;
  uint32_t kv_len;
  uint32_t kv_cap;

  void** map_u32_items;
  uint32_t map_u32_len;
  uint32_t map_u32_cap;

  void** vec_value_items;
  uint32_t vec_value_len;
  uint32_t vec_value_cap;

  void** map_value_items;
  uint32_t map_value_len;
  uint32_t map_value_cap;

  // Phase G2 scheduler + concurrency (deterministic, single-thread cooperative).
  uint32_t sched_current_task;
  uint64_t sched_now_ticks;
  uint64_t sched_seq;
  sched_stats_t sched_stats;

  rt_task_t* sched_tasks;
  uint32_t sched_tasks_len;
  uint32_t sched_tasks_cap;

  uint32_t sched_ready_head;
  uint32_t sched_ready_tail;

  rt_timer_ev_t* sched_timers;
  uint32_t sched_timers_len;
  uint32_t sched_timers_cap;

  rt_chan_bytes_t* sched_chans;
  uint32_t sched_chans_len;
  uint32_t sched_chans_cap;

  rt_select_evt_t* sched_select_evts;
  uint32_t sched_select_evts_len;
  uint32_t sched_select_evts_cap;

  // Phase G2 streaming I/O (deterministic, fixture-backed).
  rt_io_reader_t* io_readers;
  uint32_t io_readers_len;
  uint32_t io_readers_cap;

  rt_bufread_t* bufreads;
  uint32_t bufreads_len;
  uint32_t bufreads_cap;

  // Deterministic fixed-capacity scratch buffers.
  rt_scratch_u8_fixed_t* scratch_u8_fixed;
  uint32_t scratch_u8_fixed_len;
  uint32_t scratch_u8_fixed_cap;

  // Standalone OS process table (run-os*, non-deterministic).
  rt_os_proc_t* os_procs;
  uint32_t os_procs_len;
  uint32_t os_procs_cap;
  uint32_t os_procs_live;
  uint32_t os_procs_spawned;

  const char* trap_ptr;

  uint32_t last_bytes_eq_valid;
  uint32_t last_bytes_eq_a_len;
  uint32_t last_bytes_eq_b_len;
  uint8_t last_bytes_eq_a_prefix[X07_ASSERT_BYTES_EQ_PREFIX_MAX];
  uint8_t last_bytes_eq_b_prefix[X07_ASSERT_BYTES_EQ_PREFIX_MAX];
} ctx_t;

// Global ctx pointer for native extension backends that need to allocate bytes via the runtime.
static ctx_t* rt_ext_ctx = NULL;

// Native math backend entrypoints (linked from deps/x07/libx07_math.*).
bytes_t ev_math_f64_add_v1(bytes_t a, bytes_t b);
bytes_t ev_math_f64_sub_v1(bytes_t a, bytes_t b);
bytes_t ev_math_f64_mul_v1(bytes_t a, bytes_t b);
bytes_t ev_math_f64_div_v1(bytes_t a, bytes_t b);
bytes_t ev_math_f64_neg_v1(bytes_t x);
bytes_t ev_math_f64_abs_v1(bytes_t x);
bytes_t ev_math_f64_min_v1(bytes_t a, bytes_t b);
bytes_t ev_math_f64_max_v1(bytes_t a, bytes_t b);
bytes_t ev_math_f64_sqrt_v1(bytes_t x);
bytes_t ev_math_f64_sin_v1(bytes_t x);
bytes_t ev_math_f64_cos_v1(bytes_t x);
bytes_t ev_math_f64_tan_v1(bytes_t x);
bytes_t ev_math_f64_exp_v1(bytes_t x);
bytes_t ev_math_f64_ln_v1(bytes_t x);
bytes_t ev_math_f64_pow_v1(bytes_t x, bytes_t y);
bytes_t ev_math_f64_atan2_v1(bytes_t y, bytes_t x);
bytes_t ev_math_f64_floor_v1(bytes_t x);
bytes_t ev_math_f64_ceil_v1(bytes_t x);
bytes_t ev_math_f64_fmt_shortest_v1(bytes_t x);
result_bytes_t ev_math_f64_parse_v1(bytes_t s);
bytes_t ev_math_f64_from_i32_v1(int32_t x);
result_i32_t ev_math_f64_to_i32_trunc_v1(bytes_t x);
bytes_t ev_math_f64_to_bits_u64le_v1(bytes_t x);

// Native time backend entrypoints (linked from deps/x07/libx07_time.*).
uint32_t ev_time_tzdb_is_valid_tzid_v1(bytes_t tzid);
bytes_t ev_time_tzdb_offset_duration_v1(bytes_t tzid, int32_t unix_s_lo, int32_t unix_s_hi);
bytes_t ev_time_tzdb_snapshot_id_v1(void);

// Native ext-fs backend entrypoints (linked from deps/x07/libx07_ext_fs.*).
result_bytes_t x07_ext_fs_read_all_v1(bytes_t path, bytes_t caps);
result_i32_t x07_ext_fs_write_all_v1(bytes_t path, bytes_t data, bytes_t caps);
result_i32_t x07_ext_fs_append_all_v1(bytes_t path, bytes_t data, bytes_t caps);
result_i32_t x07_ext_fs_mkdirs_v1(bytes_t path, bytes_t caps);
result_i32_t x07_ext_fs_remove_file_v1(bytes_t path, bytes_t caps);
result_i32_t x07_ext_fs_remove_dir_all_v1(bytes_t path, bytes_t caps);
result_i32_t x07_ext_fs_rename_v1(bytes_t src, bytes_t dst, bytes_t caps);
result_bytes_t x07_ext_fs_list_dir_sorted_text_v1(bytes_t path, bytes_t caps);
result_bytes_t x07_ext_fs_walk_glob_sorted_text_v1(bytes_t root, bytes_t glob, bytes_t caps);
result_bytes_t x07_ext_fs_stat_v1(bytes_t path, bytes_t caps);
result_i32_t x07_ext_fs_stream_open_write_v1(bytes_t path, bytes_t caps);
result_i32_t x07_ext_fs_stream_write_all_v1(int32_t writer_handle, bytes_t data);
result_i32_t x07_ext_fs_stream_close_v1(int32_t writer_handle);
int32_t x07_ext_fs_stream_drop_v1(int32_t writer_handle);

// Native ext-stdio backend entrypoints (linked from deps/x07/libx07_ext_stdio.*).
result_bytes_t x07_ext_stdio_read_line_v1(bytes_t caps);
result_i32_t x07_ext_stdio_write_stdout_v1(bytes_t data, bytes_t caps);
result_i32_t x07_ext_stdio_write_stderr_v1(bytes_t data, bytes_t caps);
result_i32_t x07_ext_stdio_flush_stdout_v1(void);
result_i32_t x07_ext_stdio_flush_stderr_v1(void);

// Native ext-rand backend entrypoints (linked from deps/x07/libx07_ext_rand.*).
result_bytes_t x07_ext_rand_bytes_v1(int32_t n, bytes_t caps);
result_bytes_t x07_ext_rand_u64_v1(bytes_t caps);

// Native ext-db-sqlite backend entrypoints (linked from deps/x07/libx07_ext_db_sqlite.*).
bytes_t x07_ext_db_sqlite_open_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_sqlite_query_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_sqlite_exec_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_sqlite_close_v1(bytes_t req, bytes_t caps);

// Native ext-db-pg backend entrypoints (linked from deps/x07/libx07_ext_db_pg.*).
bytes_t x07_ext_db_pg_open_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_pg_query_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_pg_exec_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_pg_close_v1(bytes_t req, bytes_t caps);

// Native ext-db-mysql backend entrypoints (linked from deps/x07/libx07_ext_db_mysql.*).
bytes_t x07_ext_db_mysql_open_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_mysql_query_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_mysql_exec_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_mysql_close_v1(bytes_t req, bytes_t caps);

// Native ext-db-redis backend entrypoints (linked from deps/x07/libx07_ext_db_redis.*).
bytes_t x07_ext_db_redis_open_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_redis_cmd_v1(bytes_t req, bytes_t caps);
bytes_t x07_ext_db_redis_close_v1(bytes_t req, bytes_t caps);

// Native ext-regex backend entrypoints (linked from deps/x07/libx07_ext_regex.*).
bytes_t x07_ext_regex_compile_opts_v1(bytes_t pat, int32_t opts_u32);
bytes_t x07_ext_regex_exec_from_v1(bytes_t compiled, bytes_t text, int32_t start_i32);
bytes_t x07_ext_regex_exec_caps_from_v1(bytes_t compiled, bytes_t text, int32_t start_i32);
bytes_t x07_ext_regex_find_all_x7sl_v1(bytes_t compiled, bytes_t text, int32_t max_matches_i32);
bytes_t x07_ext_regex_split_v1(bytes_t compiled, bytes_t text, int32_t max_parts_i32);
bytes_t x07_ext_regex_replace_all_v1(bytes_t compiled, bytes_t text, bytes_t repl, int32_t cap_limit_i32);

// Native ext-jsonschema backend entrypoints (linked from deps/x07/libx07_ext_jsonschema.*).
bytes_t x07_ext_jsonschema_compile_v1(bytes_t schema_json);
bytes_t x07_ext_jsonschema_validate_v1(bytes_t compiled, bytes_t instance_json);

#ifdef X07_STANDALONE
static uint32_t rt_os_process_poll_all(ctx_t* ctx, int poll_timeout_ms);
static void rt_os_process_cleanup(ctx_t* ctx);
#else
static uint32_t rt_os_process_poll_all(ctx_t* ctx, int poll_timeout_ms) {
  (void)ctx;
  (void)poll_timeout_ms;
  return UINT32_C(0);
}
static void rt_os_process_cleanup(ctx_t* ctx) {
  (void)ctx;
}
#endif

static __attribute__((noreturn)) void rt_trap(const char* msg) {

#ifndef X07_FREESTANDING
  if (msg) (void)write(STDERR_FILENO, msg, strlen(msg));
  if (rt_ext_ctx && rt_ext_ctx->trap_ptr) {
    const char* p = rt_ext_ctx->trap_ptr;
    (void)write(STDERR_FILENO, " ptr=", 5);
    (void)write(STDERR_FILENO, p, strlen(p));
  }
  if (msg || (rt_ext_ctx && rt_ext_ctx->trap_ptr)) (void)write(STDERR_FILENO, "\n", 1);
#else
  (void)msg;
#endif
  __builtin_trap();
}

static __attribute__((noreturn)) void rt_trap_path(const char* msg, const char* path) {

#ifndef X07_FREESTANDING
  if (msg) (void)write(STDERR_FILENO, msg, strlen(msg));
  if (path) {
    (void)write(STDERR_FILENO, " path=", 6);
    (void)write(STDERR_FILENO, path, strlen(path));
  }
  if (rt_ext_ctx && rt_ext_ctx->trap_ptr) {
    const char* p = rt_ext_ctx->trap_ptr;
    (void)write(STDERR_FILENO, " ptr=", 5);
    (void)write(STDERR_FILENO, p, strlen(p));
  }
  if (msg || path || (rt_ext_ctx && rt_ext_ctx->trap_ptr)) (void)write(STDERR_FILENO, "\n", 1);
#else
  (void)msg;
  (void)path;
#endif
  __builtin_trap();
}

static void rt_fuel(ctx_t* ctx, uint64_t amount) {
  if (ctx->fuel < amount) {
    if (ctx->budget_fuel_depth != 0) rt_trap("X07T_BUDGET_EXCEEDED_FUEL");
    rt_trap("fuel exhausted");
  }
  ctx->fuel -= amount;
}

static uint32_t rt_align_u32(uint32_t x, uint32_t align) {
  return (x + (align - 1u)) & ~(align - 1u);
}

static uint16_t rt_read_u16_le(const uint8_t* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rt_read_u32_le(const uint8_t* p) {
  return (uint32_t)p[0]
       | ((uint32_t)p[1] << 8)
       | ((uint32_t)p[2] << 16)
       | ((uint32_t)p[3] << 24);
}

static void rt_write_u32_le(uint8_t* p, uint32_t x) {
  p[0] = (uint8_t)(x & UINT32_C(0xFF));
  p[1] = (uint8_t)((x >> 8) & UINT32_C(0xFF));
  p[2] = (uint8_t)((x >> 16) & UINT32_C(0xFF));
  p[3] = (uint8_t)((x >> 24) & UINT32_C(0xFF));
}

static void rt_heap_init(ctx_t* ctx) {
  if (!ctx->heap.mem) rt_trap("heap mem is NULL");
  uint32_t cap = ctx->heap.cap;
  cap &= ~(RT_HEAP_ALIGN - 1u);
  if (cap < (uint32_t)sizeof(heap_hdr_t) + RT_HEAP_ALIGN) rt_trap("heap too small");
  ctx->heap.cap = cap;

  ctx->heap.free_head = 0;
  heap_hdr_t* h = (heap_hdr_t*)(ctx->heap.mem);
  h->size = cap;
  h->next_off = RT_HEAP_NULL_OFF;
  h->magic = RT_HEAP_MAGIC_FREE;
  h->req_size = 0;
}

static heap_hdr_t* rt_heap_hdr_at(ctx_t* ctx, uint32_t off) {
  if (off == RT_HEAP_NULL_OFF) return (heap_hdr_t*)NULL;
  if (off > ctx->heap.cap || ctx->heap.cap - off < (uint32_t)sizeof(heap_hdr_t)) rt_trap("heap corrupt");
  return (heap_hdr_t*)(ctx->heap.mem + off);
}

static uint32_t rt_heap_off_of(ctx_t* ctx, heap_hdr_t* h) {
  uintptr_t base = (uintptr_t)(ctx->heap.mem);
  uintptr_t p = (uintptr_t)h;
  if (p < base) rt_trap("heap ptr underflow");
  uintptr_t off = p - base;
  if (off > (uintptr_t)UINT32_MAX) rt_trap("heap ptr overflow");
  if ((uint32_t)off > ctx->heap.cap) rt_trap("heap ptr oob");
  return (uint32_t)off;
}

static uint32_t rt_heap_is_pow2_u32(uint32_t x) {
  return x != 0 && (x & (x - 1u)) == 0;
}

static void* rt_heap_alloc(ctx_t* ctx, uint32_t size, uint32_t align) {
  if (size == 0) return (void*)ctx->heap.mem;
  if (align == 0) rt_trap("alloc align=0");
  if (!rt_heap_is_pow2_u32(align)) rt_trap("alloc align not pow2");
  if (align > RT_HEAP_ALIGN) rt_trap("alloc align too large");

  uint32_t payload = rt_heap_align_u32(size);
  uint32_t need = (uint32_t)sizeof(heap_hdr_t) + payload;
  need = rt_heap_align_u32(need);
  if (need < (uint32_t)sizeof(heap_hdr_t) + RT_HEAP_ALIGN) {
    need = (uint32_t)sizeof(heap_hdr_t) + RT_HEAP_ALIGN;
  }
  if (need > ctx->heap.cap) return NULL;

  uint32_t prev_off = RT_HEAP_NULL_OFF;
  uint32_t off = ctx->heap.free_head;
  while (off != RT_HEAP_NULL_OFF) {
    heap_hdr_t* h = rt_heap_hdr_at(ctx, off);
    if (h->magic != RT_HEAP_MAGIC_FREE) rt_trap("heap free list corrupt");
    if (h->size >= need) {
      uint32_t next_off = h->next_off;

      // Remove from free list.
      if (prev_off == RT_HEAP_NULL_OFF) {
        ctx->heap.free_head = next_off;
      } else {
        heap_hdr_t* prev = rt_heap_hdr_at(ctx, prev_off);
        prev->next_off = next_off;
      }

      uint32_t remaining = h->size - need;
      if (remaining >= (uint32_t)sizeof(heap_hdr_t) + RT_HEAP_ALIGN) {
        uint32_t rem_off = off + need;
        heap_hdr_t* rem = rt_heap_hdr_at(ctx, rem_off);
        rem->size = remaining;
        rem->next_off = next_off;
        rem->magic = RT_HEAP_MAGIC_FREE;
        rem->req_size = 0;

        if (prev_off == RT_HEAP_NULL_OFF) {
          ctx->heap.free_head = rem_off;
        } else {
          heap_hdr_t* prev = rt_heap_hdr_at(ctx, prev_off);
          prev->next_off = rem_off;
        }
        h->size = need;
      } else {
        // Don't split; keep whole block.
        need = h->size;
      }

      h->next_off = ctx->mem_epoch_id;
      h->magic = RT_HEAP_MAGIC_USED;
      h->req_size = size;

      void* ptr = (void*)(ctx->heap.mem + off + (uint32_t)sizeof(heap_hdr_t));
      memset(ptr, 0, payload);
      return ptr;
    }
    prev_off = off;
    off = h->next_off;
  }
  return NULL;
}

static void rt_heap_free(ctx_t* ctx, void* ptr) {
  if (!ptr) return;
  if ((uint8_t*)ptr == ctx->heap.mem) return;
  uint8_t* p = (uint8_t*)ptr;
  if (p < ctx->heap.mem + (uint32_t)sizeof(heap_hdr_t)) rt_trap("free oob");
  heap_hdr_t* h = (heap_hdr_t*)(p - (uint32_t)sizeof(heap_hdr_t));
  uint32_t off = rt_heap_off_of(ctx, h);
  if (h->magic != RT_HEAP_MAGIC_USED) rt_trap("double free or corrupt heap");
  uint32_t size = h->size;
  if (size < (uint32_t)sizeof(heap_hdr_t) + RT_HEAP_ALIGN) rt_trap("free corrupt size");
  if ((size & (RT_HEAP_ALIGN - 1u)) != 0) rt_trap("free corrupt size");
  if (off > ctx->heap.cap || ctx->heap.cap - off < size) rt_trap("free oob");

  // Insert into free list (sorted by address).
  uint32_t prev_off = RT_HEAP_NULL_OFF;
  uint32_t cur_off = ctx->heap.free_head;
  while (cur_off != RT_HEAP_NULL_OFF && cur_off < off) {
    heap_hdr_t* cur = rt_heap_hdr_at(ctx, cur_off);
    prev_off = cur_off;
    cur_off = cur->next_off;
  }

  h->magic = RT_HEAP_MAGIC_FREE;
  h->next_off = cur_off;
  h->req_size = 0;

  if (prev_off == RT_HEAP_NULL_OFF) {
    ctx->heap.free_head = off;
  } else {
    heap_hdr_t* prev = rt_heap_hdr_at(ctx, prev_off);
    prev->next_off = off;
  }

  // Coalesce with next.
  if (cur_off != RT_HEAP_NULL_OFF) {
    heap_hdr_t* next = rt_heap_hdr_at(ctx, cur_off);
    if (off + h->size == cur_off) {
      h->size += next->size;
      h->next_off = next->next_off;
    }
  }

  // Coalesce with prev.
  if (prev_off != RT_HEAP_NULL_OFF) {
    heap_hdr_t* prev = rt_heap_hdr_at(ctx, prev_off);
    if (prev_off + prev->size == off) {
      prev->size += h->size;
      prev->next_off = h->next_off;
    }
  }
}

static void rt_mem_epoch_reset(ctx_t* ctx) {
  ctx->mem_epoch_id += 1;
  if (ctx->mem_epoch_id == 0) ctx->mem_epoch_id = 1;

  ctx->mem_stats.alloc_calls = 0;
  ctx->mem_stats.realloc_calls = 0;
  ctx->mem_stats.free_calls = 0;
  ctx->mem_stats.bytes_alloc_total = 0;
  ctx->mem_stats.bytes_freed_total = 0;
  ctx->mem_stats.live_bytes = 0;
  ctx->mem_stats.peak_live_bytes = 0;
  ctx->mem_stats.live_allocs = 0;
  ctx->mem_stats.peak_live_allocs = 0;
  ctx->mem_stats.memcpy_bytes = 0;
}

static void rt_mem_on_alloc(ctx_t* ctx, uint32_t size, uint32_t is_realloc) {
  ctx->heap_live_bytes += (uint64_t)size;
  ctx->heap_live_allocs += 1;
  if (ctx->heap_live_bytes > ctx->heap_peak_live_bytes) {
    ctx->heap_peak_live_bytes = ctx->heap_live_bytes;
  }
  if (ctx->heap_live_allocs > ctx->heap_peak_live_allocs) {
    ctx->heap_peak_live_allocs = ctx->heap_live_allocs;
  }

  if (ctx->mem_epoch_id == 0) return;

  if (is_realloc) {
    ctx->mem_stats.realloc_calls += 1;
  } else {
    ctx->mem_stats.alloc_calls += 1;
  }
  ctx->mem_stats.bytes_alloc_total += (uint64_t)size;
  ctx->mem_stats.live_bytes += (uint64_t)size;
  ctx->mem_stats.live_allocs += 1;
  if (ctx->mem_stats.live_bytes > ctx->mem_stats.peak_live_bytes) {
    ctx->mem_stats.peak_live_bytes = ctx->mem_stats.live_bytes;
  }
  if (ctx->mem_stats.live_allocs > ctx->mem_stats.peak_live_allocs) {
    ctx->mem_stats.peak_live_allocs = ctx->mem_stats.live_allocs;
  }
}

static void rt_mem_on_free(ctx_t* ctx, uint32_t size, uint32_t is_epoch, uint32_t strict) {
  if (ctx->heap_live_bytes < (uint64_t)size) rt_trap("mem free underflow");
  if (ctx->heap_live_allocs == 0) rt_trap("mem free underflow");
  ctx->heap_live_bytes -= (uint64_t)size;
  ctx->heap_live_allocs -= 1;

  if (ctx->mem_epoch_id == 0) return;
  if (!is_epoch) return;

  ctx->mem_stats.free_calls += 1;
  ctx->mem_stats.bytes_freed_total += (uint64_t)size;
  if (ctx->mem_stats.live_bytes < (uint64_t)size || ctx->mem_stats.live_allocs == 0) {
    if (strict) rt_trap("mem epoch underflow");
    return;
  }
  ctx->mem_stats.live_bytes -= (uint64_t)size;
  ctx->mem_stats.live_allocs -= 1;
}

static void rt_mem_on_memcpy(ctx_t* ctx, uint32_t size) {
  if (ctx->mem_epoch_id == 0) return;
  ctx->mem_stats.memcpy_bytes += (uint64_t)size;
}

static uint32_t rt_mem_epoch_pause(ctx_t* ctx) {
  uint32_t saved = ctx->mem_epoch_id;
  ctx->mem_epoch_id = 0;
  return saved;
}

static void rt_mem_epoch_resume(ctx_t* ctx, uint32_t saved) {
  ctx->mem_epoch_id = saved;
}

static void* rt_alloc_raw(ctx_t* ctx, uint32_t size, uint32_t align) {
  void* ptr = rt_heap_alloc(ctx, size, align);
  if (!ptr && size) rt_trap("out of memory");
  return ptr;
}

static void* rt_default_alloc(void* alloc_ctx, uint32_t size, uint32_t align) {
  return rt_alloc_raw((ctx_t*)alloc_ctx, size, align);
}

static void* rt_default_realloc(
    void* alloc_ctx,
    void* ptr,
    uint32_t old_size,
    uint32_t new_size,
    uint32_t align
) {
  (void)ptr;
  (void)old_size;
  return rt_alloc_raw((ctx_t*)alloc_ctx, new_size, align);
}

static void rt_default_free(void* alloc_ctx, void* ptr, uint32_t size, uint32_t align) {
  (void)size;
  (void)align;
  rt_heap_free((ctx_t*)alloc_ctx, ptr);
}

static void rt_allocator_init(ctx_t* ctx) {
  ctx->allocator.ctx = ctx;
  ctx->allocator.alloc = rt_default_alloc;
  ctx->allocator.realloc = rt_default_realloc;
  ctx->allocator.free = rt_default_free;
  ctx->allocator_is_custom = 0;

  allocator_v1_t custom = x07_custom_allocator();
  if (custom.alloc || custom.realloc || custom.free) {
    if (!custom.alloc || !custom.realloc || !custom.free) rt_trap("custom allocator missing hooks");
    ctx->allocator = custom;
    ctx->allocator_is_custom = 1;
  }
}

static void* rt_alloc(ctx_t* ctx, uint32_t size, uint32_t align) {
  if (size == 0) return (void*)ctx->heap.mem;
  if (!ctx->allocator.alloc) rt_trap("allocator.alloc missing");
  void* ptr = ctx->allocator.alloc(ctx->allocator.ctx, size, align);
  if (!ptr && size) rt_trap("allocator.alloc failed");
  rt_mem_on_alloc(ctx, size, 0);
  return ptr;
}

static void* rt_alloc_realloc(
    ctx_t* ctx,
    void* old_ptr,
    uint32_t old_size,
    uint32_t new_size,
    uint32_t align
) {
  if (new_size == 0) return (void*)ctx->heap.mem;
  if (!ctx->allocator.realloc) rt_trap("allocator.realloc missing");
  void* new_ptr =
      ctx->allocator.realloc(ctx->allocator.ctx, old_ptr, old_size, new_size, align);
  if (!new_ptr && new_size) rt_trap("allocator.realloc failed");
  rt_mem_on_alloc(ctx, new_size, old_size != 0);
  return new_ptr;
}

static void rt_free(ctx_t* ctx, void* ptr, uint32_t size, uint32_t align) {
  if (!ptr) return;
  if (size == 0) return;
  uint32_t is_epoch = 1;
  uint32_t strict = ctx->allocator_is_custom ? 0 : 1;
  if (!ctx->allocator_is_custom && (uint8_t*)ptr != ctx->heap.mem) {
    uint8_t* p = (uint8_t*)ptr;
    if (p < ctx->heap.mem + (uint32_t)sizeof(heap_hdr_t)) rt_trap("free oob");
    heap_hdr_t* h = (heap_hdr_t*)(p - (uint32_t)sizeof(heap_hdr_t));
    if (h->magic != RT_HEAP_MAGIC_USED) rt_trap("free corrupt heap");
    is_epoch = (h->next_off == ctx->mem_epoch_id);
  }
  if (!ctx->allocator.free) rt_trap("allocator.free missing");
  ctx->allocator.free(ctx->allocator.ctx, ptr, size, align);
  rt_mem_on_free(ctx, size, is_epoch, strict);
}

static void rt_mem_free_all(ctx_t* ctx) {
  // Bulk reset used at process end.
  ctx->mem_stats.free_calls += 1;
  ctx->mem_stats.bytes_freed_total += ctx->mem_stats.live_bytes;
  ctx->mem_stats.live_bytes = 0;
  ctx->mem_stats.live_allocs = 0;

  ctx->heap_live_bytes = 0;
  ctx->heap_live_allocs = 0;
  rt_heap_init(ctx);
}

#ifdef X07_DEBUG_BORROW
static void rt_dbg_init(ctx_t* ctx) {
  ctx->dbg_allocs_len = 0;
  ctx->dbg_allocs_cap = X07_DBG_ALLOC_CAP;
  ctx->dbg_allocs =
      (dbg_alloc_rec_t*)rt_alloc(
          ctx,
          ctx->dbg_allocs_cap * (uint32_t)sizeof(dbg_alloc_rec_t),
          (uint32_t)_Alignof(dbg_alloc_rec_t)
      );

  ctx->dbg_borrows_len = 0;
  ctx->dbg_borrows_cap = X07_DBG_BORROW_CAP;
  ctx->dbg_borrows =
      (dbg_borrow_rec_t*)rt_alloc(
          ctx,
          ctx->dbg_borrows_cap * (uint32_t)sizeof(dbg_borrow_rec_t),
          (uint32_t)_Alignof(dbg_borrow_rec_t)
      );

  ctx->dbg_borrow_violations = 0;
}

static uint64_t rt_dbg_borrow_acquire(
    ctx_t* ctx,
    uint64_t alloc_id,
    uint32_t off_bytes,
    uint32_t len_bytes
);

static uint64_t rt_dbg_alloc_register(ctx_t* ctx, uint8_t* base_ptr, uint32_t size_bytes) {
  if (size_bytes == 0) return 0;
  if (!ctx->dbg_allocs || ctx->dbg_allocs_cap == 0) return 0;
  if (ctx->dbg_allocs_len >= ctx->dbg_allocs_cap) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint32_t idx = ctx->dbg_allocs_len++;
  dbg_alloc_rec_t* rec = &ctx->dbg_allocs[idx];
  rec->base_ptr = base_ptr;
  rec->size_bytes = size_bytes;
  rec->alive = 1;
  rec->borrow_id = rt_dbg_borrow_acquire(ctx, (uint64_t)idx + 1, 0, size_bytes);
  return (uint64_t)idx + 1;
}

static void rt_dbg_alloc_kill(ctx_t* ctx, uint64_t alloc_id) {
  if (alloc_id == 0) return;
  uint64_t idx = alloc_id - 1;
  if (!ctx->dbg_allocs || idx >= (uint64_t)ctx->dbg_allocs_len) {
    ctx->dbg_borrow_violations += 1;
    return;
  }
  ctx->dbg_allocs[idx].alive = 0;
}

static uint64_t rt_dbg_alloc_find(
    ctx_t* ctx,
    uint8_t* ptr,
    uint32_t len_bytes,
    uint32_t* out_off_bytes
) {
  if (out_off_bytes) *out_off_bytes = 0;
  if (len_bytes == 0) return 0;
  if (!ctx->dbg_allocs) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uintptr_t p = (uintptr_t)ptr;
  // Search newest-to-oldest so re-used pointers resolve to the latest allocation record.
  for (uint32_t i = ctx->dbg_allocs_len; i > 0; i--) {
    uint32_t idx = i - 1;
    dbg_alloc_rec_t* rec = &ctx->dbg_allocs[idx];
    uintptr_t base = (uintptr_t)rec->base_ptr;
    uintptr_t end = base + (uintptr_t)rec->size_bytes;
    if (end < base) continue;
    if (p >= base && p < end) {
      uint32_t off = (uint32_t)(p - base);
      if (off > rec->size_bytes || rec->size_bytes - off < len_bytes) {
        ctx->dbg_borrow_violations += 1;
        return 0;
      }
      if (out_off_bytes) *out_off_bytes = off;
      return (uint64_t)idx + 1;
    }
  }
  ctx->dbg_borrow_violations += 1;
  return 0;
}

static uint64_t rt_dbg_alloc_try_find(
    ctx_t* ctx,
    uint8_t* ptr,
    uint32_t len_bytes,
    uint32_t* out_off_bytes
) {
  if (out_off_bytes) *out_off_bytes = 0;
  if (len_bytes == 0) return 0;
  if (!ctx->dbg_allocs) return 0;
  uintptr_t p = (uintptr_t)ptr;
  // Search newest-to-oldest so re-used pointers resolve to the latest allocation record.
  for (uint32_t i = ctx->dbg_allocs_len; i > 0; i--) {
    uint32_t idx = i - 1;
    dbg_alloc_rec_t* rec = &ctx->dbg_allocs[idx];
    uintptr_t base = (uintptr_t)rec->base_ptr;
    uintptr_t end = base + (uintptr_t)rec->size_bytes;
    if (end < base) continue;
    if (p >= base && p < end) {
      uint32_t off = (uint32_t)(p - base);
      if (off > rec->size_bytes || rec->size_bytes - off < len_bytes) return 0;
      if (out_off_bytes) *out_off_bytes = off;
      return (uint64_t)idx + 1;
    }
  }
  return 0;
}

static uint64_t rt_dbg_alloc_borrow_id(ctx_t* ctx, uint64_t alloc_id) {
  if (alloc_id == 0) return 0;
  uint64_t idx = alloc_id - 1;
  if (!ctx->dbg_allocs || idx >= (uint64_t)ctx->dbg_allocs_len) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint64_t bid = ctx->dbg_allocs[idx].borrow_id;
  if (bid == 0) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  return bid;
}

static uint64_t rt_dbg_borrow_acquire(
    ctx_t* ctx,
    uint64_t alloc_id,
    uint32_t off_bytes,
    uint32_t len_bytes
) {
  if (len_bytes == 0) return 0;
  if (alloc_id == 0) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint64_t aidx = alloc_id - 1;
  if (!ctx->dbg_allocs || aidx >= (uint64_t)ctx->dbg_allocs_len) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  dbg_alloc_rec_t* a = &ctx->dbg_allocs[aidx];
  if (!a->alive) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  if (off_bytes > a->size_bytes || a->size_bytes - off_bytes < len_bytes) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  if (!ctx->dbg_borrows || ctx->dbg_borrows_cap == 0) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  if (ctx->dbg_borrows_len >= ctx->dbg_borrows_cap) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint32_t idx = ctx->dbg_borrows_len++;
  dbg_borrow_rec_t* b = &ctx->dbg_borrows[idx];
  b->alloc_id = alloc_id;
  b->off_bytes = off_bytes;
  b->len_bytes = len_bytes;
  b->active = 1;
  return (uint64_t)idx + 1;
}

static void rt_dbg_borrow_release(ctx_t* ctx, uint64_t borrow_id) {
  if (borrow_id == 0) return;
  uint64_t idx = borrow_id - 1;
  if (!ctx->dbg_borrows || idx >= (uint64_t)ctx->dbg_borrows_len) {
    ctx->dbg_borrow_violations += 1;
    return;
  }
  ctx->dbg_borrows[idx].active = 0;
}

static uint32_t rt_dbg_borrow_check(
    ctx_t* ctx,
    uint64_t borrow_id,
    uint32_t off_bytes,
    uint32_t len_bytes
) {
  if (len_bytes == 0) return 1;
  if (borrow_id == 0) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint64_t idx = borrow_id - 1;
  if (!ctx->dbg_borrows || idx >= (uint64_t)ctx->dbg_borrows_len) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  dbg_borrow_rec_t* b = &ctx->dbg_borrows[idx];
  if (!b->active) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint64_t alloc_id = b->alloc_id;
  if (alloc_id == 0) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint64_t aidx = alloc_id - 1;
  if (!ctx->dbg_allocs || aidx >= (uint64_t)ctx->dbg_allocs_len) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  dbg_alloc_rec_t* a = &ctx->dbg_allocs[aidx];
  if (!a->alive) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  if (off_bytes < b->off_bytes) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  uint32_t rel = off_bytes - b->off_bytes;
  if (rel > b->len_bytes || b->len_bytes - rel < len_bytes) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  if (off_bytes > a->size_bytes || a->size_bytes - off_bytes < len_bytes) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  return 1;
}
#endif

static inline bytes_t rt_bytes_empty(ctx_t* ctx) {
  bytes_t out;
  out.ptr = ctx->heap.mem;
  out.len = UINT32_C(0);
  return out;
}

static inline bytes_view_t rt_view_empty(ctx_t* ctx) {
  bytes_view_t out;
  out.ptr = ctx->heap.mem;
  out.len = UINT32_C(0);
#ifdef X07_DEBUG_BORROW
  out.aid = 0;
  out.bid = 0;
  out.off_bytes = 0;
#endif
  return out;
}

static inline rt_task_out_t rt_task_out_empty(ctx_t* ctx) {
  rt_task_out_t out;
  out.kind = RT_TASK_OUT_KIND_BYTES;
  out.payload.bytes = rt_bytes_empty(ctx);
  return out;
}

static bytes_t rt_bytes_alloc(ctx_t* ctx, uint32_t len) {
  if (len == 0) return rt_bytes_empty(ctx);
  bytes_t out;
  out.len = len;
  out.ptr = (uint8_t*)rt_alloc(ctx, len, 1);
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_alloc_register(ctx, out.ptr, len);
#endif
  return out;
}

// Native extension hook: allocate bytes using the currently-running ctx allocator.
bytes_t ev_bytes_alloc(uint32_t len) {
  if (!rt_ext_ctx) rt_trap(NULL);
  return rt_bytes_alloc(rt_ext_ctx, len);
}

// Native extension hook: trap without returning.
__attribute__((noreturn)) void ev_trap(int32_t code) {
  (void)code;
  rt_trap(NULL);
}

static bytes_t rt_bytes_from_literal(ctx_t* ctx, const uint8_t* ptr, uint32_t len) {
  bytes_t out = rt_bytes_alloc(ctx, len);
  if (len != 0) {
    memcpy(out.ptr, ptr, len);
    rt_mem_on_memcpy(ctx, len);
  }
  return out;
}

static bytes_view_t rt_view_from_literal(ctx_t* ctx, const uint8_t* ptr, uint32_t len) {
  if (len == 0) return rt_view_empty(ctx);
  bytes_view_t out;
  out.ptr = (uint8_t*)ptr;
  out.len = len;
#ifdef X07_DEBUG_BORROW
  uint32_t off = 0;
  uint64_t aid = rt_dbg_alloc_try_find(ctx, (uint8_t*)ptr, len, &off);
  if (aid == 0) {
    aid = rt_dbg_alloc_register(ctx, (uint8_t*)ptr, len);
    off = 0;
  }
  out.aid = aid;
  out.off_bytes = off;
  out.bid = rt_dbg_alloc_borrow_id(ctx, aid);
#endif
  return out;
}

static bytes_t rt_bytes_clone(ctx_t* ctx, bytes_t src) {
  bytes_t out = rt_bytes_alloc(ctx, src.len);
  if (src.len != 0) {
    memcpy(out.ptr, src.ptr, src.len);
    rt_mem_on_memcpy(ctx, src.len);
  }
  return out;
}

static void rt_bytes_drop(ctx_t* ctx, bytes_t* b) {
  if (!b) return;
  if (b->len == 0) {
    b->ptr = ctx->heap.mem;
    return;
  }
#ifdef X07_DEBUG_BORROW
  uint64_t aid = rt_dbg_alloc_find(ctx, b->ptr, b->len, NULL);
  rt_dbg_alloc_kill(ctx, aid);
#endif
  uint32_t size = b->len;
  // Heap allocator stores the requested size in the allocation header; use it for exact accounting.
  if (ctx->allocator.free == rt_default_free) {
    heap_hdr_t* h = (heap_hdr_t*)(b->ptr - (uint32_t)sizeof(heap_hdr_t));
    if (h->magic != RT_HEAP_MAGIC_USED) rt_trap("bytes.drop corrupt header");
    if (h->req_size == 0) rt_trap("bytes.drop corrupt header");
    size = h->req_size;
  }
  rt_free(ctx, b->ptr, size, 1);
  b->ptr = ctx->heap.mem;
  b->len = UINT32_C(0);
}

static void rt_task_out_drop(ctx_t* ctx, rt_task_out_t* out) {
  if (!out) return;
  if (out->kind == RT_TASK_OUT_KIND_BYTES) {
    rt_bytes_drop(ctx, &out->payload.bytes);
    out->payload.bytes = rt_bytes_empty(ctx);
    out->kind = RT_TASK_OUT_KIND_BYTES;
    return;
  }
  if (out->kind == RT_TASK_OUT_KIND_RESULT_BYTES) {
    if (out->payload.result_bytes.tag) {
      rt_bytes_drop(ctx, &out->payload.result_bytes.payload.ok);
    }
    out->payload.result_bytes.tag = UINT32_C(0);
    out->payload.result_bytes.payload.err = UINT32_C(0);
    out->kind = RT_TASK_OUT_KIND_BYTES;
    out->payload.bytes = rt_bytes_empty(ctx);
    return;
  }
  rt_trap("task.out.drop invalid kind");
}

static bytes_t rt_view_to_bytes(ctx_t* ctx, bytes_view_t v) {
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_borrow_check(ctx, v.bid, v.off_bytes, v.len);
#endif
  bytes_t out = rt_bytes_alloc(ctx, v.len);
  if (v.len != 0) {
    memcpy(out.ptr, v.ptr, v.len);
    rt_mem_on_memcpy(ctx, v.len);
  }
  return out;
}

#ifdef X07_DEBUG_BORROW
static uint32_t rt_dbg_bytes_check(ctx_t* ctx, bytes_t b) {
  if (b.len == 0) return 1;
  uint64_t aid = rt_dbg_alloc_find(ctx, b.ptr, b.len, NULL);
  if (aid == 0) return 0;
  dbg_alloc_rec_t* a = &ctx->dbg_allocs[aid - 1];
  if (!a->alive) {
    ctx->dbg_borrow_violations += 1;
    return 0;
  }
  return 1;
}
#endif

static uint32_t rt_bytes_get_u8(ctx_t* ctx, bytes_t b, uint32_t idx) {
  if (idx >= b.len) rt_trap("bytes.get_u8 oob");
#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_bytes_check(ctx, b)) return 0;
#else
  (void)ctx;
#endif
  return (uint32_t)b.ptr[idx];
}

static bytes_t rt_bytes_set_u8(ctx_t* ctx, bytes_t b, uint32_t idx, uint32_t v) {
  if (idx >= b.len) rt_trap("bytes.set_u8 oob");
#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_bytes_check(ctx, b)) return b;
#else
  (void)ctx;
#endif
  b.ptr[idx] = (uint8_t)(v & UINT32_C(0xFF));
  return b;
}

static bytes_t rt_bytes_copy(ctx_t* ctx, bytes_t src, bytes_t dst) {
  if (dst.len < src.len) rt_trap("bytes.copy dst too small");
#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_bytes_check(ctx, src)) return dst;
  if (!rt_dbg_bytes_check(ctx, dst)) return dst;
#endif
  if (src.len != 0) {
    memcpy(dst.ptr, src.ptr, src.len);
    rt_mem_on_memcpy(ctx, src.len);
  }
  return dst;
}

static bytes_t rt_bytes_slice(ctx_t* ctx, bytes_t b, uint32_t start, uint32_t len) {
  if (start > b.len) rt_trap("bytes.slice oob");
  if (len > b.len - start) rt_trap("bytes.slice oob");

#ifdef X07_DEBUG_BORROW
  // Bulk ops bypass per-byte checks; validate once up front.
  uint32_t ok = rt_dbg_bytes_check(ctx, b);
  if (!ok && b.len != 0) {
    return rt_bytes_alloc(ctx, len);
  }
#endif

  bytes_t out = rt_bytes_alloc(ctx, len);
  if (len != 0) {
    memcpy(out.ptr, b.ptr + start, len);
    rt_mem_on_memcpy(ctx, len);
  }
  return out;
}

static bytes_t rt_bytes_concat(ctx_t* ctx, bytes_t a, bytes_t b) {
  uint32_t la = a.len;
  uint32_t lb = b.len;
  if (UINT32_MAX - la < lb) rt_trap("bytes.concat overflow");
#ifdef X07_DEBUG_BORROW
  uint32_t ok_a = rt_dbg_bytes_check(ctx, a);
  uint32_t ok_b = rt_dbg_bytes_check(ctx, b);
  if ((!ok_a && la != 0) || (!ok_b && lb != 0)) {
    return rt_bytes_alloc(ctx, la + lb);
  }
#endif

  bytes_t out = rt_bytes_alloc(ctx, la + lb);
  if (la != 0) {
    memcpy(out.ptr, a.ptr, la);
    rt_mem_on_memcpy(ctx, la);
  }
  if (lb != 0) {
    memcpy(out.ptr + la, b.ptr, lb);
    rt_mem_on_memcpy(ctx, lb);
  }
  return out;
}

static uint32_t rt_bytes_eq(ctx_t* ctx, bytes_t a, bytes_t b) {
  uint32_t a_prefix_len = 0;
  uint32_t b_prefix_len = 0;
  ctx->last_bytes_eq_valid = 0;
#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_bytes_check(ctx, a)) return UINT32_C(0);
  if (!rt_dbg_bytes_check(ctx, b)) return UINT32_C(0);
#endif
  if (a.len != b.len) goto mismatch;
  if (a.len == 0) return UINT32_C(1);
#ifdef X07_DEBUG_BORROW
  // rt_dbg_bytes_check already ran above.
#endif
  if (memcmp(a.ptr, b.ptr, a.len) == 0) return UINT32_C(1);

mismatch:
  ctx->last_bytes_eq_valid = 1;
  ctx->last_bytes_eq_a_len = a.len;
  ctx->last_bytes_eq_b_len = b.len;
  a_prefix_len =
      (a.len < X07_ASSERT_BYTES_EQ_PREFIX_MAX) ? a.len : X07_ASSERT_BYTES_EQ_PREFIX_MAX;
  b_prefix_len =
      (b.len < X07_ASSERT_BYTES_EQ_PREFIX_MAX) ? b.len : X07_ASSERT_BYTES_EQ_PREFIX_MAX;
  if (a_prefix_len) memcpy(ctx->last_bytes_eq_a_prefix, a.ptr, a_prefix_len);
  if (b_prefix_len) memcpy(ctx->last_bytes_eq_b_prefix, b.ptr, b_prefix_len);
  return UINT32_C(0);
}

static uint32_t rt_bytes_cmp_range(
    ctx_t* ctx,
    bytes_t a,
    uint32_t a_off,
    uint32_t a_len,
    bytes_t b,
    uint32_t b_off,
    uint32_t b_len
) {
  if (a_off > a.len || a.len - a_off < a_len) rt_trap("bytes.cmp_range oob");
  if (b_off > b.len || b.len - b_off < b_len) rt_trap("bytes.cmp_range oob");

#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_bytes_check(ctx, a)) return UINT32_C(0);
  if (!rt_dbg_bytes_check(ctx, b)) return UINT32_C(0);
#else
  (void)ctx;
#endif

  uint32_t m = (a_len < b_len) ? a_len : b_len;
  if (m) {
    int cmp = memcmp(a.ptr + a_off, b.ptr + b_off, m);
    if (cmp < 0) return UINT32_MAX;
    if (cmp > 0) return UINT32_C(1);
  }
  if (a_len < b_len) return UINT32_MAX;
  if (a_len > b_len) return UINT32_C(1);
  return UINT32_C(0);
}

static bytes_view_t rt_bytes_view(ctx_t* ctx, bytes_t b) {
  bytes_view_t out;
  out.len = b.len;
#ifdef X07_DEBUG_BORROW
  if (b.len == 0) return rt_view_empty(ctx);
  uint32_t off = 0;
  uint64_t aid = rt_dbg_alloc_find(ctx, b.ptr, b.len, &off);
  out.ptr = b.ptr;
  out.aid = aid;
  out.off_bytes = off;
  out.bid = rt_dbg_alloc_borrow_id(ctx, aid);
#else
  out.ptr = (b.len == 0) ? ctx->heap.mem : b.ptr;
#endif
  return out;
}

static bytes_view_t rt_bytes_subview(ctx_t* ctx, bytes_t b, uint32_t start, uint32_t len) {
  if (start > b.len) rt_trap("bytes.subview oob");
  if (len > b.len - start) rt_trap("bytes.subview oob");
  bytes_view_t out;
  out.len = len;
#ifdef X07_DEBUG_BORROW
  if (len == 0) {
    return rt_view_empty(ctx);
  }
  uint32_t base_off = 0;
  uint64_t aid = rt_dbg_alloc_find(ctx, b.ptr, b.len, &base_off);
  uint32_t off = base_off + start;
  out.ptr = b.ptr + start;
  out.aid = aid;
  out.off_bytes = off;
  out.bid = rt_dbg_alloc_borrow_id(ctx, aid);
#else
  out.ptr = (len == 0) ? ctx->heap.mem : (b.ptr + start);
#endif
  return out;
}

static uint32_t rt_view_get_u8(ctx_t* ctx, bytes_view_t v, uint32_t idx) {
  if (idx >= v.len) rt_trap("view.get_u8 oob");
#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_borrow_check(ctx, v.bid, v.off_bytes + idx, 1)) return 0;
#else
  (void)ctx;
#endif
  return (uint32_t)v.ptr[idx];
}

static bytes_view_t rt_view_slice(ctx_t* ctx, bytes_view_t v, uint32_t start, uint32_t len) {
  if (start > v.len) rt_trap("view.slice oob");
  if (len > v.len - start) rt_trap("view.slice oob");
  bytes_view_t out;
  out.ptr = v.ptr + start;
  out.len = len;
#ifdef X07_DEBUG_BORROW
  out.aid = v.aid;
  out.bid = v.bid;
  out.off_bytes = v.off_bytes + start;
#else
  (void)ctx;
#endif
  return out;
}

static uint32_t rt_view_eq(ctx_t* ctx, bytes_view_t a, bytes_view_t b) {
  uint32_t a_prefix_len = 0;
  uint32_t b_prefix_len = 0;
  ctx->last_bytes_eq_valid = 0;
  if (a.len != b.len) goto mismatch;
  if (a.len == 0) return UINT32_C(1);
#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_borrow_check(ctx, a.bid, a.off_bytes, a.len)) return UINT32_C(0);
  if (!rt_dbg_borrow_check(ctx, b.bid, b.off_bytes, b.len)) return UINT32_C(0);
#endif
  if (memcmp(a.ptr, b.ptr, a.len) == 0) return UINT32_C(1);

mismatch:
  a_prefix_len =
      (a.len < X07_ASSERT_BYTES_EQ_PREFIX_MAX) ? a.len : X07_ASSERT_BYTES_EQ_PREFIX_MAX;
  b_prefix_len =
      (b.len < X07_ASSERT_BYTES_EQ_PREFIX_MAX) ? b.len : X07_ASSERT_BYTES_EQ_PREFIX_MAX;
#ifdef X07_DEBUG_BORROW
  if (a_prefix_len && !rt_dbg_borrow_check(ctx, a.bid, a.off_bytes, a_prefix_len)) return UINT32_C(0);
  if (b_prefix_len && !rt_dbg_borrow_check(ctx, b.bid, b.off_bytes, b_prefix_len)) return UINT32_C(0);
#endif
  ctx->last_bytes_eq_valid = 1;
  ctx->last_bytes_eq_a_len = a.len;
  ctx->last_bytes_eq_b_len = b.len;
  if (a_prefix_len) memcpy(ctx->last_bytes_eq_a_prefix, a.ptr, a_prefix_len);
  if (b_prefix_len) memcpy(ctx->last_bytes_eq_b_prefix, b.ptr, b_prefix_len);
  return UINT32_C(0);
}

static uint32_t rt_view_cmp_range(
    ctx_t* ctx,
    bytes_view_t a,
    uint32_t a_off,
    uint32_t a_len,
    bytes_view_t b,
    uint32_t b_off,
    uint32_t b_len
) {
  if (a_off > a.len || a.len - a_off < a_len) rt_trap("view.cmp_range oob");
  if (b_off > b.len || b.len - b_off < b_len) rt_trap("view.cmp_range oob");

#ifdef X07_DEBUG_BORROW
  if (!rt_dbg_borrow_check(ctx, a.bid, a.off_bytes + a_off, a_len)) return UINT32_C(0);
  if (!rt_dbg_borrow_check(ctx, b.bid, b.off_bytes + b_off, b_len)) return UINT32_C(0);
#else
  (void)ctx;
#endif

  uint32_t m = (a_len < b_len) ? a_len : b_len;
  if (m) {
    int cmp = memcmp(a.ptr + a_off, b.ptr + b_off, m);
    if (cmp < 0) return UINT32_MAX;
    if (cmp > 0) return UINT32_C(1);
  }
  if (a_len < b_len) return UINT32_MAX;
  if (a_len > b_len) return UINT32_C(1);
  return UINT32_C(0);
}

// Phase G2: deterministic single-thread cooperative scheduler + channels.

#define RT_WAIT_NONE UINT32_C(0)
#define RT_WAIT_JOIN UINT32_C(1)
#define RT_WAIT_SLEEP UINT32_C(2)
#define RT_WAIT_CHAN_SEND UINT32_C(3)
#define RT_WAIT_CHAN_RECV UINT32_C(4)
#define RT_WAIT_OS_PROC_JOIN UINT32_C(5)
#define RT_WAIT_OS_PROC_EXIT UINT32_C(6)

#define RT_TRACE_SWITCH UINT64_C(1)
#define RT_TRACE_BLOCK UINT64_C(2)
#define RT_TRACE_WAKE UINT64_C(3)
#define RT_TRACE_COMPLETE UINT64_C(4)

struct rt_task_s {
  uint32_t alive;
  uint32_t done;
  uint32_t canceled;

  uint32_t in_ready;
  uint32_t ready_next;

  uint32_t wait_kind;
  uint32_t wait_id;
  uint32_t wait_next;

  uint32_t join_wait_head;
  uint32_t join_wait_tail;

  uint32_t (*poll)(ctx_t* ctx, void* fut, rt_task_out_t* out);
  void (*drop)(ctx_t* ctx, void* fut);
  void* fut;
  rt_task_out_t out;
  uint32_t out_taken;
};

struct rt_timer_ev_s {
  uint64_t wake_time;
  uint64_t seq;
  uint32_t task_id;
};

struct rt_chan_bytes_s {
  uint32_t alive;
  uint32_t closed;
  uint32_t cap;

  bytes_t* buf;
  uint32_t head;
  uint32_t tail;
  uint32_t len;

  uint32_t send_wait_head;
  uint32_t send_wait_tail;
  uint32_t recv_wait_head;
  uint32_t recv_wait_tail;
};

struct rt_select_evt_s {
  uint32_t alive;
  uint32_t taken;

  uint64_t scope_key;

  uint32_t tag;
  uint32_t case_index;
  uint32_t src_id;

  bytes_t payload;
};

// Phase G2: deterministic streaming I/O reader handles + BufRead-like buffering.

#define RT_IO_READER_KIND_FILE UINT32_C(1)
#define RT_IO_READER_KIND_BYTES UINT32_C(2)

struct rt_io_reader_s {
  uint32_t alive;
  uint32_t kind;
  uint32_t eof;
  uint32_t pending_ticks;

#if X07_ENABLE_STREAMING_FILE_IO
  FILE* f;
#endif

  bytes_t bytes;
  uint32_t pos;
};

struct rt_bufread_s {
  uint32_t alive;
  iface_t reader;
  uint32_t eof;
  uint32_t direct_bytes;

  bytes_t buf;
  uint32_t start;
  uint32_t end;
};

static void rt_sched_trace_init(ctx_t* ctx) {
  if (ctx->sched_stats.sched_trace_hash == 0) {
    ctx->sched_stats.sched_trace_hash = UINT64_C(1469598103934665603);
  }
}

static void rt_sched_trace_u64(ctx_t* ctx, uint64_t x) {
  rt_sched_trace_init(ctx);
  ctx->sched_stats.sched_trace_hash ^= x;
  ctx->sched_stats.sched_trace_hash *= UINT64_C(1099511628211);
}

static void rt_sched_trace_event(ctx_t* ctx, uint64_t tag, uint64_t a, uint64_t b) {
  rt_sched_trace_u64(ctx, tag);
  rt_sched_trace_u64(ctx, a);
  rt_sched_trace_u64(ctx, b);
}

static rt_task_t* rt_task_ptr(ctx_t* ctx, uint32_t task_id) {
  if (task_id == 0 || task_id > ctx->sched_tasks_len) rt_trap("task invalid handle");
  rt_task_t* t = &ctx->sched_tasks[task_id - 1];
  if (!t->alive) rt_trap("task invalid handle");
  return t;
}

static rt_chan_bytes_t* rt_chan_bytes_ptr(ctx_t* ctx, uint32_t chan_id) {
  if (chan_id == 0 || chan_id > ctx->sched_chans_len) rt_trap("chan invalid handle");
  rt_chan_bytes_t* c = &ctx->sched_chans[chan_id - 1];
  if (!c->alive) rt_trap("chan invalid handle");
  return c;
}

static rt_select_evt_t* rt_select_evt_ptr(ctx_t* ctx, uint32_t evt_id) {
  if (evt_id == 0 || evt_id > ctx->sched_select_evts_len) rt_trap("X07T_SELECT_EVT_INVALID");
  rt_select_evt_t* e = &ctx->sched_select_evts[evt_id - 1];
  if (!e->alive) rt_trap("X07T_SELECT_EVT_INVALID");
  return e;
}

static rt_io_reader_t* rt_io_reader_ptr(ctx_t* ctx, uint32_t reader_id) {
  if (reader_id == 0 || reader_id > ctx->io_readers_len) rt_trap("io.reader invalid handle");
  rt_io_reader_t* r = &ctx->io_readers[reader_id - 1];
  if (!r->alive) rt_trap("io.reader invalid handle");
  return r;
}

static rt_bufread_t* rt_bufread_ptr(ctx_t* ctx, uint32_t br_id) {
  if (br_id == 0 || br_id > ctx->bufreads_len) rt_trap("bufread invalid handle");
  rt_bufread_t* br = &ctx->bufreads[br_id - 1];
  if (!br->alive) rt_trap("bufread invalid handle");
  return br;
}

static void rt_sched_tasks_ensure_cap(ctx_t* ctx, uint32_t need) {
  if (need <= ctx->sched_tasks_cap) return;
  rt_task_t* old_items = ctx->sched_tasks;
  uint32_t old_cap = ctx->sched_tasks_cap;
  uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(rt_task_t);
  uint32_t new_cap = ctx->sched_tasks_cap ? ctx->sched_tasks_cap : 8;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_task_t* items = (rt_task_t*)rt_alloc_realloc(
    ctx,
    old_items,
    old_bytes_total,
    new_cap * (uint32_t)sizeof(rt_task_t),
    (uint32_t)_Alignof(rt_task_t)
  );
  if (old_items && ctx->sched_tasks_len) {
    uint32_t bytes = ctx->sched_tasks_len * (uint32_t)sizeof(rt_task_t);
    memcpy(items, old_items, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_items && old_bytes_total) {
    rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(rt_task_t));
  }
  ctx->sched_tasks = items;
  ctx->sched_tasks_cap = new_cap;
}

static void rt_sched_chans_ensure_cap(ctx_t* ctx, uint32_t need) {
  if (need <= ctx->sched_chans_cap) return;
  rt_chan_bytes_t* old_items = ctx->sched_chans;
  uint32_t old_cap = ctx->sched_chans_cap;
  uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(rt_chan_bytes_t);
  uint32_t new_cap = ctx->sched_chans_cap ? ctx->sched_chans_cap : 8;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_chan_bytes_t* items = (rt_chan_bytes_t*)rt_alloc_realloc(
    ctx,
    old_items,
    old_bytes_total,
    new_cap * (uint32_t)sizeof(rt_chan_bytes_t),
    (uint32_t)_Alignof(rt_chan_bytes_t)
  );
  if (old_items && ctx->sched_chans_len) {
    uint32_t bytes = ctx->sched_chans_len * (uint32_t)sizeof(rt_chan_bytes_t);
    memcpy(items, old_items, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_items && old_bytes_total) {
    rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(rt_chan_bytes_t));
  }
  ctx->sched_chans = items;
  ctx->sched_chans_cap = new_cap;
}

static void rt_sched_select_evts_ensure_cap(ctx_t* ctx, uint32_t need) {
  if (need <= ctx->sched_select_evts_cap) return;
  rt_select_evt_t* old_items = ctx->sched_select_evts;
  uint32_t old_cap = ctx->sched_select_evts_cap;
  uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(rt_select_evt_t);
  uint32_t new_cap = ctx->sched_select_evts_cap ? ctx->sched_select_evts_cap : 8;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_select_evt_t* items = (rt_select_evt_t*)rt_alloc_realloc(
    ctx,
    old_items,
    old_bytes_total,
    new_cap * (uint32_t)sizeof(rt_select_evt_t),
    (uint32_t)_Alignof(rt_select_evt_t)
  );
  if (old_items && ctx->sched_select_evts_len) {
    uint32_t bytes = ctx->sched_select_evts_len * (uint32_t)sizeof(rt_select_evt_t);
    memcpy(items, old_items, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_items && old_bytes_total) {
    rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(rt_select_evt_t));
  }
  ctx->sched_select_evts = items;
  ctx->sched_select_evts_cap = new_cap;
}

static void rt_sched_timers_ensure_cap(ctx_t* ctx, uint32_t need) {
  if (need <= ctx->sched_timers_cap) return;
  rt_timer_ev_t* old_items = ctx->sched_timers;
  uint32_t old_cap = ctx->sched_timers_cap;
  uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(rt_timer_ev_t);
  uint32_t new_cap = ctx->sched_timers_cap ? ctx->sched_timers_cap : 8;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_timer_ev_t* items = (rt_timer_ev_t*)rt_alloc_realloc(
    ctx,
    old_items,
    old_bytes_total,
    new_cap * (uint32_t)sizeof(rt_timer_ev_t),
    (uint32_t)_Alignof(rt_timer_ev_t)
  );
  if (old_items && ctx->sched_timers_len) {
    uint32_t bytes = ctx->sched_timers_len * (uint32_t)sizeof(rt_timer_ev_t);
    memcpy(items, old_items, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_items && old_bytes_total) {
    rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(rt_timer_ev_t));
  }
  ctx->sched_timers = items;
  ctx->sched_timers_cap = new_cap;
}

static void rt_io_readers_ensure_cap(ctx_t* ctx, uint32_t need) {
  if (need <= ctx->io_readers_cap) return;
  rt_io_reader_t* old_items = ctx->io_readers;
  uint32_t old_cap = ctx->io_readers_cap;
  uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(rt_io_reader_t);
  uint32_t new_cap = ctx->io_readers_cap ? ctx->io_readers_cap : 8;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_io_reader_t* items = (rt_io_reader_t*)rt_alloc_realloc(
    ctx,
    old_items,
    old_bytes_total,
    new_cap * (uint32_t)sizeof(rt_io_reader_t),
    (uint32_t)_Alignof(rt_io_reader_t)
  );
  if (old_items && ctx->io_readers_len) {
    uint32_t bytes = ctx->io_readers_len * (uint32_t)sizeof(rt_io_reader_t);
    memcpy(items, old_items, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_items && old_bytes_total) {
    rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(rt_io_reader_t));
  }
  ctx->io_readers = items;
  ctx->io_readers_cap = new_cap;
}

static void rt_bufreads_ensure_cap(ctx_t* ctx, uint32_t need) {
  if (need <= ctx->bufreads_cap) return;
  rt_bufread_t* old_items = ctx->bufreads;
  uint32_t old_cap = ctx->bufreads_cap;
  uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(rt_bufread_t);
  uint32_t new_cap = ctx->bufreads_cap ? ctx->bufreads_cap : 8;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_bufread_t* items = (rt_bufread_t*)rt_alloc_realloc(
    ctx,
    old_items,
    old_bytes_total,
    new_cap * (uint32_t)sizeof(rt_bufread_t),
    (uint32_t)_Alignof(rt_bufread_t)
  );
  if (old_items && ctx->bufreads_len) {
    uint32_t bytes = ctx->bufreads_len * (uint32_t)sizeof(rt_bufread_t);
    memcpy(items, old_items, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_items && old_bytes_total) {
    rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(rt_bufread_t));
  }
  ctx->bufreads = items;
  ctx->bufreads_cap = new_cap;
}

static void rt_ready_push(ctx_t* ctx, uint32_t task_id) {
  if (task_id == 0) return;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  if (t->done) return;
  if (t->in_ready) return;
  t->in_ready = 1;
  t->ready_next = 0;
  if (ctx->sched_ready_tail == 0) {
    ctx->sched_ready_head = task_id;
    ctx->sched_ready_tail = task_id;
    return;
  }
  rt_task_t* tail = rt_task_ptr(ctx, ctx->sched_ready_tail);
  tail->ready_next = task_id;
  ctx->sched_ready_tail = task_id;
}

static uint32_t rt_ready_pop(ctx_t* ctx) {
  for (;;) {
    uint32_t task_id = ctx->sched_ready_head;
    if (task_id == 0) return 0;
    rt_task_t* t = rt_task_ptr(ctx, task_id);
    ctx->sched_ready_head = t->ready_next;
    if (ctx->sched_ready_head == 0) ctx->sched_ready_tail = 0;
    t->ready_next = 0;
    t->in_ready = 0;
    if (t->done) continue;
    return task_id;
  }
}

static void rt_wait_list_push(ctx_t* ctx, uint32_t* head, uint32_t* tail, uint32_t task_id) {
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  t->wait_next = 0;
  if (*tail == 0) {
    *head = task_id;
    *tail = task_id;
    return;
  }
  rt_task_t* last = rt_task_ptr(ctx, *tail);
  last->wait_next = task_id;
  *tail = task_id;
}

static uint32_t rt_wait_list_pop(ctx_t* ctx, uint32_t* head, uint32_t* tail) {
  for (;;) {
    uint32_t task_id = *head;
    if (task_id == 0) return 0;
    rt_task_t* t = rt_task_ptr(ctx, task_id);
    *head = t->wait_next;
    if (*head == 0) *tail = 0;
    t->wait_next = 0;
    if (t->done) continue;
    return task_id;
  }
}

static void rt_sched_wake(ctx_t* ctx, uint32_t task_id, uint32_t reason_kind, uint32_t reason_id) {
  if (task_id == 0) return;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  if (t->done) return;
  t->wait_kind = RT_WAIT_NONE;
  t->wait_id = 0;
  rt_ready_push(ctx, task_id);
  ctx->sched_stats.wake_events += 1;
  rt_sched_trace_event(
    ctx,
    RT_TRACE_WAKE,
    (uint64_t)task_id,
    ((uint64_t)reason_kind << 32) | (uint64_t)reason_id
  );
}

static uint32_t rt_timer_less(rt_timer_ev_t a, rt_timer_ev_t b) {
  if (a.wake_time < b.wake_time) return 1;
  if (a.wake_time > b.wake_time) return 0;
  return (a.seq < b.seq) ? 1 : 0;
}

static void rt_timer_push(ctx_t* ctx, uint64_t wake_time, uint32_t task_id) {
  rt_sched_timers_ensure_cap(ctx, ctx->sched_timers_len + 1);
  uint32_t i = ctx->sched_timers_len++;
  rt_timer_ev_t ev;
  ev.wake_time = wake_time;
  ev.seq = ctx->sched_seq++;
  ev.task_id = task_id;

  while (i > 0) {
    uint32_t p = (i - 1) / 2;
    rt_timer_ev_t parent = ctx->sched_timers[p];
    if (!rt_timer_less(ev, parent)) break;
    ctx->sched_timers[i] = parent;
    i = p;
  }
  ctx->sched_timers[i] = ev;
}

static uint32_t rt_timer_pop(ctx_t* ctx, rt_timer_ev_t* out) {
  if (ctx->sched_timers_len == 0) return 0;
  if (out) *out = ctx->sched_timers[0];
  ctx->sched_timers_len -= 1;
  if (ctx->sched_timers_len == 0) return 1;

  rt_timer_ev_t ev = ctx->sched_timers[ctx->sched_timers_len];
  uint32_t i = 0;
  for (;;) {
    uint32_t l = i * 2 + 1;
    uint32_t r = l + 1;
    if (l >= ctx->sched_timers_len) break;
    uint32_t m = l;
    if (r < ctx->sched_timers_len && rt_timer_less(ctx->sched_timers[r], ctx->sched_timers[l])) {
      m = r;
    }
    if (!rt_timer_less(ctx->sched_timers[m], ev)) break;
    ctx->sched_timers[i] = ctx->sched_timers[m];
    i = m;
  }
  ctx->sched_timers[i] = ev;
  return 1;
}

static uint64_t rt_timer_peek_wake(ctx_t* ctx) {
  if (ctx->sched_timers_len == 0) return UINT64_MAX;
  return ctx->sched_timers[0].wake_time;
}

static uint32_t rt_sched_step(ctx_t* ctx) {
  uint32_t task_id = rt_ready_pop(ctx);
  if (task_id != 0) {
    rt_task_t* t = rt_task_ptr(ctx, task_id);
    if (t->done) return UINT32_C(1);

    ctx->sched_stats.ctx_switches += 1;
    rt_sched_trace_event(ctx, RT_TRACE_SWITCH, (uint64_t)task_id, ctx->sched_now_ticks);

    uint32_t prev = ctx->sched_current_task;
    ctx->sched_current_task = task_id;

    rt_task_out_t out = rt_task_out_empty(ctx);
    uint32_t done = t->poll(ctx, t->fut, &out);

    ctx->sched_current_task = prev;
    t = rt_task_ptr(ctx, task_id);

    if (done) {
      t->done = 1;
      t->out = out;
      t->out_taken = 0;
      if (t->drop && t->fut) {
        t->drop(ctx, t->fut);
      }
      t->drop = NULL;
      t->fut = NULL;
      rt_sched_trace_event(ctx, RT_TRACE_COMPLETE, (uint64_t)task_id, ctx->sched_now_ticks);

      uint32_t w = t->join_wait_head;
      uint32_t wt = t->join_wait_tail;
      (void)wt;
      t->join_wait_head = 0;
      t->join_wait_tail = 0;
      while (w != 0) {
        rt_task_t* waiter = rt_task_ptr(ctx, w);
        uint32_t next = waiter->wait_next;
        waiter->wait_next = 0;
        rt_sched_wake(ctx, w, RT_WAIT_JOIN, task_id);
        w = next;
      }
      return UINT32_C(1);
    }

    if (!t->in_ready && t->wait_kind == RT_WAIT_NONE) {
      const char* dbg = getenv("X07_DEBUG_SCHED");
      if (dbg && dbg[0] == '1' && dbg[1] == '\0') {
        char msg[192];
        uint32_t st = 0;
        if (t->fut) memcpy(&st, t->fut, sizeof(uint32_t));
        (void)snprintf(
          msg,
          sizeof(msg),
          "task pending without block (task_id=%u poll=%p state=%u)",
          (unsigned)task_id,
          (void*)t->poll,
          (unsigned)st
        );
        rt_trap(msg);
      }
      char msg[96];
      (void)snprintf(
        msg,
        sizeof(msg),
        "task pending without block (task_id=%u)",
        (unsigned)task_id
      );
      rt_trap(msg);
    }
    (void)rt_os_process_poll_all(ctx, 0);
    return UINT32_C(1);
  }

  rt_timer_ev_t ev;
  while (rt_timer_pop(ctx, &ev)) {
    if (ev.task_id == 0) continue;
    rt_task_t* t = rt_task_ptr(ctx, ev.task_id);
    if (t->done) continue;
    if (ev.wake_time > ctx->sched_now_ticks) ctx->sched_now_ticks = ev.wake_time;
    ctx->sched_stats.virtual_time_end = ctx->sched_now_ticks;
    rt_sched_wake(ctx, ev.task_id, RT_WAIT_SLEEP, 0);
    return UINT32_C(1);
  }

  if (rt_os_process_poll_all(ctx, 50)) return UINT32_C(1);
  return UINT32_C(0);
}

static __attribute__((noreturn)) void rt_sched_deadlock(void) {
  rt_trap("scheduler deadlock");
}

static uint32_t rt_task_create(
    ctx_t* ctx,
    uint32_t (*poll)(ctx_t* ctx, void* fut, rt_task_out_t* out),
    void (*drop)(ctx_t* ctx, void* fut),
    void* fut
) {
  rt_sched_tasks_ensure_cap(ctx, ctx->sched_tasks_len + 1);
  uint32_t task_id = ctx->sched_tasks_len + 1;
  rt_task_t* t = &ctx->sched_tasks[task_id - 1];
  memset(t, 0, sizeof(*t));
  t->alive = 1;
  t->poll = poll;
  t->drop = drop;
  t->fut = fut;
  t->out = rt_task_out_empty(ctx);
  t->out_taken = 0;
  ctx->sched_tasks_len += 1;

  ctx->sched_stats.tasks_spawned += 1;
  rt_ready_push(ctx, task_id);
  return task_id;
}

static uint32_t rt_task_spawn(ctx_t* ctx, uint32_t task_id) {
  ctx->sched_stats.spawn_calls += 1;
  (void)rt_task_ptr(ctx, task_id);
  return task_id;
}

static uint32_t rt_task_cancel(ctx_t* ctx, uint32_t task_id) {
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  if (t->done) return UINT32_C(0);
  t->canceled = 1;
  t->done = 1;
  if (t->drop && t->fut) {
    t->drop(ctx, t->fut);
  }
  t->drop = NULL;
  t->fut = NULL;
  rt_task_out_drop(ctx, &t->out);
  t->out = rt_task_out_empty(ctx);
  t->out_taken = 0;
  rt_sched_trace_event(ctx, RT_TRACE_COMPLETE, (uint64_t)task_id, ctx->sched_now_ticks);

  uint32_t w = t->join_wait_head;
  t->join_wait_head = 0;
  t->join_wait_tail = 0;
  while (w != 0) {
    rt_task_t* waiter = rt_task_ptr(ctx, w);
    uint32_t next = waiter->wait_next;
    waiter->wait_next = 0;
    rt_sched_wake(ctx, w, RT_WAIT_JOIN, task_id);
    w = next;
  }
  return UINT32_C(1);
}

static uint32_t rt_task_join_bytes_poll(ctx_t* ctx, uint32_t task_id, bytes_t* out) {
  ctx->sched_stats.join_calls += 1;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  if (t->done) {
    if (t->out_taken) rt_trap("join already taken");
    t->out_taken = 1;
    if (t->canceled) {
      if (out) *out = rt_bytes_empty(ctx);
      t->out = rt_task_out_empty(ctx);
      return UINT32_C(1);
    }
    if (t->out.kind != RT_TASK_OUT_KIND_BYTES) rt_trap("task.join.bytes kind mismatch");
    if (out) {
      *out = t->out.payload.bytes;
    } else {
      rt_bytes_drop(ctx, &t->out.payload.bytes);
    }
    t->out = rt_task_out_empty(ctx);
    return UINT32_C(1);
  }

  uint32_t cur = ctx->sched_current_task;
  if (cur == 0) rt_trap("join.poll from main");
  if (cur == task_id) rt_trap("join self");

  rt_task_t* me = rt_task_ptr(ctx, cur);
  if (me->wait_kind == RT_WAIT_JOIN && me->wait_id == task_id) {
    return UINT32_C(0);
  }
  if (me->wait_kind != RT_WAIT_NONE) rt_trap("join while already waiting");

  me->wait_kind = RT_WAIT_JOIN;
  me->wait_id = task_id;
  ctx->sched_stats.blocked_waits += 1;
  rt_sched_trace_event(ctx, RT_TRACE_BLOCK, (uint64_t)cur, ((uint64_t)RT_WAIT_JOIN << 32) | task_id);
  rt_wait_list_push(ctx, &t->join_wait_head, &t->join_wait_tail, cur);
  return UINT32_C(0);
}

static bytes_t rt_task_join_bytes_block(ctx_t* ctx, uint32_t task_id) {
  ctx->sched_stats.join_calls += 1;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  while (!t->done) {
    if (!rt_sched_step(ctx)) rt_sched_deadlock();
    t = rt_task_ptr(ctx, task_id);
  }
  if (t->out_taken) rt_trap("join already taken");
  t->out_taken = 1;
  if (t->canceled) return rt_bytes_empty(ctx);
  if (t->out.kind != RT_TASK_OUT_KIND_BYTES) rt_trap("task.join.bytes kind mismatch");
  bytes_t out_b = t->out.payload.bytes;
  t->out = rt_task_out_empty(ctx);
  return out_b;
}

static uint32_t rt_task_is_finished(ctx_t* ctx, uint32_t task_id) {
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  return t->done ? UINT32_C(1) : UINT32_C(0);
}

static result_bytes_t rt_task_try_join_bytes(ctx_t* ctx, uint32_t task_id) {
  ctx->sched_stats.join_calls += 1;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  if (!t->done) {
    return (result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(1) };
  }
  if (t->out_taken) rt_trap("join already taken");
  t->out_taken = 1;
  if (t->canceled) {
    return (result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(2) };
  }
  if (t->out.kind != RT_TASK_OUT_KIND_BYTES) rt_trap("task.try_join.bytes kind mismatch");
  bytes_t out_b = t->out.payload.bytes;
  t->out = rt_task_out_empty(ctx);
  return (result_bytes_t){ .tag = UINT32_C(1), .payload.ok = out_b };
}

static uint32_t rt_task_join_result_bytes_poll(ctx_t* ctx, uint32_t task_id, result_bytes_t* out) {
  ctx->sched_stats.join_calls += 1;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  if (t->done) {
    if (t->out_taken) rt_trap("join already taken");
    t->out_taken = 1;
    if (t->canceled) {
      if (out) *out = (result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(2) };
      t->out = rt_task_out_empty(ctx);
      return UINT32_C(1);
    }
    if (t->out.kind != RT_TASK_OUT_KIND_RESULT_BYTES) rt_trap("task.join.result_bytes kind mismatch");
    if (out) {
      *out = t->out.payload.result_bytes;
    } else {
      if (t->out.payload.result_bytes.tag) {
        rt_bytes_drop(ctx, &t->out.payload.result_bytes.payload.ok);
      }
    }
    t->out = rt_task_out_empty(ctx);
    return UINT32_C(1);
  }

  uint32_t cur = ctx->sched_current_task;
  if (cur == 0) rt_trap("join.poll from main");
  if (cur == task_id) rt_trap("join self");

  rt_task_t* me = rt_task_ptr(ctx, cur);
  if (me->wait_kind == RT_WAIT_JOIN && me->wait_id == task_id) {
    return UINT32_C(0);
  }
  if (me->wait_kind != RT_WAIT_NONE) rt_trap("join while already waiting");

  me->wait_kind = RT_WAIT_JOIN;
  me->wait_id = task_id;
  ctx->sched_stats.blocked_waits += 1;
  rt_sched_trace_event(ctx, RT_TRACE_BLOCK, (uint64_t)cur, ((uint64_t)RT_WAIT_JOIN << 32) | task_id);
  rt_wait_list_push(ctx, &t->join_wait_head, &t->join_wait_tail, cur);
  return UINT32_C(0);
}

static result_bytes_t rt_task_join_result_bytes_block(ctx_t* ctx, uint32_t task_id) {
  ctx->sched_stats.join_calls += 1;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  while (!t->done) {
    if (!rt_sched_step(ctx)) rt_sched_deadlock();
    t = rt_task_ptr(ctx, task_id);
  }
  if (t->out_taken) rt_trap("join already taken");
  t->out_taken = 1;
  if (t->canceled) return (result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(2) };
  if (t->out.kind != RT_TASK_OUT_KIND_RESULT_BYTES) rt_trap("task.join.result_bytes kind mismatch");
  result_bytes_t out_rb = t->out.payload.result_bytes;
  t->out = rt_task_out_empty(ctx);
  return out_rb;
}

static result_result_bytes_t rt_task_try_join_result_bytes(ctx_t* ctx, uint32_t task_id) {
  ctx->sched_stats.join_calls += 1;
  rt_task_t* t = rt_task_ptr(ctx, task_id);
  if (!t->done) {
    return (result_result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(1) };
  }
  if (t->out_taken) rt_trap("join already taken");
  t->out_taken = 1;
  if (t->canceled) {
    return (result_result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(2) };
  }
  if (t->out.kind != RT_TASK_OUT_KIND_RESULT_BYTES) rt_trap("task.try_join.result_bytes kind mismatch");
  result_bytes_t out_rb = t->out.payload.result_bytes;
  t->out = rt_task_out_empty(ctx);
  return (result_result_bytes_t){ .tag = UINT32_C(1), .payload.ok = out_rb };
}

static void rt_task_yield(ctx_t* ctx) {
  ctx->sched_stats.yield_calls += 1;
  uint32_t cur = ctx->sched_current_task;
  if (cur == 0) rt_trap("yield from main");
  rt_task_t* me = rt_task_ptr(ctx, cur);
  me->wait_kind = RT_WAIT_NONE;
  me->wait_id = 0;
  rt_sched_trace_event(ctx, RT_TRACE_BLOCK, (uint64_t)cur, ((uint64_t)RT_WAIT_NONE << 32) | RT_WAIT_NONE);
  rt_ready_push(ctx, cur);
}

static uint32_t rt_task_yield_block(ctx_t* ctx) {
  ctx->sched_stats.yield_calls += 1;
  (void)rt_sched_step(ctx);
  return UINT32_C(0);
}

static void rt_task_sleep(ctx_t* ctx, uint32_t ticks) {
  ctx->sched_stats.sleep_calls += 1;
  uint32_t cur = ctx->sched_current_task;
  if (cur == 0) rt_trap("sleep from main");
  rt_task_t* me = rt_task_ptr(ctx, cur);
  if (ticks == 0) {
    rt_ready_push(ctx, cur);
    return;
  }
  if (me->wait_kind != RT_WAIT_NONE) rt_trap("sleep while already waiting");
  me->wait_kind = RT_WAIT_SLEEP;
  me->wait_id = 0;
  ctx->sched_stats.blocked_waits += 1;
  uint64_t wake_time = ctx->sched_now_ticks + (uint64_t)ticks;
  rt_sched_trace_event(ctx, RT_TRACE_BLOCK, (uint64_t)cur, ((uint64_t)RT_WAIT_SLEEP << 32) | (uint64_t)ticks);
  rt_timer_push(ctx, wake_time, cur);
}

static uint32_t rt_task_sleep_block(ctx_t* ctx, uint32_t ticks) {
  ctx->sched_stats.sleep_calls += 1;
  if (ticks == 0) return UINT32_C(0);
  uint64_t target = ctx->sched_now_ticks + (uint64_t)ticks;
  while (ctx->sched_now_ticks < target) {
    if (ctx->sched_ready_head != 0) {
      if (!rt_sched_step(ctx)) break;
      continue;
    }
    uint64_t next = rt_timer_peek_wake(ctx);
    if (next == UINT64_MAX || next > target) {
      ctx->sched_now_ticks = target;
      ctx->sched_stats.virtual_time_end = ctx->sched_now_ticks;
      break;
    }
    if (!rt_sched_step(ctx)) rt_sched_deadlock();
  }
  return UINT32_C(0);
}

typedef struct {
  uint32_t task_id;
  uint32_t kind;
  uint32_t state;
  uint32_t gen;
} rt_scope_slot_t;

typedef struct {
  uint32_t max_children;
  uint64_t max_ticks;
  uint64_t max_blocked_waits;
  uint64_t max_join_polls;
  uint32_t max_slot_result_bytes;

  uint64_t key;

  uint8_t cancel_requested;

  uint64_t snap_ticks;
  uint64_t snap_blocked_waits;
  uint64_t snap_join_polls;
  uint64_t snap_tasks_spawned;

  uint32_t child_cap;
  uint32_t child_len;
  uint32_t* child_task_ids;
  uint32_t* child_task_kinds;

  uint32_t reg_cap;
  uint32_t reg_len;
  uint32_t* reg_task_ids;

  uint32_t slot_cap;
  uint32_t slot_len;
  rt_scope_slot_t* slots;

  uint32_t join_phase;
  uint32_t join_slot_i;
  uint32_t join_child_i;

  uint32_t select_rr_cursor;
} rt_scope_t;

typedef struct {
  uint32_t active;
  uint32_t mode;
  uint32_t violated;
  uint32_t err_code;
  const uint8_t* label_ptr;
  uint32_t label_len;

  uint8_t yielded;
  uint8_t fuel_clamped;
  uint16_t reserved16;

  uint64_t max_alloc_bytes;
  uint64_t max_alloc_calls;
  uint64_t max_realloc_calls;
  uint64_t max_memcpy_bytes;
  uint64_t max_sched_ticks;
  uint64_t max_fuel;

  uint64_t snap_alloc_bytes;
  uint64_t snap_alloc_calls;
  uint64_t snap_realloc_calls;
  uint64_t snap_memcpy_bytes;
  uint64_t snap_sched_ticks;

  uint64_t snap_fuel_saved;
  uint64_t snap_fuel_start;
} rt_budget_scope_t;

#define RT_BUDGET_MODE_TRAP UINT32_C(0)
#define RT_BUDGET_MODE_RESULT_ERR UINT32_C(1)
#define RT_BUDGET_MODE_STATS_ONLY UINT32_C(2)
#define RT_BUDGET_MODE_YIELD UINT32_C(3)

#define RT_ERR_BUDGET_ALLOC_BYTES UINT32_C(0x80000001)
#define RT_ERR_BUDGET_ALLOC_CALLS UINT32_C(0x80000002)
#define RT_ERR_BUDGET_REALLOC_CALLS UINT32_C(0x80000003)
#define RT_ERR_BUDGET_MEMCPY_BYTES UINT32_C(0x80000004)
#define RT_ERR_BUDGET_SCHED_TICKS UINT32_C(0x80000005)

static void rt_budget_scope_drop(ctx_t* ctx, rt_budget_scope_t* s) {
  if (!s->active) return;
  if (s->fuel_clamped) {
    uint64_t consumed = s->snap_fuel_start - ctx->fuel;
    ctx->fuel = s->snap_fuel_saved - consumed;
    if (ctx->budget_fuel_depth == 0) rt_trap("budget fuel depth underflow");
    ctx->budget_fuel_depth -= 1;
  }
  s->active = UINT32_C(0);
}

static void rt_budget_scope_dispose_on_drop(ctx_t* ctx, rt_budget_scope_t* s) {
  rt_budget_scope_drop(ctx, s);
}

static void rt_budget_scope_init(
  ctx_t* ctx,
  rt_budget_scope_t* s,
  uint32_t mode,
  const uint8_t* label_ptr,
  uint32_t label_len,
  uint64_t max_alloc_bytes,
  uint64_t max_alloc_calls,
  uint64_t max_realloc_calls,
  uint64_t max_memcpy_bytes,
  uint64_t max_sched_ticks,
  uint64_t max_fuel
) {
  memset(s, 0, sizeof(*s));
  s->active = UINT32_C(1);
  s->mode = mode;
  s->label_ptr = label_ptr;
  s->label_len = label_len;
  s->max_alloc_bytes = max_alloc_bytes;
  s->max_alloc_calls = max_alloc_calls;
  s->max_realloc_calls = max_realloc_calls;
  s->max_memcpy_bytes = max_memcpy_bytes;
  s->max_sched_ticks = max_sched_ticks;
  s->max_fuel = max_fuel;

  s->snap_alloc_bytes = ctx->mem_stats.bytes_alloc_total;
  s->snap_alloc_calls = ctx->mem_stats.alloc_calls;
  s->snap_realloc_calls = ctx->mem_stats.realloc_calls;
  s->snap_memcpy_bytes = ctx->mem_stats.memcpy_bytes;
  s->snap_sched_ticks = ctx->sched_now_ticks;

  s->snap_fuel_saved = ctx->fuel;
  s->snap_fuel_start = ctx->fuel;
  if (max_fuel != 0 && ctx->fuel > max_fuel) {
    s->snap_fuel_start = max_fuel;
    ctx->fuel = max_fuel;
    s->fuel_clamped = 1;
    ctx->budget_fuel_depth += 1;
  }
}

static void rt_budget_scope_check_exit(ctx_t* ctx, rt_budget_scope_t* s) {
  if (!s->active) return;
  s->violated = UINT32_C(0);
  s->err_code = UINT32_C(0);

  uint64_t alloc_bytes = ctx->mem_stats.bytes_alloc_total - s->snap_alloc_bytes;
  if (s->max_alloc_bytes != 0 && alloc_bytes > s->max_alloc_bytes) {
    if (s->mode == RT_BUDGET_MODE_STATS_ONLY) return;
    if (s->mode == RT_BUDGET_MODE_RESULT_ERR) {
      s->violated = UINT32_C(1);
      s->err_code = RT_ERR_BUDGET_ALLOC_BYTES;
      return;
    }
    rt_trap("X07T_BUDGET_EXCEEDED_ALLOC_BYTES");
  }

  uint64_t alloc_calls = ctx->mem_stats.alloc_calls - s->snap_alloc_calls;
  if (s->max_alloc_calls != 0 && alloc_calls > s->max_alloc_calls) {
    if (s->mode == RT_BUDGET_MODE_STATS_ONLY) return;
    if (s->mode == RT_BUDGET_MODE_RESULT_ERR) {
      s->violated = UINT32_C(1);
      s->err_code = RT_ERR_BUDGET_ALLOC_CALLS;
      return;
    }
    rt_trap("X07T_BUDGET_EXCEEDED_ALLOC_CALLS");
  }

  uint64_t realloc_calls = ctx->mem_stats.realloc_calls - s->snap_realloc_calls;
  if (s->max_realloc_calls != 0 && realloc_calls > s->max_realloc_calls) {
    if (s->mode == RT_BUDGET_MODE_STATS_ONLY) return;
    if (s->mode == RT_BUDGET_MODE_RESULT_ERR) {
      s->violated = UINT32_C(1);
      s->err_code = RT_ERR_BUDGET_REALLOC_CALLS;
      return;
    }
    rt_trap("X07T_BUDGET_EXCEEDED_REALLOC_CALLS");
  }

  uint64_t memcpy_bytes = ctx->mem_stats.memcpy_bytes - s->snap_memcpy_bytes;
  if (s->max_memcpy_bytes != 0 && memcpy_bytes > s->max_memcpy_bytes) {
    if (s->mode == RT_BUDGET_MODE_STATS_ONLY) return;
    if (s->mode == RT_BUDGET_MODE_RESULT_ERR) {
      s->violated = UINT32_C(1);
      s->err_code = RT_ERR_BUDGET_MEMCPY_BYTES;
      return;
    }
    rt_trap("X07T_BUDGET_EXCEEDED_MEMCPY_BYTES");
  }

  uint64_t sched_ticks = ctx->sched_now_ticks - s->snap_sched_ticks;
  if (s->max_sched_ticks != 0 && sched_ticks > s->max_sched_ticks) {
    if (s->mode == RT_BUDGET_MODE_STATS_ONLY || s->mode == RT_BUDGET_MODE_YIELD) return;
    if (s->mode == RT_BUDGET_MODE_RESULT_ERR) {
      s->violated = UINT32_C(1);
      s->err_code = RT_ERR_BUDGET_SCHED_TICKS;
      return;
    }
    rt_trap("X07T_BUDGET_EXCEEDED_SCHED_TICKS");
  }
}

static uint32_t rt_budget_scope_exit_poll(ctx_t* ctx, rt_budget_scope_t* s) {
  if (!s->active) return UINT32_C(1);

  if (s->mode == RT_BUDGET_MODE_YIELD && !s->yielded && s->max_sched_ticks != 0) {
    uint64_t ticks = ctx->sched_now_ticks - s->snap_sched_ticks;
    if (ticks > s->max_sched_ticks) {
      s->yielded = 1;
      rt_task_yield(ctx);
      s->snap_sched_ticks = ctx->sched_now_ticks;
      return UINT32_C(0);
    }
  }

  rt_budget_scope_check_exit(ctx, s);
  rt_budget_scope_drop(ctx, s);
  return UINT32_C(1);
}

static void rt_budget_scope_exit_block(ctx_t* ctx, rt_budget_scope_t* s) {
  if (!s->active) return;

  if (s->mode == RT_BUDGET_MODE_YIELD && !s->yielded && s->max_sched_ticks != 0) {
    uint64_t ticks = ctx->sched_now_ticks - s->snap_sched_ticks;
    if (ticks > s->max_sched_ticks) {
      s->yielded = 1;
      (void)rt_task_yield_block(ctx);
      s->snap_sched_ticks = ctx->sched_now_ticks;
    }
  }

  rt_budget_scope_check_exit(ctx, s);
  rt_budget_scope_drop(ctx, s);
}

#define RT_SCOPE_SLOT_EMPTY UINT32_C(0)
#define RT_SCOPE_SLOT_PENDING UINT32_C(1)
#define RT_SCOPE_SLOT_TAKEN UINT32_C(2)
#define RT_SCOPE_SLOT_CONSUMED UINT32_C(3)

static void rt_scope_init(
  ctx_t* ctx,
  rt_scope_t* s,
  uint32_t max_children,
  uint64_t max_ticks,
  uint64_t max_blocked_waits,
  uint64_t max_join_polls,
  uint32_t max_slot_result_bytes
) {
  memset(s, 0, sizeof(*s));
  s->max_children = max_children;
  s->max_ticks = max_ticks;
  s->max_blocked_waits = max_blocked_waits;
  s->max_join_polls = max_join_polls;
  s->max_slot_result_bytes = max_slot_result_bytes;
  s->key = (uint64_t)(uintptr_t)s;

  s->child_cap = max_children;
  s->reg_cap = max_children;
  s->slot_cap = max_children;
  s->slot_len = max_children;

  if (max_children != 0) {
    s->child_task_ids = (uint32_t*)rt_alloc(
      ctx,
      max_children * (uint32_t)sizeof(uint32_t),
      (uint32_t)_Alignof(uint32_t)
    );
    s->child_task_kinds = (uint32_t*)rt_alloc(
      ctx,
      max_children * (uint32_t)sizeof(uint32_t),
      (uint32_t)_Alignof(uint32_t)
    );
    s->reg_task_ids = (uint32_t*)rt_alloc(
      ctx,
      max_children * (uint32_t)sizeof(uint32_t),
      (uint32_t)_Alignof(uint32_t)
    );
    s->slots = (rt_scope_slot_t*)rt_alloc(
      ctx,
      max_children * (uint32_t)sizeof(rt_scope_slot_t),
      (uint32_t)_Alignof(rt_scope_slot_t)
    );
    memset(s->slots, 0, max_children * (uint32_t)sizeof(rt_scope_slot_t));
  }

  s->snap_ticks = ctx->sched_now_ticks;
  s->snap_blocked_waits = ctx->sched_stats.blocked_waits;
  s->snap_join_polls = ctx->sched_stats.join_calls;
  s->snap_tasks_spawned = ctx->sched_stats.tasks_spawned;
}

static void rt_scope_drop(ctx_t* ctx, rt_scope_t* s) {
  if (s->key != 0 && ctx->sched_select_evts_len != 0) {
    for (uint32_t i = 0; i < ctx->sched_select_evts_len; i++) {
      rt_select_evt_t* e = &ctx->sched_select_evts[i];
      if (!e->alive) continue;
      if (e->scope_key != s->key) continue;
      rt_bytes_drop(ctx, &e->payload);
      e->payload = rt_bytes_empty(ctx);
      e->taken = 1;
      e->alive = 0;
    }
  }
  if (s->child_cap && s->child_task_ids) {
    rt_free(
      ctx,
      s->child_task_ids,
      s->child_cap * (uint32_t)sizeof(uint32_t),
      (uint32_t)_Alignof(uint32_t)
    );
  }
  if (s->child_cap && s->child_task_kinds) {
    rt_free(
      ctx,
      s->child_task_kinds,
      s->child_cap * (uint32_t)sizeof(uint32_t),
      (uint32_t)_Alignof(uint32_t)
    );
  }
  if (s->reg_cap && s->reg_task_ids) {
    rt_free(
      ctx,
      s->reg_task_ids,
      s->reg_cap * (uint32_t)sizeof(uint32_t),
      (uint32_t)_Alignof(uint32_t)
    );
  }
  if (s->slot_cap && s->slots) {
    rt_free(
      ctx,
      s->slots,
      s->slot_cap * (uint32_t)sizeof(rt_scope_slot_t),
      (uint32_t)_Alignof(rt_scope_slot_t)
    );
  }
  memset(s, 0, sizeof(*s));
}

static void rt_scope_register_task(rt_scope_t* s, uint32_t task_id) {
  if (s->reg_len >= s->reg_cap) rt_trap("X07T_SCOPE_BUDGET_CHILDREN_EXCEEDED");
  s->reg_task_ids[s->reg_len] = task_id;
  s->reg_len += 1;
}

static void rt_scope_unregister_task(rt_scope_t* s, uint32_t task_id) {
  for (uint32_t i = 0; i < s->reg_len; i++) {
    if (s->reg_task_ids[i] != task_id) continue;
    for (uint32_t j = i + 1; j < s->reg_len; j++) {
      s->reg_task_ids[j - 1] = s->reg_task_ids[j];
    }
    s->reg_len -= 1;
    if (s->reg_len < s->reg_cap) s->reg_task_ids[s->reg_len] = 0;
    return;
  }
}

static uint32_t rt_scope_start_soon(ctx_t* ctx, rt_scope_t* s, uint32_t task_id, uint32_t kind) {
  (void)ctx;
  if (s->cancel_requested) rt_trap("X07T_SCOPE_START_AFTER_CANCEL");
  rt_scope_register_task(s, task_id);
  if (s->child_len >= s->child_cap) rt_trap("X07T_SCOPE_BUDGET_CHILDREN_EXCEEDED");
  s->child_task_ids[s->child_len] = task_id;
  s->child_task_kinds[s->child_len] = kind;
  s->child_len += 1;
  return UINT32_C(1);
}

static uint32_t rt_scope_async_let(ctx_t* ctx, rt_scope_t* s, uint32_t task_id, uint32_t kind) {
  (void)ctx;
  if (s->cancel_requested) rt_trap("X07T_SCOPE_START_AFTER_CANCEL");
  rt_scope_register_task(s, task_id);
  for (uint32_t i = 0; i < s->slot_len; i++) {
    rt_scope_slot_t* slot = &s->slots[i];
    if (slot->state != RT_SCOPE_SLOT_EMPTY && slot->state != RT_SCOPE_SLOT_CONSUMED) continue;
    slot->gen += 1;
    slot->task_id = task_id;
    slot->kind = kind;
    slot->state = RT_SCOPE_SLOT_PENDING;
    uint32_t handle = (slot->gen << 16) | i;
    return handle;
  }
  rt_trap("X07T_SCOPE_BUDGET_CHILDREN_EXCEEDED");
}

static uint32_t rt_scope_slot_is_finished(ctx_t* ctx, rt_scope_t* s, uint32_t slot_id) {
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  uint32_t slot_gen = slot_id >> 16;
  if (slot_idx >= s->slot_len) rt_trap("X07T_SCOPE_SLOT_OOB");
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  if (slot->gen != slot_gen) rt_trap("X07T_SCOPE_SLOT_INVALID");
  if (slot->state == RT_SCOPE_SLOT_PENDING || slot->state == RT_SCOPE_SLOT_TAKEN) {
    return rt_task_is_finished(ctx, slot->task_id);
  }
  if (slot->state == RT_SCOPE_SLOT_CONSUMED) return UINT32_C(1);
  rt_trap("X07T_SCOPE_SLOT_INVALID");
}

static uint32_t rt_scope_slot_task_for_await(
  rt_scope_t* s,
  uint32_t slot_id,
  uint32_t expected_kind
) {
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  uint32_t slot_gen = slot_id >> 16;
  if (slot_idx >= s->slot_len) rt_trap("X07T_SCOPE_SLOT_OOB");
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  if (slot->gen != slot_gen) rt_trap("X07T_SCOPE_SLOT_INVALID");
  if (slot->state == RT_SCOPE_SLOT_PENDING) {
    slot->state = RT_SCOPE_SLOT_TAKEN;
  } else if (slot->state != RT_SCOPE_SLOT_TAKEN) {
    rt_trap("X07T_SCOPE_SLOT_ALREADY_CONSUMED");
  }
  if (slot->kind != expected_kind) rt_trap("X07T_SCOPE_SLOT_KIND_MISMATCH");
  if (slot->task_id == 0) rt_trap("X07T_SCOPE_SLOT_INVALID");
  return slot->task_id;
}

static void rt_scope_check_slot_result_size(rt_scope_t* s, bytes_t out) {
  if (s->max_slot_result_bytes != 0 && out.len > s->max_slot_result_bytes) {
    rt_trap("X07T_SCOPE_SLOT_RESULT_TOO_LARGE");
  }
}

static uint32_t rt_scope_await_slot_bytes_poll(ctx_t* ctx, rt_scope_t* s, uint32_t slot_id, bytes_t* out) {
  uint32_t task_id = rt_scope_slot_task_for_await(s, slot_id, RT_TASK_OUT_KIND_BYTES);
  uint32_t done = rt_task_join_bytes_poll(ctx, task_id, out);
  if (!done) return UINT32_C(0);
  rt_scope_check_slot_result_size(s, *out);
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  slot->state = RT_SCOPE_SLOT_CONSUMED;
  slot->task_id = 0;
  rt_scope_unregister_task(s, task_id);
  return UINT32_C(1);
}

static bytes_t rt_scope_await_slot_bytes_block(ctx_t* ctx, rt_scope_t* s, uint32_t slot_id) {
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  uint32_t slot_gen = slot_id >> 16;
  if (slot_idx >= s->slot_len) rt_trap("X07T_SCOPE_SLOT_OOB");
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  if (slot->gen != slot_gen) rt_trap("X07T_SCOPE_SLOT_INVALID");
  if (slot->state != RT_SCOPE_SLOT_PENDING) rt_trap("X07T_SCOPE_SLOT_ALREADY_CONSUMED");
  if (slot->kind != RT_TASK_OUT_KIND_BYTES) rt_trap("X07T_SCOPE_SLOT_KIND_MISMATCH");
  slot->state = RT_SCOPE_SLOT_TAKEN;
  uint32_t task_id = slot->task_id;
  bytes_t out = rt_task_join_bytes_block(ctx, task_id);
  rt_scope_check_slot_result_size(s, out);
  slot->state = RT_SCOPE_SLOT_CONSUMED;
  slot->task_id = 0;
  rt_scope_unregister_task(s, task_id);
  return out;
}

static uint32_t rt_scope_await_slot_result_bytes_poll(
  ctx_t* ctx,
  rt_scope_t* s,
  uint32_t slot_id,
  result_bytes_t* out
) {
  uint32_t task_id = rt_scope_slot_task_for_await(s, slot_id, RT_TASK_OUT_KIND_RESULT_BYTES);
  uint32_t done = rt_task_join_result_bytes_poll(ctx, task_id, out);
  if (!done) return UINT32_C(0);
  if (out->tag) rt_scope_check_slot_result_size(s, out->payload.ok);
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  slot->state = RT_SCOPE_SLOT_CONSUMED;
  slot->task_id = 0;
  rt_scope_unregister_task(s, task_id);
  return UINT32_C(1);
}

static result_bytes_t rt_scope_await_slot_result_bytes_block(ctx_t* ctx, rt_scope_t* s, uint32_t slot_id) {
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  uint32_t slot_gen = slot_id >> 16;
  if (slot_idx >= s->slot_len) rt_trap("X07T_SCOPE_SLOT_OOB");
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  if (slot->gen != slot_gen) rt_trap("X07T_SCOPE_SLOT_INVALID");
  if (slot->state != RT_SCOPE_SLOT_PENDING) rt_trap("X07T_SCOPE_SLOT_ALREADY_CONSUMED");
  if (slot->kind != RT_TASK_OUT_KIND_RESULT_BYTES) rt_trap("X07T_SCOPE_SLOT_KIND_MISMATCH");
  slot->state = RT_SCOPE_SLOT_TAKEN;
  uint32_t task_id = slot->task_id;
  result_bytes_t out = rt_task_join_result_bytes_block(ctx, task_id);
  if (out.tag) rt_scope_check_slot_result_size(s, out.payload.ok);
  slot->state = RT_SCOPE_SLOT_CONSUMED;
  slot->task_id = 0;
  rt_scope_unregister_task(s, task_id);
  return out;
}

static result_bytes_t rt_scope_try_await_slot_bytes(ctx_t* ctx, rt_scope_t* s, uint32_t slot_id) {
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  uint32_t slot_gen = slot_id >> 16;
  if (slot_idx >= s->slot_len) rt_trap("X07T_SCOPE_SLOT_OOB");
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  if (slot->gen != slot_gen) rt_trap("X07T_SCOPE_SLOT_INVALID");
  if (slot->state != RT_SCOPE_SLOT_PENDING) rt_trap("X07T_SCOPE_SLOT_ALREADY_CONSUMED");
  if (slot->kind != RT_TASK_OUT_KIND_BYTES) rt_trap("X07T_SCOPE_SLOT_KIND_MISMATCH");
  uint32_t task_id = slot->task_id;
  result_bytes_t r = rt_task_try_join_bytes(ctx, task_id);
  if (r.tag) {
    rt_scope_check_slot_result_size(s, r.payload.ok);
    slot->state = RT_SCOPE_SLOT_CONSUMED;
    slot->task_id = 0;
    rt_scope_unregister_task(s, task_id);
    return r;
  }
  if (r.payload.err == UINT32_C(2)) {
    slot->state = RT_SCOPE_SLOT_CONSUMED;
    slot->task_id = 0;
    rt_scope_unregister_task(s, task_id);
  }
  return r;
}

static result_result_bytes_t rt_scope_try_await_slot_result_bytes(ctx_t* ctx, rt_scope_t* s, uint32_t slot_id) {
  uint32_t slot_idx = slot_id & UINT32_C(0xffff);
  uint32_t slot_gen = slot_id >> 16;
  if (slot_idx >= s->slot_len) rt_trap("X07T_SCOPE_SLOT_OOB");
  rt_scope_slot_t* slot = &s->slots[slot_idx];
  if (slot->gen != slot_gen) rt_trap("X07T_SCOPE_SLOT_INVALID");
  if (slot->state != RT_SCOPE_SLOT_PENDING) rt_trap("X07T_SCOPE_SLOT_ALREADY_CONSUMED");
  if (slot->kind != RT_TASK_OUT_KIND_RESULT_BYTES) rt_trap("X07T_SCOPE_SLOT_KIND_MISMATCH");
  uint32_t task_id = slot->task_id;
  result_result_bytes_t r = rt_task_try_join_result_bytes(ctx, task_id);
  if (r.tag) {
    if (r.payload.ok.tag) rt_scope_check_slot_result_size(s, r.payload.ok.payload.ok);
    slot->state = RT_SCOPE_SLOT_CONSUMED;
    slot->task_id = 0;
    rt_scope_unregister_task(s, task_id);
    return r;
  }
  if (r.payload.err == UINT32_C(2)) {
    slot->state = RT_SCOPE_SLOT_CONSUMED;
    slot->task_id = 0;
    rt_scope_unregister_task(s, task_id);
  }
  return r;
}

static uint32_t rt_scope_wait_all_count(rt_scope_t* s) {
  uint32_t slots_pending = 0;
  for (uint32_t i = 0; i < s->slot_len; i++) {
    if (s->slots[i].state == RT_SCOPE_SLOT_PENDING) slots_pending += 1;
  }
  return s->child_len + slots_pending;
}

static void rt_scope_reset_active(rt_scope_t* s) {
  s->child_len = 0;
  s->reg_len = 0;
  s->cancel_requested = 0;
}

static uint32_t rt_scope_cancel_all(ctx_t* ctx, rt_scope_t* s) {
  uint32_t canceled = 0;
  for (uint32_t i = s->reg_len; i > 0; i--) {
    canceled += rt_task_cancel(ctx, s->reg_task_ids[i - 1]);
  }
  s->cancel_requested = 1;
  return canceled;
}

static uint32_t rt_scope_join_drop_remaining_poll(ctx_t* ctx, rt_scope_t* s) {
  if (s->join_phase == 0) {
    s->join_phase = 1;
    s->join_slot_i = 0;
    s->join_child_i = 0;
  }

  if (s->join_phase == 1) {
    while (s->join_slot_i < s->slot_len) {
      uint32_t slot_id = s->join_slot_i;
      rt_scope_slot_t* slot = &s->slots[slot_id];
      if (slot->state != RT_SCOPE_SLOT_PENDING) {
        s->join_slot_i += 1;
        continue;
      }

      if (slot->kind == RT_TASK_OUT_KIND_BYTES) {
        bytes_t out = rt_bytes_empty(ctx);
        uint32_t task_id = slot->task_id;
        uint32_t done = rt_task_join_bytes_poll(ctx, task_id, &out);
        if (!done) return UINT32_C(0);
        rt_scope_check_slot_result_size(s, out);
        rt_bytes_drop(ctx, &out);
        rt_scope_unregister_task(s, task_id);
      } else if (slot->kind == RT_TASK_OUT_KIND_RESULT_BYTES) {
        result_bytes_t out = (result_bytes_t){0};
        uint32_t task_id = slot->task_id;
        uint32_t done = rt_task_join_result_bytes_poll(ctx, task_id, &out);
        if (!done) return UINT32_C(0);
        if (out.tag) {
          rt_scope_check_slot_result_size(s, out.payload.ok);
          rt_bytes_drop(ctx, &out.payload.ok);
        }
        rt_scope_unregister_task(s, task_id);
      } else {
        rt_trap("scope slot kind invalid");
      }

      slot->state = RT_SCOPE_SLOT_CONSUMED;
      slot->task_id = 0;
      s->join_slot_i += 1;
    }
    s->join_phase = 2;
  }

  if (s->join_phase == 2) {
    while (s->join_child_i < s->child_len) {
      uint32_t task_id = s->child_task_ids[s->join_child_i];
      uint32_t kind = s->child_task_kinds[s->join_child_i];
      if (kind == RT_TASK_OUT_KIND_BYTES) {
        uint32_t done = rt_task_join_bytes_poll(ctx, task_id, NULL);
        if (!done) return UINT32_C(0);
      } else if (kind == RT_TASK_OUT_KIND_RESULT_BYTES) {
        uint32_t done = rt_task_join_result_bytes_poll(ctx, task_id, NULL);
        if (!done) return UINT32_C(0);
      } else {
        rt_trap("scope child kind invalid");
      }
      s->join_child_i += 1;
    }
    s->join_phase = 0;
    s->join_slot_i = 0;
    s->join_child_i = 0;
    return UINT32_C(1);
  }

  rt_trap("scope join invalid state");
}

static void rt_scope_join_drop_remaining_block(ctx_t* ctx, rt_scope_t* s) {
  for (uint32_t slot_id = 0; slot_id < s->slot_len; slot_id++) {
    rt_scope_slot_t* slot = &s->slots[slot_id];
    if (slot->state != RT_SCOPE_SLOT_PENDING) continue;
    if (slot->kind == RT_TASK_OUT_KIND_BYTES) {
      uint32_t task_id = slot->task_id;
      bytes_t out = rt_task_join_bytes_block(ctx, task_id);
      rt_scope_check_slot_result_size(s, out);
      rt_bytes_drop(ctx, &out);
      rt_scope_unregister_task(s, task_id);
    } else if (slot->kind == RT_TASK_OUT_KIND_RESULT_BYTES) {
      uint32_t task_id = slot->task_id;
      result_bytes_t out = rt_task_join_result_bytes_block(ctx, task_id);
      if (out.tag) {
        rt_scope_check_slot_result_size(s, out.payload.ok);
        rt_bytes_drop(ctx, &out.payload.ok);
      }
      rt_scope_unregister_task(s, task_id);
    } else {
      rt_trap("scope slot kind invalid");
    }
    slot->state = RT_SCOPE_SLOT_CONSUMED;
    slot->task_id = 0;
  }

  for (uint32_t i = 0; i < s->child_len; i++) {
    uint32_t task_id = s->child_task_ids[i];
    uint32_t kind = s->child_task_kinds[i];
    if (kind == RT_TASK_OUT_KIND_BYTES) {
      bytes_t out = rt_task_join_bytes_block(ctx, task_id);
      rt_bytes_drop(ctx, &out);
    } else if (kind == RT_TASK_OUT_KIND_RESULT_BYTES) {
      result_bytes_t out = rt_task_join_result_bytes_block(ctx, task_id);
      if (out.tag) {
        rt_bytes_drop(ctx, &out.payload.ok);
      }
    } else {
      rt_trap("scope child kind invalid");
    }
  }
}

static void rt_scope_budget_check_exit(ctx_t* ctx, rt_scope_t* s) {
  uint64_t ticks = ctx->sched_now_ticks - s->snap_ticks;
  if (s->max_ticks != 0 && ticks > s->max_ticks) rt_trap("X07T_SCOPE_BUDGET_TICKS_EXCEEDED");

  uint64_t blocked = ctx->sched_stats.blocked_waits - s->snap_blocked_waits;
  if (s->max_blocked_waits != 0 && blocked > s->max_blocked_waits) {
    rt_trap("X07T_SCOPE_BUDGET_BLOCKED_WAITS_EXCEEDED");
  }

  uint64_t joins = ctx->sched_stats.join_calls - s->snap_join_polls;
  if (s->max_join_polls != 0 && joins > s->max_join_polls) rt_trap("X07T_SCOPE_BUDGET_JOIN_POLLS_EXCEEDED");
}

static uint32_t rt_scope_exit_poll(ctx_t* ctx, rt_scope_t* s) {
  uint32_t done = rt_scope_join_drop_remaining_poll(ctx, s);
  if (!done) return UINT32_C(0);
  rt_scope_budget_check_exit(ctx, s);
  rt_scope_drop(ctx, s);
  return UINT32_C(1);
}

static void rt_scope_exit_block(ctx_t* ctx, rt_scope_t* s) {
  rt_scope_join_drop_remaining_block(ctx, s);
  rt_scope_budget_check_exit(ctx, s);
  rt_scope_drop(ctx, s);
}

static void rt_scope_dispose_on_drop(ctx_t* ctx, rt_scope_t* s) {
  // Best-effort scope cleanup used by async task drop (task cancel / task complete):
  // - cancel unfinished children in deterministic order (reverse registration)
  // - drop any finished but unconsumed outputs (without blocking)
  // - free scope buffers
  if (s->reg_len != 0) {
    for (uint32_t i = s->reg_len; i > 0; i--) {
      uint32_t task_id = s->reg_task_ids[i - 1];
      if (task_id == 0) continue;
      rt_task_t* t = rt_task_ptr(ctx, task_id);
      if (!t->done) {
        (void)rt_task_cancel(ctx, task_id);
        continue;
      }
      if (t->out_taken || t->canceled) continue;
      if (t->out.kind == RT_TASK_OUT_KIND_BYTES) {
        (void)rt_task_join_bytes_poll(ctx, task_id, NULL);
      } else if (t->out.kind == RT_TASK_OUT_KIND_RESULT_BYTES) {
        (void)rt_task_join_result_bytes_poll(ctx, task_id, NULL);
      } else {
        rt_trap("scope.drop invalid out kind");
      }
    }
  }
  rt_scope_drop(ctx, s);
}

static uint32_t rt_select_evt_new(
  ctx_t* ctx,
  uint64_t scope_key,
  uint32_t tag,
  uint32_t case_index,
  uint32_t src_id,
  bytes_t payload
) {
  // Reuse a free slot if possible.
  for (uint32_t i = 0; i < ctx->sched_select_evts_len; i++) {
    rt_select_evt_t* e = &ctx->sched_select_evts[i];
    if (e->alive) continue;
    memset(e, 0, sizeof(*e));
    e->alive = 1;
    e->tag = tag;
    e->case_index = case_index;
    e->src_id = src_id;
    e->scope_key = scope_key;
    e->payload = payload;
    return i + 1;
  }

  if (ctx->sched_select_evts_len == UINT32_MAX) rt_trap("select.evt.new overflow");
  uint32_t need = ctx->sched_select_evts_len + 1;
  rt_sched_select_evts_ensure_cap(ctx, need);
  uint32_t evt_id = need;
  rt_select_evt_t* e = &ctx->sched_select_evts[evt_id - 1];
  memset(e, 0, sizeof(*e));
  e->alive = 1;
  e->tag = tag;
  e->case_index = case_index;
  e->src_id = src_id;
  e->scope_key = scope_key;
  e->payload = payload;
  ctx->sched_select_evts_len = need;
  return evt_id;
}

static uint32_t rt_select_evt_tag(ctx_t* ctx, uint32_t evt_id) {
  rt_select_evt_t* e = rt_select_evt_ptr(ctx, evt_id);
  return e->tag;
}

static uint32_t rt_select_evt_case_index(ctx_t* ctx, uint32_t evt_id) {
  rt_select_evt_t* e = rt_select_evt_ptr(ctx, evt_id);
  return e->case_index;
}

static uint32_t rt_select_evt_src_id(ctx_t* ctx, uint32_t evt_id) {
  rt_select_evt_t* e = rt_select_evt_ptr(ctx, evt_id);
  return e->src_id;
}

static bytes_t rt_select_evt_take_bytes(ctx_t* ctx, uint32_t evt_id) {
  rt_select_evt_t* e = rt_select_evt_ptr(ctx, evt_id);
  if (e->taken) rt_trap("X07T_SELECT_EVT_ALREADY_TAKEN");
  e->taken = 1;
  bytes_t out = e->payload;
  e->payload = rt_bytes_empty(ctx);
  e->alive = 0;
  return out;
}

static void rt_select_evt_drop(ctx_t* ctx, uint32_t evt_id) {
  rt_select_evt_t* e = rt_select_evt_ptr(ctx, evt_id);
  if (e->taken) rt_trap("X07T_SELECT_EVT_ALREADY_TAKEN");
  rt_bytes_drop(ctx, &e->payload);
  e->payload = rt_bytes_empty(ctx);
  e->taken = 1;
  e->alive = 0;
}

static uint32_t rt_chan_bytes_new(ctx_t* ctx, uint32_t cap) {
  if (cap == 0) rt_trap("chan cap=0");
  rt_sched_chans_ensure_cap(ctx, ctx->sched_chans_len + 1);
  uint32_t chan_id = ctx->sched_chans_len + 1;
  rt_chan_bytes_t* c = &ctx->sched_chans[chan_id - 1];
  memset(c, 0, sizeof(*c));
  c->alive = 1;
  c->closed = 0;
  c->cap = cap;
  c->buf = (bytes_t*)rt_alloc(
    ctx,
    cap * (uint32_t)sizeof(bytes_t),
    (uint32_t)_Alignof(bytes_t)
  );
  c->head = 0;
  c->tail = 0;
  c->len = 0;
  ctx->sched_chans_len += 1;
  return chan_id;
}

static uint32_t rt_chan_bytes_send_poll(ctx_t* ctx, uint32_t chan_id, bytes_t msg) {
  ctx->sched_stats.chan_send_calls += 1;
  uint32_t cur = ctx->sched_current_task;
  if (cur == 0) rt_trap("chan.send.poll from main");

  rt_chan_bytes_t* c = rt_chan_bytes_ptr(ctx, chan_id);
  if (c->closed) rt_trap("chan.send closed");

  if (c->len < c->cap) {
    c->buf[c->tail] = msg;
    c->tail = (c->tail + 1) % c->cap;
    c->len += 1;

    uint32_t w = rt_wait_list_pop(ctx, &c->recv_wait_head, &c->recv_wait_tail);
    if (w != 0) rt_sched_wake(ctx, w, RT_WAIT_CHAN_RECV, chan_id);
    return UINT32_C(1);
  }

  rt_task_t* me = rt_task_ptr(ctx, cur);
  if (me->wait_kind == RT_WAIT_CHAN_SEND && me->wait_id == chan_id) {
    return UINT32_C(0);
  }
  if (me->wait_kind != RT_WAIT_NONE) rt_trap("chan.send while already waiting");
  me->wait_kind = RT_WAIT_CHAN_SEND;
  me->wait_id = chan_id;
  ctx->sched_stats.blocked_waits += 1;
  rt_sched_trace_event(ctx, RT_TRACE_BLOCK, (uint64_t)cur, ((uint64_t)RT_WAIT_CHAN_SEND << 32) | chan_id);
  rt_wait_list_push(ctx, &c->send_wait_head, &c->send_wait_tail, cur);
  return UINT32_C(0);
}

static uint32_t rt_chan_bytes_recv_poll(ctx_t* ctx, uint32_t chan_id, bytes_t* out) {
  ctx->sched_stats.chan_recv_calls += 1;
  uint32_t cur = ctx->sched_current_task;
  if (cur == 0) rt_trap("chan.recv.poll from main");

  rt_chan_bytes_t* c = rt_chan_bytes_ptr(ctx, chan_id);
  if (c->len != 0) {
    bytes_t msg = c->buf[c->head];
    c->head = (c->head + 1) % c->cap;
    c->len -= 1;
    if (out) *out = msg;

    uint32_t w = rt_wait_list_pop(ctx, &c->send_wait_head, &c->send_wait_tail);
    if (w != 0) rt_sched_wake(ctx, w, RT_WAIT_CHAN_SEND, chan_id);
    return UINT32_C(1);
  }

  if (c->closed) {
    if (out) *out = rt_bytes_empty(ctx);
    return UINT32_C(1);
  }

  rt_task_t* me = rt_task_ptr(ctx, cur);
  if (me->wait_kind == RT_WAIT_CHAN_RECV && me->wait_id == chan_id) {
    return UINT32_C(0);
  }
  if (me->wait_kind != RT_WAIT_NONE) rt_trap("chan.recv while already waiting");
  me->wait_kind = RT_WAIT_CHAN_RECV;
  me->wait_id = chan_id;
  ctx->sched_stats.blocked_waits += 1;
  rt_sched_trace_event(ctx, RT_TRACE_BLOCK, (uint64_t)cur, ((uint64_t)RT_WAIT_CHAN_RECV << 32) | chan_id);
  rt_wait_list_push(ctx, &c->recv_wait_head, &c->recv_wait_tail, cur);
  return UINT32_C(0);
}

static uint32_t rt_chan_bytes_send_block(ctx_t* ctx, uint32_t chan_id, bytes_t msg) {
  rt_chan_bytes_t* c = rt_chan_bytes_ptr(ctx, chan_id);
  if (c->closed) rt_trap("chan.send closed");
  while (c->len >= c->cap) {
    if (!rt_sched_step(ctx)) rt_sched_deadlock();
    c = rt_chan_bytes_ptr(ctx, chan_id);
  }
  c->buf[c->tail] = msg;
  c->tail = (c->tail + 1) % c->cap;
  c->len += 1;
  uint32_t w = rt_wait_list_pop(ctx, &c->recv_wait_head, &c->recv_wait_tail);
  if (w != 0) rt_sched_wake(ctx, w, RT_WAIT_CHAN_RECV, chan_id);
  return UINT32_C(1);
}

static uint32_t rt_chan_bytes_try_send_view(ctx_t* ctx, uint32_t chan_id, bytes_view_t msg) {
  ctx->sched_stats.chan_send_calls += 1;
  rt_chan_bytes_t* c = rt_chan_bytes_ptr(ctx, chan_id);
  if (c->closed) return UINT32_C(2);
  if (c->len >= c->cap) return UINT32_C(0);

  c->buf[c->tail] = rt_view_to_bytes(ctx, msg);
  c->tail = (c->tail + 1) % c->cap;
  c->len += 1;
  uint32_t w = rt_wait_list_pop(ctx, &c->recv_wait_head, &c->recv_wait_tail);
  if (w != 0) rt_sched_wake(ctx, w, RT_WAIT_CHAN_RECV, chan_id);
  return UINT32_C(1);
}

static result_bytes_t rt_chan_bytes_try_recv(ctx_t* ctx, uint32_t chan_id) {
  ctx->sched_stats.chan_recv_calls += 1;
  rt_chan_bytes_t* c = rt_chan_bytes_ptr(ctx, chan_id);

  if (c->len != 0) {
    bytes_t msg = c->buf[c->head];
    c->head = (c->head + 1) % c->cap;
    c->len -= 1;
    uint32_t w = rt_wait_list_pop(ctx, &c->send_wait_head, &c->send_wait_tail);
    if (w != 0) rt_sched_wake(ctx, w, RT_WAIT_CHAN_SEND, chan_id);
    return (result_bytes_t){ .tag = UINT32_C(1), .payload.ok = msg };
  }

  if (c->closed) {
    return (result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(2) };
  }
  return (result_bytes_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(1) };
}

static bytes_t rt_chan_bytes_recv_block(ctx_t* ctx, uint32_t chan_id) {
  rt_chan_bytes_t* c = rt_chan_bytes_ptr(ctx, chan_id);
  while (c->len == 0 && !c->closed) {
    if (!rt_sched_step(ctx)) rt_sched_deadlock();
    c = rt_chan_bytes_ptr(ctx, chan_id);
  }
  if (c->len == 0) return rt_bytes_empty(ctx);
  bytes_t msg = c->buf[c->head];
  c->head = (c->head + 1) % c->cap;
  c->len -= 1;
  uint32_t w = rt_wait_list_pop(ctx, &c->send_wait_head, &c->send_wait_tail);
  if (w != 0) rt_sched_wake(ctx, w, RT_WAIT_CHAN_SEND, chan_id);
  return msg;
}

static uint32_t rt_chan_bytes_close(ctx_t* ctx, uint32_t chan_id) {
  rt_chan_bytes_t* c = rt_chan_bytes_ptr(ctx, chan_id);
  if (c->closed) return UINT32_C(0);
  c->closed = 1;
  for (;;) {
    uint32_t w = rt_wait_list_pop(ctx, &c->recv_wait_head, &c->recv_wait_tail);
    if (w == 0) break;
    rt_sched_wake(ctx, w, RT_WAIT_CHAN_RECV, chan_id);
  }
  for (;;) {
    uint32_t w = rt_wait_list_pop(ctx, &c->send_wait_head, &c->send_wait_tail);
    if (w == 0) break;
    rt_sched_wake(ctx, w, RT_WAIT_CHAN_SEND, chan_id);
  }
  return UINT32_C(1);
}

#if X07_ENABLE_STREAMING_FILE_IO
static uint32_t rt_io_reader_new_file(ctx_t* ctx, FILE* f, uint32_t pending_ticks) {
  if (!f) rt_trap("io.reader null file");
  rt_io_readers_ensure_cap(ctx, ctx->io_readers_len + 1);
  uint32_t reader_id = ctx->io_readers_len + 1;
  rt_io_reader_t* r = &ctx->io_readers[reader_id - 1];
  memset(r, 0, sizeof(*r));
  r->alive = 1;
  r->kind = RT_IO_READER_KIND_FILE;
  r->eof = 0;
  r->pending_ticks = pending_ticks;
  r->f = f;
  r->bytes = rt_bytes_empty(ctx);
  r->pos = 0;
  ctx->io_readers_len += 1;
  return reader_id;
}
#else
static uint32_t rt_io_reader_new_file(ctx_t* ctx, void* f, uint32_t pending_ticks) {
  (void)ctx;
  (void)f;
  (void)pending_ticks;
  rt_trap("io.reader file disabled");
}
#endif

static uint32_t rt_io_reader_new_bytes(ctx_t* ctx, bytes_t b, uint32_t pending_ticks) {
  rt_io_readers_ensure_cap(ctx, ctx->io_readers_len + 1);
  uint32_t reader_id = ctx->io_readers_len + 1;
  rt_io_reader_t* r = &ctx->io_readers[reader_id - 1];
  memset(r, 0, sizeof(*r));
  r->alive = 1;
  r->kind = RT_IO_READER_KIND_BYTES;
  r->eof = 0;
  r->pending_ticks = pending_ticks;
#if X07_ENABLE_STREAMING_FILE_IO
  r->f = NULL;
#endif
  r->bytes = b;
  r->pos = 0;
  ctx->io_readers_len += 1;
  return reader_id;
}

static uint32_t rt_io_read_poll(ctx_t* ctx, uint32_t reader_id, uint32_t max, bytes_t* out) {
  rt_io_reader_t* r = rt_io_reader_ptr(ctx, reader_id);
  if (max == 0 || r->eof) {
    if (out) *out = rt_bytes_empty(ctx);
    return UINT32_C(1);
  }

  if (r->pending_ticks != 0) {
    uint32_t ticks = r->pending_ticks;
    r->pending_ticks = 0;
    rt_task_sleep(ctx, ticks);
    return UINT32_C(0);
  }

  if (r->kind == RT_IO_READER_KIND_BYTES) {
    bytes_t b = r->bytes;
    uint32_t pos = r->pos;
    if (pos > b.len) pos = b.len;
    uint32_t rem = b.len - pos;
    uint32_t n = (rem < max) ? rem : max;
    if (n) {
      r->pos = pos + n;
      bytes_t slice = (bytes_t){b.ptr + pos, n};
      if (out) *out = rt_bytes_clone(ctx, slice);
      return UINT32_C(1);
    }
    r->eof = 1;
    rt_bytes_drop(ctx, &r->bytes);
    r->bytes = rt_bytes_empty(ctx);
    if (out) *out = rt_bytes_empty(ctx);
    return UINT32_C(1);
  }

#if X07_ENABLE_STREAMING_FILE_IO
  if (r->kind != RT_IO_READER_KIND_FILE) rt_trap("io.read bad reader kind");
  if (!r->f) {
    r->eof = 1;
    if (out) *out = rt_bytes_empty(ctx);
    return UINT32_C(1);
  }

  int c = fgetc(r->f);
  if (c == EOF) {
    fclose(r->f);
    r->f = NULL;
    r->eof = 1;
    if (out) *out = rt_bytes_empty(ctx);
    return UINT32_C(1);
  }

  bytes_t chunk = rt_bytes_alloc(ctx, max);
  chunk.ptr[0] = (uint8_t)c;
  uint32_t got = 1;
  if (max > 1) {
    size_t n = fread(chunk.ptr + 1, 1, (size_t)(max - 1), r->f);
    if (n > (size_t)(UINT32_MAX - 1)) rt_trap("io.read too large");
    got += (uint32_t)n;
  }
  chunk.len = got;
  if (out) *out = chunk;
  return UINT32_C(1);
#else
  rt_trap("io.read bad reader kind");
#endif
}

static bytes_t rt_io_read_block(ctx_t* ctx, uint32_t reader_id, uint32_t max) {
  rt_io_reader_t* r = rt_io_reader_ptr(ctx, reader_id);
  if (max == 0 || r->eof) return rt_bytes_empty(ctx);

  if (r->pending_ticks != 0) {
    uint32_t ticks = r->pending_ticks;
    r->pending_ticks = 0;
    rt_task_sleep_block(ctx, ticks);
  }

  if (r->kind == RT_IO_READER_KIND_BYTES) {
    bytes_t b = r->bytes;
    uint32_t pos = r->pos;
    if (pos > b.len) pos = b.len;
    uint32_t rem = b.len - pos;
    uint32_t n = (rem < max) ? rem : max;
    if (n) {
      r->pos = pos + n;
      bytes_t slice = (bytes_t){b.ptr + pos, n};
      return rt_bytes_clone(ctx, slice);
    }
    r->eof = 1;
    rt_bytes_drop(ctx, &r->bytes);
    r->bytes = rt_bytes_empty(ctx);
    return rt_bytes_empty(ctx);
  }

#if X07_ENABLE_STREAMING_FILE_IO
  if (r->kind != RT_IO_READER_KIND_FILE) rt_trap("io.read bad reader kind");
  if (!r->f) {
    r->eof = 1;
    return rt_bytes_empty(ctx);
  }

  int c = fgetc(r->f);
  if (c == EOF) {
    fclose(r->f);
    r->f = NULL;
    r->eof = 1;
    return rt_bytes_empty(ctx);
  }

  bytes_t chunk = rt_bytes_alloc(ctx, max);
  chunk.ptr[0] = (uint8_t)c;
  uint32_t got = 1;
  if (max > 1) {
    size_t n = fread(chunk.ptr + 1, 1, (size_t)(max - 1), r->f);
    if (n > (size_t)(UINT32_MAX - 1)) rt_trap("io.read too large");
    got += (uint32_t)n;
  }
  chunk.len = got;
  return chunk;
#else
  rt_trap("io.read bad reader kind");
#endif
}

static bytes_t rt_iface_io_read_block(ctx_t* ctx, iface_t reader, uint32_t max) {
  if (max == 0) return rt_bytes_empty(ctx);
  if (reader.vtable == RT_IFACE_VTABLE_IO_READER) {
    return rt_io_read_block(ctx, reader.data, max);
  }
  if (reader.vtable < RT_IFACE_VTABLE_EXT_IO_READER_MIN || reader.vtable > RT_IFACE_VTABLE_EXT_IO_READER_MAX) {
    rt_trap("io.read bad iface vtable");
  }

  bytes_t chunk = rt_bytes_alloc(ctx, max);
  uint32_t got = rt_ext_io_reader_read_into(reader.vtable, reader.data, chunk.ptr, max);
  if (got == 0) {
    rt_bytes_drop(ctx, &chunk);
    return rt_bytes_empty(ctx);
  }
  chunk.len = got;
  return chunk;
}

static uint32_t rt_bufread_new(ctx_t* ctx, iface_t reader, uint32_t cap) {
  if (cap == 0) rt_trap("bufread cap=0");
  if (reader.vtable == RT_IFACE_VTABLE_IO_READER) {
    (void)rt_io_reader_ptr(ctx, reader.data);
  } else if (reader.vtable >= RT_IFACE_VTABLE_EXT_IO_READER_MIN && reader.vtable <= RT_IFACE_VTABLE_EXT_IO_READER_MAX) {
    // External IO reader: validated lazily on first read.
  } else {
    rt_trap("bufread.new bad iface vtable");
  }

  rt_bufreads_ensure_cap(ctx, ctx->bufreads_len + 1);
  uint32_t br_id = ctx->bufreads_len + 1;
  rt_bufread_t* br = &ctx->bufreads[br_id - 1];
  memset(br, 0, sizeof(*br));
  br->alive = 1;
  br->reader = reader;
  br->eof = 0;
  br->direct_bytes = 0;
  br->buf = rt_bytes_alloc(ctx, cap);
  br->start = 0;
  br->end = 0;
  ctx->bufreads_len += 1;
  return br_id;
}

static uint32_t rt_bufread_fill_poll(ctx_t* ctx, uint32_t br_id, bytes_view_t* out) {
  rt_bufread_t* br = rt_bufread_ptr(ctx, br_id);
  if (br->start > br->end) rt_trap("bufread corrupt");
  uint32_t avail = br->end - br->start;
  if (avail != 0) {
    if (out) {
      if (br->direct_bytes) {
        iface_t reader = br->reader;
        if (reader.vtable != RT_IFACE_VTABLE_IO_READER) rt_trap("bufread corrupt");
        rt_io_reader_t* r = rt_io_reader_ptr(ctx, reader.data);
        if (r->kind != RT_IO_READER_KIND_BYTES) rt_trap("bufread corrupt");
        *out = rt_bytes_subview(ctx, r->bytes, br->start, avail);
      } else {
        *out = rt_bytes_subview(ctx, br->buf, br->start, avail);
      }
    }
    return UINT32_C(1);
  }
  if (br->eof) {
    if (out) *out = rt_view_empty(ctx);
    return UINT32_C(1);
  }

  iface_t reader = br->reader;
  if (reader.vtable != RT_IFACE_VTABLE_IO_READER) {
    uint32_t cap = br->buf.len;
    uint32_t got = rt_ext_io_reader_read_into(reader.vtable, reader.data, br->buf.ptr, cap);
    br->direct_bytes = 0;
    br->start = 0;
    br->end = got;
    if (got == 0) {
      br->eof = 1;
      if (out) *out = rt_view_empty(ctx);
      return UINT32_C(1);
    }
    if (out) *out = rt_bytes_subview(ctx, br->buf, 0, got);
    return UINT32_C(1);
  }

  rt_io_reader_t* r = rt_io_reader_ptr(ctx, reader.data);
  if (r->eof) {
    br->eof = 1;
    if (out) *out = rt_view_empty(ctx);
    return UINT32_C(1);
  }
  if (r->pending_ticks != 0) {
    uint32_t ticks = r->pending_ticks;
    r->pending_ticks = 0;
    rt_task_sleep(ctx, ticks);
    return UINT32_C(0);
  }

  uint32_t cap = br->buf.len;
  uint32_t got = 0;
  if (r->kind == RT_IO_READER_KIND_BYTES) {
    bytes_t b = r->bytes;
    uint32_t pos = r->pos;
    if (pos > b.len) pos = b.len;
    uint32_t rem = b.len - pos;
    got = (rem < cap) ? rem : cap;
    if (got) {
      br->direct_bytes = 1;
      br->start = pos;
      br->end = pos + got;
      r->pos = pos + got;
      if (out) *out = rt_bytes_subview(ctx, b, br->start, got);
      return UINT32_C(1);
    } else {
      r->eof = 1;
    }
  }
#if X07_ENABLE_STREAMING_FILE_IO
  else if (r->kind == RT_IO_READER_KIND_FILE) {
    if (!r->f) {
      r->eof = 1;
    } else {
      int c = fgetc(r->f);
      if (c == EOF) {
        fclose(r->f);
        r->f = NULL;
        r->eof = 1;
      } else {
        br->buf.ptr[0] = (uint8_t)c;
        got = 1;
        if (cap > 1) {
          size_t n = fread(br->buf.ptr + 1, 1, (size_t)(cap - 1), r->f);
          if (n > (size_t)(UINT32_MAX - 1)) rt_trap("bufread.fill too large");
          got += (uint32_t)n;
        }
      }
    }
  }
#endif
  else {
    rt_trap("bufread bad reader kind");
  }

  br->direct_bytes = 0;
  br->start = 0;
  br->end = got;
  if (got == 0) {
    br->eof = 1;
    if (out) *out = rt_view_empty(ctx);
    return UINT32_C(1);
  }

  if (out) *out = rt_bytes_subview(ctx, br->buf, 0, got);
  return UINT32_C(1);
}

static bytes_view_t rt_bufread_fill_block(ctx_t* ctx, uint32_t br_id) {
  rt_bufread_t* br = rt_bufread_ptr(ctx, br_id);
  for (;;) {
    if (br->start > br->end) rt_trap("bufread corrupt");
    uint32_t avail = br->end - br->start;
    if (avail != 0) {
      if (br->direct_bytes) {
        iface_t reader = br->reader;
        if (reader.vtable != RT_IFACE_VTABLE_IO_READER) rt_trap("bufread corrupt");
        rt_io_reader_t* r = rt_io_reader_ptr(ctx, reader.data);
        if (r->kind != RT_IO_READER_KIND_BYTES) rt_trap("bufread corrupt");
        return rt_bytes_subview(ctx, r->bytes, br->start, avail);
      }
      return rt_bytes_subview(ctx, br->buf, br->start, avail);
    }
    if (br->eof) return rt_view_empty(ctx);

    iface_t reader = br->reader;
    if (reader.vtable != RT_IFACE_VTABLE_IO_READER) {
      uint32_t cap = br->buf.len;
      uint32_t got = rt_ext_io_reader_read_into(reader.vtable, reader.data, br->buf.ptr, cap);
      br->direct_bytes = 0;
      br->start = 0;
      br->end = got;
      if (got == 0) {
        br->eof = 1;
        return rt_view_empty(ctx);
      }
      return rt_bytes_subview(ctx, br->buf, 0, got);
    }

    rt_io_reader_t* r = rt_io_reader_ptr(ctx, reader.data);
    if (r->eof) {
      br->eof = 1;
      return rt_view_empty(ctx);
    }
    if (r->pending_ticks != 0) {
      uint32_t ticks = r->pending_ticks;
      r->pending_ticks = 0;
      rt_task_sleep_block(ctx, ticks);
      continue;
    }

    uint32_t cap = br->buf.len;
    uint32_t got = 0;
    if (r->kind == RT_IO_READER_KIND_BYTES) {
      bytes_t b = r->bytes;
      uint32_t pos = r->pos;
      if (pos > b.len) pos = b.len;
      uint32_t rem = b.len - pos;
      got = (rem < cap) ? rem : cap;
      if (got) {
        br->direct_bytes = 1;
        br->start = pos;
        br->end = pos + got;
        r->pos = pos + got;
        return rt_bytes_subview(ctx, b, br->start, got);
      } else {
        r->eof = 1;
      }
    }
#if X07_ENABLE_STREAMING_FILE_IO
    else if (r->kind == RT_IO_READER_KIND_FILE) {
      if (!r->f) {
        r->eof = 1;
      } else {
        int c = fgetc(r->f);
        if (c == EOF) {
          fclose(r->f);
          r->f = NULL;
          r->eof = 1;
        } else {
          br->buf.ptr[0] = (uint8_t)c;
          got = 1;
          if (cap > 1) {
            size_t n = fread(br->buf.ptr + 1, 1, (size_t)(cap - 1), r->f);
            if (n > (size_t)(UINT32_MAX - 1)) rt_trap("bufread.fill too large");
            got += (uint32_t)n;
          }
        }
      }
    }
#endif
    else {
      rt_trap("bufread bad reader kind");
    }

    br->direct_bytes = 0;
    br->start = 0;
    br->end = got;
    if (got == 0) {
      br->eof = 1;
      return rt_view_empty(ctx);
    }

    return rt_bytes_subview(ctx, br->buf, 0, got);
  }
}

static uint32_t rt_bufread_consume(ctx_t* ctx, uint32_t br_id, uint32_t n) {
  rt_bufread_t* br = rt_bufread_ptr(ctx, br_id);
  if (br->start > br->end) rt_trap("bufread corrupt");
  uint32_t avail = br->end - br->start;
  if (n > avail) rt_trap("bufread.consume oob");
  br->start += n;
  if (br->start == br->end) {
    br->start = 0;
    br->end = 0;
    br->direct_bytes = 0;
  }
  return UINT32_C(0);
}

static uint32_t rt_fs_is_safe_rel_path(bytes_view_t path) {
  if (path.len == 0) return UINT32_C(0);
  if (path.ptr[0] == (uint8_t)'/') return UINT32_C(0);

  uint32_t seg_start = 0;
  for (uint32_t i = 0; i <= path.len; i++) {
    uint8_t b = (i == path.len) ? (uint8_t)'/' : path.ptr[i];
    if (i < path.len) {
      if (b == 0 || b == (uint8_t)'\\') return UINT32_C(0);
    }
    if (b == (uint8_t)'/') {
      uint32_t seg_len = i - seg_start;
      if (seg_len == 0) return UINT32_C(0);
      if (seg_len == 1 && path.ptr[seg_start] == (uint8_t)'.') return UINT32_C(0);
      if (seg_len == 2
          && path.ptr[seg_start] == (uint8_t)'.'
          && path.ptr[seg_start + 1] == (uint8_t)'.') return UINT32_C(0);
      if (seg_len >= 5 && memcmp(path.ptr + seg_start, ".x07_", 5) == 0) return UINT32_C(0);
      seg_start = i + 1;
    }
  }
  return UINT32_C(1);
}

static void rt_kv_init(ctx_t* ctx) {
  (void)ctx;
}

static void rt_hex_bytes(const uint8_t* bytes, uint32_t len, char* out) {
  static const char LUT[16] = "0123456789abcdef";
  for (uint32_t i = 0; i < len; i++) {
    uint8_t b = bytes[i];
    out[i * 2 + 0] = LUT[b >> 4];
    out[i * 2 + 1] = LUT[b & 0x0F];
  }
  out[len * 2] = 0;
}

static uint32_t rt_codec_read_u32_le(ctx_t* ctx, bytes_view_t buf, uint32_t offset) {
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_borrow_check(ctx, buf.bid, buf.off_bytes, buf.len);
#else
  (void)ctx;
#endif
  if (offset > buf.len || buf.len - offset < 4) rt_trap("codec.read_u32_le oob");
  return (uint32_t)buf.ptr[offset]
       | ((uint32_t)buf.ptr[offset + 1] << 8)
       | ((uint32_t)buf.ptr[offset + 2] << 16)
       | ((uint32_t)buf.ptr[offset + 3] << 24);
}

static bytes_t rt_codec_write_u32_le(ctx_t* ctx, uint32_t x) {
  bytes_t out = rt_bytes_alloc(ctx, 4);
  out.ptr[0] = (uint8_t)(x & UINT32_C(0xFF));
  out.ptr[1] = (uint8_t)((x >> 8) & UINT32_C(0xFF));
  out.ptr[2] = (uint8_t)((x >> 16) & UINT32_C(0xFF));
  out.ptr[3] = (uint8_t)((x >> 24) & UINT32_C(0xFF));
  return out;
}

static bytes_t rt_fmt_u32_to_dec(ctx_t* ctx, uint32_t x) {
  uint8_t scratch[16];
  uint32_t n = 0;
  if (x == 0) {
    bytes_t out = rt_bytes_alloc(ctx, 1);
    out.ptr[0] = (uint8_t)'0';
    return out;
  }
  while (x > 0) {
    uint32_t digit = x % 10;
    x /= 10;
    scratch[n++] = (uint8_t)('0' + digit);
  }
  bytes_t out = rt_bytes_alloc(ctx, n);
  for (uint32_t i = 0; i < n; i++) {
    out.ptr[i] = scratch[n - 1 - i];
  }
  return out;
}

static bytes_t rt_fmt_s32_to_dec(ctx_t* ctx, uint32_t x) {
  if ((x & UINT32_C(0x80000000)) == 0) {
    return rt_fmt_u32_to_dec(ctx, x);
  }
  uint32_t mag = (~x) + UINT32_C(1);
  bytes_t digits = rt_fmt_u32_to_dec(ctx, mag);
  bytes_t out = rt_bytes_alloc(ctx, digits.len + 1);
  out.ptr[0] = (uint8_t)'-';
  memcpy(out.ptr + 1, digits.ptr, digits.len);
  rt_mem_on_memcpy(ctx, digits.len);
  return out;
}

static uint32_t rt_parse_u32_dec_slice(ctx_t* ctx, uint8_t* ptr, uint32_t len) {
  if (len == 0) rt_trap("parse.u32_dec empty");
  uint32_t acc = 0;
  for (uint32_t i = 0; i < len; i++) {
    uint8_t b = ptr[i];
    if (b < (uint8_t)'0' || b > (uint8_t)'9') rt_trap("parse.u32_dec non-digit");
    uint32_t digit = (uint32_t)(b - (uint8_t)'0');
    if (acc > (UINT32_MAX - digit) / 10) rt_trap("parse.u32_dec overflow");
    acc = acc * 10 + digit;
  }
  return acc;
}

static uint32_t rt_parse_u32_dec(ctx_t* ctx, bytes_view_t buf) {
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_borrow_check(ctx, buf.bid, buf.off_bytes, buf.len);
#endif
  return rt_parse_u32_dec_slice(ctx, buf.ptr, buf.len);
}

static uint32_t rt_parse_u32_dec_at(ctx_t* ctx, bytes_view_t buf, uint32_t offset) {
  if (offset > buf.len) rt_trap("parse.u32_dec_at oob");
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_borrow_check(ctx, buf.bid, buf.off_bytes + offset, buf.len - offset);
#endif
  return rt_parse_u32_dec_slice(ctx, buf.ptr + offset, buf.len - offset);
}

static uint32_t rt_prng_lcg_next_u32(uint32_t state) {
  return state * UINT32_C(1103515245) + UINT32_C(12345);
}

typedef struct {
  uint8_t* data;
  uint32_t len;
  uint32_t cap;
#ifdef X07_DEBUG_BORROW
  uint64_t dbg_aid;
#endif
} vec_u8_t;

static vec_u8_t rt_vec_u8_new(ctx_t* ctx, uint32_t cap) {
  vec_u8_t v;
  v.len = 0;
  v.cap = cap;
  v.data = (cap == 0) ? ctx->heap.mem : (uint8_t*)rt_alloc(ctx, cap, 1);
#ifdef X07_DEBUG_BORROW
  v.dbg_aid = (cap == 0) ? 0 : rt_dbg_alloc_register(ctx, v.data, cap);
#endif
  return v;
}

static void rt_vec_u8_drop(ctx_t* ctx, vec_u8_t* v) {
  if (!v) return;
  if (v->cap == 0) {
    v->data = ctx->heap.mem;
    v->len = 0;
    return;
  }
#ifdef X07_DEBUG_BORROW
  rt_dbg_alloc_kill(ctx, v->dbg_aid);
  v->dbg_aid = 0;
#endif
  rt_free(ctx, v->data, v->cap, 1);
  v->data = ctx->heap.mem;
  v->len = 0;
  v->cap = 0;
}

static uint32_t rt_vec_u8_len(ctx_t* ctx, vec_u8_t v) {
  (void)ctx;
  return v.len;
}

static uint32_t rt_vec_u8_cap(ctx_t* ctx, vec_u8_t v) {
  (void)ctx;
  return v.cap;
}

static vec_u8_t rt_vec_u8_clear(ctx_t* ctx, vec_u8_t v) {
  (void)ctx;
  v.len = 0;
  return v;
}

static uint32_t rt_vec_u8_get(ctx_t* ctx, vec_u8_t v, uint32_t idx) {
  (void)ctx;
  if (idx >= v.len) rt_trap("vec_u8.get oob");
  return (uint32_t)v.data[idx];
}

static vec_u8_t rt_vec_u8_set(ctx_t* ctx, vec_u8_t v, uint32_t idx, uint32_t val) {
  (void)ctx;
  if (idx >= v.len) rt_trap("vec_u8.set oob");
  v.data[idx] = (uint8_t)(val & UINT32_C(0xFF));
  return v;
}

static vec_u8_t rt_vec_u8_push(ctx_t* ctx, vec_u8_t v, uint32_t val) {
  if (v.len == v.cap) {
    uint8_t* old_data = v.cap ? v.data : NULL;
    uint32_t old_cap = v.cap;
    uint32_t new_cap = v.cap ? (v.cap * 2) : 1;
    uint8_t* data = (uint8_t*)rt_alloc_realloc(
        ctx,
        old_data,
        old_cap,
        new_cap,
        1
    );
    if (v.data && v.len) {
      memcpy(data, v.data, v.len);
      rt_mem_on_memcpy(ctx, v.len);
    }
#ifdef X07_DEBUG_BORROW
    rt_dbg_alloc_kill(ctx, v.dbg_aid);
    v.dbg_aid = rt_dbg_alloc_register(ctx, data, new_cap);
#endif
    if (old_data && old_cap) {
      rt_free(ctx, old_data, old_cap, 1);
    }
    v.data = data;
    v.cap = new_cap;
  }
  v.data[v.len++] = (uint8_t)(val & UINT32_C(0xFF));
  return v;
}

static vec_u8_t rt_vec_u8_reserve_exact(ctx_t* ctx, vec_u8_t v, uint32_t additional) {
  if (additional > UINT32_MAX - v.len) rt_trap("vec_u8.reserve_exact overflow");
  uint32_t need = v.len + additional;
  if (need <= v.cap) return v;

  uint8_t* old_data = v.cap ? v.data : NULL;
  uint32_t old_cap = v.cap;
  uint8_t* data = (uint8_t*)rt_alloc_realloc(
      ctx,
      old_data,
      old_cap,
      need,
      1
  );
  if (v.data && v.len) {
    memcpy(data, v.data, v.len);
    rt_mem_on_memcpy(ctx, v.len);
  }
#ifdef X07_DEBUG_BORROW
  rt_dbg_alloc_kill(ctx, v.dbg_aid);
  v.dbg_aid = rt_dbg_alloc_register(ctx, data, need);
#endif
  if (old_data && old_cap) {
    rt_free(ctx, old_data, old_cap, 1);
  }
  v.data = data;
  v.cap = need;
  return v;
}

static vec_u8_t rt_vec_u8_extend_zeroes(ctx_t* ctx, vec_u8_t v, uint32_t n) {
  if (n > UINT32_MAX - v.len) rt_trap("vec_u8.extend_zeroes overflow");
  uint32_t need = v.len + n;
  if (need > v.cap) {
    uint8_t* old_data = v.cap ? v.data : NULL;
    uint32_t old_cap = v.cap;
    uint32_t new_cap = v.cap ? v.cap : 1;
    while (new_cap < need) {
      if (new_cap > UINT32_MAX / 2) {
        new_cap = need;
        break;
      }
      new_cap *= 2;
    }

    uint8_t* data = (uint8_t*)rt_alloc_realloc(
        ctx,
        old_data,
        old_cap,
        new_cap,
        1
    );
    if (v.data && v.len) {
      memcpy(data, v.data, v.len);
      rt_mem_on_memcpy(ctx, v.len);
    }
#ifdef X07_DEBUG_BORROW
    rt_dbg_alloc_kill(ctx, v.dbg_aid);
    v.dbg_aid = rt_dbg_alloc_register(ctx, data, new_cap);
#endif
    if (old_data && old_cap) {
      rt_free(ctx, old_data, old_cap, 1);
    }
    v.data = data;
    v.cap = new_cap;
  }

  if (n) {
    memset(v.data + v.len, 0, n);
  }
  v.len += n;
  return v;
}

static vec_u8_t rt_vec_u8_extend_bytes(ctx_t* ctx, vec_u8_t v, bytes_view_t b) {
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_borrow_check(ctx, b.bid, b.off_bytes, b.len);
#endif
  if (b.len > UINT32_MAX - v.len) rt_trap("vec_u8.extend_bytes overflow");
  uint32_t need = v.len + b.len;
  if (need > v.cap) {
    uint8_t* old_data = v.cap ? v.data : NULL;
    uint32_t old_cap = v.cap;
    uint32_t new_cap = v.cap ? v.cap : 1;
    while (new_cap < need) {
      if (new_cap > UINT32_MAX / 2) {
        new_cap = need;
        break;
      }
      new_cap *= 2;
    }

    uint8_t* data = (uint8_t*)rt_alloc_realloc(
        ctx,
        old_data,
        old_cap,
        new_cap,
        1
    );
    if (v.data && v.len) {
      memcpy(data, v.data, v.len);
      rt_mem_on_memcpy(ctx, v.len);
    }
#ifdef X07_DEBUG_BORROW
    rt_dbg_alloc_kill(ctx, v.dbg_aid);
    v.dbg_aid = rt_dbg_alloc_register(ctx, data, new_cap);
#endif
    if (old_data && old_cap) {
      rt_free(ctx, old_data, old_cap, 1);
    }
    v.data = data;
    v.cap = new_cap;
  }

  if (b.len) {
    memcpy(v.data + v.len, b.ptr, b.len);
    rt_mem_on_memcpy(ctx, b.len);
  }
  v.len += b.len;
  return v;
}

static vec_u8_t rt_vec_u8_extend_bytes_range(
    ctx_t* ctx,
    vec_u8_t v,
    bytes_view_t b,
    uint32_t start,
    uint32_t len
) {
  if (start > b.len || b.len - start < len) rt_trap("vec_u8.extend_bytes_range oob");
  bytes_view_t sub;
  sub.ptr = b.ptr + start;
  sub.len = len;
#ifdef X07_DEBUG_BORROW
  sub.aid = b.aid;
  sub.bid = b.bid;
  if (UINT32_MAX - b.off_bytes < start) rt_trap("vec_u8.extend_bytes_range off overflow");
  sub.off_bytes = b.off_bytes + start;
#endif
  return rt_vec_u8_extend_bytes(ctx, v, sub);
}

static bytes_view_t rt_vec_u8_as_view(ctx_t* ctx, vec_u8_t v) {
  bytes_view_t out;
  out.len = v.len;
#ifdef X07_DEBUG_BORROW
  if (out.len == 0) {
    out.ptr = ctx->heap.mem;
    out.aid = 0;
    out.bid = 0;
    out.off_bytes = 0;
    return out;
  }
  out.ptr = v.data;
  out.aid = v.dbg_aid;
  out.off_bytes = 0;
  out.bid = rt_dbg_alloc_borrow_id(ctx, out.aid);
#else
  out.ptr = (out.len == 0) ? ctx->heap.mem : v.data;
#endif
  return out;
}

static bytes_t rt_vec_u8_into_bytes(ctx_t* ctx, vec_u8_t* v) {
  if (!v) return rt_bytes_empty(ctx);
  if (v->len == 0) {
    rt_vec_u8_drop(ctx, v);
    return rt_bytes_empty(ctx);
  }

  bytes_t out;
  out.ptr = v->data;
  out.len = v->len;

  v->data = ctx->heap.mem;
  v->len = 0;
  v->cap = 0;
#ifdef X07_DEBUG_BORROW
  v->dbg_aid = 0;
#endif
  return out;
}

#ifndef X07_JSON_JCS_ENABLED
#define X07_JSON_JCS_ENABLED 0
#endif

// --- X07_JSON_JCS_END

// --- X07_STREAM_XF_PLUGIN_END

struct rt_scratch_u8_fixed_s {
  uint32_t alive;
  vec_u8_t buf;
};

static void rt_scratch_u8_fixed_ensure_cap(ctx_t* ctx, uint32_t need) {
  if (need <= ctx->scratch_u8_fixed_cap) return;
  rt_scratch_u8_fixed_t* old_items = ctx->scratch_u8_fixed;
  uint32_t old_cap = ctx->scratch_u8_fixed_cap;
  uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(rt_scratch_u8_fixed_t);
  uint32_t new_cap = ctx->scratch_u8_fixed_cap ? ctx->scratch_u8_fixed_cap : 8;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_scratch_u8_fixed_t* items = (rt_scratch_u8_fixed_t*)rt_alloc_realloc(
    ctx,
    old_items,
    old_bytes_total,
    new_cap * (uint32_t)sizeof(rt_scratch_u8_fixed_t),
    (uint32_t)_Alignof(rt_scratch_u8_fixed_t)
  );
  if (old_items && ctx->scratch_u8_fixed_len) {
    uint32_t bytes = ctx->scratch_u8_fixed_len * (uint32_t)sizeof(rt_scratch_u8_fixed_t);
    memcpy(items, old_items, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_items && old_bytes_total) {
    rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(rt_scratch_u8_fixed_t));
  }
  ctx->scratch_u8_fixed = items;
  ctx->scratch_u8_fixed_cap = new_cap;
}

static rt_scratch_u8_fixed_t* rt_scratch_u8_fixed_ptr(ctx_t* ctx, uint32_t handle) {
  if (handle == 0 || handle > ctx->scratch_u8_fixed_len) rt_trap("scratch_u8_fixed invalid handle");
  rt_scratch_u8_fixed_t* s = &ctx->scratch_u8_fixed[handle - 1];
  if (!s->alive) rt_trap("scratch_u8_fixed invalid handle");
  return s;
}

static uint32_t rt_scratch_u8_fixed_new(ctx_t* ctx, uint32_t cap) {
  // Reuse a free slot if possible.
  for (uint32_t i = 0; i < ctx->scratch_u8_fixed_len; i++) {
    rt_scratch_u8_fixed_t* s = &ctx->scratch_u8_fixed[i];
    if (s->alive) continue;
    s->alive = 1;
    s->buf = rt_vec_u8_new(ctx, cap);
    return i + 1;
  }

  if (ctx->scratch_u8_fixed_len == UINT32_MAX) rt_trap("scratch_u8_fixed.new overflow");
  uint32_t need = ctx->scratch_u8_fixed_len + 1;
  rt_scratch_u8_fixed_ensure_cap(ctx, need);
  uint32_t handle = need;
  rt_scratch_u8_fixed_t* s = &ctx->scratch_u8_fixed[handle - 1];
  s->alive = 1;
  s->buf = rt_vec_u8_new(ctx, cap);
  ctx->scratch_u8_fixed_len = need;
  return handle;
}

static uint32_t rt_scratch_u8_fixed_clear(ctx_t* ctx, uint32_t handle) {
  rt_scratch_u8_fixed_t* s = rt_scratch_u8_fixed_ptr(ctx, handle);
  s->buf.len = 0;
  return handle;
}

static uint32_t rt_scratch_u8_fixed_len(ctx_t* ctx, uint32_t handle) {
  rt_scratch_u8_fixed_t* s = rt_scratch_u8_fixed_ptr(ctx, handle);
  return s->buf.len;
}

static uint32_t rt_scratch_u8_fixed_cap(ctx_t* ctx, uint32_t handle) {
  rt_scratch_u8_fixed_t* s = rt_scratch_u8_fixed_ptr(ctx, handle);
  return s->buf.cap;
}

static bytes_view_t rt_scratch_u8_fixed_as_view(ctx_t* ctx, uint32_t handle) {
  rt_scratch_u8_fixed_t* s = rt_scratch_u8_fixed_ptr(ctx, handle);
  return rt_vec_u8_as_view(ctx, s->buf);
}

static result_i32_t rt_scratch_u8_fixed_try_write(ctx_t* ctx, uint32_t handle, bytes_view_t b) {
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_borrow_check(ctx, b.bid, b.off_bytes, b.len);
#endif
  rt_scratch_u8_fixed_t* s = rt_scratch_u8_fixed_ptr(ctx, handle);
  if (b.len > UINT32_MAX - s->buf.len) {
    return (result_i32_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(8) };
  }
  uint32_t need = s->buf.len + b.len;
  if (need > s->buf.cap) {
    return (result_i32_t){ .tag = UINT32_C(0), .payload.err = UINT32_C(8) };
  }
  if (b.len) {
    memcpy(s->buf.data + s->buf.len, b.ptr, b.len);
    rt_mem_on_memcpy(ctx, b.len);
  }
  s->buf.len = need;
  return (result_i32_t){ .tag = UINT32_C(1), .payload.ok = b.len };
}

static uint32_t rt_scratch_u8_fixed_drop(ctx_t* ctx, uint32_t handle) {
  if (handle == 0 || handle > ctx->scratch_u8_fixed_len) return UINT32_C(0);
  rt_scratch_u8_fixed_t* s = &ctx->scratch_u8_fixed[handle - 1];
  if (!s->alive) return UINT32_C(0);
  rt_vec_u8_drop(ctx, &s->buf);
  s->alive = 0;
  return UINT32_C(1);
}

typedef struct {
  uint32_t cap;
  uint32_t len;
  uint32_t* keys;
  uint32_t* vals;
} map_u32_t;

static map_u32_t* rt_map_u32_ptr(ctx_t* ctx, uint32_t handle) {
  if (handle == 0 || handle > ctx->map_u32_len) rt_trap("map_u32 invalid handle");
  map_u32_t* m = (map_u32_t*)ctx->map_u32_items[handle - 1];
  if (!m) rt_trap("map_u32 invalid handle");
  return m;
}

static uint32_t rt_is_pow2_u32(uint32_t x) {
  return (x != 0) && ((x & (x - 1)) == 0);
}

static uint32_t rt_map_u32_new(ctx_t* ctx, uint32_t cap) {
  if (!rt_is_pow2_u32(cap)) rt_trap("map_u32.new cap must be power of two");
  if (ctx->map_u32_len == ctx->map_u32_cap) {
    void** old_items = ctx->map_u32_items;
    uint32_t old_cap = ctx->map_u32_cap;
    uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(void*);
    uint32_t new_cap = ctx->map_u32_cap ? (ctx->map_u32_cap * 2) : 8;
    void** items = (void**)rt_alloc_realloc(
      ctx,
      old_items,
      old_bytes_total,
      new_cap * (uint32_t)sizeof(void*),
      (uint32_t)_Alignof(void*)
    );
    if (old_items && ctx->map_u32_len) {
      uint32_t bytes = ctx->map_u32_len * (uint32_t)sizeof(void*);
      memcpy(items, old_items, bytes);
      rt_mem_on_memcpy(ctx, bytes);
    }
    if (old_items && old_bytes_total) {
      rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(void*));
    }
    ctx->map_u32_items = items;
    ctx->map_u32_cap = new_cap;
  }
  map_u32_t* m = (map_u32_t*)rt_alloc(ctx, (uint32_t)sizeof(map_u32_t), (uint32_t)_Alignof(map_u32_t));
  m->cap = cap;
  m->len = 0;
  m->keys = (uint32_t*)rt_alloc(ctx, cap * (uint32_t)sizeof(uint32_t), (uint32_t)_Alignof(uint32_t));
  m->vals = (uint32_t*)rt_alloc(ctx, cap * (uint32_t)sizeof(uint32_t), (uint32_t)_Alignof(uint32_t));
  memset(m->keys, 0xFF, cap * (uint32_t)sizeof(uint32_t));
  ctx->map_u32_items[ctx->map_u32_len++] = m;
  return ctx->map_u32_len;
}

static uint32_t rt_map_u32_len(ctx_t* ctx, uint32_t handle) {
  return rt_map_u32_ptr(ctx, handle)->len;
}

static uint32_t rt_map_u32_hash(uint32_t key) {
  return key * UINT32_C(2654435769);
}

static uint32_t rt_map_u32_get(ctx_t* ctx, uint32_t handle, uint32_t key, uint32_t default_) {
  map_u32_t* m = rt_map_u32_ptr(ctx, handle);
  if (key == UINT32_C(0xFFFFFFFF)) rt_trap("map_u32.get key=-1 reserved");
  uint32_t mask = m->cap - 1;
  uint32_t idx = rt_map_u32_hash(key) & mask;
  uint32_t start = idx;
  for (;;) {
    uint32_t slot = m->keys[idx];
    if (slot == key) return m->vals[idx];
    if (slot == UINT32_C(0xFFFFFFFF)) return default_;
    idx = (idx + 1) & mask;
    if (idx == start) return default_;
  }
}

static uint32_t rt_map_u32_set(ctx_t* ctx, uint32_t handle, uint32_t key, uint32_t val) {
  map_u32_t* m = rt_map_u32_ptr(ctx, handle);
  if (key == UINT32_C(0xFFFFFFFF)) rt_trap("map_u32.set key=-1 reserved");
  uint32_t mask = m->cap - 1;
  uint32_t idx = rt_map_u32_hash(key) & mask;
  uint32_t start = idx;
  for (;;) {
    uint32_t slot = m->keys[idx];
    if (slot == key) {
      m->vals[idx] = val;
      return UINT32_C(0);
    }
    if (slot == UINT32_C(0xFFFFFFFF)) {
      m->keys[idx] = key;
      m->vals[idx] = val;
      m->len += 1;
      return UINT32_C(1);
    }
    idx = (idx + 1) & mask;
    if (idx == start) rt_trap("map_u32 full");
  }
}

static uint32_t rt_map_u32_contains(ctx_t* ctx, uint32_t handle, uint32_t key) {
  map_u32_t* m = rt_map_u32_ptr(ctx, handle);
  if (key == UINT32_C(0xFFFFFFFF)) rt_trap("map_u32.contains key=-1 reserved");
  uint32_t mask = m->cap - 1;
  uint32_t idx = rt_map_u32_hash(key) & mask;
  uint32_t start = idx;
  for (;;) {
    uint32_t slot = m->keys[idx];
    if (slot == key) return UINT32_C(1);
    if (slot == UINT32_C(0xFFFFFFFF)) return UINT32_C(0);
    idx = (idx + 1) & mask;
    if (idx == start) return UINT32_C(0);
  }
}

static uint32_t rt_map_u32_remove(ctx_t* ctx, uint32_t handle, uint32_t key) {
  map_u32_t* m = rt_map_u32_ptr(ctx, handle);
  if (key == UINT32_C(0xFFFFFFFF)) rt_trap("map_u32.remove key=-1 reserved");
  uint32_t mask = m->cap - 1;
  uint32_t idx = rt_map_u32_hash(key) & mask;
  uint32_t start = idx;
  for (;;) {
    uint32_t slot = m->keys[idx];
    if (slot == key) break;
    if (slot == UINT32_C(0xFFFFFFFF)) return UINT32_C(0);
    idx = (idx + 1) & mask;
    if (idx == start) return UINT32_C(0);
  }

  m->keys[idx] = UINT32_C(0xFFFFFFFF);
  m->vals[idx] = 0;
  m->len -= 1;

  uint32_t j = (idx + 1) & mask;
  while (m->keys[j] != UINT32_C(0xFFFFFFFF)) {
    uint32_t k = m->keys[j];
    uint32_t v = m->vals[j];
    m->keys[j] = UINT32_C(0xFFFFFFFF);
    m->vals[j] = 0;
    m->len -= 1;
    (void)rt_map_u32_set(ctx, handle, k, v);
    j = (j + 1) & mask;
  }

  return UINT32_C(1);
}

static inline void rt_store_u32_le(uint8_t* out, uint32_t x) {
  out[0] = (uint8_t)(x & UINT32_C(0xFF));
  out[1] = (uint8_t)((x >> 8) & UINT32_C(0xFF));
  out[2] = (uint8_t)((x >> 16) & UINT32_C(0xFF));
  out[3] = (uint8_t)((x >> 24) & UINT32_C(0xFF));
}

static bytes_t rt_set_u32_dump_u32le(ctx_t* ctx, uint32_t handle) {
  map_u32_t* m = rt_map_u32_ptr(ctx, handle);
  if (m->len == 0) return rt_bytes_empty(ctx);
  if (m->len > UINT32_MAX / UINT32_C(4)) rt_trap("set_u32.dump_u32le overflow");
  bytes_t out = rt_bytes_alloc(ctx, m->len * UINT32_C(4));
  uint32_t off = 0;
  for (uint32_t i = 0; i < m->cap; i++) {
    uint32_t k = m->keys[i];
    if (k == UINT32_C(0xFFFFFFFF)) continue;
    if (off > out.len - UINT32_C(4)) rt_trap("set_u32.dump_u32le len mismatch");
    rt_store_u32_le(out.ptr + off, k);
    off += UINT32_C(4);
  }
  if (off != out.len) rt_trap("set_u32.dump_u32le len mismatch");
  return out;
}

static bytes_t rt_map_u32_dump_kv_u32le_u32le(ctx_t* ctx, uint32_t handle) {
  map_u32_t* m = rt_map_u32_ptr(ctx, handle);
  if (m->len == 0) return rt_bytes_empty(ctx);
  if (m->len > UINT32_MAX / UINT32_C(8)) rt_trap("map_u32.dump_kv_u32le_u32le overflow");
  bytes_t out = rt_bytes_alloc(ctx, m->len * UINT32_C(8));
  uint32_t off = 0;
  for (uint32_t i = 0; i < m->cap; i++) {
    uint32_t k = m->keys[i];
    if (k == UINT32_C(0xFFFFFFFF)) continue;
    if (off > out.len - UINT32_C(8)) rt_trap("map_u32.dump_kv_u32le_u32le len mismatch");
    rt_store_u32_le(out.ptr + off, k);
    rt_store_u32_le(out.ptr + off + UINT32_C(4), m->vals[i]);
    off += UINT32_C(8);
  }
  if (off != out.len) rt_trap("map_u32.dump_kv_u32le_u32le len mismatch");
  return out;
}

static uint32_t rt_hash_mix32(uint32_t h) {
  h ^= h >> 16;
  h *= UINT32_C(2246822507);
  h ^= h >> 13;
  h *= UINT32_C(3266489909);
  h ^= h >> 16;
  return h;
}

static uint32_t rt_hash_fnv1a32_view(ctx_t* ctx, bytes_view_t v) {
#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_borrow_check(ctx, v.bid, v.off_bytes, v.len);
#else
  (void)ctx;
#endif
  uint32_t h = UINT32_C(2166136261);
  for (uint32_t i = 0; i < v.len; i++) {
    h ^= (uint32_t)v.ptr[i];
    h *= UINT32_C(16777619);
  }
  return h;
}

static int32_t rt_cmp_u32_to_i32(uint32_t x07_cmp) {
  if (x07_cmp == UINT32_MAX) return -1;
  if (x07_cmp == UINT32_C(1)) return 1;
  return 0;
}

typedef struct {
  uint32_t size;
  uint32_t align;
  void (*drop_in_place)(ctx_t* ctx, void* p);
  void (*clone_into)(ctx_t* ctx, void* dst, const void* src);
  uint32_t (*eq)(ctx_t* ctx, const void* a, const void* b);
  uint32_t (*hash32)(ctx_t* ctx, const void* a);
  int32_t (*cmp)(ctx_t* ctx, const void* a, const void* b);
} rt_tyops_v1;

static void rt_tyops_drop_nop(ctx_t* ctx, void* p) {
  (void)ctx;
  (void)p;
}

static void rt_tyops_clone_u32_into(ctx_t* ctx, void* dst, const void* src) {
  (void)ctx;
  *(uint32_t*)dst = *(const uint32_t*)src;
}

static uint32_t rt_tyops_eq_u32(ctx_t* ctx, const void* a, const void* b) {
  (void)ctx;
  return (*(const uint32_t*)a == *(const uint32_t*)b) ? UINT32_C(1) : UINT32_C(0);
}

static uint32_t rt_tyops_hash32_u32(ctx_t* ctx, const void* a) {
  (void)ctx;
  return rt_hash_mix32(*(const uint32_t*)a);
}

static int32_t rt_tyops_cmp_i32(ctx_t* ctx, const void* a, const void* b) {
  (void)ctx;
  int32_t aa = (int32_t)(*(const uint32_t*)a);
  int32_t bb = (int32_t)(*(const uint32_t*)b);
  if (aa < bb) return -1;
  if (aa > bb) return 1;
  return 0;
}

static int32_t rt_tyops_cmp_u32(ctx_t* ctx, const void* a, const void* b) {
  (void)ctx;
  uint32_t aa = *(const uint32_t*)a;
  uint32_t bb = *(const uint32_t*)b;
  if (aa < bb) return -1;
  if (aa > bb) return 1;
  return 0;
}

static void rt_tyops_drop_bytes(ctx_t* ctx, void* p) {
  rt_bytes_drop(ctx, (bytes_t*)p);
}

static void rt_tyops_clone_bytes_into(ctx_t* ctx, void* dst, const void* src) {
  bytes_t b = *(const bytes_t*)src;
  *(bytes_t*)dst = rt_bytes_clone(ctx, b);
}

static uint32_t rt_tyops_eq_bytes(ctx_t* ctx, const void* a, const void* b) {
  bytes_t ba = *(const bytes_t*)a;
  bytes_t bb = *(const bytes_t*)b;
  return rt_view_eq(ctx, rt_bytes_view(ctx, ba), rt_bytes_view(ctx, bb));
}

static uint32_t rt_tyops_hash32_bytes(ctx_t* ctx, const void* a) {
  bytes_t b = *(const bytes_t*)a;
  bytes_view_t v = rt_bytes_view(ctx, b);
  return rt_hash_mix32(rt_hash_fnv1a32_view(ctx, v));
}

static int32_t rt_tyops_cmp_bytes(ctx_t* ctx, const void* a, const void* b) {
  bytes_t ba = *(const bytes_t*)a;
  bytes_t bb = *(const bytes_t*)b;
  uint32_t r = rt_bytes_cmp_range(ctx, ba, 0, ba.len, bb, 0, bb.len);
  return rt_cmp_u32_to_i32(r);
}

static void rt_tyops_clone_bytes_view_into(ctx_t* ctx, void* dst, const void* src) {
  (void)ctx;
  *(bytes_view_t*)dst = *(const bytes_view_t*)src;
}

static uint32_t rt_tyops_eq_bytes_view(ctx_t* ctx, const void* a, const void* b) {
  bytes_view_t va = *(const bytes_view_t*)a;
  bytes_view_t vb = *(const bytes_view_t*)b;
  return rt_view_eq(ctx, va, vb);
}

static uint32_t rt_tyops_hash32_bytes_view(ctx_t* ctx, const void* a) {
  bytes_view_t v = *(const bytes_view_t*)a;
  return rt_hash_mix32(rt_hash_fnv1a32_view(ctx, v));
}

static int32_t rt_tyops_cmp_bytes_view(ctx_t* ctx, const void* a, const void* b) {
  bytes_view_t va = *(const bytes_view_t*)a;
  bytes_view_t vb = *(const bytes_view_t*)b;
  uint32_t r = rt_view_cmp_range(ctx, va, 0, va.len, vb, 0, vb.len);
  return rt_cmp_u32_to_i32(r);
}

static const rt_tyops_v1 RT_TYOPS_V1[5] = {
  {0},
  {
    .size = UINT32_C(4),
    .align = (uint32_t)_Alignof(uint32_t),
    .drop_in_place = rt_tyops_drop_nop,
    .clone_into = rt_tyops_clone_u32_into,
    .eq = rt_tyops_eq_u32,
    .hash32 = rt_tyops_hash32_u32,
    .cmp = rt_tyops_cmp_i32,
  },
  {
    .size = UINT32_C(4),
    .align = (uint32_t)_Alignof(uint32_t),
    .drop_in_place = rt_tyops_drop_nop,
    .clone_into = rt_tyops_clone_u32_into,
    .eq = rt_tyops_eq_u32,
    .hash32 = rt_tyops_hash32_u32,
    .cmp = rt_tyops_cmp_u32,
  },
  {
    .size = (uint32_t)sizeof(bytes_t),
    .align = (uint32_t)_Alignof(bytes_t),
    .drop_in_place = rt_tyops_drop_bytes,
    .clone_into = rt_tyops_clone_bytes_into,
    .eq = rt_tyops_eq_bytes,
    .hash32 = rt_tyops_hash32_bytes,
    .cmp = rt_tyops_cmp_bytes,
  },
  {
    .size = (uint32_t)sizeof(bytes_view_t),
    .align = (uint32_t)_Alignof(bytes_view_t),
    .drop_in_place = rt_tyops_drop_nop,
    .clone_into = rt_tyops_clone_bytes_view_into,
    .eq = rt_tyops_eq_bytes_view,
    .hash32 = rt_tyops_hash32_bytes_view,
    .cmp = rt_tyops_cmp_bytes_view,
  },
};

static const rt_tyops_v1* rt_tyops_v1_get(ctx_t* ctx, uint32_t ty_id) {
  (void)ctx;
  if (ty_id == UINT32_C(1)
      || ty_id == UINT32_C(2)
      || ty_id == UINT32_C(3)
      || ty_id == UINT32_C(4)) {
    return &RT_TYOPS_V1[ty_id];
  }
  rt_trap("value ty_id invalid");
}

typedef struct {
  uint32_t ty_id;
  uint32_t len;
  uint32_t cap;
  const rt_tyops_v1* ops;
  uint8_t* data;
} vec_value_t;

static vec_value_t* rt_vec_value_ptr(ctx_t* ctx, uint32_t handle) {
  if (handle == 0 || handle > ctx->vec_value_len) rt_trap("vec_value invalid handle");
  vec_value_t* v = (vec_value_t*)ctx->vec_value_items[handle - 1];
  if (!v) rt_trap("vec_value invalid handle");
  return v;
}

static void rt_vec_value_grow_exact(ctx_t* ctx, vec_value_t* v, uint32_t new_cap) {
  if (new_cap <= v->cap) return;
  uint32_t esz = v->ops->size;
  uint32_t align = v->ops->align;
  if (esz != 0 && new_cap > UINT32_MAX / esz) rt_trap("vec_value cap overflow");
  uint32_t old_bytes_total = v->cap * esz;
  uint32_t new_bytes_total = new_cap * esz;

  uint8_t* old_data = v->cap ? v->data : NULL;
  uint8_t* data = (uint8_t*)rt_alloc_realloc(ctx, old_data, old_bytes_total, new_bytes_total, align);
  if (old_data && v->len) {
    uint32_t bytes = v->len * esz;
    memcpy(data, v->data, bytes);
    rt_mem_on_memcpy(ctx, bytes);
  }
  if (old_data && old_bytes_total) {
    rt_free(ctx, old_data, old_bytes_total, align);
  }
  v->data = data;
  v->cap = new_cap;
}

static void rt_vec_value_reserve_exact_in_place(ctx_t* ctx, vec_value_t* v, uint32_t additional) {
  if (additional > UINT32_MAX - v->len) rt_trap("vec_value.reserve_exact overflow");
  uint32_t need = v->len + additional;
  if (need <= v->cap) return;
  rt_vec_value_grow_exact(ctx, v, need);
}

static void rt_vec_value_grow_for_push(ctx_t* ctx, vec_value_t* v, uint32_t need) {
  uint32_t new_cap = v->cap ? v->cap : 1;
  while (new_cap < need) {
    if (new_cap > UINT32_MAX / 2) {
      new_cap = need;
      break;
    }
    new_cap *= 2;
  }
  rt_vec_value_grow_exact(ctx, v, new_cap);
}

static uint32_t rt_vec_value_push_raw(ctx_t* ctx, vec_value_t* v, const void* x) {
  if (v->len == UINT32_MAX) rt_trap("vec_value.push overflow");
  uint32_t need = v->len + 1;
  if (need > v->cap) {
    rt_vec_value_grow_for_push(ctx, v, need);
  }
  uint32_t esz = v->ops->size;
  uint8_t* dst = v->data + (v->len * esz);
  memcpy(dst, x, esz);
  rt_mem_on_memcpy(ctx, esz);
  v->len = need;
  return UINT32_C(0);
}

static uint32_t rt_vec_value_with_capacity_v1(ctx_t* ctx, uint32_t ty_id, uint32_t cap) {
  const rt_tyops_v1* ops = rt_tyops_v1_get(ctx, ty_id);

  if (ctx->vec_value_len == ctx->vec_value_cap) {
    void** old_items = ctx->vec_value_items;
    uint32_t old_cap = ctx->vec_value_cap;
    uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(void*);
    uint32_t new_cap = ctx->vec_value_cap ? (ctx->vec_value_cap * 2) : 8;
    void** items = (void**)rt_alloc_realloc(
      ctx,
      old_items,
      old_bytes_total,
      new_cap * (uint32_t)sizeof(void*),
      (uint32_t)_Alignof(void*)
    );
    if (old_items && ctx->vec_value_len) {
      uint32_t bytes = ctx->vec_value_len * (uint32_t)sizeof(void*);
      memcpy(items, old_items, bytes);
      rt_mem_on_memcpy(ctx, bytes);
    }
    if (old_items && old_bytes_total) {
      rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(void*));
    }
    ctx->vec_value_items = items;
    ctx->vec_value_cap = new_cap;
  }

  vec_value_t* v = (vec_value_t*)rt_alloc(
    ctx,
    (uint32_t)sizeof(vec_value_t),
    (uint32_t)_Alignof(vec_value_t)
  );
  v->ty_id = ty_id;
  v->ops = ops;
  v->len = 0;
  v->cap = cap;
  if (cap == 0) {
    v->data = ctx->heap.mem;
  } else {
    if (ops->size != 0 && cap > UINT32_MAX / ops->size) rt_trap("vec_value.with_capacity cap overflow");
    uint32_t bytes_total = cap * ops->size;
    v->data = (uint8_t*)rt_alloc(ctx, bytes_total, ops->align);
  }

  ctx->vec_value_items[ctx->vec_value_len++] = v;
  return ctx->vec_value_len;
}

static uint32_t rt_vec_value_len(ctx_t* ctx, uint32_t handle) {
  return rt_vec_value_ptr(ctx, handle)->len;
}

static uint32_t rt_vec_value_reserve_exact(ctx_t* ctx, uint32_t handle, uint32_t additional) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  rt_vec_value_reserve_exact_in_place(ctx, v, additional);
  return handle;
}

static uint32_t rt_vec_value_push_i32_v1(ctx_t* ctx, uint32_t handle, uint32_t x) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (!(v->ty_id == UINT32_C(1) || v->ty_id == UINT32_C(2))) rt_trap("vec_value.push_i32_v1 ty mismatch");
  (void)rt_vec_value_push_raw(ctx, v, &x);
  return handle;
}

static uint32_t rt_vec_value_push_bytes_v1(ctx_t* ctx, uint32_t handle, bytes_t x) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (v->ty_id != UINT32_C(3)) rt_trap("vec_value.push_bytes_v1 ty mismatch");
  (void)rt_vec_value_push_raw(ctx, v, &x);
  return handle;
}

static uint32_t rt_vec_value_push_bytes_view_v1(ctx_t* ctx, uint32_t handle, bytes_view_t x) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (v->ty_id != UINT32_C(4)) rt_trap("vec_value.push_bytes_view_v1 ty mismatch");
  (void)rt_vec_value_push_raw(ctx, v, &x);
  return handle;
}

static uint32_t rt_vec_value_get_i32_v1(ctx_t* ctx, uint32_t handle, uint32_t idx, uint32_t default_) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (!(v->ty_id == UINT32_C(1) || v->ty_id == UINT32_C(2))) rt_trap("vec_value.get_i32_v1 ty mismatch");
  if (idx >= v->len) return default_;
  uint8_t* src = v->data + (idx * v->ops->size);
  return *(uint32_t*)src;
}

static bytes_t rt_vec_value_get_bytes_v1(ctx_t* ctx, uint32_t handle, uint32_t idx, bytes_t default_) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (v->ty_id != UINT32_C(3)) rt_trap("vec_value.get_bytes_v1 ty mismatch");
  if (idx >= v->len) return default_;
  rt_bytes_drop(ctx, &default_);
  bytes_t out = rt_bytes_empty(ctx);
  uint8_t* src = v->data + (idx * v->ops->size);
  v->ops->clone_into(ctx, &out, src);
  return out;
}

static bytes_view_t rt_vec_value_get_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t idx,
    bytes_view_t default_
) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (v->ty_id != UINT32_C(4)) rt_trap("vec_value.get_bytes_view_v1 ty mismatch");
  if (idx >= v->len) return default_;
  uint8_t* src = v->data + (idx * v->ops->size);
  return *(bytes_view_t*)src;
}

static uint32_t rt_vec_value_set_i32_v1(ctx_t* ctx, uint32_t handle, uint32_t idx, uint32_t x) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (!(v->ty_id == UINT32_C(1) || v->ty_id == UINT32_C(2))) rt_trap("vec_value.set_i32_v1 ty mismatch");
  if (idx >= v->len) rt_trap("vec_value.set oob");
  uint8_t* dst = v->data + (idx * v->ops->size);
  *(uint32_t*)dst = x;
  return handle;
}

static uint32_t rt_vec_value_set_bytes_v1(ctx_t* ctx, uint32_t handle, uint32_t idx, bytes_t x) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (v->ty_id != UINT32_C(3)) rt_trap("vec_value.set_bytes_v1 ty mismatch");
  if (idx >= v->len) rt_trap("vec_value.set oob");
  uint8_t* dst = v->data + (idx * v->ops->size);
  v->ops->drop_in_place(ctx, dst);
  memcpy(dst, &x, v->ops->size);
  rt_mem_on_memcpy(ctx, v->ops->size);
  return handle;
}

static uint32_t rt_vec_value_set_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t idx,
    bytes_view_t x
) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (v->ty_id != UINT32_C(4)) rt_trap("vec_value.set_bytes_view_v1 ty mismatch");
  if (idx >= v->len) rt_trap("vec_value.set oob");
  uint8_t* dst = v->data + (idx * v->ops->size);
  *(bytes_view_t*)dst = x;
  return handle;
}

static uint32_t rt_vec_value_pop(ctx_t* ctx, uint32_t handle) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  if (v->len == 0) return handle;
  uint32_t idx = v->len - 1;
  uint8_t* dst = v->data + (idx * v->ops->size);
  v->ops->drop_in_place(ctx, dst);
  v->len = idx;
  return handle;
}

static uint32_t rt_vec_value_clear(ctx_t* ctx, uint32_t handle) {
  vec_value_t* v = rt_vec_value_ptr(ctx, handle);
  uint32_t esz = v->ops->size;
  for (uint32_t i = 0; i < v->len; i++) {
    v->ops->drop_in_place(ctx, v->data + (i * esz));
  }
  v->len = 0;
  return handle;
}

typedef struct {
  uint32_t cap;
  uint32_t len;
  uint32_t k_ty_id;
  uint32_t v_ty_id;
  const rt_tyops_v1* k_ops;
  const rt_tyops_v1* v_ops;

  uint8_t* ctrl;    // 0 empty, 1 filled, 2 tombstone
  uint32_t* hashes; // cached hash32(key)
  uint8_t* keys;    // raw key bytes
  uint8_t* vals;    // raw val bytes
} map_value_t;

static map_value_t* rt_map_value_ptr(ctx_t* ctx, uint32_t handle) {
  if (handle == 0 || handle > ctx->map_value_len) rt_trap("map_value invalid handle");
  map_value_t* m = (map_value_t*)ctx->map_value_items[handle - 1];
  if (!m) rt_trap("map_value invalid handle");
  return m;
}

static void rt_map_value_alloc_arrays(ctx_t* ctx, map_value_t* m, uint32_t cap) {
  if (cap == 0) rt_trap("map_value cap=0");
  if (!rt_is_pow2_u32(cap)) rt_trap("map_value cap must be power of two");

  m->cap = cap;
  m->len = 0;

  m->ctrl = (uint8_t*)rt_alloc(ctx, cap, 1);
  memset(m->ctrl, 0, cap);

  if (cap > UINT32_MAX / (uint32_t)sizeof(uint32_t)) rt_trap("map_value hashes overflow");
  uint32_t hashes_bytes = cap * (uint32_t)sizeof(uint32_t);
  m->hashes = (uint32_t*)rt_alloc(ctx, hashes_bytes, (uint32_t)_Alignof(uint32_t));
  memset(m->hashes, 0, hashes_bytes);

  uint32_t ksz = m->k_ops->size;
  uint32_t vsz = m->v_ops->size;
  if (ksz != 0 && cap > UINT32_MAX / ksz) rt_trap("map_value keys overflow");
  if (vsz != 0 && cap > UINT32_MAX / vsz) rt_trap("map_value vals overflow");
  m->keys = (uint8_t*)rt_alloc(ctx, cap * ksz, m->k_ops->align);
  m->vals = (uint8_t*)rt_alloc(ctx, cap * vsz, m->v_ops->align);
}

static void rt_map_value_free_arrays(ctx_t* ctx, map_value_t* m) {
  if (m->ctrl && m->cap) rt_free(ctx, m->ctrl, m->cap, 1);
  if (m->hashes && m->cap) {
    rt_free(
      ctx,
      m->hashes,
      m->cap * (uint32_t)sizeof(uint32_t),
      (uint32_t)_Alignof(uint32_t)
    );
  }
  if (m->keys && m->cap) rt_free(ctx, m->keys, m->cap * m->k_ops->size, m->k_ops->align);
  if (m->vals && m->cap) rt_free(ctx, m->vals, m->cap * m->v_ops->size, m->v_ops->align);

  m->ctrl = NULL;
  m->hashes = NULL;
  m->keys = NULL;
  m->vals = NULL;
  m->cap = 0;
  m->len = 0;
}

static uint8_t* rt_map_value_key_ptr(map_value_t* m, uint32_t idx) {
  return m->keys + (idx * m->k_ops->size);
}

static uint8_t* rt_map_value_val_ptr(map_value_t* m, uint32_t idx) {
  return m->vals + (idx * m->v_ops->size);
}

static uint32_t rt_map_value_lookup_idx(
    ctx_t* ctx,
    map_value_t* m,
    const void* key,
    uint32_t hash,
    uint32_t* out_idx
) {
  if (m->cap == 0) return 0;
  uint32_t mask = m->cap - 1;
  uint32_t idx = hash & mask;
  uint32_t start = idx;
  for (;;) {
    uint8_t c = m->ctrl[idx];
    if (c == 0) return 0;
    if (c == 1) {
      if (m->hashes[idx] == hash) {
        uint8_t* kptr = rt_map_value_key_ptr(m, idx);
        if (m->k_ops->eq(ctx, kptr, key)) {
          if (out_idx) *out_idx = idx;
          return 1;
        }
      }
    }
    idx = (idx + 1) & mask;
    if (idx == start) return 0;
  }
}

static uint32_t rt_map_value_find_slot(
    ctx_t* ctx,
    map_value_t* m,
    const void* key,
    uint32_t hash,
    uint32_t* out_idx,
    uint32_t* out_found
) {
  if (m->cap == 0) rt_trap("map_value cap=0");
  uint32_t mask = m->cap - 1;
  uint32_t idx = hash & mask;
  uint32_t start = idx;
  uint32_t first_tomb = UINT32_MAX;
  for (;;) {
    uint8_t c = m->ctrl[idx];
    if (c == 0) {
      if (out_found) *out_found = 0;
      if (out_idx) *out_idx = (first_tomb == UINT32_MAX) ? idx : first_tomb;
      return 1;
    }
    if (c == 2) {
      if (first_tomb == UINT32_MAX) first_tomb = idx;
    } else if (c == 1) {
      if (m->hashes[idx] == hash) {
        uint8_t* kptr = rt_map_value_key_ptr(m, idx);
        if (m->k_ops->eq(ctx, kptr, key)) {
          if (out_found) *out_found = 1;
          if (out_idx) *out_idx = idx;
          return 1;
        }
      }
    } else {
      rt_trap("map_value ctrl corrupt");
    }

    idx = (idx + 1) & mask;
    if (idx == start) {
      if (first_tomb != UINT32_MAX) {
        if (out_found) *out_found = 0;
        if (out_idx) *out_idx = first_tomb;
        return 1;
      }
      return 0;
    }
  }
}

static void rt_map_value_rehash(ctx_t* ctx, map_value_t* m, uint32_t new_cap) {
  if (!rt_is_pow2_u32(new_cap)) rt_trap("map_value.rehash cap must be power of two");

  map_value_t tmp = *m;
  rt_map_value_alloc_arrays(ctx, &tmp, new_cap);

  uint32_t ksz = m->k_ops->size;
  uint32_t vsz = m->v_ops->size;

  for (uint32_t i = 0; i < m->cap; i++) {
    if (m->ctrl[i] != 1) continue;
    uint32_t hash = m->hashes[i];

    uint32_t mask = tmp.cap - 1;
    uint32_t idx = hash & mask;
    uint32_t start = idx;
    for (;;) {
      if (tmp.ctrl[idx] == 0) break;
      idx = (idx + 1) & mask;
      if (idx == start) rt_trap("map_value.rehash full");
    }

    tmp.ctrl[idx] = 1;
    tmp.hashes[idx] = hash;
    memcpy(rt_map_value_key_ptr(&tmp, idx), rt_map_value_key_ptr(m, i), ksz);
    rt_mem_on_memcpy(ctx, ksz);
    memcpy(rt_map_value_val_ptr(&tmp, idx), rt_map_value_val_ptr(m, i), vsz);
    rt_mem_on_memcpy(ctx, vsz);
    tmp.len += 1;
  }

  rt_map_value_free_arrays(ctx, m);
  *m = tmp;
}

static void rt_map_value_ensure_room_for_insert(ctx_t* ctx, map_value_t* m) {
  uint64_t lhs = ((uint64_t)m->len + 1ULL) * 10ULL;
  uint64_t rhs = (uint64_t)m->cap * 7ULL;
  if (lhs <= rhs) return;

  if (m->cap > UINT32_MAX / 2) rt_trap("map_value cap overflow");
  rt_map_value_rehash(ctx, m, m->cap * 2);
}

static uint32_t rt_map_value_set_raw(
    ctx_t* ctx,
    map_value_t* m,
    const void* key,
    const void* val,
    uint32_t* out_replaced
) {
  rt_map_value_ensure_room_for_insert(ctx, m);

  uint32_t hash = m->k_ops->hash32(ctx, key);
  uint32_t idx = 0;
  uint32_t found = 0;
  if (!rt_map_value_find_slot(ctx, m, key, hash, &idx, &found)) {
    if (m->cap > UINT32_MAX / 2) rt_trap("map_value cap overflow");
    rt_map_value_rehash(ctx, m, m->cap * 2);
    if (!rt_map_value_find_slot(ctx, m, key, hash, &idx, &found)) {
      rt_trap("map_value.set no slot");
    }
  }

  uint32_t ksz = m->k_ops->size;
  uint32_t vsz = m->v_ops->size;
  uint8_t* vptr = rt_map_value_val_ptr(m, idx);

  if (found) {
    if (out_replaced) *out_replaced = 1;
    m->v_ops->drop_in_place(ctx, vptr);
    memcpy(vptr, val, vsz);
    rt_mem_on_memcpy(ctx, vsz);
    return 0;
  }

  if (out_replaced) *out_replaced = 0;
  m->ctrl[idx] = 1;
  m->hashes[idx] = hash;
  memcpy(rt_map_value_key_ptr(m, idx), key, ksz);
  rt_mem_on_memcpy(ctx, ksz);
  memcpy(vptr, val, vsz);
  rt_mem_on_memcpy(ctx, vsz);
  m->len += 1;
  return 0;
}

static uint32_t rt_map_value_remove_raw(ctx_t* ctx, map_value_t* m, const void* key) {
  uint32_t hash = m->k_ops->hash32(ctx, key);
  uint32_t idx = 0;
  if (!rt_map_value_lookup_idx(ctx, m, key, hash, &idx)) return 0;

  uint8_t* kptr = rt_map_value_key_ptr(m, idx);
  uint8_t* vptr = rt_map_value_val_ptr(m, idx);
  m->k_ops->drop_in_place(ctx, kptr);
  m->v_ops->drop_in_place(ctx, vptr);
  m->ctrl[idx] = 2;
  if (m->len == 0) rt_trap("map_value.remove len underflow");
  m->len -= 1;
  return 1;
}

static uint32_t rt_map_value_clear_in_place(ctx_t* ctx, map_value_t* m) {
  for (uint32_t i = 0; i < m->cap; i++) {
    if (m->ctrl[i] != 1) continue;
    m->k_ops->drop_in_place(ctx, rt_map_value_key_ptr(m, i));
    m->v_ops->drop_in_place(ctx, rt_map_value_val_ptr(m, i));
  }
  memset(m->ctrl, 0, m->cap);
  memset(m->hashes, 0, m->cap * (uint32_t)sizeof(uint32_t));
  m->len = 0;
  return 0;
}

static uint32_t rt_map_value_new_v1(ctx_t* ctx, uint32_t k_ty_id, uint32_t v_ty_id, uint32_t cap) {
  if (!rt_is_pow2_u32(cap)) rt_trap("map_value.new cap must be power of two");
  const rt_tyops_v1* k_ops = rt_tyops_v1_get(ctx, k_ty_id);
  const rt_tyops_v1* v_ops = rt_tyops_v1_get(ctx, v_ty_id);
  if (!k_ops->eq || !k_ops->hash32) rt_trap("map_value.new key ops missing");

  if (ctx->map_value_len == ctx->map_value_cap) {
    void** old_items = ctx->map_value_items;
    uint32_t old_cap = ctx->map_value_cap;
    uint32_t old_bytes_total = old_cap * (uint32_t)sizeof(void*);
    uint32_t new_cap = ctx->map_value_cap ? (ctx->map_value_cap * 2) : 8;
    void** items = (void**)rt_alloc_realloc(
      ctx,
      old_items,
      old_bytes_total,
      new_cap * (uint32_t)sizeof(void*),
      (uint32_t)_Alignof(void*)
    );
    if (old_items && ctx->map_value_len) {
      uint32_t bytes = ctx->map_value_len * (uint32_t)sizeof(void*);
      memcpy(items, old_items, bytes);
      rt_mem_on_memcpy(ctx, bytes);
    }
    if (old_items && old_bytes_total) {
      rt_free(ctx, old_items, old_bytes_total, (uint32_t)_Alignof(void*));
    }
    ctx->map_value_items = items;
    ctx->map_value_cap = new_cap;
  }

  map_value_t* m = (map_value_t*)rt_alloc(
    ctx,
    (uint32_t)sizeof(map_value_t),
    (uint32_t)_Alignof(map_value_t)
  );
  memset(m, 0, sizeof(map_value_t));
  m->k_ty_id = k_ty_id;
  m->v_ty_id = v_ty_id;
  m->k_ops = k_ops;
  m->v_ops = v_ops;
  rt_map_value_alloc_arrays(ctx, m, cap);

  ctx->map_value_items[ctx->map_value_len++] = m;
  return ctx->map_value_len;
}

static uint32_t rt_map_value_len(ctx_t* ctx, uint32_t handle) {
  return rt_map_value_ptr(ctx, handle)->len;
}

static uint32_t rt_map_value_clear(ctx_t* ctx, uint32_t handle) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  (void)rt_map_value_clear_in_place(ctx, m);
  return handle;
}

static uint32_t rt_map_value_contains_i32_v1(ctx_t* ctx, uint32_t handle, uint32_t key) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.contains_i32_v1 key ty mismatch");
  uint32_t hash = m->k_ops->hash32(ctx, &key);
  return rt_map_value_lookup_idx(ctx, m, &key, hash, NULL);
}

static uint32_t rt_map_value_contains_bytes_v1(ctx_t* ctx, uint32_t handle, bytes_t key) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.contains_bytes_v1 key ty mismatch");
  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t ok = rt_map_value_lookup_idx(ctx, m, &key, hash, NULL);
  rt_bytes_drop(ctx, &key);
  return ok;
}

static uint32_t rt_map_value_contains_bytes_view_v1(ctx_t* ctx, uint32_t handle, bytes_view_t key) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.contains_bytes_view_v1 key ty mismatch");
  uint32_t hash = m->k_ops->hash32(ctx, &key);
  return rt_map_value_lookup_idx(ctx, m, &key, hash, NULL);
}

static uint32_t rt_map_value_remove_i32_v1(ctx_t* ctx, uint32_t handle, uint32_t key) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.remove_i32_v1 key ty mismatch");
  (void)rt_map_value_remove_raw(ctx, m, &key);
  return handle;
}

static uint32_t rt_map_value_remove_bytes_v1(ctx_t* ctx, uint32_t handle, bytes_t key) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.remove_bytes_v1 key ty mismatch");
  (void)rt_map_value_remove_raw(ctx, m, &key);
  rt_bytes_drop(ctx, &key);
  return handle;
}

static uint32_t rt_map_value_remove_bytes_view_v1(ctx_t* ctx, uint32_t handle, bytes_view_t key) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.remove_bytes_view_v1 key ty mismatch");
  (void)rt_map_value_remove_raw(ctx, m, &key);
  return handle;
}

static uint32_t rt_map_value_get_i32_i32_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t key,
    uint32_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.get key ty mismatch");
  if (!(m->v_ty_id == UINT32_C(1) || m->v_ty_id == UINT32_C(2))) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  if (!rt_map_value_lookup_idx(ctx, m, &key, hash, &idx)) return default_;
  uint8_t* vptr = rt_map_value_val_ptr(m, idx);
  return *(uint32_t*)vptr;
}

static bytes_t rt_map_value_get_i32_bytes_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t key,
    bytes_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.get key ty mismatch");
  if (m->v_ty_id != UINT32_C(3)) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  if (!rt_map_value_lookup_idx(ctx, m, &key, hash, &idx)) return default_;

  rt_bytes_drop(ctx, &default_);
  bytes_t out = rt_bytes_empty(ctx);
  m->v_ops->clone_into(ctx, &out, rt_map_value_val_ptr(m, idx));
  return out;
}

static bytes_view_t rt_map_value_get_i32_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t key,
    bytes_view_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.get key ty mismatch");
  if (m->v_ty_id != UINT32_C(4)) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  if (!rt_map_value_lookup_idx(ctx, m, &key, hash, &idx)) return default_;
  return *(bytes_view_t*)rt_map_value_val_ptr(m, idx);
}

static uint32_t rt_map_value_get_bytes_i32_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_t key,
    uint32_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.get key ty mismatch");
  if (!(m->v_ty_id == UINT32_C(1) || m->v_ty_id == UINT32_C(2))) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  uint32_t found = rt_map_value_lookup_idx(ctx, m, &key, hash, &idx);
  rt_bytes_drop(ctx, &key);
  if (!found) return default_;
  return *(uint32_t*)rt_map_value_val_ptr(m, idx);
}

static bytes_t rt_map_value_get_bytes_bytes_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_t key,
    bytes_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.get key ty mismatch");
  if (m->v_ty_id != UINT32_C(3)) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  uint32_t found = rt_map_value_lookup_idx(ctx, m, &key, hash, &idx);
  rt_bytes_drop(ctx, &key);
  if (!found) return default_;

  rt_bytes_drop(ctx, &default_);
  bytes_t out = rt_bytes_empty(ctx);
  m->v_ops->clone_into(ctx, &out, rt_map_value_val_ptr(m, idx));
  return out;
}

static bytes_view_t rt_map_value_get_bytes_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_t key,
    bytes_view_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.get key ty mismatch");
  if (m->v_ty_id != UINT32_C(4)) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  uint32_t found = rt_map_value_lookup_idx(ctx, m, &key, hash, &idx);
  rt_bytes_drop(ctx, &key);
  if (!found) return default_;
  return *(bytes_view_t*)rt_map_value_val_ptr(m, idx);
}

static uint32_t rt_map_value_get_bytes_view_i32_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_view_t key,
    uint32_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.get key ty mismatch");
  if (!(m->v_ty_id == UINT32_C(1) || m->v_ty_id == UINT32_C(2))) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  if (!rt_map_value_lookup_idx(ctx, m, &key, hash, &idx)) return default_;
  return *(uint32_t*)rt_map_value_val_ptr(m, idx);
}

static bytes_t rt_map_value_get_bytes_view_bytes_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_view_t key,
    bytes_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.get key ty mismatch");
  if (m->v_ty_id != UINT32_C(3)) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  if (!rt_map_value_lookup_idx(ctx, m, &key, hash, &idx)) return default_;

  rt_bytes_drop(ctx, &default_);
  bytes_t out = rt_bytes_empty(ctx);
  m->v_ops->clone_into(ctx, &out, rt_map_value_val_ptr(m, idx));
  return out;
}

static bytes_view_t rt_map_value_get_bytes_view_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_view_t key,
    bytes_view_t default_
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.get key ty mismatch");
  if (m->v_ty_id != UINT32_C(4)) rt_trap("map_value.get val ty mismatch");

  uint32_t hash = m->k_ops->hash32(ctx, &key);
  uint32_t idx = 0;
  if (!rt_map_value_lookup_idx(ctx, m, &key, hash, &idx)) return default_;
  return *(bytes_view_t*)rt_map_value_val_ptr(m, idx);
}

static uint32_t rt_map_value_set_i32_i32_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t key,
    uint32_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.set key ty mismatch");
  if (!(m->v_ty_id == UINT32_C(1) || m->v_ty_id == UINT32_C(2))) rt_trap("map_value.set val ty mismatch");
  (void)rt_map_value_set_raw(ctx, m, &key, &val, NULL);
  return handle;
}

static uint32_t rt_map_value_set_i32_bytes_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t key,
    bytes_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.set key ty mismatch");
  if (m->v_ty_id != UINT32_C(3)) rt_trap("map_value.set val ty mismatch");
  (void)rt_map_value_set_raw(ctx, m, &key, &val, NULL);
  return handle;
}

static uint32_t rt_map_value_set_i32_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    uint32_t key,
    bytes_view_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (!(m->k_ty_id == UINT32_C(1) || m->k_ty_id == UINT32_C(2))) rt_trap("map_value.set key ty mismatch");
  if (m->v_ty_id != UINT32_C(4)) rt_trap("map_value.set val ty mismatch");
  (void)rt_map_value_set_raw(ctx, m, &key, &val, NULL);
  return handle;
}

static uint32_t rt_map_value_set_bytes_i32_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_t key,
    uint32_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.set key ty mismatch");
  if (!(m->v_ty_id == UINT32_C(1) || m->v_ty_id == UINT32_C(2))) rt_trap("map_value.set val ty mismatch");
  uint32_t replaced = 0;
  (void)rt_map_value_set_raw(ctx, m, &key, &val, &replaced);
  if (replaced) rt_bytes_drop(ctx, &key);
  return handle;
}

static uint32_t rt_map_value_set_bytes_bytes_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_t key,
    bytes_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.set key ty mismatch");
  if (m->v_ty_id != UINT32_C(3)) rt_trap("map_value.set val ty mismatch");
  uint32_t replaced = 0;
  (void)rt_map_value_set_raw(ctx, m, &key, &val, &replaced);
  if (replaced) rt_bytes_drop(ctx, &key);
  return handle;
}

static uint32_t rt_map_value_set_bytes_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_t key,
    bytes_view_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(3)) rt_trap("map_value.set key ty mismatch");
  if (m->v_ty_id != UINT32_C(4)) rt_trap("map_value.set val ty mismatch");
  uint32_t replaced = 0;
  (void)rt_map_value_set_raw(ctx, m, &key, &val, &replaced);
  if (replaced) rt_bytes_drop(ctx, &key);
  return handle;
}

static uint32_t rt_map_value_set_bytes_view_i32_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_view_t key,
    uint32_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.set key ty mismatch");
  if (!(m->v_ty_id == UINT32_C(1) || m->v_ty_id == UINT32_C(2))) rt_trap("map_value.set val ty mismatch");
  (void)rt_map_value_set_raw(ctx, m, &key, &val, NULL);
  return handle;
}

static uint32_t rt_map_value_set_bytes_view_bytes_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_view_t key,
    bytes_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.set key ty mismatch");
  if (m->v_ty_id != UINT32_C(3)) rt_trap("map_value.set val ty mismatch");
  (void)rt_map_value_set_raw(ctx, m, &key, &val, NULL);
  return handle;
}

static uint32_t rt_map_value_set_bytes_view_bytes_view_v1(
    ctx_t* ctx,
    uint32_t handle,
    bytes_view_t key,
    bytes_view_t val
) {
  map_value_t* m = rt_map_value_ptr(ctx, handle);
  if (m->k_ty_id != UINT32_C(4)) rt_trap("map_value.set key ty mismatch");
  if (m->v_ty_id != UINT32_C(4)) rt_trap("map_value.set val ty mismatch");
  (void)rt_map_value_set_raw(ctx, m, &key, &val, NULL);
  return handle;
}

static void rt_ctx_cleanup(ctx_t* ctx) {
#ifdef X07_DEBUG_BORROW
  if (ctx->dbg_borrows && ctx->dbg_borrows_cap) {
    rt_free(
      ctx,
      ctx->dbg_borrows,
      ctx->dbg_borrows_cap * (uint32_t)sizeof(dbg_borrow_rec_t),
      (uint32_t)_Alignof(dbg_borrow_rec_t)
    );
  }
  if (ctx->dbg_allocs && ctx->dbg_allocs_cap) {
    rt_free(
      ctx,
      ctx->dbg_allocs,
      ctx->dbg_allocs_cap * (uint32_t)sizeof(dbg_alloc_rec_t),
      (uint32_t)_Alignof(dbg_alloc_rec_t)
    );
  }
  ctx->dbg_borrows = NULL;
  ctx->dbg_borrows_len = 0;
  ctx->dbg_borrows_cap = 0;
  ctx->dbg_allocs = NULL;
  ctx->dbg_allocs_len = 0;
  ctx->dbg_allocs_cap = 0;
#endif

  rt_os_process_cleanup(ctx);

  for (uint32_t i = 0; i < ctx->bufreads_len; i++) {
    rt_bufread_t* br = &ctx->bufreads[i];
    if (!br->alive) continue;
    rt_bytes_drop(ctx, &br->buf);
    br->buf = rt_bytes_empty(ctx);
    br->alive = 0;
  }
  if (ctx->bufreads && ctx->bufreads_cap) {
    rt_free(
      ctx,
      ctx->bufreads,
      ctx->bufreads_cap * (uint32_t)sizeof(rt_bufread_t),
      (uint32_t)_Alignof(rt_bufread_t)
    );
  }
  ctx->bufreads = NULL;
  ctx->bufreads_len = 0;
  ctx->bufreads_cap = 0;

  for (uint32_t i = 0; i < ctx->scratch_u8_fixed_len; i++) {
    rt_scratch_u8_fixed_t* s = &ctx->scratch_u8_fixed[i];
    if (!s->alive) continue;
    rt_vec_u8_drop(ctx, &s->buf);
    s->alive = 0;
  }
  if (ctx->scratch_u8_fixed && ctx->scratch_u8_fixed_cap) {
    rt_free(
      ctx,
      ctx->scratch_u8_fixed,
      ctx->scratch_u8_fixed_cap * (uint32_t)sizeof(rt_scratch_u8_fixed_t),
      (uint32_t)_Alignof(rt_scratch_u8_fixed_t)
    );
  }
  ctx->scratch_u8_fixed = NULL;
  ctx->scratch_u8_fixed_len = 0;
  ctx->scratch_u8_fixed_cap = 0;

  for (uint32_t i = 0; i < ctx->io_readers_len; i++) {
    rt_io_reader_t* r = &ctx->io_readers[i];
    if (!r->alive) continue;
#if X07_ENABLE_STREAMING_FILE_IO
    if (r->kind == RT_IO_READER_KIND_FILE && r->f) {
      fclose(r->f);
      r->f = NULL;
    }
#endif
    if (r->kind == RT_IO_READER_KIND_BYTES) {
      rt_bytes_drop(ctx, &r->bytes);
      r->bytes = rt_bytes_empty(ctx);
    }
    r->alive = 0;
  }
  if (ctx->io_readers && ctx->io_readers_cap) {
    rt_free(
      ctx,
      ctx->io_readers,
      ctx->io_readers_cap * (uint32_t)sizeof(rt_io_reader_t),
      (uint32_t)_Alignof(rt_io_reader_t)
    );
  }
  ctx->io_readers = NULL;
  ctx->io_readers_len = 0;
  ctx->io_readers_cap = 0;

  for (uint32_t i = 0; i < ctx->sched_chans_len; i++) {
    rt_chan_bytes_t* c = &ctx->sched_chans[i];
    if (!c->alive) continue;
    for (uint32_t j = 0; j < c->len; j++) {
      uint32_t idx = (c->head + j) % c->cap;
      rt_bytes_drop(ctx, &c->buf[idx]);
    }
    if (c->buf && c->cap) {
      rt_free(ctx, c->buf, c->cap * (uint32_t)sizeof(bytes_t), (uint32_t)_Alignof(bytes_t));
    }
    c->buf = NULL;
    c->alive = 0;
  }
  if (ctx->sched_chans && ctx->sched_chans_cap) {
    rt_free(
      ctx,
      ctx->sched_chans,
      ctx->sched_chans_cap * (uint32_t)sizeof(rt_chan_bytes_t),
      (uint32_t)_Alignof(rt_chan_bytes_t)
    );
  }
  ctx->sched_chans = NULL;
  ctx->sched_chans_len = 0;
  ctx->sched_chans_cap = 0;

  for (uint32_t i = 0; i < ctx->sched_select_evts_len; i++) {
    rt_select_evt_t* e = &ctx->sched_select_evts[i];
    if (!e->alive) continue;
    rt_bytes_drop(ctx, &e->payload);
    e->payload = rt_bytes_empty(ctx);
    e->alive = 0;
    e->taken = 0;
  }
  if (ctx->sched_select_evts && ctx->sched_select_evts_cap) {
    rt_free(
      ctx,
      ctx->sched_select_evts,
      ctx->sched_select_evts_cap * (uint32_t)sizeof(rt_select_evt_t),
      (uint32_t)_Alignof(rt_select_evt_t)
    );
  }
  ctx->sched_select_evts = NULL;
  ctx->sched_select_evts_len = 0;
  ctx->sched_select_evts_cap = 0;

  if (ctx->sched_timers && ctx->sched_timers_cap) {
    rt_free(
      ctx,
      ctx->sched_timers,
      ctx->sched_timers_cap * (uint32_t)sizeof(rt_timer_ev_t),
      (uint32_t)_Alignof(rt_timer_ev_t)
    );
  }
  ctx->sched_timers = NULL;
  ctx->sched_timers_len = 0;
  ctx->sched_timers_cap = 0;

  for (uint32_t i = 0; i < ctx->sched_tasks_len; i++) {
    rt_task_t* t = &ctx->sched_tasks[i];
    if (!t->alive) continue;
    if (!t->done) t->canceled = 1;
    t->done = 1;
    if (t->drop && t->fut) {
      t->drop(ctx, t->fut);
    }
    t->drop = NULL;
    t->fut = NULL;
    rt_task_out_drop(ctx, &t->out);
    t->out = rt_task_out_empty(ctx);
    t->out_taken = 0;
    t->alive = 0;
  }
  if (ctx->sched_tasks && ctx->sched_tasks_cap) {
    rt_free(
      ctx,
      ctx->sched_tasks,
      ctx->sched_tasks_cap * (uint32_t)sizeof(rt_task_t),
      (uint32_t)_Alignof(rt_task_t)
    );
  }
  ctx->sched_tasks = NULL;
  ctx->sched_tasks_len = 0;
  ctx->sched_tasks_cap = 0;
  ctx->sched_ready_head = 0;
  ctx->sched_ready_tail = 0;

  if (ctx->fs_latency_entries && ctx->fs_latency_len) {
    rt_free(
      ctx,
      ctx->fs_latency_entries,
      ctx->fs_latency_len * (uint32_t)sizeof(fs_latency_entry_t),
      (uint32_t)_Alignof(fs_latency_entry_t)
    );
  }
  ctx->fs_latency_entries = NULL;
  ctx->fs_latency_len = 0;
  rt_bytes_drop(ctx, &ctx->fs_latency_blob);
  ctx->fs_latency_blob = rt_bytes_empty(ctx);

#if X07_ENABLE_RR
  for (uint32_t i = 0; i < ctx->rr_handles_len; i++) {
    rr_handle_t* h = &ctx->rr_handles[i];
    if (!h->alive) continue;

    for (uint32_t j = 0; j < h->cassettes_len; j++) {
      rr_cassette_t* c = &h->cassettes[j];
      if (c->append_f) {
        fclose((FILE*)c->append_f);
        c->append_f = NULL;
      }
      if (c->entries && c->entries_cap) {
        rt_free(
          ctx,
          c->entries,
          c->entries_cap * (uint32_t)sizeof(rr_entry_desc_t),
          (uint32_t)_Alignof(rr_entry_desc_t)
        );
      }
      c->entries = NULL;
      c->entries_len = 0;
      c->entries_cap = 0;
      rt_bytes_drop(ctx, &c->blob);
      c->blob = rt_bytes_empty(ctx);
      rt_bytes_drop(ctx, &c->path);
      c->path = rt_bytes_empty(ctx);
      c->file_bytes = 0;
    }

    if (h->cassettes && h->cassettes_cap) {
      rt_free(
        ctx,
        h->cassettes,
        h->cassettes_cap * (uint32_t)sizeof(rr_cassette_t),
        (uint32_t)_Alignof(rr_cassette_t)
      );
    }
    h->cassettes = NULL;
    h->cassettes_len = 0;
    h->cassettes_cap = 0;
    h->alive = 0;
  }
  if (ctx->rr_handles && ctx->rr_handles_cap) {
    rt_free(
      ctx,
      ctx->rr_handles,
      ctx->rr_handles_cap * (uint32_t)sizeof(rr_handle_t),
      (uint32_t)_Alignof(rr_handle_t)
    );
  }
  ctx->rr_handles = NULL;
  ctx->rr_handles_len = 0;
  ctx->rr_handles_cap = 0;
  ctx->rr_current = 0;
#endif

  if (ctx->kv_latency_entries && ctx->kv_latency_len) {
    rt_free(
      ctx,
      ctx->kv_latency_entries,
      ctx->kv_latency_len * (uint32_t)sizeof(kv_latency_entry_t),
      (uint32_t)_Alignof(kv_latency_entry_t)
    );
  }
  ctx->kv_latency_entries = NULL;
  ctx->kv_latency_len = 0;
  rt_bytes_drop(ctx, &ctx->kv_latency_blob);
  ctx->kv_latency_blob = rt_bytes_empty(ctx);

#if X07_ENABLE_KV
  for (uint32_t i = 0; i < ctx->kv_len; i++) {
    rt_bytes_drop(ctx, &ctx->kv_items[i].key);
    rt_bytes_drop(ctx, &ctx->kv_items[i].val);
  }
  if (ctx->kv_items && ctx->kv_cap) {
    rt_free(
      ctx,
      ctx->kv_items,
      ctx->kv_cap * (uint32_t)sizeof(kv_entry_t),
      (uint32_t)_Alignof(kv_entry_t)
    );
  }
#endif
  ctx->kv_items = NULL;
  ctx->kv_len = 0;
  ctx->kv_cap = 0;

  for (uint32_t i = 0; i < ctx->map_u32_len; i++) {
    map_u32_t* m = (map_u32_t*)ctx->map_u32_items[i];
    if (!m) continue;
    if (m->keys && m->cap) {
      rt_free(ctx, m->keys, m->cap * (uint32_t)sizeof(uint32_t), (uint32_t)_Alignof(uint32_t));
    }
    if (m->vals && m->cap) {
      rt_free(ctx, m->vals, m->cap * (uint32_t)sizeof(uint32_t), (uint32_t)_Alignof(uint32_t));
    }
    rt_free(ctx, m, (uint32_t)sizeof(map_u32_t), (uint32_t)_Alignof(map_u32_t));
  }
  if (ctx->map_u32_items && ctx->map_u32_cap) {
    rt_free(
      ctx,
      ctx->map_u32_items,
      ctx->map_u32_cap * (uint32_t)sizeof(void*),
      (uint32_t)_Alignof(void*)
    );
  }
  ctx->map_u32_items = NULL;
  ctx->map_u32_len = 0;
  ctx->map_u32_cap = 0;

  for (uint32_t i = 0; i < ctx->vec_value_len; i++) {
    vec_value_t* v = (vec_value_t*)ctx->vec_value_items[i];
    if (!v) continue;
    uint32_t esz = v->ops->size;
    for (uint32_t j = 0; j < v->len; j++) {
      v->ops->drop_in_place(ctx, v->data + (j * esz));
    }
    if (v->data && v->cap) {
      if (esz != 0 && v->cap > UINT32_MAX / esz) rt_trap("vec_value.cleanup overflow");
      rt_free(ctx, v->data, v->cap * esz, v->ops->align);
    }
    rt_free(ctx, v, (uint32_t)sizeof(vec_value_t), (uint32_t)_Alignof(vec_value_t));
  }
  if (ctx->vec_value_items && ctx->vec_value_cap) {
    rt_free(
      ctx,
      ctx->vec_value_items,
      ctx->vec_value_cap * (uint32_t)sizeof(void*),
      (uint32_t)_Alignof(void*)
    );
  }
  ctx->vec_value_items = NULL;
  ctx->vec_value_len = 0;
  ctx->vec_value_cap = 0;

  for (uint32_t i = 0; i < ctx->map_value_len; i++) {
    map_value_t* m = (map_value_t*)ctx->map_value_items[i];
    if (!m) continue;
    (void)rt_map_value_clear_in_place(ctx, m);
    rt_map_value_free_arrays(ctx, m);
    rt_free(ctx, m, (uint32_t)sizeof(map_value_t), (uint32_t)_Alignof(map_value_t));
  }
  if (ctx->map_value_items && ctx->map_value_cap) {
    rt_free(
      ctx,
      ctx->map_value_items,
      ctx->map_value_cap * (uint32_t)sizeof(void*),
      (uint32_t)_Alignof(void*)
    );
  }
  ctx->map_value_items = NULL;
  ctx->map_value_len = 0;
  ctx->map_value_cap = 0;
}

static bytes_t user_app_button_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_app_effects1_v1(ctx_t* ctx, bytes_view_t input, bytes_t p0);
static bytes_t user_app_effects2_v1(ctx_t* ctx, bytes_view_t input, bytes_t p0, bytes_t p1);
static bytes_t user_app_extract_or_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2);
static bytes_t user_app_field_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2);
static bytes_t user_app_state_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4);
static bytes_t user_app_step_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_app_ui_tree_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4);
static bytes_t user_std_bytes_copy(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_fmt_u32_to_dec(ctx_t* ctx, bytes_view_t input, uint32_t p0);
static uint32_t user_std_json__cmp_member_key(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, uint32_t p2, uint32_t p3);
static vec_u8_t user_std_json__emit_member_kv(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, uint32_t p2, vec_u8_t p3);
static vec_u8_t user_std_json__emit_object_sorted(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, uint32_t p2, vec_u8_t p3);
static bytes_t user_std_json__err(ctx_t* ctx, bytes_view_t input);
static bytes_t user_std_json__extract_from_object(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2, uint32_t p3, uint32_t p4);
static bytes_t user_std_json__extract_from_value(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2, uint32_t p3, uint32_t p4);
static uint32_t user_std_json__find_byte(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__is_digit(ctx_t* ctx, bytes_view_t input, uint32_t p0);
static uint32_t user_std_json__is_hex_digit(ctx_t* ctx, bytes_view_t input, uint32_t p0);
static uint32_t user_std_json__is_ws(ctx_t* ctx, bytes_view_t input, uint32_t p0);
static uint32_t user_std_json__match_bytes_at(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2);
static bytes_t user_std_json__parse_array(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static bytes_t user_std_json__parse_lit(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2);
static bytes_t user_std_json__parse_number(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static bytes_t user_std_json__parse_object(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static bytes_t user_std_json__parse_string(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static bytes_t user_std_json__parse_value(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static bytes_view_t user_std_json__segment_at(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__skip_array(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__skip_lit(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2);
static uint32_t user_std_json__skip_number(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__skip_object(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__skip_string(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__skip_value(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__skip_ws(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__string_end_or_err(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static uint32_t user_std_json__string_escape_end_or_err(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1);
static bytes_t user_std_json_extract_path_canon_or_err(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_vec_as_bytes(ctx_t* ctx, bytes_view_t input, vec_u8_t p0);
static vec_u8_t user_std_vec_extend_bytes(ctx_t* ctx, bytes_view_t input, vec_u8_t p0, bytes_view_t p1);
static vec_u8_t user_std_vec_push(ctx_t* ctx, bytes_view_t input, vec_u8_t p0, uint32_t p1);
static vec_u8_t user_std_vec_with_capacity(ctx_t* ctx, bytes_view_t input, uint32_t p0);
static bytes_t user_std_web_ui_effects_http__json_string_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_http__request_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4);
static bytes_t user_std_web_ui_effects_http_dispatch_response_body_text_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_http_dispatch_response_status_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_http_request_get_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_std_web_ui_effects_http_request_post_json_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2);
static bytes_t user_std_web_ui_effects_nav_dispatch_href_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_nav_push_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_storage_dispatch_get_value_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_storage_dispatch_set_ok_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_storage_get_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_effects_storage_set_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_std_web_ui_patch_diff_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_std_web_ui_patch_empty_v1(ctx_t* ctx, bytes_view_t input);
static bytes_t user_std_web_ui_patch_replace_root_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_std_web_ui_runtime_frame_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4);
static bytes_t user_std_web_ui_vdom_children1_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_vdom_children2_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_std_web_ui_vdom_children3_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2);
static bytes_t user_std_web_ui_vdom_el_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4);
static bytes_t user_std_web_ui_vdom_fragment_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_std_web_ui_vdom_on_click_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);
static bytes_t user_std_web_ui_vdom_on_empty_v1(ctx_t* ctx, bytes_view_t input);
static bytes_t user_std_web_ui_vdom_props_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2);
static bytes_t user_std_web_ui_vdom_text_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1);
static bytes_t user_std_web_ui_vdom_tree_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0);

#line 1 "app.x07.json"
static bytes_t user_app_button_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_3[] = "{}";
    t_view_2 = rt_view_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(2));
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_5[] = "[]";
    t_view_4 = rt_view_from_literal(ctx, (const uint8_t*)lit_5, UINT32_C(2));
    bytes_view_t t_view_6 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_7[] = "{}";
    t_view_6 = rt_view_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(2));
    v_1 = user_std_web_ui_vdom_props_v1(ctx, input, t_view_2, t_view_4, t_view_6);
    bytes_t v_8 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_9 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_9 = p0;
    v_8 = user_std_web_ui_vdom_on_click_v1(ctx, input, t_view_9);
    bytes_t v_10 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_11 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_11 = p0;
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p1;
    v_10 = user_std_web_ui_vdom_text_v1(ctx, input, t_view_11, t_view_12);
    bytes_t v_13 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_14 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_14 = rt_bytes_view(ctx, v_10);
    v_13 = user_std_web_ui_vdom_children1_v1(ctx, input, t_view_14);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_15 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_16[] = "\"button\"";
    t_view_15 = rt_view_from_literal(ctx, (const uint8_t*)lit_16, UINT32_C(8));
    bytes_view_t t_view_17 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_17 = p0;
    bytes_view_t t_view_18 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_18 = rt_bytes_view(ctx, v_1);
    bytes_view_t t_view_19 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_19 = rt_bytes_view(ctx, v_8);
    bytes_view_t t_view_20 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_20 = rt_bytes_view(ctx, v_13);
    out = user_std_web_ui_vdom_el_v1(ctx, input, t_view_15, t_view_17, t_view_18, t_view_19, t_view_20);
    rt_bytes_drop(ctx, &v_13);
    rt_bytes_drop(ctx, &v_10);
    rt_bytes_drop(ctx, &v_8);
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "app.x07.json"
static bytes_t user_app_effects1_v1(ctx_t* ctx, bytes_view_t input, bytes_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "[";
  t_bytes_2 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(1));
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  t_bytes_4 = p0;
  p0 = rt_bytes_empty(ctx);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_4);
  bytes_t t_bytes_5 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_6[] = "]";
  t_bytes_5 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_6, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_5);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_5);
  rt_bytes_drop(ctx, &p0);
  return out;
}

#line 1 "app.x07.json"
static bytes_t user_app_effects2_v1(ctx_t* ctx, bytes_view_t input, bytes_t p0, bytes_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_5[] = "[";
  t_bytes_4 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_5, UINT32_C(1));
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  t_bytes_6 = p0;
  p0 = rt_bytes_empty(ctx);
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_6);
  bytes_t t_bytes_7 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_8[] = ",";
  t_bytes_7 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_8, UINT32_C(1));
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_7);
  bytes_t t_bytes_9 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  t_bytes_9 = p1;
  p1 = rt_bytes_empty(ctx);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_9);
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_11[] = "]";
  t_bytes_10 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_11, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_7);
  rt_bytes_drop(ctx, &t_bytes_9);
  rt_bytes_drop(ctx, &p0);
  rt_bytes_drop(ctx, &p1);
  return out;
}

#line 1 "app.x07.json"
static bytes_t user_app_extract_or_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    bytes_view_t t_view_3 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_3 = p1;
    v_1 = user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx, input, t_view_2, t_view_3);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_5 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_5 = rt_bytes_view(ctx, v_1);
    bytes_view_t t_view_6 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_7[] = "ERR";
    t_view_6 = rt_view_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(3));
    t_i32_4 = rt_view_eq(ctx, t_view_5, t_view_6);
    if (t_i32_4 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      bytes_view_t t_view_8 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      t_view_8 = p2;
      out = rt_view_to_bytes(ctx, t_view_8);
    } else {
      rt_fuel(ctx, 1);
      out = v_1;
      v_1 = rt_bytes_empty(ctx);
    }
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "app.x07.json"
static bytes_t user_app_field_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_3[] = "{}";
    t_view_2 = rt_view_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(2));
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_5[] = "[]";
    t_view_4 = rt_view_from_literal(ctx, (const uint8_t*)lit_5, UINT32_C(2));
    bytes_view_t t_view_6 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_7[] = "{}";
    t_view_6 = rt_view_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(2));
    v_1 = user_std_web_ui_vdom_props_v1(ctx, input, t_view_2, t_view_4, t_view_6);
    bytes_t v_8 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    v_8 = user_std_web_ui_vdom_on_empty_v1(ctx, input);
    bytes_t v_9 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_10 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_11[] = "\"label\"";
    t_view_10 = rt_view_from_literal(ctx, (const uint8_t*)lit_11, UINT32_C(7));
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p1;
    v_9 = user_std_web_ui_vdom_text_v1(ctx, input, t_view_10, t_view_12);
    bytes_t v_13 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_14 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_15[] = "\"value\"";
    t_view_14 = rt_view_from_literal(ctx, (const uint8_t*)lit_15, UINT32_C(7));
    bytes_view_t t_view_16 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_16 = p2;
    v_13 = user_std_web_ui_vdom_text_v1(ctx, input, t_view_14, t_view_16);
    bytes_t v_17 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_18 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_18 = rt_bytes_view(ctx, v_9);
    bytes_view_t t_view_19 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_19 = rt_bytes_view(ctx, v_13);
    v_17 = user_std_web_ui_vdom_children2_v1(ctx, input, t_view_18, t_view_19);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_20 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_21[] = "\"div\"";
    t_view_20 = rt_view_from_literal(ctx, (const uint8_t*)lit_21, UINT32_C(5));
    bytes_view_t t_view_22 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_22 = p0;
    bytes_view_t t_view_23 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_23 = rt_bytes_view(ctx, v_1);
    bytes_view_t t_view_24 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_24 = rt_bytes_view(ctx, v_8);
    bytes_view_t t_view_25 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_25 = rt_bytes_view(ctx, v_17);
    out = user_std_web_ui_vdom_el_v1(ctx, input, t_view_20, t_view_22, t_view_23, t_view_24, t_view_25);
    rt_bytes_drop(ctx, &v_17);
    rt_bytes_drop(ctx, &v_9);
    rt_bytes_drop(ctx, &v_8);
    rt_bytes_drop(ctx, &v_1);
    rt_bytes_drop(ctx, &v_13);
  }
  return out;
}

#line 1 "app.x07.json"
static bytes_t user_app_state_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_5 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_7 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_8 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_9 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_11[] = "{\"route\":";
  t_bytes_10 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_11, UINT32_C(9));
  bytes_t t_bytes_12 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_13 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_13 = p0;
  t_bytes_12 = rt_view_to_bytes(ctx, t_view_13);
  t_bytes_9 = rt_bytes_concat(ctx, t_bytes_10, t_bytes_12);
  bytes_t t_bytes_14 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_15[] = ",\"display\":";
  t_bytes_14 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_15, UINT32_C(11));
  t_bytes_8 = rt_bytes_concat(ctx, t_bytes_9, t_bytes_14);
  bytes_t t_bytes_16 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_17 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_17 = p1;
  t_bytes_16 = rt_view_to_bytes(ctx, t_view_17);
  t_bytes_7 = rt_bytes_concat(ctx, t_bytes_8, t_bytes_16);
  bytes_t t_bytes_18 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_19[] = ",\"cache\":";
  t_bytes_18 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_19, UINT32_C(9));
  t_bytes_6 = rt_bytes_concat(ctx, t_bytes_7, t_bytes_18);
  bytes_t t_bytes_20 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_21 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_21 = p2;
  t_bytes_20 = rt_view_to_bytes(ctx, t_view_21);
  t_bytes_5 = rt_bytes_concat(ctx, t_bytes_6, t_bytes_20);
  bytes_t t_bytes_22 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_23[] = ",\"source\":";
  t_bytes_22 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_23, UINT32_C(10));
  t_bytes_4 = rt_bytes_concat(ctx, t_bytes_5, t_bytes_22);
  bytes_t t_bytes_24 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_25 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_25 = p3;
  t_bytes_24 = rt_view_to_bytes(ctx, t_view_25);
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_24);
  bytes_t t_bytes_26 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_27[] = ",\"error\":";
  t_bytes_26 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_27, UINT32_C(9));
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_26);
  bytes_t t_bytes_28 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_29 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_29 = p4;
  t_bytes_28 = rt_view_to_bytes(ctx, t_view_29);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_28);
  bytes_t t_bytes_30 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_31[] = "}";
  t_bytes_30 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_31, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_30);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_14);
  rt_bytes_drop(ctx, &t_bytes_16);
  rt_bytes_drop(ctx, &t_bytes_18);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_20);
  rt_bytes_drop(ctx, &t_bytes_22);
  rt_bytes_drop(ctx, &t_bytes_24);
  rt_bytes_drop(ctx, &t_bytes_26);
  rt_bytes_drop(ctx, &t_bytes_28);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_30);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_5);
  rt_bytes_drop(ctx, &t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_7);
  rt_bytes_drop(ctx, &t_bytes_8);
  rt_bytes_drop(ctx, &t_bytes_9);
  return out;
}

#line 1 "app.x07.json"
static bytes_t user_app_step_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_2[] = "\"\"";
    v_1 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_2, UINT32_C(2));
    bytes_t v_3 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_4[] = "\"/\"";
    v_3 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_4, UINT32_C(3));
    bytes_t v_5 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_6[] = "\"boot\"";
    v_5 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_6, UINT32_C(6));
    bytes_t v_7 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_8[] = "\"nav\"";
    v_7 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_8, UINT32_C(5));
    bytes_t v_9 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_10[] = "\"storage\"";
    v_9 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_10, UINT32_C(9));
    bytes_t v_11 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_12[] = "\"http\"";
    v_11 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_12, UINT32_C(6));
    bytes_t v_13 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_14[] = "\"app.cache\"";
    v_13 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_14, UINT32_C(11));
    bytes_t v_15 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_16 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_16 = p0;
    bytes_view_t t_view_17 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_18[] = "state.route";
    t_view_17 = rt_view_from_literal(ctx, (const uint8_t*)lit_18, UINT32_C(11));
    bytes_view_t t_view_19 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_19 = rt_bytes_view(ctx, v_3);
    v_15 = user_app_extract_or_v1(ctx, input, t_view_16, t_view_17, t_view_19);
    bytes_t v_20 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_21 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_21 = p0;
    bytes_view_t t_view_22 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_23[] = "state.display";
    t_view_22 = rt_view_from_literal(ctx, (const uint8_t*)lit_23, UINT32_C(13));
    bytes_view_t t_view_24 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_24 = rt_bytes_view(ctx, v_1);
    v_20 = user_app_extract_or_v1(ctx, input, t_view_21, t_view_22, t_view_24);
    bytes_t v_25 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_26 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_26 = p0;
    bytes_view_t t_view_27 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_28[] = "state.cache";
    t_view_27 = rt_view_from_literal(ctx, (const uint8_t*)lit_28, UINT32_C(11));
    bytes_view_t t_view_29 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_29 = rt_bytes_view(ctx, v_1);
    v_25 = user_app_extract_or_v1(ctx, input, t_view_26, t_view_27, t_view_29);
    bytes_t v_30 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_31 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_31 = p0;
    bytes_view_t t_view_32 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_33[] = "state.source";
    t_view_32 = rt_view_from_literal(ctx, (const uint8_t*)lit_33, UINT32_C(12));
    bytes_view_t t_view_34 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_34 = rt_bytes_view(ctx, v_5);
    v_30 = user_app_extract_or_v1(ctx, input, t_view_31, t_view_32, t_view_34);
    bytes_t v_35 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_36 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_36 = p0;
    bytes_view_t t_view_37 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_38[] = "state.error";
    t_view_37 = rt_view_from_literal(ctx, (const uint8_t*)lit_38, UINT32_C(11));
    bytes_view_t t_view_39 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_39 = rt_bytes_view(ctx, v_1);
    v_35 = user_app_extract_or_v1(ctx, input, t_view_36, t_view_37, t_view_39);
    bytes_t v_40 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_41 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_41 = p0;
    bytes_view_t t_view_42 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_43[] = "event.type";
    t_view_42 = rt_view_from_literal(ctx, (const uint8_t*)lit_43, UINT32_C(10));
    bytes_view_t t_view_44 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_45[] = "\"init\"";
    t_view_44 = rt_view_from_literal(ctx, (const uint8_t*)lit_45, UINT32_C(6));
    v_40 = user_app_extract_or_v1(ctx, input, t_view_41, t_view_42, t_view_44);
    bytes_t v_46 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_47 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_47 = p0;
    bytes_view_t t_view_48 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_49[] = "event.target";
    t_view_48 = rt_view_from_literal(ctx, (const uint8_t*)lit_49, UINT32_C(12));
    bytes_view_t t_view_50 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_50 = rt_bytes_view(ctx, v_1);
    v_46 = user_app_extract_or_v1(ctx, input, t_view_47, t_view_48, t_view_50);
    bytes_t v_51 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_52 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_52 = p0;
    v_51 = user_std_web_ui_effects_nav_dispatch_href_canon_or_err_v1(ctx, input, t_view_52);
    uint32_t v_53 = UINT32_C(0);
    rt_fuel(ctx, 1);
    {
      bytes_t v_54 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      bytes_view_t t_view_55 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      t_view_55 = rt_bytes_view(ctx, v_51);
      v_54 = rt_view_to_bytes(ctx, t_view_55);
      rt_fuel(ctx, 1);
      uint32_t t_i32_56 = UINT32_C(0);
      rt_fuel(ctx, 1);
      bytes_view_t t_view_57 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      t_view_57 = rt_bytes_view(ctx, v_54);
      bytes_view_t t_view_58 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      static const char lit_59[] = "ERR";
      t_view_58 = rt_view_from_literal(ctx, (const uint8_t*)lit_59, UINT32_C(3));
      t_i32_56 = rt_view_eq(ctx, t_view_57, t_view_58);
      if (t_i32_56 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        v_53 = UINT32_C(0);
      } else {
        rt_fuel(ctx, 1);
        uint32_t t_i32_60 = UINT32_C(0);
        rt_fuel(ctx, 1);
        bytes_view_t t_view_61 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_61 = rt_bytes_view(ctx, v_51);
        bytes_view_t t_view_62 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_62 = rt_bytes_view(ctx, v_15);
        t_i32_60 = rt_view_eq(ctx, t_view_61, t_view_62);
        if (t_i32_60 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          v_53 = UINT32_C(0);
        } else {
          rt_fuel(ctx, 1);
          v_53 = UINT32_C(1);
        }
      }
      rt_bytes_drop(ctx, &v_54);
    }
    bytes_t v_63 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_64 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_64 = p0;
    v_63 = user_std_web_ui_effects_http_dispatch_response_status_canon_or_err_v1(ctx, input, t_view_64);
    bytes_t v_65 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_66 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_66 = p0;
    v_65 = user_std_web_ui_effects_http_dispatch_response_body_text_canon_or_err_v1(ctx, input, t_view_66);
    uint32_t v_67 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_68 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_69 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_69 = rt_bytes_view(ctx, v_63);
    bytes_view_t t_view_70 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_71[] = "ERR";
    t_view_70 = rt_view_from_literal(ctx, (const uint8_t*)lit_71, UINT32_C(3));
    t_i32_68 = rt_view_eq(ctx, t_view_69, t_view_70);
    if (t_i32_68 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_67 = UINT32_C(0);
    } else {
      rt_fuel(ctx, 1);
      v_67 = UINT32_C(1);
    }
    uint32_t v_72 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_73 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_74 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_74 = rt_bytes_view(ctx, v_65);
    bytes_view_t t_view_75 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_76[] = "ERR";
    t_view_75 = rt_view_from_literal(ctx, (const uint8_t*)lit_76, UINT32_C(3));
    t_i32_73 = rt_view_eq(ctx, t_view_74, t_view_75);
    if (t_i32_73 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_72 = UINT32_C(0);
    } else {
      rt_fuel(ctx, 1);
      v_72 = UINT32_C(1);
    }
    uint32_t v_77 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_78 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_78 = v_67;
    if (t_i32_78 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_77 = UINT32_C(1);
    } else {
      rt_fuel(ctx, 1);
      v_77 = v_72;
    }
    bytes_t v_79 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    uint32_t t_i32_80 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_80 = v_67;
    if (t_i32_80 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      bytes_view_t t_view_81 = rt_view_empty(ctx);
      t_view_81 = rt_bytes_view(ctx, v_63);
      v_79 = user_std_bytes_copy(ctx, input, t_view_81);
    } else {
      rt_fuel(ctx, 1);
      static const char lit_82[] = "0";
      v_79 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_82, UINT32_C(1));
    }
    bytes_t v_83 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    uint32_t t_i32_84 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_84 = v_72;
    if (t_i32_84 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      bytes_view_t t_view_85 = rt_view_empty(ctx);
      t_view_85 = rt_bytes_view(ctx, v_65);
      v_83 = user_std_bytes_copy(ctx, input, t_view_85);
    } else {
      rt_fuel(ctx, 1);
      static const char lit_86[] = "\"\"";
      v_83 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_86, UINT32_C(2));
    }
    uint32_t v_87 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_87 = v_77;
    bytes_t v_88 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_89 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_89 = p0;
    v_88 = user_std_web_ui_effects_storage_dispatch_get_value_canon_or_err_v1(ctx, input, t_view_89);
    uint32_t v_90 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_91 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_92 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_92 = rt_bytes_view(ctx, v_88);
    bytes_view_t t_view_93 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_94[] = "ERR";
    t_view_93 = rt_view_from_literal(ctx, (const uint8_t*)lit_94, UINT32_C(3));
    t_i32_91 = rt_view_eq(ctx, t_view_92, t_view_93);
    if (t_i32_91 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_90 = UINT32_C(0);
    } else {
      rt_fuel(ctx, 1);
      v_90 = UINT32_C(1);
    }
    bytes_t v_95 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_96 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_96 = p0;
    v_95 = user_std_web_ui_effects_storage_dispatch_set_ok_canon_or_err_v1(ctx, input, t_view_96);
    uint32_t v_97 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_98 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_99 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_99 = rt_bytes_view(ctx, v_95);
    bytes_view_t t_view_100 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_101[] = "ERR";
    t_view_100 = rt_view_from_literal(ctx, (const uint8_t*)lit_101, UINT32_C(3));
    t_i32_98 = rt_view_eq(ctx, t_view_99, t_view_100);
    if (t_i32_98 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_97 = UINT32_C(0);
    } else {
      rt_fuel(ctx, 1);
      v_97 = UINT32_C(1);
    }
    uint32_t v_102 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_103 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_103 = v_87;
    if (t_i32_103 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      {
        bytes_t v_104 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        bytes_view_t t_view_105 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_105 = rt_bytes_view(ctx, v_79);
        v_104 = rt_view_to_bytes(ctx, t_view_105);
        rt_fuel(ctx, 1);
        uint32_t t_i32_106 = UINT32_C(0);
        rt_fuel(ctx, 1);
        bytes_view_t t_view_107 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_107 = rt_bytes_view(ctx, v_104);
        bytes_view_t t_view_108 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        static const char lit_109[] = "200";
        t_view_108 = rt_view_from_literal(ctx, (const uint8_t*)lit_109, UINT32_C(3));
        t_i32_106 = rt_view_eq(ctx, t_view_107, t_view_108);
        if (t_i32_106 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          v_102 = UINT32_C(1);
        } else {
          rt_fuel(ctx, 1);
          uint32_t t_i32_110 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_111 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_111 = rt_bytes_view(ctx, v_79);
          bytes_view_t t_view_112 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          static const char lit_113[] = "201";
          t_view_112 = rt_view_from_literal(ctx, (const uint8_t*)lit_113, UINT32_C(3));
          t_i32_110 = rt_view_eq(ctx, t_view_111, t_view_112);
          if (t_i32_110 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            v_102 = UINT32_C(1);
          } else {
            rt_fuel(ctx, 1);
            v_102 = UINT32_C(0);
          }
        }
        rt_bytes_drop(ctx, &v_104);
      }
    } else {
      rt_fuel(ctx, 1);
      v_102 = UINT32_C(0);
    }
    bytes_t v_114 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    uint32_t t_i32_115 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_115 = v_90;
    if (t_i32_115 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_114 = v_88;
      v_88 = rt_bytes_empty(ctx);
    } else {
      rt_fuel(ctx, 1);
      static const char lit_116[] = "null";
      v_114 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_116, UINT32_C(4));
    }
    bytes_t v_117 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_118 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_118 = rt_bytes_view(ctx, v_114);
    v_117 = rt_view_to_bytes(ctx, t_view_118);
    bytes_t v_119 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_120 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_120 = rt_bytes_view(ctx, v_114);
    v_119 = rt_view_to_bytes(ctx, t_view_120);
    bytes_t v_121 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_122 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_122 = rt_bytes_view(ctx, v_114);
    v_121 = rt_view_to_bytes(ctx, t_view_122);
    uint32_t v_123 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_124 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_124 = v_87;
    if (t_i32_124 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_123 = UINT32_C(0);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_125 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_125 = v_90;
      if (t_i32_125 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        uint32_t t_i32_126 = UINT32_C(0);
        rt_fuel(ctx, 1);
        bytes_view_t t_view_127 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_127 = rt_bytes_view(ctx, v_25);
        bytes_view_t t_view_128 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_128 = rt_bytes_view(ctx, v_1);
        t_i32_126 = rt_view_eq(ctx, t_view_127, t_view_128);
        if (t_i32_126 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          uint32_t t_i32_129 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_130 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_130 = rt_bytes_view(ctx, v_117);
          bytes_view_t t_view_131 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          static const char lit_132[] = "null";
          t_view_131 = rt_view_from_literal(ctx, (const uint8_t*)lit_132, UINT32_C(4));
          t_i32_129 = rt_view_eq(ctx, t_view_130, t_view_131);
          if (t_i32_129 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            v_123 = UINT32_C(0);
          } else {
            rt_fuel(ctx, 1);
            v_123 = UINT32_C(1);
          }
        } else {
          rt_fuel(ctx, 1);
          v_123 = UINT32_C(0);
        }
      } else {
        rt_fuel(ctx, 1);
        v_123 = UINT32_C(0);
      }
    }
    uint32_t v_133 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_134 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_134 = v_87;
    if (t_i32_134 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_133 = UINT32_C(1);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_135 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_135 = v_123;
      if (t_i32_135 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        v_133 = UINT32_C(1);
      } else {
        rt_fuel(ctx, 1);
        uint32_t t_i32_136 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_136 = v_97;
        if (t_i32_136 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          v_133 = UINT32_C(1);
        } else {
          rt_fuel(ctx, 1);
          uint32_t t_i32_137 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_137 = v_53;
          if (t_i32_137 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            v_133 = UINT32_C(1);
          } else {
            rt_fuel(ctx, 1);
            v_133 = UINT32_C(0);
          }
        }
      }
    }
    bytes_t v_138 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_139 = rt_view_empty(ctx);
    t_view_139 = rt_bytes_view(ctx, v_15);
    v_138 = user_std_bytes_copy(ctx, input, t_view_139);
    bytes_t v_140 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_141 = rt_view_empty(ctx);
    t_view_141 = rt_bytes_view(ctx, v_20);
    v_140 = user_std_bytes_copy(ctx, input, t_view_141);
    bytes_t v_142 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_143 = rt_view_empty(ctx);
    t_view_143 = rt_bytes_view(ctx, v_25);
    v_142 = user_std_bytes_copy(ctx, input, t_view_143);
    bytes_t v_144 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_145 = rt_view_empty(ctx);
    t_view_145 = rt_bytes_view(ctx, v_30);
    v_144 = user_std_bytes_copy(ctx, input, t_view_145);
    bytes_t v_146 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_147 = rt_view_empty(ctx);
    t_view_147 = rt_bytes_view(ctx, v_35);
    v_146 = user_std_bytes_copy(ctx, input, t_view_147);
    bytes_t v_148 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_149[] = "[]";
    v_148 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_149, UINT32_C(2));
    uint32_t t_i32_150 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_150 = v_53;
    if (t_i32_150 != UINT32_C(0)) {
      {
        bytes_t t_set_151 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_set_151 = v_51;
        v_51 = rt_bytes_empty(ctx);
        rt_bytes_drop(ctx, &v_138);
        v_138 = t_set_151;
        t_set_151 = rt_bytes_empty(ctx);
        bytes_t t_set_152 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_set_152 = v_7;
        v_7 = rt_bytes_empty(ctx);
        rt_bytes_drop(ctx, &v_144);
        v_144 = t_set_152;
        t_set_152 = rt_bytes_empty(ctx);
        uint32_t t_i32_153 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_153 = UINT32_C(0);
      }
    } else {
      uint32_t t_i32_154 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_154 = UINT32_C(0);
    }
    uint32_t t_i32_155 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_155 = v_123;
    if (t_i32_155 != UINT32_C(0)) {
      {
        bytes_t t_set_156 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_set_156 = v_119;
        v_119 = rt_bytes_empty(ctx);
        rt_bytes_drop(ctx, &v_140);
        v_140 = t_set_156;
        t_set_156 = rt_bytes_empty(ctx);
        bytes_t t_set_157 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_set_157 = v_121;
        v_121 = rt_bytes_empty(ctx);
        rt_bytes_drop(ctx, &v_142);
        v_142 = t_set_157;
        t_set_157 = rt_bytes_empty(ctx);
        bytes_t t_set_158 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        static const char lit_159[] = "\"\"";
        t_set_158 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_159, UINT32_C(2));
        rt_bytes_drop(ctx, &v_146);
        v_146 = t_set_158;
        t_set_158 = rt_bytes_empty(ctx);
        bytes_t t_set_160 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_set_160 = v_9;
        v_9 = rt_bytes_empty(ctx);
        rt_bytes_drop(ctx, &v_144);
        v_144 = t_set_160;
        t_set_160 = rt_bytes_empty(ctx);
        uint32_t t_i32_161 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_161 = UINT32_C(0);
      }
    } else {
      uint32_t t_i32_162 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_162 = UINT32_C(0);
    }
    uint32_t t_i32_163 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_163 = v_87;
    if (t_i32_163 != UINT32_C(0)) {
      {
        bytes_t t_set_164 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        bytes_view_t t_view_165 = rt_view_empty(ctx);
        t_view_165 = rt_bytes_view(ctx, v_83);
        t_set_164 = user_std_bytes_copy(ctx, input, t_view_165);
        rt_bytes_drop(ctx, &v_140);
        v_140 = t_set_164;
        t_set_164 = rt_bytes_empty(ctx);
        uint32_t t_i32_166 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_166 = v_102;
        if (t_i32_166 != UINT32_C(0)) {
          {
            bytes_t t_set_167 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_168 = rt_view_empty(ctx);
            t_view_168 = rt_bytes_view(ctx, v_83);
            t_set_167 = user_std_bytes_copy(ctx, input, t_view_168);
            rt_bytes_drop(ctx, &v_142);
            v_142 = t_set_167;
            t_set_167 = rt_bytes_empty(ctx);
            bytes_t t_set_169 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            static const char lit_170[] = "\"\"";
            t_set_169 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_170, UINT32_C(2));
            rt_bytes_drop(ctx, &v_146);
            v_146 = t_set_169;
            t_set_169 = rt_bytes_empty(ctx);
            bytes_t t_set_171 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            bytes_t t_bytes_172 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_173 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_173 = rt_bytes_view(ctx, v_13);
            bytes_view_t t_view_174 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_174 = rt_bytes_view(ctx, v_83);
            t_bytes_172 = user_std_web_ui_effects_storage_set_v1(ctx, input, t_view_173, t_view_174);
            t_set_171 = user_app_effects1_v1(ctx, input, t_bytes_172);
            t_bytes_172 = rt_bytes_empty(ctx);
            rt_bytes_drop(ctx, &v_148);
            v_148 = t_set_171;
            t_set_171 = rt_bytes_empty(ctx);
            uint32_t t_i32_175 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_175 = UINT32_C(0);
            rt_bytes_drop(ctx, &t_bytes_172);
          }
        } else {
          {
            bytes_t t_set_176 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_set_176 = v_83;
            v_83 = rt_bytes_empty(ctx);
            rt_bytes_drop(ctx, &v_146);
            v_146 = t_set_176;
            t_set_176 = rt_bytes_empty(ctx);
            uint32_t t_i32_177 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_177 = UINT32_C(0);
          }
        }
        bytes_t t_set_178 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_set_178 = v_11;
        v_11 = rt_bytes_empty(ctx);
        rt_bytes_drop(ctx, &v_144);
        v_144 = t_set_178;
        t_set_178 = rt_bytes_empty(ctx);
        uint32_t t_i32_179 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_179 = UINT32_C(0);
      }
    } else {
      uint32_t t_i32_180 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_180 = UINT32_C(0);
    }
    uint32_t t_i32_181 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_181 = v_133;
    if (t_i32_181 != UINT32_C(0)) {
      uint32_t t_i32_182 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_182 = UINT32_C(0);
    } else {
      {
        bytes_t v_183 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        bytes_view_t t_view_184 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_184 = rt_bytes_view(ctx, v_40);
        v_183 = rt_view_to_bytes(ctx, t_view_184);
        uint32_t t_i32_185 = UINT32_C(0);
        rt_fuel(ctx, 1);
        bytes_view_t t_view_186 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_186 = rt_bytes_view(ctx, v_183);
        bytes_view_t t_view_187 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        static const char lit_188[] = "\"init\"";
        t_view_187 = rt_view_from_literal(ctx, (const uint8_t*)lit_188, UINT32_C(6));
        t_i32_185 = rt_view_eq(ctx, t_view_186, t_view_187);
        if (t_i32_185 != UINT32_C(0)) {
          {
            bytes_t t_set_189 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            static const char lit_190[] = "\"req_init\"";
            t_set_189 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_190, UINT32_C(10));
            rt_bytes_drop(ctx, &v_144);
            v_144 = t_set_189;
            t_set_189 = rt_bytes_empty(ctx);
            bytes_t t_set_191 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            static const char lit_192[] = "\"\"";
            t_set_191 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_192, UINT32_C(2));
            rt_bytes_drop(ctx, &v_146);
            v_146 = t_set_191;
            t_set_191 = rt_bytes_empty(ctx);
            bytes_t t_set_193 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            bytes_t t_bytes_194 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_195 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_195 = rt_bytes_view(ctx, v_13);
            t_bytes_194 = user_std_web_ui_effects_storage_get_v1(ctx, input, t_view_195);
            bytes_t t_bytes_196 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_197 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            static const char lit_198[] = "\"req_init\"";
            t_view_197 = rt_view_from_literal(ctx, (const uint8_t*)lit_198, UINT32_C(10));
            bytes_view_t t_view_199 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            static const char lit_200[] = "\"/api/projects\"";
            t_view_199 = rt_view_from_literal(ctx, (const uint8_t*)lit_200, UINT32_C(15));
            t_bytes_196 = user_std_web_ui_effects_http_request_get_v1(ctx, input, t_view_197, t_view_199);
            t_set_193 = user_app_effects2_v1(ctx, input, t_bytes_194, t_bytes_196);
            t_bytes_194 = rt_bytes_empty(ctx);
            t_bytes_196 = rt_bytes_empty(ctx);
            rt_bytes_drop(ctx, &v_148);
            v_148 = t_set_193;
            t_set_193 = rt_bytes_empty(ctx);
            uint32_t t_i32_201 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_201 = UINT32_C(0);
            rt_bytes_drop(ctx, &t_bytes_194);
            rt_bytes_drop(ctx, &t_bytes_196);
          }
        } else {
          uint32_t t_i32_202 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_203 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_203 = rt_bytes_view(ctx, v_40);
          bytes_view_t t_view_204 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          static const char lit_205[] = "\"click\"";
          t_view_204 = rt_view_from_literal(ctx, (const uint8_t*)lit_205, UINT32_C(7));
          t_i32_202 = rt_view_eq(ctx, t_view_203, t_view_204);
          if (t_i32_202 != UINT32_C(0)) {
            {
              {
                bytes_t v_206 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                bytes_view_t t_view_207 = rt_view_empty(ctx);
                rt_fuel(ctx, 1);
                t_view_207 = rt_bytes_view(ctx, v_46);
                v_206 = rt_view_to_bytes(ctx, t_view_207);
                uint32_t t_i32_208 = UINT32_C(0);
                rt_fuel(ctx, 1);
                bytes_view_t t_view_209 = rt_view_empty(ctx);
                rt_fuel(ctx, 1);
                t_view_209 = rt_bytes_view(ctx, v_206);
                bytes_view_t t_view_210 = rt_view_empty(ctx);
                rt_fuel(ctx, 1);
                static const char lit_211[] = "\"refresh\"";
                t_view_210 = rt_view_from_literal(ctx, (const uint8_t*)lit_211, UINT32_C(9));
                t_i32_208 = rt_view_eq(ctx, t_view_209, t_view_210);
                if (t_i32_208 != UINT32_C(0)) {
                  {
                    bytes_t t_set_212 = rt_bytes_empty(ctx);
                    rt_fuel(ctx, 1);
                    static const char lit_213[] = "\"req_refresh\"";
                    t_set_212 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_213, UINT32_C(13));
                    rt_bytes_drop(ctx, &v_144);
                    v_144 = t_set_212;
                    t_set_212 = rt_bytes_empty(ctx);
                    bytes_t t_set_214 = rt_bytes_empty(ctx);
                    rt_fuel(ctx, 1);
                    static const char lit_215[] = "\"\"";
                    t_set_214 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_215, UINT32_C(2));
                    rt_bytes_drop(ctx, &v_146);
                    v_146 = t_set_214;
                    t_set_214 = rt_bytes_empty(ctx);
                    bytes_t t_set_216 = rt_bytes_empty(ctx);
                    rt_fuel(ctx, 1);
                    bytes_t t_bytes_217 = rt_bytes_empty(ctx);
                    rt_fuel(ctx, 1);
                    bytes_view_t t_view_218 = rt_view_empty(ctx);
                    rt_fuel(ctx, 1);
                    static const char lit_219[] = "\"req_refresh\"";
                    t_view_218 = rt_view_from_literal(ctx, (const uint8_t*)lit_219, UINT32_C(13));
                    bytes_view_t t_view_220 = rt_view_empty(ctx);
                    rt_fuel(ctx, 1);
                    static const char lit_221[] = "\"/api/projects\"";
                    t_view_220 = rt_view_from_literal(ctx, (const uint8_t*)lit_221, UINT32_C(15));
                    t_bytes_217 = user_std_web_ui_effects_http_request_get_v1(ctx, input, t_view_218, t_view_220);
                    t_set_216 = user_app_effects1_v1(ctx, input, t_bytes_217);
                    t_bytes_217 = rt_bytes_empty(ctx);
                    rt_bytes_drop(ctx, &v_148);
                    v_148 = t_set_216;
                    t_set_216 = rt_bytes_empty(ctx);
                    uint32_t t_i32_222 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_222 = UINT32_C(0);
                    rt_bytes_drop(ctx, &t_bytes_217);
                  }
                } else {
                  {
                    bytes_t v_223 = rt_bytes_empty(ctx);
                    rt_fuel(ctx, 1);
                    bytes_view_t t_view_224 = rt_view_empty(ctx);
                    rt_fuel(ctx, 1);
                    t_view_224 = rt_bytes_view(ctx, v_46);
                    v_223 = rt_view_to_bytes(ctx, t_view_224);
                    uint32_t t_i32_225 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    bytes_view_t t_view_226 = rt_view_empty(ctx);
                    rt_fuel(ctx, 1);
                    t_view_226 = rt_bytes_view(ctx, v_223);
                    bytes_view_t t_view_227 = rt_view_empty(ctx);
                    rt_fuel(ctx, 1);
                    static const char lit_228[] = "\"create\"";
                    t_view_227 = rt_view_from_literal(ctx, (const uint8_t*)lit_228, UINT32_C(8));
                    t_i32_225 = rt_view_eq(ctx, t_view_226, t_view_227);
                    if (t_i32_225 != UINT32_C(0)) {
                      {
                        bytes_t t_set_229 = rt_bytes_empty(ctx);
                        rt_fuel(ctx, 1);
                        static const char lit_230[] = "\"req_create\"";
                        t_set_229 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_230, UINT32_C(12));
                        rt_bytes_drop(ctx, &v_144);
                        v_144 = t_set_229;
                        t_set_229 = rt_bytes_empty(ctx);
                        bytes_t t_set_231 = rt_bytes_empty(ctx);
                        rt_fuel(ctx, 1);
                        static const char lit_232[] = "\"\"";
                        t_set_231 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_232, UINT32_C(2));
                        rt_bytes_drop(ctx, &v_146);
                        v_146 = t_set_231;
                        t_set_231 = rt_bytes_empty(ctx);
                        bytes_t t_set_233 = rt_bytes_empty(ctx);
                        rt_fuel(ctx, 1);
                        bytes_t t_bytes_234 = rt_bytes_empty(ctx);
                        rt_fuel(ctx, 1);
                        bytes_view_t t_view_235 = rt_view_empty(ctx);
                        rt_fuel(ctx, 1);
                        static const char lit_236[] = "\"req_create\"";
                        t_view_235 = rt_view_from_literal(ctx, (const uint8_t*)lit_236, UINT32_C(12));
                        bytes_view_t t_view_237 = rt_view_empty(ctx);
                        rt_fuel(ctx, 1);
                        static const char lit_238[] = "\"/api/projects\"";
                        t_view_237 = rt_view_from_literal(ctx, (const uint8_t*)lit_238, UINT32_C(15));
                        bytes_view_t t_view_239 = rt_view_empty(ctx);
                        rt_fuel(ctx, 1);
                        static const char lit_240[] = "{\"name\":\"Atlas\"}";
                        t_view_239 = rt_view_from_literal(ctx, (const uint8_t*)lit_240, UINT32_C(16));
                        t_bytes_234 = user_std_web_ui_effects_http_request_post_json_v1(ctx, input, t_view_235, t_view_237, t_view_239);
                        t_set_233 = user_app_effects1_v1(ctx, input, t_bytes_234);
                        t_bytes_234 = rt_bytes_empty(ctx);
                        rt_bytes_drop(ctx, &v_148);
                        v_148 = t_set_233;
                        t_set_233 = rt_bytes_empty(ctx);
                        uint32_t t_i32_241 = UINT32_C(0);
                        rt_fuel(ctx, 1);
                        t_i32_241 = UINT32_C(0);
                        rt_bytes_drop(ctx, &t_bytes_234);
                      }
                    } else {
                      {
                        bytes_t v_242 = rt_bytes_empty(ctx);
                        rt_fuel(ctx, 1);
                        bytes_view_t t_view_243 = rt_view_empty(ctx);
                        rt_fuel(ctx, 1);
                        t_view_243 = rt_bytes_view(ctx, v_46);
                        v_242 = rt_view_to_bytes(ctx, t_view_243);
                        uint32_t t_i32_244 = UINT32_C(0);
                        rt_fuel(ctx, 1);
                        bytes_view_t t_view_245 = rt_view_empty(ctx);
                        rt_fuel(ctx, 1);
                        t_view_245 = rt_bytes_view(ctx, v_242);
                        bytes_view_t t_view_246 = rt_view_empty(ctx);
                        rt_fuel(ctx, 1);
                        static const char lit_247[] = "\"invalid-create\"";
                        t_view_246 = rt_view_from_literal(ctx, (const uint8_t*)lit_247, UINT32_C(16));
                        t_i32_244 = rt_view_eq(ctx, t_view_245, t_view_246);
                        if (t_i32_244 != UINT32_C(0)) {
                          {
                            bytes_t t_set_248 = rt_bytes_empty(ctx);
                            rt_fuel(ctx, 1);
                            static const char lit_249[] = "\"req_invalid_create\"";
                            t_set_248 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_249, UINT32_C(20));
                            rt_bytes_drop(ctx, &v_144);
                            v_144 = t_set_248;
                            t_set_248 = rt_bytes_empty(ctx);
                            bytes_t t_set_250 = rt_bytes_empty(ctx);
                            rt_fuel(ctx, 1);
                            static const char lit_251[] = "\"\"";
                            t_set_250 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_251, UINT32_C(2));
                            rt_bytes_drop(ctx, &v_146);
                            v_146 = t_set_250;
                            t_set_250 = rt_bytes_empty(ctx);
                            bytes_t t_set_252 = rt_bytes_empty(ctx);
                            rt_fuel(ctx, 1);
                            bytes_t t_bytes_253 = rt_bytes_empty(ctx);
                            rt_fuel(ctx, 1);
                            bytes_view_t t_view_254 = rt_view_empty(ctx);
                            rt_fuel(ctx, 1);
                            static const char lit_255[] = "\"req_invalid_create\"";
                            t_view_254 = rt_view_from_literal(ctx, (const uint8_t*)lit_255, UINT32_C(20));
                            bytes_view_t t_view_256 = rt_view_empty(ctx);
                            rt_fuel(ctx, 1);
                            static const char lit_257[] = "\"/api/projects\"";
                            t_view_256 = rt_view_from_literal(ctx, (const uint8_t*)lit_257, UINT32_C(15));
                            bytes_view_t t_view_258 = rt_view_empty(ctx);
                            rt_fuel(ctx, 1);
                            static const char lit_259[] = "{\"name\":\"\"}";
                            t_view_258 = rt_view_from_literal(ctx, (const uint8_t*)lit_259, UINT32_C(11));
                            t_bytes_253 = user_std_web_ui_effects_http_request_post_json_v1(ctx, input, t_view_254, t_view_256, t_view_258);
                            t_set_252 = user_app_effects1_v1(ctx, input, t_bytes_253);
                            t_bytes_253 = rt_bytes_empty(ctx);
                            rt_bytes_drop(ctx, &v_148);
                            v_148 = t_set_252;
                            t_set_252 = rt_bytes_empty(ctx);
                            uint32_t t_i32_260 = UINT32_C(0);
                            rt_fuel(ctx, 1);
                            t_i32_260 = UINT32_C(0);
                            rt_bytes_drop(ctx, &t_bytes_253);
                          }
                        } else {
                          {
                            bytes_t v_261 = rt_bytes_empty(ctx);
                            rt_fuel(ctx, 1);
                            bytes_view_t t_view_262 = rt_view_empty(ctx);
                            rt_fuel(ctx, 1);
                            t_view_262 = rt_bytes_view(ctx, v_46);
                            v_261 = rt_view_to_bytes(ctx, t_view_262);
                            uint32_t t_i32_263 = UINT32_C(0);
                            rt_fuel(ctx, 1);
                            bytes_view_t t_view_264 = rt_view_empty(ctx);
                            rt_fuel(ctx, 1);
                            t_view_264 = rt_bytes_view(ctx, v_261);
                            bytes_view_t t_view_265 = rt_view_empty(ctx);
                            rt_fuel(ctx, 1);
                            static const char lit_266[] = "\"list\"";
                            t_view_265 = rt_view_from_literal(ctx, (const uint8_t*)lit_266, UINT32_C(6));
                            t_i32_263 = rt_view_eq(ctx, t_view_264, t_view_265);
                            if (t_i32_263 != UINT32_C(0)) {
                              {
                                bytes_t t_set_267 = rt_bytes_empty(ctx);
                                rt_fuel(ctx, 1);
                                static const char lit_268[] = "\"req_list\"";
                                t_set_267 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_268, UINT32_C(10));
                                rt_bytes_drop(ctx, &v_144);
                                v_144 = t_set_267;
                                t_set_267 = rt_bytes_empty(ctx);
                                bytes_t t_set_269 = rt_bytes_empty(ctx);
                                rt_fuel(ctx, 1);
                                static const char lit_270[] = "\"\"";
                                t_set_269 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_270, UINT32_C(2));
                                rt_bytes_drop(ctx, &v_146);
                                v_146 = t_set_269;
                                t_set_269 = rt_bytes_empty(ctx);
                                bytes_t t_set_271 = rt_bytes_empty(ctx);
                                rt_fuel(ctx, 1);
                                bytes_t t_bytes_272 = rt_bytes_empty(ctx);
                                rt_fuel(ctx, 1);
                                bytes_view_t t_view_273 = rt_view_empty(ctx);
                                rt_fuel(ctx, 1);
                                static const char lit_274[] = "\"/projects\"";
                                t_view_273 = rt_view_from_literal(ctx, (const uint8_t*)lit_274, UINT32_C(11));
                                t_bytes_272 = user_std_web_ui_effects_nav_push_v1(ctx, input, t_view_273);
                                bytes_t t_bytes_275 = rt_bytes_empty(ctx);
                                rt_fuel(ctx, 1);
                                bytes_view_t t_view_276 = rt_view_empty(ctx);
                                rt_fuel(ctx, 1);
                                static const char lit_277[] = "\"req_list\"";
                                t_view_276 = rt_view_from_literal(ctx, (const uint8_t*)lit_277, UINT32_C(10));
                                bytes_view_t t_view_278 = rt_view_empty(ctx);
                                rt_fuel(ctx, 1);
                                static const char lit_279[] = "\"/api/projects\"";
                                t_view_278 = rt_view_from_literal(ctx, (const uint8_t*)lit_279, UINT32_C(15));
                                t_bytes_275 = user_std_web_ui_effects_http_request_get_v1(ctx, input, t_view_276, t_view_278);
                                t_set_271 = user_app_effects2_v1(ctx, input, t_bytes_272, t_bytes_275);
                                t_bytes_272 = rt_bytes_empty(ctx);
                                t_bytes_275 = rt_bytes_empty(ctx);
                                rt_bytes_drop(ctx, &v_148);
                                v_148 = t_set_271;
                                t_set_271 = rt_bytes_empty(ctx);
                                uint32_t t_i32_280 = UINT32_C(0);
                                rt_fuel(ctx, 1);
                                t_i32_280 = UINT32_C(0);
                                rt_bytes_drop(ctx, &t_bytes_272);
                                rt_bytes_drop(ctx, &t_bytes_275);
                              }
                            } else {
                              {
                                bytes_t v_281 = rt_bytes_empty(ctx);
                                rt_fuel(ctx, 1);
                                bytes_view_t t_view_282 = rt_view_empty(ctx);
                                rt_fuel(ctx, 1);
                                t_view_282 = rt_bytes_view(ctx, v_46);
                                v_281 = rt_view_to_bytes(ctx, t_view_282);
                                uint32_t t_i32_283 = UINT32_C(0);
                                rt_fuel(ctx, 1);
                                bytes_view_t t_view_284 = rt_view_empty(ctx);
                                rt_fuel(ctx, 1);
                                t_view_284 = rt_bytes_view(ctx, v_281);
                                bytes_view_t t_view_285 = rt_view_empty(ctx);
                                rt_fuel(ctx, 1);
                                static const char lit_286[] = "\"detail\"";
                                t_view_285 = rt_view_from_literal(ctx, (const uint8_t*)lit_286, UINT32_C(8));
                                t_i32_283 = rt_view_eq(ctx, t_view_284, t_view_285);
                                if (t_i32_283 != UINT32_C(0)) {
                                  {
                                    bytes_t t_set_287 = rt_bytes_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    static const char lit_288[] = "\"req_detail\"";
                                    t_set_287 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_288, UINT32_C(12));
                                    rt_bytes_drop(ctx, &v_144);
                                    v_144 = t_set_287;
                                    t_set_287 = rt_bytes_empty(ctx);
                                    bytes_t t_set_289 = rt_bytes_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    static const char lit_290[] = "\"\"";
                                    t_set_289 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_290, UINT32_C(2));
                                    rt_bytes_drop(ctx, &v_146);
                                    v_146 = t_set_289;
                                    t_set_289 = rt_bytes_empty(ctx);
                                    bytes_t t_set_291 = rt_bytes_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    bytes_t t_bytes_292 = rt_bytes_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    bytes_view_t t_view_293 = rt_view_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    static const char lit_294[] = "\"/projects/proj_001\"";
                                    t_view_293 = rt_view_from_literal(ctx, (const uint8_t*)lit_294, UINT32_C(20));
                                    t_bytes_292 = user_std_web_ui_effects_nav_push_v1(ctx, input, t_view_293);
                                    bytes_t t_bytes_295 = rt_bytes_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    bytes_view_t t_view_296 = rt_view_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    static const char lit_297[] = "\"req_detail\"";
                                    t_view_296 = rt_view_from_literal(ctx, (const uint8_t*)lit_297, UINT32_C(12));
                                    bytes_view_t t_view_298 = rt_view_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    static const char lit_299[] = "\"/api/projects/proj_001\"";
                                    t_view_298 = rt_view_from_literal(ctx, (const uint8_t*)lit_299, UINT32_C(24));
                                    t_bytes_295 = user_std_web_ui_effects_http_request_get_v1(ctx, input, t_view_296, t_view_298);
                                    t_set_291 = user_app_effects2_v1(ctx, input, t_bytes_292, t_bytes_295);
                                    t_bytes_292 = rt_bytes_empty(ctx);
                                    t_bytes_295 = rt_bytes_empty(ctx);
                                    rt_bytes_drop(ctx, &v_148);
                                    v_148 = t_set_291;
                                    t_set_291 = rt_bytes_empty(ctx);
                                    uint32_t t_i32_300 = UINT32_C(0);
                                    rt_fuel(ctx, 1);
                                    t_i32_300 = UINT32_C(0);
                                    rt_bytes_drop(ctx, &t_bytes_292);
                                    rt_bytes_drop(ctx, &t_bytes_295);
                                  }
                                } else {
                                  {
                                    bytes_t v_301 = rt_bytes_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    bytes_view_t t_view_302 = rt_view_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    t_view_302 = rt_bytes_view(ctx, v_46);
                                    v_301 = rt_view_to_bytes(ctx, t_view_302);
                                    uint32_t t_i32_303 = UINT32_C(0);
                                    rt_fuel(ctx, 1);
                                    bytes_view_t t_view_304 = rt_view_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    t_view_304 = rt_bytes_view(ctx, v_301);
                                    bytes_view_t t_view_305 = rt_view_empty(ctx);
                                    rt_fuel(ctx, 1);
                                    static const char lit_306[] = "\"item1\"";
                                    t_view_305 = rt_view_from_literal(ctx, (const uint8_t*)lit_306, UINT32_C(7));
                                    t_i32_303 = rt_view_eq(ctx, t_view_304, t_view_305);
                                    if (t_i32_303 != UINT32_C(0)) {
                                      {
                                        bytes_t t_set_307 = rt_bytes_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        static const char lit_308[] = "\"req_item1\"";
                                        t_set_307 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_308, UINT32_C(11));
                                        rt_bytes_drop(ctx, &v_144);
                                        v_144 = t_set_307;
                                        t_set_307 = rt_bytes_empty(ctx);
                                        bytes_t t_set_309 = rt_bytes_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        static const char lit_310[] = "\"\"";
                                        t_set_309 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_310, UINT32_C(2));
                                        rt_bytes_drop(ctx, &v_146);
                                        v_146 = t_set_309;
                                        t_set_309 = rt_bytes_empty(ctx);
                                        bytes_t t_set_311 = rt_bytes_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        bytes_t t_bytes_312 = rt_bytes_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        bytes_view_t t_view_313 = rt_view_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        static const char lit_314[] = "\"/projects/proj_001/items/item_001\"";
                                        t_view_313 = rt_view_from_literal(ctx, (const uint8_t*)lit_314, UINT32_C(35));
                                        t_bytes_312 = user_std_web_ui_effects_nav_push_v1(ctx, input, t_view_313);
                                        bytes_t t_bytes_315 = rt_bytes_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        bytes_view_t t_view_316 = rt_view_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        static const char lit_317[] = "\"req_item1\"";
                                        t_view_316 = rt_view_from_literal(ctx, (const uint8_t*)lit_317, UINT32_C(11));
                                        bytes_view_t t_view_318 = rt_view_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        static const char lit_319[] = "\"/api/projects/proj_001/items\"";
                                        t_view_318 = rt_view_from_literal(ctx, (const uint8_t*)lit_319, UINT32_C(30));
                                        bytes_view_t t_view_320 = rt_view_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        static const char lit_321[] = "{\"title\":\"Roadmap\"}";
                                        t_view_320 = rt_view_from_literal(ctx, (const uint8_t*)lit_321, UINT32_C(19));
                                        t_bytes_315 = user_std_web_ui_effects_http_request_post_json_v1(ctx, input, t_view_316, t_view_318, t_view_320);
                                        t_set_311 = user_app_effects2_v1(ctx, input, t_bytes_312, t_bytes_315);
                                        t_bytes_312 = rt_bytes_empty(ctx);
                                        t_bytes_315 = rt_bytes_empty(ctx);
                                        rt_bytes_drop(ctx, &v_148);
                                        v_148 = t_set_311;
                                        t_set_311 = rt_bytes_empty(ctx);
                                        uint32_t t_i32_322 = UINT32_C(0);
                                        rt_fuel(ctx, 1);
                                        t_i32_322 = UINT32_C(0);
                                        rt_bytes_drop(ctx, &t_bytes_312);
                                        rt_bytes_drop(ctx, &t_bytes_315);
                                      }
                                    } else {
                                      {
                                        bytes_t v_323 = rt_bytes_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        bytes_view_t t_view_324 = rt_view_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        t_view_324 = rt_bytes_view(ctx, v_46);
                                        v_323 = rt_view_to_bytes(ctx, t_view_324);
                                        uint32_t t_i32_325 = UINT32_C(0);
                                        rt_fuel(ctx, 1);
                                        bytes_view_t t_view_326 = rt_view_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        t_view_326 = rt_bytes_view(ctx, v_323);
                                        bytes_view_t t_view_327 = rt_view_empty(ctx);
                                        rt_fuel(ctx, 1);
                                        static const char lit_328[] = "\"item2\"";
                                        t_view_327 = rt_view_from_literal(ctx, (const uint8_t*)lit_328, UINT32_C(7));
                                        t_i32_325 = rt_view_eq(ctx, t_view_326, t_view_327);
                                        if (t_i32_325 != UINT32_C(0)) {
                                          {
                                            bytes_t t_set_329 = rt_bytes_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            static const char lit_330[] = "\"req_item2\"";
                                            t_set_329 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_330, UINT32_C(11));
                                            rt_bytes_drop(ctx, &v_144);
                                            v_144 = t_set_329;
                                            t_set_329 = rt_bytes_empty(ctx);
                                            bytes_t t_set_331 = rt_bytes_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            static const char lit_332[] = "\"\"";
                                            t_set_331 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_332, UINT32_C(2));
                                            rt_bytes_drop(ctx, &v_146);
                                            v_146 = t_set_331;
                                            t_set_331 = rt_bytes_empty(ctx);
                                            bytes_t t_set_333 = rt_bytes_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            bytes_t t_bytes_334 = rt_bytes_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            bytes_view_t t_view_335 = rt_view_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            static const char lit_336[] = "\"/projects/proj_001/items/item_002\"";
                                            t_view_335 = rt_view_from_literal(ctx, (const uint8_t*)lit_336, UINT32_C(35));
                                            t_bytes_334 = user_std_web_ui_effects_nav_push_v1(ctx, input, t_view_335);
                                            bytes_t t_bytes_337 = rt_bytes_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            bytes_view_t t_view_338 = rt_view_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            static const char lit_339[] = "\"req_item2\"";
                                            t_view_338 = rt_view_from_literal(ctx, (const uint8_t*)lit_339, UINT32_C(11));
                                            bytes_view_t t_view_340 = rt_view_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            static const char lit_341[] = "\"/api/projects/proj_001/items\"";
                                            t_view_340 = rt_view_from_literal(ctx, (const uint8_t*)lit_341, UINT32_C(30));
                                            bytes_view_t t_view_342 = rt_view_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            static const char lit_343[] = "{\"title\":\"Budget notes\"}";
                                            t_view_342 = rt_view_from_literal(ctx, (const uint8_t*)lit_343, UINT32_C(24));
                                            t_bytes_337 = user_std_web_ui_effects_http_request_post_json_v1(ctx, input, t_view_338, t_view_340, t_view_342);
                                            t_set_333 = user_app_effects2_v1(ctx, input, t_bytes_334, t_bytes_337);
                                            t_bytes_334 = rt_bytes_empty(ctx);
                                            t_bytes_337 = rt_bytes_empty(ctx);
                                            rt_bytes_drop(ctx, &v_148);
                                            v_148 = t_set_333;
                                            t_set_333 = rt_bytes_empty(ctx);
                                            uint32_t t_i32_344 = UINT32_C(0);
                                            rt_fuel(ctx, 1);
                                            t_i32_344 = UINT32_C(0);
                                            rt_bytes_drop(ctx, &t_bytes_334);
                                            rt_bytes_drop(ctx, &t_bytes_337);
                                          }
                                        } else {
                                          {
                                            bytes_t v_345 = rt_bytes_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            bytes_view_t t_view_346 = rt_view_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            t_view_346 = rt_bytes_view(ctx, v_46);
                                            v_345 = rt_view_to_bytes(ctx, t_view_346);
                                            uint32_t t_i32_347 = UINT32_C(0);
                                            rt_fuel(ctx, 1);
                                            bytes_view_t t_view_348 = rt_view_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            t_view_348 = rt_bytes_view(ctx, v_345);
                                            bytes_view_t t_view_349 = rt_view_empty(ctx);
                                            rt_fuel(ctx, 1);
                                            static const char lit_350[] = "\"invalid-item\"";
                                            t_view_349 = rt_view_from_literal(ctx, (const uint8_t*)lit_350, UINT32_C(14));
                                            t_i32_347 = rt_view_eq(ctx, t_view_348, t_view_349);
                                            if (t_i32_347 != UINT32_C(0)) {
                                              {
                                                bytes_t t_set_351 = rt_bytes_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                static const char lit_352[] = "\"req_invalid_item\"";
                                                t_set_351 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_352, UINT32_C(18));
                                                rt_bytes_drop(ctx, &v_144);
                                                v_144 = t_set_351;
                                                t_set_351 = rt_bytes_empty(ctx);
                                                bytes_t t_set_353 = rt_bytes_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                static const char lit_354[] = "\"\"";
                                                t_set_353 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_354, UINT32_C(2));
                                                rt_bytes_drop(ctx, &v_146);
                                                v_146 = t_set_353;
                                                t_set_353 = rt_bytes_empty(ctx);
                                                bytes_t t_set_355 = rt_bytes_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                bytes_t t_bytes_356 = rt_bytes_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                bytes_view_t t_view_357 = rt_view_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                static const char lit_358[] = "\"/projects/proj_001/items/invalid\"";
                                                t_view_357 = rt_view_from_literal(ctx, (const uint8_t*)lit_358, UINT32_C(34));
                                                t_bytes_356 = user_std_web_ui_effects_nav_push_v1(ctx, input, t_view_357);
                                                bytes_t t_bytes_359 = rt_bytes_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                bytes_view_t t_view_360 = rt_view_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                static const char lit_361[] = "\"req_invalid_item\"";
                                                t_view_360 = rt_view_from_literal(ctx, (const uint8_t*)lit_361, UINT32_C(18));
                                                bytes_view_t t_view_362 = rt_view_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                static const char lit_363[] = "\"/api/projects/proj_001/items\"";
                                                t_view_362 = rt_view_from_literal(ctx, (const uint8_t*)lit_363, UINT32_C(30));
                                                bytes_view_t t_view_364 = rt_view_empty(ctx);
                                                rt_fuel(ctx, 1);
                                                static const char lit_365[] = "{\"x\":\"true\"}";
                                                t_view_364 = rt_view_from_literal(ctx, (const uint8_t*)lit_365, UINT32_C(12));
                                                t_bytes_359 = user_std_web_ui_effects_http_request_post_json_v1(ctx, input, t_view_360, t_view_362, t_view_364);
                                                t_set_355 = user_app_effects2_v1(ctx, input, t_bytes_356, t_bytes_359);
                                                t_bytes_356 = rt_bytes_empty(ctx);
                                                t_bytes_359 = rt_bytes_empty(ctx);
                                                rt_bytes_drop(ctx, &v_148);
                                                v_148 = t_set_355;
                                                t_set_355 = rt_bytes_empty(ctx);
                                                uint32_t t_i32_366 = UINT32_C(0);
                                                rt_fuel(ctx, 1);
                                                t_i32_366 = UINT32_C(0);
                                                rt_bytes_drop(ctx, &t_bytes_356);
                                                rt_bytes_drop(ctx, &t_bytes_359);
                                              }
                                            } else {
                                              uint32_t t_i32_367 = UINT32_C(0);
                                              rt_fuel(ctx, 1);
                                              bytes_view_t t_view_368 = rt_view_empty(ctx);
                                              rt_fuel(ctx, 1);
                                              t_view_368 = rt_bytes_view(ctx, v_46);
                                              bytes_view_t t_view_369 = rt_view_empty(ctx);
                                              rt_fuel(ctx, 1);
                                              static const char lit_370[] = "\"perf\"";
                                              t_view_369 = rt_view_from_literal(ctx, (const uint8_t*)lit_370, UINT32_C(6));
                                              t_i32_367 = rt_view_eq(ctx, t_view_368, t_view_369);
                                              if (t_i32_367 != UINT32_C(0)) {
                                                {
                                                  bytes_t t_set_371 = rt_bytes_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  static const char lit_372[] = "\"req_perf\"";
                                                  t_set_371 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_372, UINT32_C(10));
                                                  rt_bytes_drop(ctx, &v_144);
                                                  v_144 = t_set_371;
                                                  t_set_371 = rt_bytes_empty(ctx);
                                                  bytes_t t_set_373 = rt_bytes_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  static const char lit_374[] = "\"\"";
                                                  t_set_373 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_374, UINT32_C(2));
                                                  rt_bytes_drop(ctx, &v_146);
                                                  v_146 = t_set_373;
                                                  t_set_373 = rt_bytes_empty(ctx);
                                                  bytes_t t_set_375 = rt_bytes_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  bytes_t t_bytes_376 = rt_bytes_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  bytes_view_t t_view_377 = rt_view_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  static const char lit_378[] = "\"/projects/proj_001/items/perf\"";
                                                  t_view_377 = rt_view_from_literal(ctx, (const uint8_t*)lit_378, UINT32_C(31));
                                                  t_bytes_376 = user_std_web_ui_effects_nav_push_v1(ctx, input, t_view_377);
                                                  bytes_t t_bytes_379 = rt_bytes_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  bytes_view_t t_view_380 = rt_view_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  static const char lit_381[] = "\"req_perf\"";
                                                  t_view_380 = rt_view_from_literal(ctx, (const uint8_t*)lit_381, UINT32_C(10));
                                                  bytes_view_t t_view_382 = rt_view_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  static const char lit_383[] = "\"/api/projects/proj_001/items\"";
                                                  t_view_382 = rt_view_from_literal(ctx, (const uint8_t*)lit_383, UINT32_C(30));
                                                  bytes_view_t t_view_384 = rt_view_empty(ctx);
                                                  rt_fuel(ctx, 1);
                                                  static const char lit_385[] = "{\"title\":\"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}";
                                                  t_view_384 = rt_view_from_literal(ctx, (const uint8_t*)lit_385, UINT32_C(48));
                                                  t_bytes_379 = user_std_web_ui_effects_http_request_post_json_v1(ctx, input, t_view_380, t_view_382, t_view_384);
                                                  t_set_375 = user_app_effects2_v1(ctx, input, t_bytes_376, t_bytes_379);
                                                  t_bytes_376 = rt_bytes_empty(ctx);
                                                  t_bytes_379 = rt_bytes_empty(ctx);
                                                  rt_bytes_drop(ctx, &v_148);
                                                  v_148 = t_set_375;
                                                  t_set_375 = rt_bytes_empty(ctx);
                                                  uint32_t t_i32_386 = UINT32_C(0);
                                                  rt_fuel(ctx, 1);
                                                  t_i32_386 = UINT32_C(0);
                                                  rt_bytes_drop(ctx, &t_bytes_376);
                                                  rt_bytes_drop(ctx, &t_bytes_379);
                                                }
                                              } else {
                                                uint32_t t_i32_387 = UINT32_C(0);
                                                rt_fuel(ctx, 1);
                                                t_i32_387 = UINT32_C(0);
                                              }
                                            }
                                            rt_bytes_drop(ctx, &v_345);
                                          }
                                        }
                                        rt_bytes_drop(ctx, &v_323);
                                      }
                                    }
                                    rt_bytes_drop(ctx, &v_301);
                                  }
                                }
                                rt_bytes_drop(ctx, &v_281);
                              }
                            }
                            rt_bytes_drop(ctx, &v_261);
                          }
                        }
                        rt_bytes_drop(ctx, &v_242);
                      }
                    }
                    rt_bytes_drop(ctx, &v_223);
                  }
                }
                rt_bytes_drop(ctx, &v_206);
              }
              uint32_t t_i32_388 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_388 = UINT32_C(0);
            }
          } else {
            uint32_t t_i32_389 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_389 = UINT32_C(0);
          }
        }
        rt_bytes_drop(ctx, &v_183);
      }
    }
    bytes_t v_390 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_391 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_391 = rt_bytes_view(ctx, v_138);
    bytes_view_t t_view_392 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_392 = rt_bytes_view(ctx, v_140);
    bytes_view_t t_view_393 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_393 = rt_bytes_view(ctx, v_142);
    bytes_view_t t_view_394 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_394 = rt_bytes_view(ctx, v_144);
    bytes_view_t t_view_395 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_395 = rt_bytes_view(ctx, v_146);
    v_390 = user_app_state_v1(ctx, input, t_view_391, t_view_392, t_view_393, t_view_394, t_view_395);
    bytes_t v_396 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_397 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_397 = rt_bytes_view(ctx, v_15);
    bytes_view_t t_view_398 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_398 = rt_bytes_view(ctx, v_20);
    bytes_view_t t_view_399 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_399 = rt_bytes_view(ctx, v_25);
    bytes_view_t t_view_400 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_400 = rt_bytes_view(ctx, v_30);
    bytes_view_t t_view_401 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_401 = rt_bytes_view(ctx, v_35);
    v_396 = user_app_ui_tree_v1(ctx, input, t_view_397, t_view_398, t_view_399, t_view_400, t_view_401);
    bytes_t v_402 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_403 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_403 = rt_bytes_view(ctx, v_138);
    bytes_view_t t_view_404 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_404 = rt_bytes_view(ctx, v_140);
    bytes_view_t t_view_405 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_405 = rt_bytes_view(ctx, v_142);
    bytes_view_t t_view_406 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_406 = rt_bytes_view(ctx, v_144);
    bytes_view_t t_view_407 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_407 = rt_bytes_view(ctx, v_146);
    v_402 = user_app_ui_tree_v1(ctx, input, t_view_403, t_view_404, t_view_405, t_view_406, t_view_407);
    bytes_t v_408 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_409 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_409 = rt_bytes_view(ctx, v_396);
    bytes_view_t t_view_410 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_410 = rt_bytes_view(ctx, v_402);
    v_408 = user_std_web_ui_patch_diff_v1(ctx, input, t_view_409, t_view_410);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_411 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_411 = rt_bytes_view(ctx, v_390);
    bytes_view_t t_view_412 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_412 = rt_bytes_view(ctx, v_402);
    bytes_view_t t_view_413 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_413 = rt_bytes_view(ctx, v_408);
    bytes_view_t t_view_414 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_414 = rt_bytes_view(ctx, v_148);
    bytes_view_t t_view_415 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_416[] = "{}";
    t_view_415 = rt_view_from_literal(ctx, (const uint8_t*)lit_416, UINT32_C(2));
    out = user_std_web_ui_runtime_frame_v1(ctx, input, t_view_411, t_view_412, t_view_413, t_view_414, t_view_415);
    rt_bytes_drop(ctx, &v_83);
    rt_bytes_drop(ctx, &v_65);
    rt_bytes_drop(ctx, &v_5);
    rt_bytes_drop(ctx, &v_25);
    rt_bytes_drop(ctx, &v_13);
    rt_bytes_drop(ctx, &v_3);
    rt_bytes_drop(ctx, &v_20);
    rt_bytes_drop(ctx, &v_1);
    rt_bytes_drop(ctx, &v_35);
    rt_bytes_drop(ctx, &v_46);
    rt_bytes_drop(ctx, &v_40);
    rt_bytes_drop(ctx, &v_51);
    rt_bytes_drop(ctx, &v_11);
    rt_bytes_drop(ctx, &v_7);
    rt_bytes_drop(ctx, &v_142);
    rt_bytes_drop(ctx, &v_140);
    rt_bytes_drop(ctx, &v_148);
    rt_bytes_drop(ctx, &v_146);
    rt_bytes_drop(ctx, &v_138);
    rt_bytes_drop(ctx, &v_144);
    rt_bytes_drop(ctx, &v_408);
    rt_bytes_drop(ctx, &v_396);
    rt_bytes_drop(ctx, &v_15);
    rt_bytes_drop(ctx, &v_30);
    rt_bytes_drop(ctx, &v_390);
    rt_bytes_drop(ctx, &v_79);
    rt_bytes_drop(ctx, &v_63);
    rt_bytes_drop(ctx, &v_95);
    rt_bytes_drop(ctx, &v_9);
    rt_bytes_drop(ctx, &v_121);
    rt_bytes_drop(ctx, &v_117);
    rt_bytes_drop(ctx, &v_119);
    rt_bytes_drop(ctx, &v_114);
    rt_bytes_drop(ctx, &v_88);
    rt_bytes_drop(ctx, &v_402);
  }
  return out;
}

#line 1 "app.x07.json"
static bytes_t user_app_ui_tree_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_3[] = "\"refresh\"";
    t_view_2 = rt_view_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(9));
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_5[] = "\"refresh\"";
    t_view_4 = rt_view_from_literal(ctx, (const uint8_t*)lit_5, UINT32_C(9));
    v_1 = user_app_button_v1(ctx, input, t_view_2, t_view_4);
    bytes_t v_6 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_7 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_8[] = "\"create\"";
    t_view_7 = rt_view_from_literal(ctx, (const uint8_t*)lit_8, UINT32_C(8));
    bytes_view_t t_view_9 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_10[] = "\"create\"";
    t_view_9 = rt_view_from_literal(ctx, (const uint8_t*)lit_10, UINT32_C(8));
    v_6 = user_app_button_v1(ctx, input, t_view_7, t_view_9);
    bytes_t v_11 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_13[] = "\"invalid-create\"";
    t_view_12 = rt_view_from_literal(ctx, (const uint8_t*)lit_13, UINT32_C(16));
    bytes_view_t t_view_14 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_15[] = "\"invalid-create\"";
    t_view_14 = rt_view_from_literal(ctx, (const uint8_t*)lit_15, UINT32_C(16));
    v_11 = user_app_button_v1(ctx, input, t_view_12, t_view_14);
    bytes_t v_16 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_17 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_18[] = "\"list\"";
    t_view_17 = rt_view_from_literal(ctx, (const uint8_t*)lit_18, UINT32_C(6));
    bytes_view_t t_view_19 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_20[] = "\"list\"";
    t_view_19 = rt_view_from_literal(ctx, (const uint8_t*)lit_20, UINT32_C(6));
    v_16 = user_app_button_v1(ctx, input, t_view_17, t_view_19);
    bytes_t v_21 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_22 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_23[] = "\"detail\"";
    t_view_22 = rt_view_from_literal(ctx, (const uint8_t*)lit_23, UINT32_C(8));
    bytes_view_t t_view_24 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_25[] = "\"detail\"";
    t_view_24 = rt_view_from_literal(ctx, (const uint8_t*)lit_25, UINT32_C(8));
    v_21 = user_app_button_v1(ctx, input, t_view_22, t_view_24);
    bytes_t v_26 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_27 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_28[] = "\"item1\"";
    t_view_27 = rt_view_from_literal(ctx, (const uint8_t*)lit_28, UINT32_C(7));
    bytes_view_t t_view_29 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_30[] = "\"item1\"";
    t_view_29 = rt_view_from_literal(ctx, (const uint8_t*)lit_30, UINT32_C(7));
    v_26 = user_app_button_v1(ctx, input, t_view_27, t_view_29);
    bytes_t v_31 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_32 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_33[] = "\"item2\"";
    t_view_32 = rt_view_from_literal(ctx, (const uint8_t*)lit_33, UINT32_C(7));
    bytes_view_t t_view_34 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_35[] = "\"item2\"";
    t_view_34 = rt_view_from_literal(ctx, (const uint8_t*)lit_35, UINT32_C(7));
    v_31 = user_app_button_v1(ctx, input, t_view_32, t_view_34);
    bytes_t v_36 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_37 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_38[] = "\"invalid-item\"";
    t_view_37 = rt_view_from_literal(ctx, (const uint8_t*)lit_38, UINT32_C(14));
    bytes_view_t t_view_39 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_40[] = "\"invalid-item\"";
    t_view_39 = rt_view_from_literal(ctx, (const uint8_t*)lit_40, UINT32_C(14));
    v_36 = user_app_button_v1(ctx, input, t_view_37, t_view_39);
    bytes_t v_41 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_42 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_43[] = "\"perf\"";
    t_view_42 = rt_view_from_literal(ctx, (const uint8_t*)lit_43, UINT32_C(6));
    bytes_view_t t_view_44 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_45[] = "\"perf\"";
    t_view_44 = rt_view_from_literal(ctx, (const uint8_t*)lit_45, UINT32_C(6));
    v_41 = user_app_button_v1(ctx, input, t_view_42, t_view_44);
    bytes_t v_46 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_47 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_47 = rt_bytes_view(ctx, v_1);
    bytes_view_t t_view_48 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_48 = rt_bytes_view(ctx, v_6);
    bytes_view_t t_view_49 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_49 = rt_bytes_view(ctx, v_11);
    v_46 = user_std_web_ui_vdom_children3_v1(ctx, input, t_view_47, t_view_48, t_view_49);
    bytes_t v_50 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_51 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_52[] = "\"row1\"";
    t_view_51 = rt_view_from_literal(ctx, (const uint8_t*)lit_52, UINT32_C(6));
    bytes_view_t t_view_53 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_53 = rt_bytes_view(ctx, v_46);
    v_50 = user_std_web_ui_vdom_fragment_v1(ctx, input, t_view_51, t_view_53);
    bytes_t v_54 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_55 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_55 = rt_bytes_view(ctx, v_16);
    bytes_view_t t_view_56 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_56 = rt_bytes_view(ctx, v_21);
    bytes_view_t t_view_57 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_57 = rt_bytes_view(ctx, v_26);
    v_54 = user_std_web_ui_vdom_children3_v1(ctx, input, t_view_55, t_view_56, t_view_57);
    bytes_t v_58 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_59 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_60[] = "\"row2\"";
    t_view_59 = rt_view_from_literal(ctx, (const uint8_t*)lit_60, UINT32_C(6));
    bytes_view_t t_view_61 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_61 = rt_bytes_view(ctx, v_54);
    v_58 = user_std_web_ui_vdom_fragment_v1(ctx, input, t_view_59, t_view_61);
    bytes_t v_62 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_63 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_63 = rt_bytes_view(ctx, v_31);
    bytes_view_t t_view_64 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_64 = rt_bytes_view(ctx, v_36);
    bytes_view_t t_view_65 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_65 = rt_bytes_view(ctx, v_41);
    v_62 = user_std_web_ui_vdom_children3_v1(ctx, input, t_view_63, t_view_64, t_view_65);
    bytes_t v_66 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_67 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_68[] = "\"row3\"";
    t_view_67 = rt_view_from_literal(ctx, (const uint8_t*)lit_68, UINT32_C(6));
    bytes_view_t t_view_69 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_69 = rt_bytes_view(ctx, v_62);
    v_66 = user_std_web_ui_vdom_fragment_v1(ctx, input, t_view_67, t_view_69);
    bytes_t v_70 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_71 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_71 = rt_bytes_view(ctx, v_50);
    bytes_view_t t_view_72 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_72 = rt_bytes_view(ctx, v_58);
    bytes_view_t t_view_73 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_73 = rt_bytes_view(ctx, v_66);
    v_70 = user_std_web_ui_vdom_children3_v1(ctx, input, t_view_71, t_view_72, t_view_73);
    bytes_t v_74 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_75 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_76[] = "\"buttons_group\"";
    t_view_75 = rt_view_from_literal(ctx, (const uint8_t*)lit_76, UINT32_C(15));
    bytes_view_t t_view_77 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_77 = rt_bytes_view(ctx, v_70);
    v_74 = user_std_web_ui_vdom_fragment_v1(ctx, input, t_view_75, t_view_77);
    bytes_t v_78 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_79 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_80[] = "\"route\"";
    t_view_79 = rt_view_from_literal(ctx, (const uint8_t*)lit_80, UINT32_C(7));
    bytes_view_t t_view_81 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_82[] = "\"route: \"";
    t_view_81 = rt_view_from_literal(ctx, (const uint8_t*)lit_82, UINT32_C(9));
    bytes_view_t t_view_83 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_83 = p0;
    v_78 = user_app_field_v1(ctx, input, t_view_79, t_view_81, t_view_83);
    bytes_t v_84 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_85 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_86[] = "\"display\"";
    t_view_85 = rt_view_from_literal(ctx, (const uint8_t*)lit_86, UINT32_C(9));
    bytes_view_t t_view_87 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_88[] = "\"display: \"";
    t_view_87 = rt_view_from_literal(ctx, (const uint8_t*)lit_88, UINT32_C(11));
    bytes_view_t t_view_89 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_89 = p1;
    v_84 = user_app_field_v1(ctx, input, t_view_85, t_view_87, t_view_89);
    bytes_t v_90 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_91 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_92[] = "\"cache\"";
    t_view_91 = rt_view_from_literal(ctx, (const uint8_t*)lit_92, UINT32_C(7));
    bytes_view_t t_view_93 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_94[] = "\"cache: \"";
    t_view_93 = rt_view_from_literal(ctx, (const uint8_t*)lit_94, UINT32_C(9));
    bytes_view_t t_view_95 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_95 = p2;
    v_90 = user_app_field_v1(ctx, input, t_view_91, t_view_93, t_view_95);
    bytes_t v_96 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_97 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_98[] = "\"source\"";
    t_view_97 = rt_view_from_literal(ctx, (const uint8_t*)lit_98, UINT32_C(8));
    bytes_view_t t_view_99 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_100[] = "\"source: \"";
    t_view_99 = rt_view_from_literal(ctx, (const uint8_t*)lit_100, UINT32_C(10));
    bytes_view_t t_view_101 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_101 = p3;
    v_96 = user_app_field_v1(ctx, input, t_view_97, t_view_99, t_view_101);
    bytes_t v_102 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_103 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_104[] = "\"error\"";
    t_view_103 = rt_view_from_literal(ctx, (const uint8_t*)lit_104, UINT32_C(7));
    bytes_view_t t_view_105 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_106[] = "\"error: \"";
    t_view_105 = rt_view_from_literal(ctx, (const uint8_t*)lit_106, UINT32_C(9));
    bytes_view_t t_view_107 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_107 = p4;
    v_102 = user_app_field_v1(ctx, input, t_view_103, t_view_105, t_view_107);
    bytes_t v_108 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_109 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_109 = rt_bytes_view(ctx, v_78);
    bytes_view_t t_view_110 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_110 = rt_bytes_view(ctx, v_84);
    bytes_view_t t_view_111 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_111 = rt_bytes_view(ctx, v_90);
    v_108 = user_std_web_ui_vdom_children3_v1(ctx, input, t_view_109, t_view_110, t_view_111);
    bytes_t v_112 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_113 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_114[] = "\"status_row1\"";
    t_view_113 = rt_view_from_literal(ctx, (const uint8_t*)lit_114, UINT32_C(13));
    bytes_view_t t_view_115 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_115 = rt_bytes_view(ctx, v_108);
    v_112 = user_std_web_ui_vdom_fragment_v1(ctx, input, t_view_113, t_view_115);
    bytes_t v_116 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_117 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_117 = rt_bytes_view(ctx, v_96);
    bytes_view_t t_view_118 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_118 = rt_bytes_view(ctx, v_102);
    v_116 = user_std_web_ui_vdom_children2_v1(ctx, input, t_view_117, t_view_118);
    bytes_t v_119 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_120 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_121[] = "\"status_row2\"";
    t_view_120 = rt_view_from_literal(ctx, (const uint8_t*)lit_121, UINT32_C(13));
    bytes_view_t t_view_122 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_122 = rt_bytes_view(ctx, v_116);
    v_119 = user_std_web_ui_vdom_fragment_v1(ctx, input, t_view_120, t_view_122);
    bytes_t v_123 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_124 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_124 = rt_bytes_view(ctx, v_112);
    bytes_view_t t_view_125 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_125 = rt_bytes_view(ctx, v_119);
    v_123 = user_std_web_ui_vdom_children2_v1(ctx, input, t_view_124, t_view_125);
    bytes_t v_126 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_127 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_128[] = "\"status_group\"";
    t_view_127 = rt_view_from_literal(ctx, (const uint8_t*)lit_128, UINT32_C(14));
    bytes_view_t t_view_129 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_129 = rt_bytes_view(ctx, v_123);
    v_126 = user_std_web_ui_vdom_fragment_v1(ctx, input, t_view_127, t_view_129);
    bytes_t v_130 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_131 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_132[] = "\"title\"";
    t_view_131 = rt_view_from_literal(ctx, (const uint8_t*)lit_132, UINT32_C(7));
    bytes_view_t t_view_133 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_134[] = "\"frontend\"";
    t_view_133 = rt_view_from_literal(ctx, (const uint8_t*)lit_134, UINT32_C(10));
    v_130 = user_std_web_ui_vdom_text_v1(ctx, input, t_view_131, t_view_133);
    bytes_t v_135 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_136 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_136 = rt_bytes_view(ctx, v_130);
    bytes_view_t t_view_137 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_137 = rt_bytes_view(ctx, v_74);
    bytes_view_t t_view_138 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_138 = rt_bytes_view(ctx, v_126);
    v_135 = user_std_web_ui_vdom_children3_v1(ctx, input, t_view_136, t_view_137, t_view_138);
    bytes_t v_139 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_140 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_141[] = "{}";
    t_view_140 = rt_view_from_literal(ctx, (const uint8_t*)lit_141, UINT32_C(2));
    bytes_view_t t_view_142 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_143[] = "[]";
    t_view_142 = rt_view_from_literal(ctx, (const uint8_t*)lit_143, UINT32_C(2));
    bytes_view_t t_view_144 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_145[] = "{}";
    t_view_144 = rt_view_from_literal(ctx, (const uint8_t*)lit_145, UINT32_C(2));
    v_139 = user_std_web_ui_vdom_props_v1(ctx, input, t_view_140, t_view_142, t_view_144);
    bytes_t v_146 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    v_146 = user_std_web_ui_vdom_on_empty_v1(ctx, input);
    bytes_t v_147 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_148 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_149[] = "\"div\"";
    t_view_148 = rt_view_from_literal(ctx, (const uint8_t*)lit_149, UINT32_C(5));
    bytes_view_t t_view_150 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_151[] = "\"root\"";
    t_view_150 = rt_view_from_literal(ctx, (const uint8_t*)lit_151, UINT32_C(6));
    bytes_view_t t_view_152 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_152 = rt_bytes_view(ctx, v_139);
    bytes_view_t t_view_153 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_153 = rt_bytes_view(ctx, v_146);
    bytes_view_t t_view_154 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_154 = rt_bytes_view(ctx, v_135);
    v_147 = user_std_web_ui_vdom_el_v1(ctx, input, t_view_148, t_view_150, t_view_152, t_view_153, t_view_154);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_155 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_155 = rt_bytes_view(ctx, v_147);
    out = user_std_web_ui_vdom_tree_v1(ctx, input, t_view_155);
    rt_bytes_drop(ctx, &v_6);
    rt_bytes_drop(ctx, &v_21);
    rt_bytes_drop(ctx, &v_11);
    rt_bytes_drop(ctx, &v_36);
    rt_bytes_drop(ctx, &v_26);
    rt_bytes_drop(ctx, &v_31);
    rt_bytes_drop(ctx, &v_16);
    rt_bytes_drop(ctx, &v_41);
    rt_bytes_drop(ctx, &v_1);
    rt_bytes_drop(ctx, &v_70);
    rt_bytes_drop(ctx, &v_74);
    rt_bytes_drop(ctx, &v_90);
    rt_bytes_drop(ctx, &v_84);
    rt_bytes_drop(ctx, &v_102);
    rt_bytes_drop(ctx, &v_78);
    rt_bytes_drop(ctx, &v_96);
    rt_bytes_drop(ctx, &v_135);
    rt_bytes_drop(ctx, &v_147);
    rt_bytes_drop(ctx, &v_146);
    rt_bytes_drop(ctx, &v_139);
    rt_bytes_drop(ctx, &v_50);
    rt_bytes_drop(ctx, &v_46);
    rt_bytes_drop(ctx, &v_58);
    rt_bytes_drop(ctx, &v_54);
    rt_bytes_drop(ctx, &v_66);
    rt_bytes_drop(ctx, &v_62);
    rt_bytes_drop(ctx, &v_123);
    rt_bytes_drop(ctx, &v_126);
    rt_bytes_drop(ctx, &v_112);
    rt_bytes_drop(ctx, &v_108);
    rt_bytes_drop(ctx, &v_119);
    rt_bytes_drop(ctx, &v_116);
    rt_bytes_drop(ctx, &v_130);
  }
  return out;
}

#line 1 "std/bytes.x07.json"
static bytes_t user_std_bytes_copy(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_1 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_1 = p0;
  out = rt_view_to_bytes(ctx, t_view_1);
  return out;
}

#line 1 "std/fmt.x07.json"
static bytes_t user_std_fmt_u32_to_dec(ctx_t* ctx, bytes_view_t input, uint32_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_1 = p0;
  out = rt_fmt_u32_to_dec(ctx, t_i32_1);
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__cmp_member_key(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, uint32_t p2, uint32_t p3) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p1;
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_4 = p2;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = UINT32_C(2);
    t_i32_3 = t_i32_4 << (t_i32_5 & UINT32_C(31));
    v_1 = rt_codec_read_u32_le(ctx, t_view_2, t_i32_3);
    uint32_t v_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_7 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_7 = p1;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_9 = p3;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = UINT32_C(2);
    t_i32_8 = t_i32_9 << (t_i32_10 & UINT32_C(31));
    v_6 = rt_codec_read_u32_le(ctx, t_view_7, t_i32_8);
    uint32_t v_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p0;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = v_1;
    v_11 = rt_codec_read_u32_le(ctx, t_view_12, t_i32_13);
    uint32_t v_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_15 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_15 = p0;
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = v_6;
    v_14 = rt_codec_read_u32_le(ctx, t_view_15, t_i32_16);
    uint32_t v_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_18 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_18 = v_1;
    uint32_t t_i32_19 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_19 = UINT32_C(4);
    v_17 = t_i32_18 + t_i32_19;
    uint32_t v_20 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_21 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_21 = v_6;
    uint32_t t_i32_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_22 = UINT32_C(4);
    v_20 = t_i32_21 + t_i32_22;
    uint32_t v_23 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_24 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_25 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_25 = v_11;
    uint32_t t_i32_26 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_26 = v_14;
    t_i32_24 = (t_i32_25 < t_i32_26);
    if (t_i32_24 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      v_23 = v_11;
    } else {
      rt_fuel(ctx, 1);
      v_23 = v_14;
    }
    uint32_t t_i32_27 = UINT32_C(0);
    uint32_t v_28 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_28 = UINT32_C(0);
    uint32_t t_i32_29 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_29 = v_23;
    for (;;) {
      if (v_28 >= t_i32_29) break;
      {
        {
          uint32_t v_30 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_31 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_31 = p0;
          uint32_t t_i32_32 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_33 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_33 = v_17;
          uint32_t t_i32_34 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_34 = v_28;
          t_i32_32 = t_i32_33 + t_i32_34;
          ctx->trap_ptr = "/decls/1/body/8/4/1/2";
          v_30 = rt_view_get_u8(ctx, t_view_31, t_i32_32);
          ctx->trap_ptr = NULL;
          uint32_t v_35 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_36 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_36 = p0;
          uint32_t t_i32_37 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_38 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_38 = v_20;
          uint32_t t_i32_39 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_39 = v_28;
          t_i32_37 = t_i32_38 + t_i32_39;
          ctx->trap_ptr = "/decls/1/body/8/4/2/2";
          v_35 = rt_view_get_u8(ctx, t_view_36, t_i32_37);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_40 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_41 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_41 = v_30;
          uint32_t t_i32_42 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_42 = v_35;
          t_i32_40 = (t_i32_41 < t_i32_42);
          if (t_i32_40 != UINT32_C(0)) {
            uint32_t t_i32_43 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_43 = UINT32_C(4294967295);
            return t_i32_43;
          } else {
            uint32_t t_i32_44 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_44 = UINT32_C(0);
          }
          uint32_t t_i32_45 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_46 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_46 = v_35;
          uint32_t t_i32_47 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_47 = v_30;
          t_i32_45 = (t_i32_46 < t_i32_47);
          if (t_i32_45 != UINT32_C(0)) {
            uint32_t t_i32_48 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_48 = UINT32_C(1);
            return t_i32_48;
          } else {
            uint32_t t_i32_49 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_49 = UINT32_C(0);
          }
          uint32_t t_i32_50 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_50 = UINT32_C(0);
        }
      }
      v_28 = v_28 + UINT32_C(1);
    }
    t_i32_27 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_51 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_52 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_52 = v_11;
    uint32_t t_i32_53 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_53 = v_14;
    t_i32_51 = (t_i32_52 == t_i32_53);
    if (t_i32_51 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      out = UINT32_C(0);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_54 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_55 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_55 = v_11;
      uint32_t t_i32_56 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_56 = v_14;
      t_i32_54 = (t_i32_55 < t_i32_56);
      if (t_i32_54 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        out = UINT32_C(4294967295);
      } else {
        rt_fuel(ctx, 1);
        out = UINT32_C(1);
      }
    }
  }
  return out;
}

#line 1 "std/json.x07.json"
static vec_u8_t user_std_json__emit_member_kv(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, uint32_t p2, vec_u8_t p3) {
  vec_u8_t out = (vec_u8_t){0};
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p1;
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_4 = p2;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = UINT32_C(2);
    t_i32_3 = t_i32_4 << (t_i32_5 & UINT32_C(31));
    v_1 = rt_codec_read_u32_le(ctx, t_view_2, t_i32_3);
    uint32_t v_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_7 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_7 = p0;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = v_1;
    v_6 = rt_codec_read_u32_le(ctx, t_view_7, t_i32_8);
    uint32_t v_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = v_1;
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_11 = UINT32_C(4);
    v_9 = t_i32_10 + t_i32_11;
    uint32_t v_12 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = v_9;
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_14 = v_6;
    v_12 = t_i32_13 + t_i32_14;
    uint32_t v_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_16 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_16 = p0;
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_17 = v_12;
    v_15 = rt_codec_read_u32_le(ctx, t_view_16, t_i32_17);
    uint32_t v_18 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_19 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_19 = v_12;
    uint32_t t_i32_20 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_20 = UINT32_C(4);
    v_18 = t_i32_19 + t_i32_20;
    vec_u8_t t_set_21 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_22 = UINT32_C(34);
    p3 = rt_vec_u8_push(ctx, p3, t_i32_22);
    t_set_21 = p3;
    p3 = (vec_u8_t){0};
    p3 = t_set_21;
    t_set_21 = (vec_u8_t){0};
    vec_u8_t t_set_23 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    bytes_view_t t_view_24 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_24 = p0;
    uint32_t t_i32_25 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_25 = v_9;
    uint32_t t_i32_26 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_26 = v_6;
    p3 = rt_vec_u8_extend_bytes_range(ctx, p3, t_view_24, t_i32_25, t_i32_26);
    t_set_23 = p3;
    p3 = (vec_u8_t){0};
    p3 = t_set_23;
    t_set_23 = (vec_u8_t){0};
    vec_u8_t t_set_27 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_28 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_28 = UINT32_C(34);
    p3 = rt_vec_u8_push(ctx, p3, t_i32_28);
    t_set_27 = p3;
    p3 = (vec_u8_t){0};
    p3 = t_set_27;
    t_set_27 = (vec_u8_t){0};
    vec_u8_t t_set_29 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_30 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_30 = UINT32_C(58);
    p3 = rt_vec_u8_push(ctx, p3, t_i32_30);
    t_set_29 = p3;
    p3 = (vec_u8_t){0};
    p3 = t_set_29;
    t_set_29 = (vec_u8_t){0};
    vec_u8_t t_set_31 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    bytes_view_t t_view_32 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_32 = p0;
    uint32_t t_i32_33 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_33 = v_18;
    uint32_t t_i32_34 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_34 = v_15;
    p3 = rt_vec_u8_extend_bytes_range(ctx, p3, t_view_32, t_i32_33, t_i32_34);
    t_set_31 = p3;
    p3 = (vec_u8_t){0};
    p3 = t_set_31;
    t_set_31 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    out = p3;
    p3 = (vec_u8_t){0};
  }
  rt_vec_u8_drop(ctx, &p3);
  return out;
}

#line 1 "std/json.x07.json"
static vec_u8_t user_std_json__emit_object_sorted(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, uint32_t p2, vec_u8_t p3) {
  vec_u8_t out = (vec_u8_t){0};
  rt_fuel(ctx, 1);
  {
    vec_u8_t t_set_1 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_2 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_2 = UINT32_C(123);
    p3 = rt_vec_u8_push(ctx, p3, t_i32_2);
    t_set_1 = p3;
    p3 = (vec_u8_t){0};
    p3 = t_set_1;
    t_set_1 = (vec_u8_t){0};
    bytes_t v_3 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_4 = p2;
    v_3 = rt_bytes_alloc(ctx, t_i32_4);
    uint32_t v_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_5 = UINT32_C(1);
    uint32_t t_i32_6 = UINT32_C(0);
    uint32_t v_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_7 = UINT32_C(0);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = p2;
    for (;;) {
      if (v_7 >= t_i32_8) break;
      {
        {
          uint32_t v_9 = UINT32_C(0);
          rt_fuel(ctx, 1);
          v_9 = UINT32_C(4294967295);
          uint32_t t_i32_10 = UINT32_C(0);
          uint32_t v_11 = UINT32_C(0);
          rt_fuel(ctx, 1);
          v_11 = UINT32_C(0);
          uint32_t t_i32_12 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_12 = p2;
          for (;;) {
            if (v_11 >= t_i32_12) break;
            {
              {
                uint32_t t_i32_13 = UINT32_C(0);
                rt_fuel(ctx, 1);
                uint32_t t_i32_14 = UINT32_C(0);
                rt_fuel(ctx, 1);
                bytes_view_t t_view_15 = rt_view_empty(ctx);
                t_view_15 = rt_bytes_view(ctx, v_3);
                uint32_t t_i32_16 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_16 = v_11;
                ctx->trap_ptr = "/decls/3/body/4/4/2/4/1/1/1";
                t_i32_14 = rt_view_get_u8(ctx, t_view_15, t_i32_16);
                ctx->trap_ptr = NULL;
                uint32_t t_i32_17 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_17 = UINT32_C(1);
                t_i32_13 = (t_i32_14 == t_i32_17);
                if (t_i32_13 != UINT32_C(0)) {
                  uint32_t t_i32_18 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_18 = UINT32_C(0);
                } else {
                  uint32_t t_i32_19 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  uint32_t t_i32_20 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_20 = v_9;
                  uint32_t t_i32_21 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_21 = UINT32_C(4294967295);
                  t_i32_19 = (t_i32_20 == t_i32_21);
                  if (t_i32_19 != UINT32_C(0)) {
                    rt_fuel(ctx, 1);
                    v_9 = v_11;
                  } else {
                    uint32_t t_i32_22 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    uint32_t t_i32_23 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    bytes_view_t t_view_24 = rt_view_empty(ctx);
                    rt_fuel(ctx, 1);
                    t_view_24 = p0;
                    bytes_view_t t_view_25 = rt_view_empty(ctx);
                    rt_fuel(ctx, 1);
                    t_view_25 = p1;
                    uint32_t t_i32_26 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_26 = v_11;
                    uint32_t t_i32_27 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_27 = v_9;
                    t_i32_23 = user_std_json__cmp_member_key(ctx, input, t_view_24, t_view_25, t_i32_26, t_i32_27);
                    uint32_t t_i32_28 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_28 = UINT32_C(0);
                    t_i32_22 = ((t_i32_23 ^ UINT32_C(0x80000000)) < (t_i32_28 ^ UINT32_C(0x80000000)));
                    if (t_i32_22 != UINT32_C(0)) {
                      rt_fuel(ctx, 1);
                      v_9 = v_11;
                    } else {
                      uint32_t t_i32_29 = UINT32_C(0);
                      rt_fuel(ctx, 1);
                      t_i32_29 = UINT32_C(0);
                    }
                  }
                }
                uint32_t t_i32_30 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_30 = UINT32_C(0);
              }
            }
            v_11 = v_11 + UINT32_C(1);
          }
          t_i32_10 = UINT32_C(0);
          bytes_t t_set_31 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          uint32_t t_i32_32 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_32 = v_9;
          uint32_t t_i32_33 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_33 = UINT32_C(1);
          ctx->trap_ptr = "/decls/3/body/4/4/3/2";
          v_3 = rt_bytes_set_u8(ctx, v_3, t_i32_32, t_i32_33);
          ctx->trap_ptr = NULL;
          t_set_31 = v_3;
          v_3 = rt_bytes_empty(ctx);
          v_3 = t_set_31;
          t_set_31 = rt_bytes_empty(ctx);
          uint32_t t_i32_34 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_35 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_35 = v_5;
          uint32_t t_i32_36 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_36 = UINT32_C(0);
          t_i32_34 = (t_i32_35 == t_i32_36);
          if (t_i32_34 != UINT32_C(0)) {
            {
              vec_u8_t t_set_37 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              uint32_t t_i32_38 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_38 = UINT32_C(44);
              p3 = rt_vec_u8_push(ctx, p3, t_i32_38);
              t_set_37 = p3;
              p3 = (vec_u8_t){0};
              p3 = t_set_37;
              t_set_37 = (vec_u8_t){0};
              uint32_t t_i32_39 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_39 = UINT32_C(0);
            }
          } else {
            rt_fuel(ctx, 1);
            v_5 = UINT32_C(0);
          }
          vec_u8_t t_set_40 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          bytes_view_t t_view_41 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_41 = p0;
          bytes_view_t t_view_42 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_42 = p1;
          uint32_t t_i32_43 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_43 = v_9;
          vec_u8_t t_vec_u8_44 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          t_vec_u8_44 = p3;
          p3 = (vec_u8_t){0};
          t_set_40 = user_std_json__emit_member_kv(ctx, input, t_view_41, t_view_42, t_i32_43, t_vec_u8_44);
          t_vec_u8_44 = (vec_u8_t){0};
          p3 = t_set_40;
          t_set_40 = (vec_u8_t){0};
          uint32_t t_i32_45 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_45 = UINT32_C(0);
          rt_vec_u8_drop(ctx, &t_vec_u8_44);
        }
      }
      v_7 = v_7 + UINT32_C(1);
    }
    t_i32_6 = UINT32_C(0);
    vec_u8_t t_set_46 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_47 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_47 = UINT32_C(125);
    p3 = rt_vec_u8_push(ctx, p3, t_i32_47);
    t_set_46 = p3;
    p3 = (vec_u8_t){0};
    p3 = t_set_46;
    t_set_46 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    out = p3;
    p3 = (vec_u8_t){0};
    rt_bytes_drop(ctx, &v_3);
  }
  rt_vec_u8_drop(ctx, &p3);
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__err(ctx_t* ctx, bytes_view_t input) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_1[] = "ERR";
  out = rt_bytes_from_literal(ctx, (const uint8_t*)lit_1, UINT32_C(3));
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__extract_from_object(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2, uint32_t p3, uint32_t p4) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = p1;
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = UINT32_C(1);
    t_i32_5 = t_i32_6 + t_i32_7;
    v_3 = user_std_json__skip_ws(ctx, input, t_view_4, t_i32_5);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_9 = v_3;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = v_1;
    t_i32_8 = (t_i32_9 >= t_i32_10);
    if (t_i32_8 != UINT32_C(0)) {
      bytes_t t_bytes_11 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_11 = user_std_json__err(ctx, input);
      return t_bytes_11;
    } else {
      uint32_t t_i32_12 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_12 = UINT32_C(0);
    }
    bytes_view_t v_13 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_14 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_14 = p2;
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_15 = p3;
    v_13 = user_std_json__segment_at(ctx, input, t_view_14, t_i32_15);
    uint32_t v_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_17 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_17 = v_13;
    v_16 = t_view_17.len;
    uint32_t t_i32_18 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_19 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_19 = v_16;
    uint32_t t_i32_20 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_20 = UINT32_C(0);
    t_i32_18 = (t_i32_19 == t_i32_20);
    if (t_i32_18 != UINT32_C(0)) {
      bytes_t t_bytes_21 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_21 = user_std_json__err(ctx, input);
      return t_bytes_21;
    } else {
      uint32_t t_i32_22 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_22 = UINT32_C(0);
    }
    uint32_t t_i32_23 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_24 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_25 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_25 = p0;
    uint32_t t_i32_26 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_26 = v_3;
    ctx->trap_ptr = "/decls/5/body/7/1/1";
    t_i32_24 = rt_view_get_u8(ctx, t_view_25, t_i32_26);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_27 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_27 = UINT32_C(125);
    t_i32_23 = (t_i32_24 == t_i32_27);
    if (t_i32_23 != UINT32_C(0)) {
      bytes_t t_bytes_28 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_28 = user_std_json__err(ctx, input);
      return t_bytes_28;
    } else {
      uint32_t t_i32_29 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_29 = UINT32_C(0);
    }
    uint32_t t_i32_30 = UINT32_C(0);
    uint32_t v_31 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_31 = UINT32_C(0);
    uint32_t t_i32_32 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_33 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_33 = v_1;
    uint32_t t_i32_34 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_34 = UINT32_C(1);
    t_i32_32 = t_i32_33 + t_i32_34;
    for (;;) {
      if (v_31 >= t_i32_32) break;
      {
        {
          uint32_t t_i32_35 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_36 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_36 = v_3;
          uint32_t t_i32_37 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_37 = v_1;
          t_i32_35 = (t_i32_36 >= t_i32_37);
          if (t_i32_35 != UINT32_C(0)) {
            bytes_t t_bytes_38 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_38 = user_std_json__err(ctx, input);
            return t_bytes_38;
          } else {
            uint32_t t_i32_39 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_39 = UINT32_C(0);
          }
          uint32_t t_i32_40 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_41 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_42 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_42 = p0;
          uint32_t t_i32_43 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_43 = v_3;
          ctx->trap_ptr = "/decls/5/body/8/4/2/1/1";
          t_i32_41 = rt_view_get_u8(ctx, t_view_42, t_i32_43);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_44 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_44 = UINT32_C(34);
          t_i32_40 = (t_i32_41 == t_i32_44);
          if (t_i32_40 != UINT32_C(0)) {
            uint32_t t_i32_45 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_45 = UINT32_C(0);
          } else {
            bytes_t t_bytes_46 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_46 = user_std_json__err(ctx, input);
            return t_bytes_46;
          }
          uint32_t v_47 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_48 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_48 = v_3;
          uint32_t t_i32_49 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_49 = UINT32_C(1);
          v_47 = t_i32_48 + t_i32_49;
          uint32_t v_50 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_51 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_51 = p0;
          uint32_t t_i32_52 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_52 = v_47;
          v_50 = user_std_json__string_end_or_err(ctx, input, t_view_51, t_i32_52);
          uint32_t t_i32_53 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_54 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_54 = v_50;
          uint32_t t_i32_55 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_55 = UINT32_C(0);
          t_i32_53 = ((t_i32_54 ^ UINT32_C(0x80000000)) < (t_i32_55 ^ UINT32_C(0x80000000)));
          if (t_i32_53 != UINT32_C(0)) {
            bytes_t t_bytes_56 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_56 = user_std_json__err(ctx, input);
            return t_bytes_56;
          } else {
            uint32_t t_i32_57 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_57 = UINT32_C(0);
          }
          uint32_t v_58 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_59 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_59 = v_50;
          uint32_t t_i32_60 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_60 = v_47;
          v_58 = t_i32_59 - t_i32_60;
          rt_fuel(ctx, 1);
          uint32_t t_i32_61 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_61 = v_50;
          uint32_t t_i32_62 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_62 = UINT32_C(1);
          v_3 = t_i32_61 + t_i32_62;
          rt_fuel(ctx, 1);
          bytes_view_t t_view_63 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_63 = p0;
          uint32_t t_i32_64 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_64 = v_3;
          v_3 = user_std_json__skip_ws(ctx, input, t_view_63, t_i32_64);
          uint32_t t_i32_65 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_66 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_67 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_67 = v_3;
          uint32_t t_i32_68 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_68 = v_1;
          t_i32_66 = (t_i32_67 >= t_i32_68);
          if (t_i32_66 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            t_i32_65 = UINT32_C(1);
          } else {
            rt_fuel(ctx, 1);
            uint32_t t_i32_69 = UINT32_C(0);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_70 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_70 = p0;
            uint32_t t_i32_71 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_71 = v_3;
            ctx->trap_ptr = "/decls/5/body/8/4/9/1/3/1";
            t_i32_69 = rt_view_get_u8(ctx, t_view_70, t_i32_71);
            ctx->trap_ptr = NULL;
            uint32_t t_i32_72 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_72 = UINT32_C(58);
            t_i32_65 = (t_i32_69 == t_i32_72);
          }
          if (t_i32_65 != UINT32_C(0)) {
            uint32_t t_i32_73 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_73 = UINT32_C(0);
          } else {
            bytes_t t_bytes_74 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_74 = user_std_json__err(ctx, input);
            return t_bytes_74;
          }
          rt_fuel(ctx, 1);
          uint32_t t_i32_75 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_75 = v_3;
          uint32_t t_i32_76 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_76 = UINT32_C(1);
          v_3 = t_i32_75 + t_i32_76;
          uint32_t t_i32_77 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_78 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_79 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_79 = v_58;
          uint32_t t_i32_80 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_80 = v_16;
          t_i32_78 = (t_i32_79 == t_i32_80);
          if (t_i32_78 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            uint32_t t_i32_81 = UINT32_C(0);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_82 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_82 = p0;
            uint32_t t_i32_83 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_83 = v_47;
            uint32_t t_i32_84 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_84 = v_58;
            bytes_view_t t_view_85 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_85 = v_13;
            uint32_t t_i32_86 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_86 = UINT32_C(0);
            uint32_t t_i32_87 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_87 = v_16;
            t_i32_81 = rt_view_cmp_range(ctx, t_view_82, t_i32_83, t_i32_84, t_view_85, t_i32_86, t_i32_87);
            uint32_t t_i32_88 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_88 = UINT32_C(0);
            t_i32_77 = (t_i32_81 == t_i32_88);
          } else {
            rt_fuel(ctx, 1);
            t_i32_77 = UINT32_C(0);
          }
          if (t_i32_77 != UINT32_C(0)) {
            {
              uint32_t v_89 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_90 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_90 = p3;
              uint32_t t_i32_91 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_91 = UINT32_C(1);
              v_89 = t_i32_90 + t_i32_91;
              bytes_t t_bytes_92 = rt_bytes_empty(ctx);
              rt_fuel(ctx, 1);
              bytes_view_t t_view_93 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_93 = p0;
              uint32_t t_i32_94 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_94 = v_3;
              bytes_view_t t_view_95 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_95 = p2;
              uint32_t t_i32_96 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_96 = v_89;
              uint32_t t_i32_97 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_97 = p4;
              t_bytes_92 = user_std_json__extract_from_value(ctx, input, t_view_93, t_i32_94, t_view_95, t_i32_96, t_i32_97);
              return t_bytes_92;
            }
          } else {
            {
              uint32_t v_98 = UINT32_C(0);
              rt_fuel(ctx, 1);
              bytes_view_t t_view_99 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_99 = p0;
              uint32_t t_i32_100 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_100 = v_3;
              v_98 = user_std_json__skip_value(ctx, input, t_view_99, t_i32_100);
              uint32_t t_i32_101 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_102 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_102 = v_98;
              uint32_t t_i32_103 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_103 = UINT32_C(0);
              t_i32_101 = ((t_i32_102 ^ UINT32_C(0x80000000)) < (t_i32_103 ^ UINT32_C(0x80000000)));
              if (t_i32_101 != UINT32_C(0)) {
                bytes_t t_bytes_104 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                t_bytes_104 = user_std_json__err(ctx, input);
                return t_bytes_104;
              } else {
                rt_fuel(ctx, 1);
                bytes_view_t t_view_105 = rt_view_empty(ctx);
                rt_fuel(ctx, 1);
                t_view_105 = p0;
                uint32_t t_i32_106 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_106 = v_98;
                v_3 = user_std_json__skip_ws(ctx, input, t_view_105, t_i32_106);
              }
            }
          }
          uint32_t t_i32_107 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_108 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_108 = v_3;
          uint32_t t_i32_109 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_109 = v_1;
          t_i32_107 = (t_i32_108 >= t_i32_109);
          if (t_i32_107 != UINT32_C(0)) {
            bytes_t t_bytes_110 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_110 = user_std_json__err(ctx, input);
            return t_bytes_110;
          } else {
            uint32_t t_i32_111 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_111 = UINT32_C(0);
          }
          uint32_t v_112 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_113 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_113 = p0;
          uint32_t t_i32_114 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_114 = v_3;
          ctx->trap_ptr = "/decls/5/body/8/4/13/2";
          v_112 = rt_view_get_u8(ctx, t_view_113, t_i32_114);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_115 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_116 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_116 = v_112;
          uint32_t t_i32_117 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_117 = UINT32_C(44);
          t_i32_115 = (t_i32_116 == t_i32_117);
          if (t_i32_115 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_118 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_118 = p0;
            uint32_t t_i32_119 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_120 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_120 = v_3;
            uint32_t t_i32_121 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_121 = UINT32_C(1);
            t_i32_119 = t_i32_120 + t_i32_121;
            v_3 = user_std_json__skip_ws(ctx, input, t_view_118, t_i32_119);
          } else {
            bytes_t t_bytes_122 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_122 = user_std_json__err(ctx, input);
            return t_bytes_122;
          }
          uint32_t t_i32_123 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_123 = UINT32_C(0);
        }
      }
      v_31 = v_31 + UINT32_C(1);
    }
    t_i32_30 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = user_std_json__err(ctx, input);
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__extract_from_value(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2, uint32_t p3, uint32_t p4) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p1;
    v_3 = user_std_json__skip_ws(ctx, input, t_view_4, t_i32_5);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = v_3;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = v_1;
    t_i32_6 = (t_i32_7 >= t_i32_8);
    if (t_i32_6 != UINT32_C(0)) {
      bytes_t t_bytes_9 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_9 = user_std_json__err(ctx, input);
      return t_bytes_9;
    } else {
      uint32_t t_i32_10 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_10 = UINT32_C(0);
    }
    rt_fuel(ctx, 1);
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_12 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_12 = p3;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = p4;
    t_i32_11 = (t_i32_12 == t_i32_13);
    if (t_i32_11 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      bytes_view_t t_view_14 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      t_view_14 = p0;
      uint32_t t_i32_15 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_15 = v_3;
      out = user_std_json__parse_value(ctx, input, t_view_14, t_i32_15);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_16 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_17 = UINT32_C(0);
      rt_fuel(ctx, 1);
      bytes_view_t t_view_18 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      t_view_18 = p0;
      uint32_t t_i32_19 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_19 = v_3;
      ctx->trap_ptr = "/decls/6/body/4/3/1/1";
      t_i32_17 = rt_view_get_u8(ctx, t_view_18, t_i32_19);
      ctx->trap_ptr = NULL;
      uint32_t t_i32_20 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_20 = UINT32_C(123);
      t_i32_16 = (t_i32_17 == t_i32_20);
      if (t_i32_16 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        bytes_view_t t_view_21 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_21 = p0;
        uint32_t t_i32_22 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_22 = v_3;
        bytes_view_t t_view_23 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_23 = p2;
        uint32_t t_i32_24 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_24 = p3;
        uint32_t t_i32_25 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_25 = p4;
        out = user_std_json__extract_from_object(ctx, input, t_view_21, t_i32_22, t_view_23, t_i32_24, t_i32_25);
      } else {
        rt_fuel(ctx, 1);
        out = user_std_json__err(ctx, input);
      }
    }
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__find_byte(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t t_i32_3 = UINT32_C(0);
    uint32_t v_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_4 = UINT32_C(0);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = v_1;
    for (;;) {
      if (v_4 >= t_i32_5) break;
      {
        {
          uint32_t t_i32_6 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_7 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_8 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_8 = p0;
          uint32_t t_i32_9 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_9 = v_4;
          ctx->trap_ptr = "/decls/7/body/2/4/1/1/1";
          t_i32_7 = rt_view_get_u8(ctx, t_view_8, t_i32_9);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_10 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_10 = p1;
          t_i32_6 = (t_i32_7 == t_i32_10);
          if (t_i32_6 != UINT32_C(0)) {
            uint32_t t_i32_11 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_11 = v_4;
            return t_i32_11;
          } else {
            uint32_t t_i32_12 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_12 = UINT32_C(0);
          }
          uint32_t t_i32_13 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_13 = UINT32_C(0);
        }
      }
      v_4 = v_4 + UINT32_C(1);
    }
    t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = UINT32_C(4294967295);
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__is_digit(ctx_t* ctx, bytes_view_t input, uint32_t p0) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_2 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_2 = p0;
  uint32_t t_i32_3 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_3 = UINT32_C(48);
  t_i32_1 = (t_i32_2 >= t_i32_3);
  if (t_i32_1 != UINT32_C(0)) {
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p0;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = UINT32_C(58);
    t_i32_4 = (t_i32_5 < t_i32_6);
    if (t_i32_4 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      out = UINT32_C(1);
    } else {
      rt_fuel(ctx, 1);
      out = UINT32_C(0);
    }
  } else {
    rt_fuel(ctx, 1);
    out = UINT32_C(0);
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__is_hex_digit(ctx_t* ctx, bytes_view_t input, uint32_t p0) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_2 = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_3 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_3 = p0;
  uint32_t t_i32_4 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_4 = UINT32_C(48);
  t_i32_2 = (t_i32_3 >= t_i32_4);
  if (t_i32_2 != UINT32_C(0)) {
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p0;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = UINT32_C(58);
    t_i32_1 = (t_i32_5 < t_i32_6);
  } else {
    rt_fuel(ctx, 1);
    t_i32_1 = UINT32_C(0);
  }
  if (t_i32_1 != UINT32_C(0)) {
    rt_fuel(ctx, 1);
    out = UINT32_C(1);
  } else {
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_9 = p0;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = UINT32_C(65);
    t_i32_8 = (t_i32_9 >= t_i32_10);
    if (t_i32_8 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      uint32_t t_i32_11 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_11 = p0;
      uint32_t t_i32_12 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_12 = UINT32_C(71);
      t_i32_7 = (t_i32_11 < t_i32_12);
    } else {
      rt_fuel(ctx, 1);
      t_i32_7 = UINT32_C(0);
    }
    if (t_i32_7 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      out = UINT32_C(1);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_13 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_14 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_15 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_15 = p0;
      uint32_t t_i32_16 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_16 = UINT32_C(97);
      t_i32_14 = (t_i32_15 >= t_i32_16);
      if (t_i32_14 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        uint32_t t_i32_17 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_17 = p0;
        uint32_t t_i32_18 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_18 = UINT32_C(103);
        t_i32_13 = (t_i32_17 < t_i32_18);
      } else {
        rt_fuel(ctx, 1);
        t_i32_13 = UINT32_C(0);
      }
      if (t_i32_13 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        out = UINT32_C(1);
      } else {
        rt_fuel(ctx, 1);
        out = UINT32_C(0);
      }
    }
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__is_ws(ctx_t* ctx, bytes_view_t input, uint32_t p0) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_2 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_2 = p0;
  uint32_t t_i32_3 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_3 = UINT32_C(32);
  t_i32_1 = (t_i32_2 == t_i32_3);
  if (t_i32_1 != UINT32_C(0)) {
    rt_fuel(ctx, 1);
    out = UINT32_C(1);
  } else {
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p0;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = UINT32_C(9);
    t_i32_4 = (t_i32_5 == t_i32_6);
    if (t_i32_4 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      out = UINT32_C(1);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_7 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_8 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_8 = p0;
      uint32_t t_i32_9 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_9 = UINT32_C(10);
      t_i32_7 = (t_i32_8 == t_i32_9);
      if (t_i32_7 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        out = UINT32_C(1);
      } else {
        rt_fuel(ctx, 1);
        uint32_t t_i32_10 = UINT32_C(0);
        rt_fuel(ctx, 1);
        uint32_t t_i32_11 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_11 = p0;
        uint32_t t_i32_12 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_12 = UINT32_C(13);
        t_i32_10 = (t_i32_11 == t_i32_12);
        if (t_i32_10 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          out = UINT32_C(1);
        } else {
          rt_fuel(ctx, 1);
          out = UINT32_C(0);
        }
      }
    }
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__match_bytes_at(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p2;
    v_3 = t_view_4.len;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = p1;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = v_3;
    t_i32_6 = t_i32_7 + t_i32_8;
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = v_1;
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_11 = UINT32_C(1);
    t_i32_9 = t_i32_10 + t_i32_11;
    t_i32_5 = (t_i32_6 >= t_i32_9);
    if (t_i32_5 != UINT32_C(0)) {
      uint32_t t_i32_12 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_12 = UINT32_C(0);
      return t_i32_12;
    } else {
      uint32_t t_i32_13 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_13 = UINT32_C(0);
    }
    uint32_t t_i32_14 = UINT32_C(0);
    uint32_t v_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_15 = UINT32_C(0);
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = v_3;
    for (;;) {
      if (v_15 >= t_i32_16) break;
      {
        {
          uint32_t t_i32_17 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_18 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_19 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_19 = p0;
          uint32_t t_i32_20 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_21 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_21 = p1;
          uint32_t t_i32_22 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_22 = v_15;
          t_i32_20 = t_i32_21 + t_i32_22;
          ctx->trap_ptr = "/decls/11/body/4/4/1/1/1";
          t_i32_18 = rt_view_get_u8(ctx, t_view_19, t_i32_20);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_23 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_24 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_24 = p2;
          uint32_t t_i32_25 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_25 = v_15;
          ctx->trap_ptr = "/decls/11/body/4/4/1/1/2";
          t_i32_23 = rt_view_get_u8(ctx, t_view_24, t_i32_25);
          ctx->trap_ptr = NULL;
          t_i32_17 = (t_i32_18 == t_i32_23);
          if (t_i32_17 != UINT32_C(0)) {
            uint32_t t_i32_26 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_26 = UINT32_C(0);
          } else {
            uint32_t t_i32_27 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_27 = UINT32_C(0);
            return t_i32_27;
          }
          uint32_t t_i32_28 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_28 = UINT32_C(0);
        }
      }
      v_15 = v_15 + UINT32_C(1);
    }
    t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = UINT32_C(1);
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__parse_array(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    vec_u8_t v_3 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_5 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_5 = p0;
    t_i32_4 = t_view_5.len;
    v_3 = rt_vec_u8_new(ctx, t_i32_4);
    vec_u8_t t_set_6 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = UINT32_C(91);
    v_3 = rt_vec_u8_push(ctx, v_3, t_i32_7);
    t_set_6 = v_3;
    v_3 = (vec_u8_t){0};
    v_3 = t_set_6;
    t_set_6 = (vec_u8_t){0};
    uint32_t v_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_9 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_9 = p0;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_11 = p1;
    uint32_t t_i32_12 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_12 = UINT32_C(1);
    t_i32_10 = t_i32_11 + t_i32_12;
    v_8 = user_std_json__skip_ws(ctx, input, t_view_9, t_i32_10);
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_14 = v_8;
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_15 = v_1;
    t_i32_13 = (t_i32_14 >= t_i32_15);
    if (t_i32_13 != UINT32_C(0)) {
      bytes_t t_bytes_16 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_16 = user_std_json__err(ctx, input);
      rt_vec_u8_drop(ctx, &v_3);
      return t_bytes_16;
    } else {
      uint32_t t_i32_17 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_17 = UINT32_C(0);
    }
    uint32_t t_i32_18 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_19 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_20 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_20 = p0;
    uint32_t t_i32_21 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_21 = v_8;
    ctx->trap_ptr = "/decls/15/body/6/1/1";
    t_i32_19 = rt_view_get_u8(ctx, t_view_20, t_i32_21);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_22 = UINT32_C(93);
    t_i32_18 = (t_i32_19 == t_i32_22);
    if (t_i32_18 != UINT32_C(0)) {
      {
        vec_u8_t t_set_23 = (vec_u8_t){0};
        rt_fuel(ctx, 1);
        uint32_t t_i32_24 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_24 = UINT32_C(93);
        v_3 = rt_vec_u8_push(ctx, v_3, t_i32_24);
        t_set_23 = v_3;
        v_3 = (vec_u8_t){0};
        v_3 = t_set_23;
        t_set_23 = (vec_u8_t){0};
        bytes_t t_bytes_25 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_bytes_25 = rt_vec_u8_into_bytes(ctx, &v_3);
        rt_vec_u8_drop(ctx, &v_3);
        return t_bytes_25;
      }
    } else {
      uint32_t t_i32_26 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_26 = UINT32_C(0);
    }
    uint32_t v_27 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_27 = UINT32_C(1);
    uint32_t t_i32_28 = UINT32_C(0);
    uint32_t v_29 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_29 = UINT32_C(0);
    uint32_t t_i32_30 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_31 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_31 = v_1;
    uint32_t t_i32_32 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_32 = UINT32_C(1);
    t_i32_30 = t_i32_31 + t_i32_32;
    for (;;) {
      if (v_29 >= t_i32_30) break;
      {
        {
          uint32_t t_i32_33 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_34 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_34 = v_27;
          uint32_t t_i32_35 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_35 = UINT32_C(0);
          t_i32_33 = (t_i32_34 == t_i32_35);
          if (t_i32_33 != UINT32_C(0)) {
            {
              vec_u8_t t_set_36 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              uint32_t t_i32_37 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_37 = UINT32_C(44);
              v_3 = rt_vec_u8_push(ctx, v_3, t_i32_37);
              t_set_36 = v_3;
              v_3 = (vec_u8_t){0};
              v_3 = t_set_36;
              t_set_36 = (vec_u8_t){0};
              uint32_t t_i32_38 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_38 = UINT32_C(0);
            }
          } else {
            rt_fuel(ctx, 1);
            v_27 = UINT32_C(0);
          }
          bytes_t v_39 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_40 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_40 = p0;
          uint32_t t_i32_41 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_41 = v_8;
          v_39 = user_std_json__parse_value(ctx, input, t_view_40, t_i32_41);
          uint32_t t_i32_42 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_43 = rt_view_empty(ctx);
          t_view_43 = rt_bytes_view(ctx, v_39);
          bytes_t t_bytes_44 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          t_bytes_44 = user_std_json__err(ctx, input);
          bytes_view_t t_view_45 = rt_view_empty(ctx);
          t_view_45 = rt_bytes_view(ctx, t_bytes_44);
          t_i32_42 = rt_view_eq(ctx, t_view_43, t_view_45);
          if (t_i32_42 != UINT32_C(0)) {
            bytes_t t_bytes_46 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_46 = user_std_json__err(ctx, input);
            rt_bytes_drop(ctx, &t_bytes_44);
            rt_bytes_drop(ctx, &v_39);
            rt_vec_u8_drop(ctx, &v_3);
            return t_bytes_46;
          } else {
            uint32_t t_i32_47 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_47 = UINT32_C(0);
          }
          vec_u8_t t_set_48 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          bytes_view_t t_view_49 = rt_view_empty(ctx);
          t_view_49 = rt_bytes_view(ctx, v_39);
          v_3 = rt_vec_u8_extend_bytes(ctx, v_3, t_view_49);
          t_set_48 = v_3;
          v_3 = (vec_u8_t){0};
          v_3 = t_set_48;
          t_set_48 = (vec_u8_t){0};
          uint32_t v_50 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_51 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_51 = p0;
          uint32_t t_i32_52 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_52 = v_8;
          v_50 = user_std_json__skip_value(ctx, input, t_view_51, t_i32_52);
          uint32_t t_i32_53 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_54 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_54 = v_50;
          uint32_t t_i32_55 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_55 = UINT32_C(0);
          t_i32_53 = ((t_i32_54 ^ UINT32_C(0x80000000)) < (t_i32_55 ^ UINT32_C(0x80000000)));
          if (t_i32_53 != UINT32_C(0)) {
            bytes_t t_bytes_56 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_56 = user_std_json__err(ctx, input);
            rt_bytes_drop(ctx, &t_bytes_44);
            rt_bytes_drop(ctx, &v_39);
            rt_vec_u8_drop(ctx, &v_3);
            return t_bytes_56;
          } else {
            uint32_t t_i32_57 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_57 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          bytes_view_t t_view_58 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_58 = p0;
          uint32_t t_i32_59 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_59 = v_50;
          v_8 = user_std_json__skip_ws(ctx, input, t_view_58, t_i32_59);
          uint32_t t_i32_60 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_61 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_61 = v_8;
          uint32_t t_i32_62 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_62 = v_1;
          t_i32_60 = (t_i32_61 >= t_i32_62);
          if (t_i32_60 != UINT32_C(0)) {
            bytes_t t_bytes_63 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_63 = user_std_json__err(ctx, input);
            rt_bytes_drop(ctx, &t_bytes_44);
            rt_bytes_drop(ctx, &v_39);
            rt_vec_u8_drop(ctx, &v_3);
            return t_bytes_63;
          } else {
            uint32_t t_i32_64 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_64 = UINT32_C(0);
          }
          uint32_t v_65 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_66 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_66 = p0;
          uint32_t t_i32_67 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_67 = v_8;
          ctx->trap_ptr = "/decls/15/body/8/4/9/2";
          v_65 = rt_view_get_u8(ctx, t_view_66, t_i32_67);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_68 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_69 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_69 = v_65;
          uint32_t t_i32_70 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_70 = UINT32_C(44);
          t_i32_68 = (t_i32_69 == t_i32_70);
          if (t_i32_68 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_71 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_71 = p0;
            uint32_t t_i32_72 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_73 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_73 = v_8;
            uint32_t t_i32_74 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_74 = UINT32_C(1);
            t_i32_72 = t_i32_73 + t_i32_74;
            v_8 = user_std_json__skip_ws(ctx, input, t_view_71, t_i32_72);
          } else {
            uint32_t t_i32_75 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_76 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_76 = v_65;
            uint32_t t_i32_77 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_77 = UINT32_C(93);
            t_i32_75 = (t_i32_76 == t_i32_77);
            if (t_i32_75 != UINT32_C(0)) {
              {
                vec_u8_t t_set_78 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                uint32_t t_i32_79 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_79 = UINT32_C(93);
                v_3 = rt_vec_u8_push(ctx, v_3, t_i32_79);
                t_set_78 = v_3;
                v_3 = (vec_u8_t){0};
                v_3 = t_set_78;
                t_set_78 = (vec_u8_t){0};
                bytes_t t_bytes_80 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                t_bytes_80 = rt_vec_u8_into_bytes(ctx, &v_3);
                rt_bytes_drop(ctx, &t_bytes_44);
                rt_bytes_drop(ctx, &v_39);
                rt_vec_u8_drop(ctx, &v_3);
                return t_bytes_80;
              }
            } else {
              bytes_t t_bytes_81 = rt_bytes_empty(ctx);
              rt_fuel(ctx, 1);
              t_bytes_81 = user_std_json__err(ctx, input);
              rt_bytes_drop(ctx, &t_bytes_44);
              rt_bytes_drop(ctx, &v_39);
              rt_vec_u8_drop(ctx, &v_3);
              return t_bytes_81;
            }
          }
          uint32_t t_i32_82 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_82 = UINT32_C(0);
          rt_bytes_drop(ctx, &t_bytes_44);
          rt_bytes_drop(ctx, &v_39);
        }
      }
      v_29 = v_29 + UINT32_C(1);
    }
    t_i32_28 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = user_std_json__err(ctx, input);
    rt_vec_u8_drop(ctx, &v_3);
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__parse_lit(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_2 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_2 = p0;
  uint32_t t_i32_3 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_3 = p1;
  bytes_view_t t_view_4 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_4 = p2;
  t_i32_1 = user_std_json__match_bytes_at(ctx, input, t_view_2, t_i32_3, t_view_4);
  if (t_i32_1 != UINT32_C(0)) {
    rt_fuel(ctx, 1);
    bytes_view_t t_view_5 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_5 = p0;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = p1;
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_8 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_8 = p2;
    t_i32_7 = t_view_8.len;
    ctx->trap_ptr = "/decls/17/body/1/2";
    out = rt_view_to_bytes(ctx, rt_view_slice(ctx, t_view_5, t_i32_6, t_i32_7));
    ctx->trap_ptr = NULL;
  } else {
    rt_fuel(ctx, 1);
    out = user_std_json__err(ctx, input);
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__parse_number(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_3 = p1;
    v_1 = user_std_json__skip_number(ctx, input, t_view_2, t_i32_3);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = v_1;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = UINT32_C(0);
    t_i32_4 = ((t_i32_5 ^ UINT32_C(0x80000000)) < (t_i32_6 ^ UINT32_C(0x80000000)));
    if (t_i32_4 != UINT32_C(0)) {
      bytes_t t_bytes_7 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_7 = user_std_json__err(ctx, input);
      return t_bytes_7;
    } else {
      uint32_t t_i32_8 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_8 = UINT32_C(0);
    }
    rt_fuel(ctx, 1);
    bytes_view_t t_view_9 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_9 = p0;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = p1;
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_12 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_12 = v_1;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = p1;
    t_i32_11 = t_i32_12 - t_i32_13;
    ctx->trap_ptr = "/decls/18/body/3";
    out = rt_view_to_bytes(ctx, rt_view_slice(ctx, t_view_9, t_i32_10, t_i32_11));
    ctx->trap_ptr = NULL;
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__parse_object(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = p1;
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = UINT32_C(1);
    t_i32_5 = t_i32_6 + t_i32_7;
    v_3 = user_std_json__skip_ws(ctx, input, t_view_4, t_i32_5);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_9 = v_3;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = v_1;
    t_i32_8 = (t_i32_9 >= t_i32_10);
    if (t_i32_8 != UINT32_C(0)) {
      bytes_t t_bytes_11 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_11 = user_std_json__err(ctx, input);
      return t_bytes_11;
    } else {
      uint32_t t_i32_12 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_12 = UINT32_C(0);
    }
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_15 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_15 = p0;
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = v_3;
    ctx->trap_ptr = "/decls/19/body/4/1/1";
    t_i32_14 = rt_view_get_u8(ctx, t_view_15, t_i32_16);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_17 = UINT32_C(125);
    t_i32_13 = (t_i32_14 == t_i32_17);
    if (t_i32_13 != UINT32_C(0)) {
      {
        vec_u8_t v_18 = (vec_u8_t){0};
        rt_fuel(ctx, 1);
        uint32_t t_i32_19 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_19 = UINT32_C(2);
        v_18 = rt_vec_u8_new(ctx, t_i32_19);
        vec_u8_t t_set_20 = (vec_u8_t){0};
        rt_fuel(ctx, 1);
        uint32_t t_i32_21 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_21 = UINT32_C(123);
        v_18 = rt_vec_u8_push(ctx, v_18, t_i32_21);
        t_set_20 = v_18;
        v_18 = (vec_u8_t){0};
        v_18 = t_set_20;
        t_set_20 = (vec_u8_t){0};
        vec_u8_t t_set_22 = (vec_u8_t){0};
        rt_fuel(ctx, 1);
        uint32_t t_i32_23 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_23 = UINT32_C(125);
        v_18 = rt_vec_u8_push(ctx, v_18, t_i32_23);
        t_set_22 = v_18;
        v_18 = (vec_u8_t){0};
        v_18 = t_set_22;
        t_set_22 = (vec_u8_t){0};
        bytes_t t_bytes_24 = rt_bytes_empty(ctx);
        rt_fuel(ctx, 1);
        t_bytes_24 = rt_vec_u8_into_bytes(ctx, &v_18);
        rt_vec_u8_drop(ctx, &v_18);
        return t_bytes_24;
        rt_vec_u8_drop(ctx, &v_18);
      }
    } else {
      uint32_t t_i32_25 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_25 = UINT32_C(0);
    }
    vec_u8_t v_26 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_27 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_27 = UINT32_C(0);
    v_26 = rt_vec_u8_new(ctx, t_i32_27);
    vec_u8_t v_28 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_29 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_29 = UINT32_C(0);
    v_28 = rt_vec_u8_new(ctx, t_i32_29);
    uint32_t v_30 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_30 = UINT32_C(0);
    uint32_t t_i32_31 = UINT32_C(0);
    uint32_t v_32 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_32 = UINT32_C(0);
    uint32_t t_i32_33 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_34 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_34 = v_1;
    uint32_t t_i32_35 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_35 = UINT32_C(1);
    t_i32_33 = t_i32_34 + t_i32_35;
    for (;;) {
      if (v_32 >= t_i32_33) break;
      {
        {
          uint32_t t_i32_36 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_37 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_37 = v_3;
          uint32_t t_i32_38 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_38 = v_1;
          t_i32_36 = (t_i32_37 >= t_i32_38);
          if (t_i32_36 != UINT32_C(0)) {
            bytes_t t_bytes_39 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_39 = user_std_json__err(ctx, input);
            rt_vec_u8_drop(ctx, &v_26);
            rt_vec_u8_drop(ctx, &v_28);
            return t_bytes_39;
          } else {
            uint32_t t_i32_40 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_40 = UINT32_C(0);
          }
          uint32_t t_i32_41 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_42 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_43 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_43 = p0;
          uint32_t t_i32_44 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_44 = v_3;
          ctx->trap_ptr = "/decls/19/body/8/4/2/1/1";
          t_i32_42 = rt_view_get_u8(ctx, t_view_43, t_i32_44);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_45 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_45 = UINT32_C(34);
          t_i32_41 = (t_i32_42 == t_i32_45);
          if (t_i32_41 != UINT32_C(0)) {
            uint32_t t_i32_46 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_46 = UINT32_C(0);
          } else {
            bytes_t t_bytes_47 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_47 = user_std_json__err(ctx, input);
            rt_vec_u8_drop(ctx, &v_26);
            rt_vec_u8_drop(ctx, &v_28);
            return t_bytes_47;
          }
          uint32_t v_48 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_49 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_49 = v_3;
          uint32_t t_i32_50 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_50 = UINT32_C(1);
          v_48 = t_i32_49 + t_i32_50;
          uint32_t v_51 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_52 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_52 = p0;
          uint32_t t_i32_53 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_53 = v_48;
          v_51 = user_std_json__string_end_or_err(ctx, input, t_view_52, t_i32_53);
          uint32_t t_i32_54 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_55 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_55 = v_51;
          uint32_t t_i32_56 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_56 = UINT32_C(0);
          t_i32_54 = ((t_i32_55 ^ UINT32_C(0x80000000)) < (t_i32_56 ^ UINT32_C(0x80000000)));
          if (t_i32_54 != UINT32_C(0)) {
            bytes_t t_bytes_57 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_57 = user_std_json__err(ctx, input);
            rt_vec_u8_drop(ctx, &v_26);
            rt_vec_u8_drop(ctx, &v_28);
            return t_bytes_57;
          } else {
            uint32_t t_i32_58 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_58 = UINT32_C(0);
          }
          uint32_t v_59 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_60 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_60 = v_51;
          uint32_t t_i32_61 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_61 = v_48;
          v_59 = t_i32_60 - t_i32_61;
          rt_fuel(ctx, 1);
          uint32_t t_i32_62 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_62 = v_51;
          uint32_t t_i32_63 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_63 = UINT32_C(1);
          v_3 = t_i32_62 + t_i32_63;
          rt_fuel(ctx, 1);
          bytes_view_t t_view_64 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_64 = p0;
          uint32_t t_i32_65 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_65 = v_3;
          v_3 = user_std_json__skip_ws(ctx, input, t_view_64, t_i32_65);
          uint32_t t_i32_66 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_67 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_68 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_68 = v_3;
          uint32_t t_i32_69 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_69 = v_1;
          t_i32_67 = (t_i32_68 >= t_i32_69);
          if (t_i32_67 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            t_i32_66 = UINT32_C(1);
          } else {
            rt_fuel(ctx, 1);
            uint32_t t_i32_70 = UINT32_C(0);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_71 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_71 = p0;
            uint32_t t_i32_72 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_72 = v_3;
            ctx->trap_ptr = "/decls/19/body/8/4/9/1/3/1";
            t_i32_70 = rt_view_get_u8(ctx, t_view_71, t_i32_72);
            ctx->trap_ptr = NULL;
            uint32_t t_i32_73 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_73 = UINT32_C(58);
            t_i32_66 = (t_i32_70 == t_i32_73);
          }
          if (t_i32_66 != UINT32_C(0)) {
            uint32_t t_i32_74 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_74 = UINT32_C(0);
          } else {
            bytes_t t_bytes_75 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_75 = user_std_json__err(ctx, input);
            rt_vec_u8_drop(ctx, &v_26);
            rt_vec_u8_drop(ctx, &v_28);
            return t_bytes_75;
          }
          rt_fuel(ctx, 1);
          uint32_t t_i32_76 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_76 = v_3;
          uint32_t t_i32_77 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_77 = UINT32_C(1);
          v_3 = t_i32_76 + t_i32_77;
          bytes_t v_78 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_79 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_79 = p0;
          uint32_t t_i32_80 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_80 = v_3;
          v_78 = user_std_json__parse_value(ctx, input, t_view_79, t_i32_80);
          uint32_t t_i32_81 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_82 = rt_view_empty(ctx);
          t_view_82 = rt_bytes_view(ctx, v_78);
          bytes_t t_bytes_83 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          t_bytes_83 = user_std_json__err(ctx, input);
          bytes_view_t t_view_84 = rt_view_empty(ctx);
          t_view_84 = rt_bytes_view(ctx, t_bytes_83);
          t_i32_81 = rt_view_eq(ctx, t_view_82, t_view_84);
          if (t_i32_81 != UINT32_C(0)) {
            bytes_t t_bytes_85 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_85 = user_std_json__err(ctx, input);
            rt_bytes_drop(ctx, &t_bytes_83);
            rt_bytes_drop(ctx, &v_78);
            rt_vec_u8_drop(ctx, &v_26);
            rt_vec_u8_drop(ctx, &v_28);
            return t_bytes_85;
          } else {
            uint32_t t_i32_86 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_86 = UINT32_C(0);
          }
          uint32_t v_87 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_88 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_88 = p0;
          uint32_t t_i32_89 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_89 = v_3;
          v_87 = user_std_json__skip_value(ctx, input, t_view_88, t_i32_89);
          uint32_t t_i32_90 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_91 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_91 = v_87;
          uint32_t t_i32_92 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_92 = UINT32_C(0);
          t_i32_90 = ((t_i32_91 ^ UINT32_C(0x80000000)) < (t_i32_92 ^ UINT32_C(0x80000000)));
          if (t_i32_90 != UINT32_C(0)) {
            bytes_t t_bytes_93 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_93 = user_std_json__err(ctx, input);
            rt_bytes_drop(ctx, &t_bytes_83);
            rt_bytes_drop(ctx, &v_78);
            rt_vec_u8_drop(ctx, &v_26);
            rt_vec_u8_drop(ctx, &v_28);
            return t_bytes_93;
          } else {
            uint32_t t_i32_94 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_94 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          bytes_view_t t_view_95 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_95 = p0;
          uint32_t t_i32_96 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_96 = v_87;
          v_3 = user_std_json__skip_ws(ctx, input, t_view_95, t_i32_96);
          uint32_t v_97 = UINT32_C(0);
          rt_fuel(ctx, 1);
          v_97 = rt_vec_u8_len(ctx, v_26);
          vec_u8_t t_set_98 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          bytes_t t_bytes_99 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          uint32_t t_i32_100 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_100 = v_97;
          t_bytes_99 = rt_codec_write_u32_le(ctx, t_i32_100);
          bytes_view_t t_view_101 = rt_view_empty(ctx);
          t_view_101 = rt_bytes_view(ctx, t_bytes_99);
          v_28 = rt_vec_u8_extend_bytes(ctx, v_28, t_view_101);
          t_set_98 = v_28;
          v_28 = (vec_u8_t){0};
          v_28 = t_set_98;
          t_set_98 = (vec_u8_t){0};
          vec_u8_t t_set_102 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          bytes_t t_bytes_103 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          uint32_t t_i32_104 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_104 = v_59;
          t_bytes_103 = rt_codec_write_u32_le(ctx, t_i32_104);
          bytes_view_t t_view_105 = rt_view_empty(ctx);
          t_view_105 = rt_bytes_view(ctx, t_bytes_103);
          v_26 = rt_vec_u8_extend_bytes(ctx, v_26, t_view_105);
          t_set_102 = v_26;
          v_26 = (vec_u8_t){0};
          v_26 = t_set_102;
          t_set_102 = (vec_u8_t){0};
          vec_u8_t t_set_106 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          bytes_view_t t_view_107 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_107 = p0;
          uint32_t t_i32_108 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_108 = v_48;
          uint32_t t_i32_109 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_109 = v_59;
          v_26 = rt_vec_u8_extend_bytes_range(ctx, v_26, t_view_107, t_i32_108, t_i32_109);
          t_set_106 = v_26;
          v_26 = (vec_u8_t){0};
          v_26 = t_set_106;
          t_set_106 = (vec_u8_t){0};
          uint32_t v_110 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_111 = rt_view_empty(ctx);
          t_view_111 = rt_bytes_view(ctx, v_78);
          v_110 = t_view_111.len;
          vec_u8_t t_set_112 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          bytes_t t_bytes_113 = rt_bytes_empty(ctx);
          rt_fuel(ctx, 1);
          uint32_t t_i32_114 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_114 = v_110;
          t_bytes_113 = rt_codec_write_u32_le(ctx, t_i32_114);
          bytes_view_t t_view_115 = rt_view_empty(ctx);
          t_view_115 = rt_bytes_view(ctx, t_bytes_113);
          v_26 = rt_vec_u8_extend_bytes(ctx, v_26, t_view_115);
          t_set_112 = v_26;
          v_26 = (vec_u8_t){0};
          v_26 = t_set_112;
          t_set_112 = (vec_u8_t){0};
          vec_u8_t t_set_116 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          bytes_view_t t_view_117 = rt_view_empty(ctx);
          t_view_117 = rt_bytes_view(ctx, v_78);
          v_26 = rt_vec_u8_extend_bytes(ctx, v_26, t_view_117);
          t_set_116 = v_26;
          v_26 = (vec_u8_t){0};
          v_26 = t_set_116;
          t_set_116 = (vec_u8_t){0};
          rt_fuel(ctx, 1);
          uint32_t t_i32_118 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_118 = v_30;
          uint32_t t_i32_119 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_119 = UINT32_C(1);
          v_30 = t_i32_118 + t_i32_119;
          uint32_t t_i32_120 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_121 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_121 = v_3;
          uint32_t t_i32_122 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_122 = v_1;
          t_i32_120 = (t_i32_121 >= t_i32_122);
          if (t_i32_120 != UINT32_C(0)) {
            bytes_t t_bytes_123 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            t_bytes_123 = user_std_json__err(ctx, input);
            rt_bytes_drop(ctx, &t_bytes_103);
            rt_bytes_drop(ctx, &t_bytes_113);
            rt_bytes_drop(ctx, &t_bytes_83);
            rt_bytes_drop(ctx, &t_bytes_99);
            rt_bytes_drop(ctx, &v_78);
            rt_vec_u8_drop(ctx, &v_26);
            rt_vec_u8_drop(ctx, &v_28);
            return t_bytes_123;
          } else {
            uint32_t t_i32_124 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_124 = UINT32_C(0);
          }
          uint32_t v_125 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_126 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_126 = p0;
          uint32_t t_i32_127 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_127 = v_3;
          ctx->trap_ptr = "/decls/19/body/8/4/25/2";
          v_125 = rt_view_get_u8(ctx, t_view_126, t_i32_127);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_128 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_129 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_129 = v_125;
          uint32_t t_i32_130 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_130 = UINT32_C(44);
          t_i32_128 = (t_i32_129 == t_i32_130);
          if (t_i32_128 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_131 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_131 = p0;
            uint32_t t_i32_132 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_133 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_133 = v_3;
            uint32_t t_i32_134 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_134 = UINT32_C(1);
            t_i32_132 = t_i32_133 + t_i32_134;
            v_3 = user_std_json__skip_ws(ctx, input, t_view_131, t_i32_132);
          } else {
            uint32_t t_i32_135 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_136 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_136 = v_125;
            uint32_t t_i32_137 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_137 = UINT32_C(125);
            t_i32_135 = (t_i32_136 == t_i32_137);
            if (t_i32_135 != UINT32_C(0)) {
              {
                bytes_t v_138 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                v_138 = rt_vec_u8_into_bytes(ctx, &v_26);
                bytes_t v_139 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                v_139 = rt_vec_u8_into_bytes(ctx, &v_28);
                vec_u8_t v_140 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                uint32_t t_i32_141 = UINT32_C(0);
                rt_fuel(ctx, 1);
                bytes_view_t t_view_142 = rt_view_empty(ctx);
                rt_fuel(ctx, 1);
                t_view_142 = p0;
                t_i32_141 = t_view_142.len;
                v_140 = rt_vec_u8_new(ctx, t_i32_141);
                vec_u8_t v_143 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                bytes_view_t t_view_144 = rt_view_empty(ctx);
                t_view_144 = rt_bytes_view(ctx, v_138);
                bytes_view_t t_view_145 = rt_view_empty(ctx);
                t_view_145 = rt_bytes_view(ctx, v_139);
                uint32_t t_i32_146 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_146 = v_30;
                vec_u8_t t_vec_u8_147 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                t_vec_u8_147 = v_140;
                v_140 = (vec_u8_t){0};
                v_143 = user_std_json__emit_object_sorted(ctx, input, t_view_144, t_view_145, t_i32_146, t_vec_u8_147);
                t_vec_u8_147 = (vec_u8_t){0};
                bytes_t t_bytes_148 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                t_bytes_148 = rt_vec_u8_into_bytes(ctx, &v_143);
                rt_vec_u8_drop(ctx, &t_vec_u8_147);
                rt_bytes_drop(ctx, &v_138);
                rt_bytes_drop(ctx, &v_139);
                rt_vec_u8_drop(ctx, &v_140);
                rt_vec_u8_drop(ctx, &v_143);
                rt_bytes_drop(ctx, &t_bytes_103);
                rt_bytes_drop(ctx, &t_bytes_113);
                rt_bytes_drop(ctx, &t_bytes_83);
                rt_bytes_drop(ctx, &t_bytes_99);
                rt_bytes_drop(ctx, &v_78);
                rt_vec_u8_drop(ctx, &v_26);
                rt_vec_u8_drop(ctx, &v_28);
                return t_bytes_148;
                rt_vec_u8_drop(ctx, &t_vec_u8_147);
                rt_bytes_drop(ctx, &v_138);
                rt_bytes_drop(ctx, &v_139);
                rt_vec_u8_drop(ctx, &v_140);
                rt_vec_u8_drop(ctx, &v_143);
              }
            } else {
              bytes_t t_bytes_149 = rt_bytes_empty(ctx);
              rt_fuel(ctx, 1);
              t_bytes_149 = user_std_json__err(ctx, input);
              rt_bytes_drop(ctx, &t_bytes_103);
              rt_bytes_drop(ctx, &t_bytes_113);
              rt_bytes_drop(ctx, &t_bytes_83);
              rt_bytes_drop(ctx, &t_bytes_99);
              rt_bytes_drop(ctx, &v_78);
              rt_vec_u8_drop(ctx, &v_26);
              rt_vec_u8_drop(ctx, &v_28);
              return t_bytes_149;
            }
          }
          uint32_t t_i32_150 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_150 = UINT32_C(0);
          rt_bytes_drop(ctx, &t_bytes_103);
          rt_bytes_drop(ctx, &t_bytes_113);
          rt_bytes_drop(ctx, &t_bytes_83);
          rt_bytes_drop(ctx, &t_bytes_99);
          rt_bytes_drop(ctx, &v_78);
        }
      }
      v_32 = v_32 + UINT32_C(1);
    }
    t_i32_31 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = user_std_json__err(ctx, input);
    rt_vec_u8_drop(ctx, &v_26);
    rt_vec_u8_drop(ctx, &v_28);
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__parse_string(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_2 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_2 = p1;
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_3 = UINT32_C(1);
    v_1 = t_i32_2 + t_i32_3;
    uint32_t v_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_5 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_5 = p0;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = v_1;
    v_4 = user_std_json__string_end_or_err(ctx, input, t_view_5, t_i32_6);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = v_4;
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_9 = UINT32_C(0);
    t_i32_7 = ((t_i32_8 ^ UINT32_C(0x80000000)) < (t_i32_9 ^ UINT32_C(0x80000000)));
    if (t_i32_7 != UINT32_C(0)) {
      bytes_t t_bytes_10 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_10 = user_std_json__err(ctx, input);
      return t_bytes_10;
    } else {
      uint32_t t_i32_11 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_11 = UINT32_C(0);
    }
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p0;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = p1;
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = v_4;
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_17 = p1;
    t_i32_15 = t_i32_16 - t_i32_17;
    uint32_t t_i32_18 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_18 = UINT32_C(1);
    t_i32_14 = t_i32_15 + t_i32_18;
    ctx->trap_ptr = "/decls/21/body/4";
    out = rt_view_to_bytes(ctx, rt_view_slice(ctx, t_view_12, t_i32_13, t_i32_14));
    ctx->trap_ptr = NULL;
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json__parse_value(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p1;
    v_3 = user_std_json__skip_ws(ctx, input, t_view_4, t_i32_5);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = v_3;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = v_1;
    t_i32_6 = (t_i32_7 >= t_i32_8);
    if (t_i32_6 != UINT32_C(0)) {
      bytes_t t_bytes_9 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_9 = user_std_json__err(ctx, input);
      return t_bytes_9;
    } else {
      uint32_t t_i32_10 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_10 = UINT32_C(0);
    }
    uint32_t v_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p0;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = v_3;
    ctx->trap_ptr = "/decls/22/body/4/2";
    v_11 = rt_view_get_u8(ctx, t_view_12, t_i32_13);
    ctx->trap_ptr = NULL;
    rt_fuel(ctx, 1);
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_15 = v_11;
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = UINT32_C(123);
    t_i32_14 = (t_i32_15 == t_i32_16);
    if (t_i32_14 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      bytes_view_t t_view_17 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      t_view_17 = p0;
      uint32_t t_i32_18 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_18 = v_3;
      out = user_std_json__parse_object(ctx, input, t_view_17, t_i32_18);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_19 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_20 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_20 = v_11;
      uint32_t t_i32_21 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_21 = UINT32_C(91);
      t_i32_19 = (t_i32_20 == t_i32_21);
      if (t_i32_19 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        bytes_view_t t_view_22 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_22 = p0;
        uint32_t t_i32_23 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_23 = v_3;
        out = user_std_json__parse_array(ctx, input, t_view_22, t_i32_23);
      } else {
        rt_fuel(ctx, 1);
        uint32_t t_i32_24 = UINT32_C(0);
        rt_fuel(ctx, 1);
        uint32_t t_i32_25 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_25 = v_11;
        uint32_t t_i32_26 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_26 = UINT32_C(34);
        t_i32_24 = (t_i32_25 == t_i32_26);
        if (t_i32_24 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          bytes_view_t t_view_27 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_27 = p0;
          uint32_t t_i32_28 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_28 = v_3;
          out = user_std_json__parse_string(ctx, input, t_view_27, t_i32_28);
        } else {
          rt_fuel(ctx, 1);
          uint32_t t_i32_29 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_30 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_30 = v_11;
          uint32_t t_i32_31 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_31 = UINT32_C(116);
          t_i32_29 = (t_i32_30 == t_i32_31);
          if (t_i32_29 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_32 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_32 = p0;
            uint32_t t_i32_33 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_33 = v_3;
            bytes_t t_bytes_34 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            static const char lit_35[] = "true";
            t_bytes_34 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_35, UINT32_C(4));
            bytes_view_t t_view_36 = rt_view_empty(ctx);
            t_view_36 = rt_bytes_view(ctx, t_bytes_34);
            out = user_std_json__parse_lit(ctx, input, t_view_32, t_i32_33, t_view_36);
            rt_bytes_drop(ctx, &t_bytes_34);
          } else {
            rt_fuel(ctx, 1);
            uint32_t t_i32_37 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_38 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_38 = v_11;
            uint32_t t_i32_39 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_39 = UINT32_C(102);
            t_i32_37 = (t_i32_38 == t_i32_39);
            if (t_i32_37 != UINT32_C(0)) {
              rt_fuel(ctx, 1);
              bytes_view_t t_view_40 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_40 = p0;
              uint32_t t_i32_41 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_41 = v_3;
              bytes_t t_bytes_42 = rt_bytes_empty(ctx);
              rt_fuel(ctx, 1);
              static const char lit_43[] = "false";
              t_bytes_42 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_43, UINT32_C(5));
              bytes_view_t t_view_44 = rt_view_empty(ctx);
              t_view_44 = rt_bytes_view(ctx, t_bytes_42);
              out = user_std_json__parse_lit(ctx, input, t_view_40, t_i32_41, t_view_44);
              rt_bytes_drop(ctx, &t_bytes_42);
            } else {
              rt_fuel(ctx, 1);
              uint32_t t_i32_45 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_46 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_46 = v_11;
              uint32_t t_i32_47 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_47 = UINT32_C(110);
              t_i32_45 = (t_i32_46 == t_i32_47);
              if (t_i32_45 != UINT32_C(0)) {
                rt_fuel(ctx, 1);
                bytes_view_t t_view_48 = rt_view_empty(ctx);
                rt_fuel(ctx, 1);
                t_view_48 = p0;
                uint32_t t_i32_49 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_49 = v_3;
                bytes_t t_bytes_50 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                static const char lit_51[] = "null";
                t_bytes_50 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_51, UINT32_C(4));
                bytes_view_t t_view_52 = rt_view_empty(ctx);
                t_view_52 = rt_bytes_view(ctx, t_bytes_50);
                out = user_std_json__parse_lit(ctx, input, t_view_48, t_i32_49, t_view_52);
                rt_bytes_drop(ctx, &t_bytes_50);
              } else {
                rt_fuel(ctx, 1);
                uint32_t t_i32_53 = UINT32_C(0);
                rt_fuel(ctx, 1);
                uint32_t t_i32_54 = UINT32_C(0);
                rt_fuel(ctx, 1);
                uint32_t t_i32_55 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_55 = v_11;
                uint32_t t_i32_56 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_56 = UINT32_C(45);
                t_i32_54 = (t_i32_55 == t_i32_56);
                if (t_i32_54 != UINT32_C(0)) {
                  rt_fuel(ctx, 1);
                  t_i32_53 = UINT32_C(1);
                } else {
                  rt_fuel(ctx, 1);
                  uint32_t t_i32_57 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_57 = v_11;
                  t_i32_53 = user_std_json__is_digit(ctx, input, t_i32_57);
                }
                if (t_i32_53 != UINT32_C(0)) {
                  rt_fuel(ctx, 1);
                  bytes_view_t t_view_58 = rt_view_empty(ctx);
                  rt_fuel(ctx, 1);
                  t_view_58 = p0;
                  uint32_t t_i32_59 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_59 = v_3;
                  out = user_std_json__parse_number(ctx, input, t_view_58, t_i32_59);
                } else {
                  rt_fuel(ctx, 1);
                  out = user_std_json__err(ctx, input);
                }
              }
            }
          }
        }
      }
    }
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_view_t user_std_json__segment_at(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  bytes_view_t out = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_1 = UINT32_C(0);
    uint32_t t_i32_2 = UINT32_C(0);
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_3 = UINT32_C(0);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p1;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = UINT32_C(1);
    t_i32_4 = t_i32_5 + t_i32_6;
    for (;;) {
      if (v_3 >= t_i32_4) break;
      {
        {
          uint32_t v_7 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_8 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_8 = p0;
          uint32_t t_i32_9 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_9 = v_1;
          v_7 = rt_codec_read_u32_le(ctx, t_view_8, t_i32_9);
          uint32_t v_10 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_11 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_11 = v_1;
          uint32_t t_i32_12 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_12 = UINT32_C(4);
          v_10 = t_i32_11 + t_i32_12;
          uint32_t t_i32_13 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_14 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_14 = v_3;
          uint32_t t_i32_15 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_15 = p1;
          t_i32_13 = (t_i32_14 == t_i32_15);
          if (t_i32_13 != UINT32_C(0)) {
            bytes_view_t t_view_16 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_17 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_17 = p0;
            uint32_t t_i32_18 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_18 = v_10;
            uint32_t t_i32_19 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_19 = v_7;
            ctx->trap_ptr = "/decls/24/body/2/4/3/2/1";
            t_view_16 = rt_view_slice(ctx, t_view_17, t_i32_18, t_i32_19);
            ctx->trap_ptr = NULL;
            return t_view_16;
          } else {
            rt_fuel(ctx, 1);
            uint32_t t_i32_20 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_20 = v_10;
            uint32_t t_i32_21 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_21 = v_7;
            v_1 = t_i32_20 + t_i32_21;
          }
          uint32_t t_i32_22 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_22 = UINT32_C(0);
        }
      }
      v_3 = v_3 + UINT32_C(1);
    }
    t_i32_2 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_23 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_23 = p0;
    uint32_t t_i32_24 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_24 = UINT32_C(0);
    uint32_t t_i32_25 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_25 = UINT32_C(0);
    ctx->trap_ptr = "/decls/24/body/3";
    out = rt_view_slice(ctx, t_view_23, t_i32_24, t_i32_25);
    ctx->trap_ptr = NULL;
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__skip_array(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = p1;
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = UINT32_C(1);
    t_i32_5 = t_i32_6 + t_i32_7;
    v_3 = user_std_json__skip_ws(ctx, input, t_view_4, t_i32_5);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_9 = v_3;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = v_1;
    t_i32_8 = (t_i32_9 >= t_i32_10);
    if (t_i32_8 != UINT32_C(0)) {
      uint32_t t_i32_11 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_11 = UINT32_C(4294967295);
      return t_i32_11;
    } else {
      uint32_t t_i32_12 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_12 = UINT32_C(0);
    }
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_15 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_15 = p0;
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = v_3;
    ctx->trap_ptr = "/decls/25/body/4/1/1";
    t_i32_14 = rt_view_get_u8(ctx, t_view_15, t_i32_16);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_17 = UINT32_C(93);
    t_i32_13 = (t_i32_14 == t_i32_17);
    if (t_i32_13 != UINT32_C(0)) {
      uint32_t t_i32_18 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_19 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_19 = v_3;
      uint32_t t_i32_20 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_20 = UINT32_C(1);
      t_i32_18 = t_i32_19 + t_i32_20;
      return t_i32_18;
    } else {
      uint32_t t_i32_21 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_21 = UINT32_C(0);
    }
    uint32_t t_i32_22 = UINT32_C(0);
    uint32_t v_23 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_23 = UINT32_C(0);
    uint32_t t_i32_24 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_25 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_25 = v_1;
    uint32_t t_i32_26 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_26 = UINT32_C(1);
    t_i32_24 = t_i32_25 + t_i32_26;
    for (;;) {
      if (v_23 >= t_i32_24) break;
      {
        {
          rt_fuel(ctx, 1);
          bytes_view_t t_view_27 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_27 = p0;
          uint32_t t_i32_28 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_28 = v_3;
          v_3 = user_std_json__skip_value(ctx, input, t_view_27, t_i32_28);
          uint32_t t_i32_29 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_30 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_30 = v_3;
          uint32_t t_i32_31 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_31 = UINT32_C(0);
          t_i32_29 = ((t_i32_30 ^ UINT32_C(0x80000000)) < (t_i32_31 ^ UINT32_C(0x80000000)));
          if (t_i32_29 != UINT32_C(0)) {
            uint32_t t_i32_32 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_32 = UINT32_C(4294967295);
            return t_i32_32;
          } else {
            uint32_t t_i32_33 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_33 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          bytes_view_t t_view_34 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_34 = p0;
          uint32_t t_i32_35 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_35 = v_3;
          v_3 = user_std_json__skip_ws(ctx, input, t_view_34, t_i32_35);
          uint32_t t_i32_36 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_37 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_37 = v_3;
          uint32_t t_i32_38 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_38 = v_1;
          t_i32_36 = (t_i32_37 >= t_i32_38);
          if (t_i32_36 != UINT32_C(0)) {
            uint32_t t_i32_39 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_39 = UINT32_C(4294967295);
            return t_i32_39;
          } else {
            uint32_t t_i32_40 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_40 = UINT32_C(0);
          }
          uint32_t v_41 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_42 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_42 = p0;
          uint32_t t_i32_43 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_43 = v_3;
          ctx->trap_ptr = "/decls/25/body/5/4/5/2";
          v_41 = rt_view_get_u8(ctx, t_view_42, t_i32_43);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_44 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_45 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_45 = v_41;
          uint32_t t_i32_46 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_46 = UINT32_C(44);
          t_i32_44 = (t_i32_45 == t_i32_46);
          if (t_i32_44 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_47 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_47 = p0;
            uint32_t t_i32_48 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_49 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_49 = v_3;
            uint32_t t_i32_50 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_50 = UINT32_C(1);
            t_i32_48 = t_i32_49 + t_i32_50;
            v_3 = user_std_json__skip_ws(ctx, input, t_view_47, t_i32_48);
          } else {
            uint32_t t_i32_51 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_52 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_52 = v_41;
            uint32_t t_i32_53 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_53 = UINT32_C(93);
            t_i32_51 = (t_i32_52 == t_i32_53);
            if (t_i32_51 != UINT32_C(0)) {
              uint32_t t_i32_54 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_55 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_55 = v_3;
              uint32_t t_i32_56 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_56 = UINT32_C(1);
              t_i32_54 = t_i32_55 + t_i32_56;
              return t_i32_54;
            } else {
              uint32_t t_i32_57 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_57 = UINT32_C(4294967295);
              return t_i32_57;
            }
          }
          uint32_t t_i32_58 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_58 = UINT32_C(0);
        }
      }
      v_23 = v_23 + UINT32_C(1);
    }
    t_i32_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = UINT32_C(4294967295);
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__skip_lit(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1, bytes_view_t p2) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_2 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_2 = p0;
  uint32_t t_i32_3 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_3 = p1;
  bytes_view_t t_view_4 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_4 = p2;
  t_i32_1 = user_std_json__match_bytes_at(ctx, input, t_view_2, t_i32_3, t_view_4);
  if (t_i32_1 != UINT32_C(0)) {
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p1;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_7 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_7 = p2;
    t_i32_6 = t_view_7.len;
    out = t_i32_5 + t_i32_6;
  } else {
    rt_fuel(ctx, 1);
    out = UINT32_C(4294967295);
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__skip_number(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_3 = p1;
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_6 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_6 = p0;
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = v_3;
    ctx->trap_ptr = "/decls/27/body/3/1/1";
    t_i32_5 = rt_view_get_u8(ctx, t_view_6, t_i32_7);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = UINT32_C(45);
    t_i32_4 = (t_i32_5 == t_i32_8);
    if (t_i32_4 != UINT32_C(0)) {
      {
        rt_fuel(ctx, 1);
        uint32_t t_i32_9 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_9 = v_3;
        uint32_t t_i32_10 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_10 = UINT32_C(1);
        v_3 = t_i32_9 + t_i32_10;
        uint32_t t_i32_11 = UINT32_C(0);
        rt_fuel(ctx, 1);
        uint32_t t_i32_12 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_12 = v_3;
        uint32_t t_i32_13 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_13 = v_1;
        t_i32_11 = (t_i32_12 >= t_i32_13);
        if (t_i32_11 != UINT32_C(0)) {
          uint32_t t_i32_14 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_14 = UINT32_C(4294967295);
          return t_i32_14;
        } else {
          uint32_t t_i32_15 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_15 = UINT32_C(0);
        }
      }
    } else {
      uint32_t t_i32_16 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_16 = UINT32_C(0);
    }
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_18 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_18 = v_3;
    uint32_t t_i32_19 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_19 = v_1;
    t_i32_17 = (t_i32_18 >= t_i32_19);
    if (t_i32_17 != UINT32_C(0)) {
      uint32_t t_i32_20 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_20 = UINT32_C(4294967295);
      return t_i32_20;
    } else {
      uint32_t t_i32_21 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_21 = UINT32_C(0);
    }
    uint32_t v_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_23 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_23 = p0;
    uint32_t t_i32_24 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_24 = v_3;
    ctx->trap_ptr = "/decls/27/body/5/2";
    v_22 = rt_view_get_u8(ctx, t_view_23, t_i32_24);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_25 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_26 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_27 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_27 = v_22;
    t_i32_26 = user_std_json__is_digit(ctx, input, t_i32_27);
    uint32_t t_i32_28 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_28 = UINT32_C(0);
    t_i32_25 = (t_i32_26 == t_i32_28);
    if (t_i32_25 != UINT32_C(0)) {
      uint32_t t_i32_29 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_29 = UINT32_C(4294967295);
      return t_i32_29;
    } else {
      uint32_t t_i32_30 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_30 = UINT32_C(0);
    }
    uint32_t t_i32_31 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_32 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_32 = v_22;
    uint32_t t_i32_33 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_33 = UINT32_C(48);
    t_i32_31 = (t_i32_32 == t_i32_33);
    if (t_i32_31 != UINT32_C(0)) {
      {
        rt_fuel(ctx, 1);
        uint32_t t_i32_34 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_34 = v_3;
        uint32_t t_i32_35 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_35 = UINT32_C(1);
        v_3 = t_i32_34 + t_i32_35;
        uint32_t t_i32_36 = UINT32_C(0);
        rt_fuel(ctx, 1);
        uint32_t t_i32_37 = UINT32_C(0);
        rt_fuel(ctx, 1);
        uint32_t t_i32_38 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_38 = v_3;
        uint32_t t_i32_39 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_39 = v_1;
        t_i32_37 = (t_i32_38 < t_i32_39);
        if (t_i32_37 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          uint32_t t_i32_40 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_41 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_41 = p0;
          uint32_t t_i32_42 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_42 = v_3;
          ctx->trap_ptr = "/decls/27/body/7/2/2/1/2/1";
          t_i32_40 = rt_view_get_u8(ctx, t_view_41, t_i32_42);
          ctx->trap_ptr = NULL;
          t_i32_36 = user_std_json__is_digit(ctx, input, t_i32_40);
        } else {
          rt_fuel(ctx, 1);
          t_i32_36 = UINT32_C(0);
        }
        if (t_i32_36 != UINT32_C(0)) {
          uint32_t t_i32_43 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_43 = UINT32_C(4294967295);
          return t_i32_43;
        } else {
          uint32_t t_i32_44 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_44 = UINT32_C(0);
        }
      }
    } else {
      {
        rt_fuel(ctx, 1);
        uint32_t t_i32_45 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_45 = v_3;
        uint32_t t_i32_46 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_46 = UINT32_C(1);
        v_3 = t_i32_45 + t_i32_46;
        uint32_t v_47 = UINT32_C(0);
        rt_fuel(ctx, 1);
        v_47 = UINT32_C(0);
        uint32_t t_i32_48 = UINT32_C(0);
        uint32_t v_49 = UINT32_C(0);
        rt_fuel(ctx, 1);
        v_49 = UINT32_C(0);
        uint32_t t_i32_50 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_50 = v_1;
        for (;;) {
          if (v_49 >= t_i32_50) break;
          {
            {
              uint32_t t_i32_51 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_51 = v_47;
              if (t_i32_51 != UINT32_C(0)) {
                uint32_t t_i32_52 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_52 = UINT32_C(0);
              } else {
                uint32_t t_i32_53 = UINT32_C(0);
                rt_fuel(ctx, 1);
                uint32_t t_i32_54 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_54 = v_3;
                uint32_t t_i32_55 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_55 = v_1;
                t_i32_53 = (t_i32_54 >= t_i32_55);
                if (t_i32_53 != UINT32_C(0)) {
                  rt_fuel(ctx, 1);
                  v_47 = UINT32_C(1);
                } else {
                  uint32_t t_i32_56 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  uint32_t t_i32_57 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  bytes_view_t t_view_58 = rt_view_empty(ctx);
                  rt_fuel(ctx, 1);
                  t_view_58 = p0;
                  uint32_t t_i32_59 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_59 = v_3;
                  ctx->trap_ptr = "/decls/27/body/7/3/3/4/1/3/3/1/1";
                  t_i32_57 = rt_view_get_u8(ctx, t_view_58, t_i32_59);
                  ctx->trap_ptr = NULL;
                  t_i32_56 = user_std_json__is_digit(ctx, input, t_i32_57);
                  if (t_i32_56 != UINT32_C(0)) {
                    rt_fuel(ctx, 1);
                    uint32_t t_i32_60 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_60 = v_3;
                    uint32_t t_i32_61 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_61 = UINT32_C(1);
                    v_3 = t_i32_60 + t_i32_61;
                  } else {
                    rt_fuel(ctx, 1);
                    v_47 = UINT32_C(1);
                  }
                }
              }
              uint32_t t_i32_62 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_62 = UINT32_C(0);
            }
          }
          v_49 = v_49 + UINT32_C(1);
        }
        t_i32_48 = UINT32_C(0);
      }
    }
    rt_fuel(ctx, 1);
    out = v_3;
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__skip_object(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = p1;
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = UINT32_C(1);
    t_i32_5 = t_i32_6 + t_i32_7;
    v_3 = user_std_json__skip_ws(ctx, input, t_view_4, t_i32_5);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_9 = v_3;
    uint32_t t_i32_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_10 = v_1;
    t_i32_8 = (t_i32_9 >= t_i32_10);
    if (t_i32_8 != UINT32_C(0)) {
      uint32_t t_i32_11 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_11 = UINT32_C(4294967295);
      return t_i32_11;
    } else {
      uint32_t t_i32_12 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_12 = UINT32_C(0);
    }
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_15 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_15 = p0;
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = v_3;
    ctx->trap_ptr = "/decls/28/body/4/1/1";
    t_i32_14 = rt_view_get_u8(ctx, t_view_15, t_i32_16);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_17 = UINT32_C(125);
    t_i32_13 = (t_i32_14 == t_i32_17);
    if (t_i32_13 != UINT32_C(0)) {
      uint32_t t_i32_18 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_19 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_19 = v_3;
      uint32_t t_i32_20 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_20 = UINT32_C(1);
      t_i32_18 = t_i32_19 + t_i32_20;
      return t_i32_18;
    } else {
      uint32_t t_i32_21 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_21 = UINT32_C(0);
    }
    uint32_t t_i32_22 = UINT32_C(0);
    uint32_t v_23 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_23 = UINT32_C(0);
    uint32_t t_i32_24 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_25 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_25 = v_1;
    uint32_t t_i32_26 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_26 = UINT32_C(1);
    t_i32_24 = t_i32_25 + t_i32_26;
    for (;;) {
      if (v_23 >= t_i32_24) break;
      {
        {
          uint32_t t_i32_27 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_28 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_28 = v_3;
          uint32_t t_i32_29 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_29 = v_1;
          t_i32_27 = (t_i32_28 >= t_i32_29);
          if (t_i32_27 != UINT32_C(0)) {
            uint32_t t_i32_30 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_30 = UINT32_C(4294967295);
            return t_i32_30;
          } else {
            uint32_t t_i32_31 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_31 = UINT32_C(0);
          }
          uint32_t t_i32_32 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_33 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_34 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_34 = p0;
          uint32_t t_i32_35 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_35 = v_3;
          ctx->trap_ptr = "/decls/28/body/5/4/2/1/1";
          t_i32_33 = rt_view_get_u8(ctx, t_view_34, t_i32_35);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_36 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_36 = UINT32_C(34);
          t_i32_32 = (t_i32_33 == t_i32_36);
          if (t_i32_32 != UINT32_C(0)) {
            uint32_t t_i32_37 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_37 = UINT32_C(0);
          } else {
            uint32_t t_i32_38 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_38 = UINT32_C(4294967295);
            return t_i32_38;
          }
          uint32_t v_39 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_40 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_40 = p0;
          uint32_t t_i32_41 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_42 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_42 = v_3;
          uint32_t t_i32_43 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_43 = UINT32_C(1);
          t_i32_41 = t_i32_42 + t_i32_43;
          v_39 = user_std_json__string_end_or_err(ctx, input, t_view_40, t_i32_41);
          uint32_t t_i32_44 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_45 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_45 = v_39;
          uint32_t t_i32_46 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_46 = UINT32_C(0);
          t_i32_44 = ((t_i32_45 ^ UINT32_C(0x80000000)) < (t_i32_46 ^ UINT32_C(0x80000000)));
          if (t_i32_44 != UINT32_C(0)) {
            uint32_t t_i32_47 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_47 = UINT32_C(4294967295);
            return t_i32_47;
          } else {
            uint32_t t_i32_48 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_48 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          uint32_t t_i32_49 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_49 = v_39;
          uint32_t t_i32_50 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_50 = UINT32_C(1);
          v_3 = t_i32_49 + t_i32_50;
          rt_fuel(ctx, 1);
          bytes_view_t t_view_51 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_51 = p0;
          uint32_t t_i32_52 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_52 = v_3;
          v_3 = user_std_json__skip_ws(ctx, input, t_view_51, t_i32_52);
          uint32_t t_i32_53 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_54 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_55 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_55 = v_3;
          uint32_t t_i32_56 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_56 = v_1;
          t_i32_54 = (t_i32_55 >= t_i32_56);
          if (t_i32_54 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            t_i32_53 = UINT32_C(1);
          } else {
            rt_fuel(ctx, 1);
            uint32_t t_i32_57 = UINT32_C(0);
            rt_fuel(ctx, 1);
            bytes_view_t t_view_58 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_58 = p0;
            uint32_t t_i32_59 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_59 = v_3;
            ctx->trap_ptr = "/decls/28/body/5/4/7/1/3/1";
            t_i32_57 = rt_view_get_u8(ctx, t_view_58, t_i32_59);
            ctx->trap_ptr = NULL;
            uint32_t t_i32_60 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_60 = UINT32_C(58);
            t_i32_53 = (t_i32_57 == t_i32_60);
          }
          if (t_i32_53 != UINT32_C(0)) {
            uint32_t t_i32_61 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_61 = UINT32_C(0);
          } else {
            uint32_t t_i32_62 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_62 = UINT32_C(4294967295);
            return t_i32_62;
          }
          rt_fuel(ctx, 1);
          uint32_t t_i32_63 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_63 = v_3;
          uint32_t t_i32_64 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_64 = UINT32_C(1);
          v_3 = t_i32_63 + t_i32_64;
          rt_fuel(ctx, 1);
          bytes_view_t t_view_65 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_65 = p0;
          uint32_t t_i32_66 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_66 = v_3;
          v_3 = user_std_json__skip_value(ctx, input, t_view_65, t_i32_66);
          uint32_t t_i32_67 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_68 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_68 = v_3;
          uint32_t t_i32_69 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_69 = UINT32_C(0);
          t_i32_67 = ((t_i32_68 ^ UINT32_C(0x80000000)) < (t_i32_69 ^ UINT32_C(0x80000000)));
          if (t_i32_67 != UINT32_C(0)) {
            uint32_t t_i32_70 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_70 = UINT32_C(4294967295);
            return t_i32_70;
          } else {
            uint32_t t_i32_71 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_71 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          bytes_view_t t_view_72 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_72 = p0;
          uint32_t t_i32_73 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_73 = v_3;
          v_3 = user_std_json__skip_ws(ctx, input, t_view_72, t_i32_73);
          uint32_t t_i32_74 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_75 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_75 = v_3;
          uint32_t t_i32_76 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_76 = v_1;
          t_i32_74 = (t_i32_75 >= t_i32_76);
          if (t_i32_74 != UINT32_C(0)) {
            uint32_t t_i32_77 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_77 = UINT32_C(4294967295);
            return t_i32_77;
          } else {
            uint32_t t_i32_78 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_78 = UINT32_C(0);
          }
          uint32_t v_79 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_80 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_80 = p0;
          uint32_t t_i32_81 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_81 = v_3;
          ctx->trap_ptr = "/decls/28/body/5/4/13/2";
          v_79 = rt_view_get_u8(ctx, t_view_80, t_i32_81);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_82 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_83 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_83 = v_79;
          uint32_t t_i32_84 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_84 = UINT32_C(44);
          t_i32_82 = (t_i32_83 == t_i32_84);
          if (t_i32_82 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_85 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_85 = p0;
            uint32_t t_i32_86 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_87 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_87 = v_3;
            uint32_t t_i32_88 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_88 = UINT32_C(1);
            t_i32_86 = t_i32_87 + t_i32_88;
            v_3 = user_std_json__skip_ws(ctx, input, t_view_85, t_i32_86);
          } else {
            uint32_t t_i32_89 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_90 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_90 = v_79;
            uint32_t t_i32_91 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_91 = UINT32_C(125);
            t_i32_89 = (t_i32_90 == t_i32_91);
            if (t_i32_89 != UINT32_C(0)) {
              uint32_t t_i32_92 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_93 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_93 = v_3;
              uint32_t t_i32_94 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_94 = UINT32_C(1);
              t_i32_92 = t_i32_93 + t_i32_94;
              return t_i32_92;
            } else {
              uint32_t t_i32_95 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_95 = UINT32_C(4294967295);
              return t_i32_95;
            }
          }
          uint32_t t_i32_96 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_96 = UINT32_C(0);
        }
      }
      v_23 = v_23 + UINT32_C(1);
    }
    t_i32_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = UINT32_C(4294967295);
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__skip_string(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_4 = p1;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = UINT32_C(1);
    t_i32_3 = t_i32_4 + t_i32_5;
    v_1 = user_std_json__string_end_or_err(ctx, input, t_view_2, t_i32_3);
    rt_fuel(ctx, 1);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = v_1;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = UINT32_C(0);
    t_i32_6 = ((t_i32_7 ^ UINT32_C(0x80000000)) < (t_i32_8 ^ UINT32_C(0x80000000)));
    if (t_i32_6 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      out = UINT32_C(4294967295);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_9 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_9 = v_1;
      uint32_t t_i32_10 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_10 = UINT32_C(1);
      out = t_i32_9 + t_i32_10;
    }
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__skip_value(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = p1;
    v_3 = user_std_json__skip_ws(ctx, input, t_view_4, t_i32_5);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = v_3;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = v_1;
    t_i32_6 = (t_i32_7 >= t_i32_8);
    if (t_i32_6 != UINT32_C(0)) {
      uint32_t t_i32_9 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_9 = UINT32_C(4294967295);
      return t_i32_9;
    } else {
      uint32_t t_i32_10 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_10 = UINT32_C(0);
    }
    uint32_t v_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p0;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = v_3;
    ctx->trap_ptr = "/decls/30/body/4/2";
    v_11 = rt_view_get_u8(ctx, t_view_12, t_i32_13);
    ctx->trap_ptr = NULL;
    rt_fuel(ctx, 1);
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_15 = v_11;
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = UINT32_C(123);
    t_i32_14 = (t_i32_15 == t_i32_16);
    if (t_i32_14 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      bytes_view_t t_view_17 = rt_view_empty(ctx);
      rt_fuel(ctx, 1);
      t_view_17 = p0;
      uint32_t t_i32_18 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_18 = v_3;
      out = user_std_json__skip_object(ctx, input, t_view_17, t_i32_18);
    } else {
      rt_fuel(ctx, 1);
      uint32_t t_i32_19 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_20 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_20 = v_11;
      uint32_t t_i32_21 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_21 = UINT32_C(91);
      t_i32_19 = (t_i32_20 == t_i32_21);
      if (t_i32_19 != UINT32_C(0)) {
        rt_fuel(ctx, 1);
        bytes_view_t t_view_22 = rt_view_empty(ctx);
        rt_fuel(ctx, 1);
        t_view_22 = p0;
        uint32_t t_i32_23 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_23 = v_3;
        out = user_std_json__skip_array(ctx, input, t_view_22, t_i32_23);
      } else {
        rt_fuel(ctx, 1);
        uint32_t t_i32_24 = UINT32_C(0);
        rt_fuel(ctx, 1);
        uint32_t t_i32_25 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_25 = v_11;
        uint32_t t_i32_26 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_26 = UINT32_C(34);
        t_i32_24 = (t_i32_25 == t_i32_26);
        if (t_i32_24 != UINT32_C(0)) {
          rt_fuel(ctx, 1);
          bytes_view_t t_view_27 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_27 = p0;
          uint32_t t_i32_28 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_28 = v_3;
          out = user_std_json__skip_string(ctx, input, t_view_27, t_i32_28);
        } else {
          rt_fuel(ctx, 1);
          uint32_t t_i32_29 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_30 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_30 = v_11;
          uint32_t t_i32_31 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_31 = UINT32_C(116);
          t_i32_29 = (t_i32_30 == t_i32_31);
          if (t_i32_29 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_32 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_32 = p0;
            uint32_t t_i32_33 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_33 = v_3;
            bytes_t t_bytes_34 = rt_bytes_empty(ctx);
            rt_fuel(ctx, 1);
            static const char lit_35[] = "true";
            t_bytes_34 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_35, UINT32_C(4));
            bytes_view_t t_view_36 = rt_view_empty(ctx);
            t_view_36 = rt_bytes_view(ctx, t_bytes_34);
            out = user_std_json__skip_lit(ctx, input, t_view_32, t_i32_33, t_view_36);
            rt_bytes_drop(ctx, &t_bytes_34);
          } else {
            rt_fuel(ctx, 1);
            uint32_t t_i32_37 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_38 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_38 = v_11;
            uint32_t t_i32_39 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_39 = UINT32_C(102);
            t_i32_37 = (t_i32_38 == t_i32_39);
            if (t_i32_37 != UINT32_C(0)) {
              rt_fuel(ctx, 1);
              bytes_view_t t_view_40 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_40 = p0;
              uint32_t t_i32_41 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_41 = v_3;
              bytes_t t_bytes_42 = rt_bytes_empty(ctx);
              rt_fuel(ctx, 1);
              static const char lit_43[] = "false";
              t_bytes_42 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_43, UINT32_C(5));
              bytes_view_t t_view_44 = rt_view_empty(ctx);
              t_view_44 = rt_bytes_view(ctx, t_bytes_42);
              out = user_std_json__skip_lit(ctx, input, t_view_40, t_i32_41, t_view_44);
              rt_bytes_drop(ctx, &t_bytes_42);
            } else {
              rt_fuel(ctx, 1);
              uint32_t t_i32_45 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_46 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_46 = v_11;
              uint32_t t_i32_47 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_47 = UINT32_C(110);
              t_i32_45 = (t_i32_46 == t_i32_47);
              if (t_i32_45 != UINT32_C(0)) {
                rt_fuel(ctx, 1);
                bytes_view_t t_view_48 = rt_view_empty(ctx);
                rt_fuel(ctx, 1);
                t_view_48 = p0;
                uint32_t t_i32_49 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_49 = v_3;
                bytes_t t_bytes_50 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                static const char lit_51[] = "null";
                t_bytes_50 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_51, UINT32_C(4));
                bytes_view_t t_view_52 = rt_view_empty(ctx);
                t_view_52 = rt_bytes_view(ctx, t_bytes_50);
                out = user_std_json__skip_lit(ctx, input, t_view_48, t_i32_49, t_view_52);
                rt_bytes_drop(ctx, &t_bytes_50);
              } else {
                rt_fuel(ctx, 1);
                uint32_t t_i32_53 = UINT32_C(0);
                rt_fuel(ctx, 1);
                uint32_t t_i32_54 = UINT32_C(0);
                rt_fuel(ctx, 1);
                uint32_t t_i32_55 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_55 = v_11;
                uint32_t t_i32_56 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_56 = UINT32_C(45);
                t_i32_54 = (t_i32_55 == t_i32_56);
                if (t_i32_54 != UINT32_C(0)) {
                  rt_fuel(ctx, 1);
                  t_i32_53 = UINT32_C(1);
                } else {
                  rt_fuel(ctx, 1);
                  uint32_t t_i32_57 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_57 = v_11;
                  t_i32_53 = user_std_json__is_digit(ctx, input, t_i32_57);
                }
                if (t_i32_53 != UINT32_C(0)) {
                  rt_fuel(ctx, 1);
                  bytes_view_t t_view_58 = rt_view_empty(ctx);
                  rt_fuel(ctx, 1);
                  t_view_58 = p0;
                  uint32_t t_i32_59 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_59 = v_3;
                  out = user_std_json__skip_number(ctx, input, t_view_58, t_i32_59);
                } else {
                  rt_fuel(ctx, 1);
                  out = UINT32_C(4294967295);
                }
              }
            }
          }
        }
      }
    }
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__skip_ws(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t t_i32_3 = UINT32_C(0);
    uint32_t v_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_4 = p1;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = v_1;
    for (;;) {
      if (v_4 >= t_i32_5) break;
      {
        {
          uint32_t v_6 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_7 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_7 = p0;
          uint32_t t_i32_8 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_8 = v_4;
          ctx->trap_ptr = "/decls/31/body/2/4/1/2";
          v_6 = rt_view_get_u8(ctx, t_view_7, t_i32_8);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_9 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_10 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_10 = v_6;
          t_i32_9 = user_std_json__is_ws(ctx, input, t_i32_10);
          if (t_i32_9 != UINT32_C(0)) {
            uint32_t t_i32_11 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_11 = UINT32_C(0);
          } else {
            uint32_t t_i32_12 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_12 = v_4;
            return t_i32_12;
          }
        }
      }
      v_4 = v_4 + UINT32_C(1);
    }
    t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = v_1;
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__string_end_or_err(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_3 = p1;
    uint32_t t_i32_4 = UINT32_C(0);
    uint32_t v_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_5 = UINT32_C(0);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = v_1;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = UINT32_C(1);
    t_i32_6 = t_i32_7 + t_i32_8;
    for (;;) {
      if (v_5 >= t_i32_6) break;
      {
        {
          uint32_t t_i32_9 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_10 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_10 = v_3;
          uint32_t t_i32_11 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_11 = v_1;
          t_i32_9 = (t_i32_10 >= t_i32_11);
          if (t_i32_9 != UINT32_C(0)) {
            uint32_t t_i32_12 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_12 = UINT32_C(4294967295);
            return t_i32_12;
          } else {
            uint32_t t_i32_13 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_13 = UINT32_C(0);
          }
          uint32_t v_14 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_15 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_15 = p0;
          uint32_t t_i32_16 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_16 = v_3;
          ctx->trap_ptr = "/decls/32/body/3/4/2/2";
          v_14 = rt_view_get_u8(ctx, t_view_15, t_i32_16);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_17 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_18 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_18 = v_14;
          uint32_t t_i32_19 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_19 = UINT32_C(34);
          t_i32_17 = (t_i32_18 == t_i32_19);
          if (t_i32_17 != UINT32_C(0)) {
            uint32_t t_i32_20 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_20 = v_3;
            return t_i32_20;
          } else {
            uint32_t t_i32_21 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_21 = UINT32_C(0);
          }
          uint32_t t_i32_22 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_23 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_23 = v_14;
          uint32_t t_i32_24 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_24 = UINT32_C(32);
          t_i32_22 = (t_i32_23 < t_i32_24);
          if (t_i32_22 != UINT32_C(0)) {
            uint32_t t_i32_25 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_25 = UINT32_C(4294967295);
            return t_i32_25;
          } else {
            uint32_t t_i32_26 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_26 = UINT32_C(0);
          }
          uint32_t t_i32_27 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_28 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_28 = v_14;
          uint32_t t_i32_29 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_29 = UINT32_C(92);
          t_i32_27 = (t_i32_28 == t_i32_29);
          if (t_i32_27 != UINT32_C(0)) {
            rt_fuel(ctx, 1);
            bytes_view_t t_view_30 = rt_view_empty(ctx);
            rt_fuel(ctx, 1);
            t_view_30 = p0;
            uint32_t t_i32_31 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_31 = v_3;
            v_3 = user_std_json__string_escape_end_or_err(ctx, input, t_view_30, t_i32_31);
          } else {
            rt_fuel(ctx, 1);
            uint32_t t_i32_32 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_32 = v_3;
            uint32_t t_i32_33 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_33 = UINT32_C(1);
            v_3 = t_i32_32 + t_i32_33;
          }
          uint32_t t_i32_34 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_35 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_35 = v_3;
          uint32_t t_i32_36 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_36 = UINT32_C(0);
          t_i32_34 = ((t_i32_35 ^ UINT32_C(0x80000000)) < (t_i32_36 ^ UINT32_C(0x80000000)));
          if (t_i32_34 != UINT32_C(0)) {
            uint32_t t_i32_37 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_37 = UINT32_C(4294967295);
            return t_i32_37;
          } else {
            uint32_t t_i32_38 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_38 = UINT32_C(0);
          }
          uint32_t t_i32_39 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_39 = UINT32_C(0);
        }
      }
      v_5 = v_5 + UINT32_C(1);
    }
    t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    out = UINT32_C(4294967295);
  }
  return out;
}

#line 1 "std/json.x07.json"
static uint32_t user_std_json__string_escape_end_or_err(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, uint32_t p1) {
  uint32_t out = UINT32_C(0);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    v_1 = t_view_2.len;
    uint32_t v_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_4 = p1;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = UINT32_C(1);
    v_3 = t_i32_4 + t_i32_5;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_7 = v_3;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = v_1;
    t_i32_6 = (t_i32_7 >= t_i32_8);
    if (t_i32_6 != UINT32_C(0)) {
      uint32_t t_i32_9 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_9 = UINT32_C(4294967295);
      return t_i32_9;
    } else {
      uint32_t t_i32_10 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_10 = UINT32_C(0);
    }
    uint32_t v_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p0;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = v_3;
    ctx->trap_ptr = "/decls/33/body/4/2";
    v_11 = rt_view_get_u8(ctx, t_view_12, t_i32_13);
    ctx->trap_ptr = NULL;
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_15 = v_11;
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = UINT32_C(34);
    t_i32_14 = (t_i32_15 == t_i32_16);
    if (t_i32_14 != UINT32_C(0)) {
      uint32_t t_i32_17 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_18 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_18 = v_3;
      uint32_t t_i32_19 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_19 = UINT32_C(1);
      t_i32_17 = t_i32_18 + t_i32_19;
      return t_i32_17;
    } else {
      uint32_t t_i32_20 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_20 = UINT32_C(0);
    }
    uint32_t t_i32_21 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_22 = v_11;
    uint32_t t_i32_23 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_23 = UINT32_C(92);
    t_i32_21 = (t_i32_22 == t_i32_23);
    if (t_i32_21 != UINT32_C(0)) {
      uint32_t t_i32_24 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_25 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_25 = v_3;
      uint32_t t_i32_26 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_26 = UINT32_C(1);
      t_i32_24 = t_i32_25 + t_i32_26;
      return t_i32_24;
    } else {
      uint32_t t_i32_27 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_27 = UINT32_C(0);
    }
    uint32_t t_i32_28 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_29 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_29 = v_11;
    uint32_t t_i32_30 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_30 = UINT32_C(47);
    t_i32_28 = (t_i32_29 == t_i32_30);
    if (t_i32_28 != UINT32_C(0)) {
      uint32_t t_i32_31 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_32 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_32 = v_3;
      uint32_t t_i32_33 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_33 = UINT32_C(1);
      t_i32_31 = t_i32_32 + t_i32_33;
      return t_i32_31;
    } else {
      uint32_t t_i32_34 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_34 = UINT32_C(0);
    }
    uint32_t t_i32_35 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_36 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_36 = v_11;
    uint32_t t_i32_37 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_37 = UINT32_C(98);
    t_i32_35 = (t_i32_36 == t_i32_37);
    if (t_i32_35 != UINT32_C(0)) {
      uint32_t t_i32_38 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_39 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_39 = v_3;
      uint32_t t_i32_40 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_40 = UINT32_C(1);
      t_i32_38 = t_i32_39 + t_i32_40;
      return t_i32_38;
    } else {
      uint32_t t_i32_41 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_41 = UINT32_C(0);
    }
    uint32_t t_i32_42 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_43 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_43 = v_11;
    uint32_t t_i32_44 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_44 = UINT32_C(102);
    t_i32_42 = (t_i32_43 == t_i32_44);
    if (t_i32_42 != UINT32_C(0)) {
      uint32_t t_i32_45 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_46 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_46 = v_3;
      uint32_t t_i32_47 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_47 = UINT32_C(1);
      t_i32_45 = t_i32_46 + t_i32_47;
      return t_i32_45;
    } else {
      uint32_t t_i32_48 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_48 = UINT32_C(0);
    }
    uint32_t t_i32_49 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_50 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_50 = v_11;
    uint32_t t_i32_51 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_51 = UINT32_C(110);
    t_i32_49 = (t_i32_50 == t_i32_51);
    if (t_i32_49 != UINT32_C(0)) {
      uint32_t t_i32_52 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_53 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_53 = v_3;
      uint32_t t_i32_54 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_54 = UINT32_C(1);
      t_i32_52 = t_i32_53 + t_i32_54;
      return t_i32_52;
    } else {
      uint32_t t_i32_55 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_55 = UINT32_C(0);
    }
    uint32_t t_i32_56 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_57 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_57 = v_11;
    uint32_t t_i32_58 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_58 = UINT32_C(114);
    t_i32_56 = (t_i32_57 == t_i32_58);
    if (t_i32_56 != UINT32_C(0)) {
      uint32_t t_i32_59 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_60 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_60 = v_3;
      uint32_t t_i32_61 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_61 = UINT32_C(1);
      t_i32_59 = t_i32_60 + t_i32_61;
      return t_i32_59;
    } else {
      uint32_t t_i32_62 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_62 = UINT32_C(0);
    }
    uint32_t t_i32_63 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_64 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_64 = v_11;
    uint32_t t_i32_65 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_65 = UINT32_C(116);
    t_i32_63 = (t_i32_64 == t_i32_65);
    if (t_i32_63 != UINT32_C(0)) {
      uint32_t t_i32_66 = UINT32_C(0);
      rt_fuel(ctx, 1);
      uint32_t t_i32_67 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_67 = v_3;
      uint32_t t_i32_68 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_68 = UINT32_C(1);
      t_i32_66 = t_i32_67 + t_i32_68;
      return t_i32_66;
    } else {
      uint32_t t_i32_69 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_69 = UINT32_C(0);
    }
    uint32_t t_i32_70 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_71 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_71 = v_11;
    uint32_t t_i32_72 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_72 = UINT32_C(117);
    t_i32_70 = (t_i32_71 == t_i32_72);
    if (t_i32_70 != UINT32_C(0)) {
      {
        {
          uint32_t v_73 = UINT32_C(0);
          rt_fuel(ctx, 1);
          v_73 = UINT32_C(0);
          {
            {
              rt_fuel(ctx, 1);
              uint32_t t_i32_74 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_74 = v_3;
              uint32_t t_i32_75 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_75 = UINT32_C(1);
              v_3 = t_i32_74 + t_i32_75;
              uint32_t t_i32_76 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_77 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_77 = v_3;
              uint32_t t_i32_78 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_78 = v_1;
              t_i32_76 = (t_i32_77 >= t_i32_78);
              if (t_i32_76 != UINT32_C(0)) {
                uint32_t t_i32_79 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_79 = UINT32_C(4294967295);
                return t_i32_79;
              } else {
                uint32_t t_i32_80 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_80 = UINT32_C(0);
              }
              uint32_t t_i32_81 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_82 = UINT32_C(0);
              rt_fuel(ctx, 1);
              bytes_view_t t_view_83 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_83 = p0;
              uint32_t t_i32_84 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_84 = v_3;
              ctx->trap_ptr = "/decls/33/body/13/2/1/4/3/1/1";
              t_i32_82 = rt_view_get_u8(ctx, t_view_83, t_i32_84);
              ctx->trap_ptr = NULL;
              t_i32_81 = user_std_json__is_hex_digit(ctx, input, t_i32_82);
              if (t_i32_81 != UINT32_C(0)) {
                uint32_t t_i32_85 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_85 = UINT32_C(0);
              } else {
                uint32_t t_i32_86 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_86 = UINT32_C(4294967295);
                return t_i32_86;
              }
              uint32_t t_i32_87 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_87 = UINT32_C(0);
            }
            uint32_t t_i32_88 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_88 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          uint32_t t_i32_89 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_89 = v_73;
          uint32_t t_i32_90 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_90 = UINT32_C(1);
          v_73 = t_i32_89 + t_i32_90;
          {
            {
              rt_fuel(ctx, 1);
              uint32_t t_i32_91 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_91 = v_3;
              uint32_t t_i32_92 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_92 = UINT32_C(1);
              v_3 = t_i32_91 + t_i32_92;
              uint32_t t_i32_93 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_94 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_94 = v_3;
              uint32_t t_i32_95 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_95 = v_1;
              t_i32_93 = (t_i32_94 >= t_i32_95);
              if (t_i32_93 != UINT32_C(0)) {
                uint32_t t_i32_96 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_96 = UINT32_C(4294967295);
                return t_i32_96;
              } else {
                uint32_t t_i32_97 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_97 = UINT32_C(0);
              }
              uint32_t t_i32_98 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_99 = UINT32_C(0);
              rt_fuel(ctx, 1);
              bytes_view_t t_view_100 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_100 = p0;
              uint32_t t_i32_101 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_101 = v_3;
              ctx->trap_ptr = "/decls/33/body/13/2/1/4/3/1/1";
              t_i32_99 = rt_view_get_u8(ctx, t_view_100, t_i32_101);
              ctx->trap_ptr = NULL;
              t_i32_98 = user_std_json__is_hex_digit(ctx, input, t_i32_99);
              if (t_i32_98 != UINT32_C(0)) {
                uint32_t t_i32_102 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_102 = UINT32_C(0);
              } else {
                uint32_t t_i32_103 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_103 = UINT32_C(4294967295);
                return t_i32_103;
              }
              uint32_t t_i32_104 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_104 = UINT32_C(0);
            }
            uint32_t t_i32_105 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_105 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          uint32_t t_i32_106 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_106 = v_73;
          uint32_t t_i32_107 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_107 = UINT32_C(1);
          v_73 = t_i32_106 + t_i32_107;
          {
            {
              rt_fuel(ctx, 1);
              uint32_t t_i32_108 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_108 = v_3;
              uint32_t t_i32_109 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_109 = UINT32_C(1);
              v_3 = t_i32_108 + t_i32_109;
              uint32_t t_i32_110 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_111 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_111 = v_3;
              uint32_t t_i32_112 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_112 = v_1;
              t_i32_110 = (t_i32_111 >= t_i32_112);
              if (t_i32_110 != UINT32_C(0)) {
                uint32_t t_i32_113 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_113 = UINT32_C(4294967295);
                return t_i32_113;
              } else {
                uint32_t t_i32_114 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_114 = UINT32_C(0);
              }
              uint32_t t_i32_115 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_116 = UINT32_C(0);
              rt_fuel(ctx, 1);
              bytes_view_t t_view_117 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_117 = p0;
              uint32_t t_i32_118 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_118 = v_3;
              ctx->trap_ptr = "/decls/33/body/13/2/1/4/3/1/1";
              t_i32_116 = rt_view_get_u8(ctx, t_view_117, t_i32_118);
              ctx->trap_ptr = NULL;
              t_i32_115 = user_std_json__is_hex_digit(ctx, input, t_i32_116);
              if (t_i32_115 != UINT32_C(0)) {
                uint32_t t_i32_119 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_119 = UINT32_C(0);
              } else {
                uint32_t t_i32_120 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_120 = UINT32_C(4294967295);
                return t_i32_120;
              }
              uint32_t t_i32_121 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_121 = UINT32_C(0);
            }
            uint32_t t_i32_122 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_122 = UINT32_C(0);
          }
          rt_fuel(ctx, 1);
          uint32_t t_i32_123 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_123 = v_73;
          uint32_t t_i32_124 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_124 = UINT32_C(1);
          v_73 = t_i32_123 + t_i32_124;
          {
            {
              rt_fuel(ctx, 1);
              uint32_t t_i32_125 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_125 = v_3;
              uint32_t t_i32_126 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_126 = UINT32_C(1);
              v_3 = t_i32_125 + t_i32_126;
              uint32_t t_i32_127 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_128 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_128 = v_3;
              uint32_t t_i32_129 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_129 = v_1;
              t_i32_127 = (t_i32_128 >= t_i32_129);
              if (t_i32_127 != UINT32_C(0)) {
                uint32_t t_i32_130 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_130 = UINT32_C(4294967295);
                return t_i32_130;
              } else {
                uint32_t t_i32_131 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_131 = UINT32_C(0);
              }
              uint32_t t_i32_132 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_133 = UINT32_C(0);
              rt_fuel(ctx, 1);
              bytes_view_t t_view_134 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_134 = p0;
              uint32_t t_i32_135 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_135 = v_3;
              ctx->trap_ptr = "/decls/33/body/13/2/1/4/3/1/1";
              t_i32_133 = rt_view_get_u8(ctx, t_view_134, t_i32_135);
              ctx->trap_ptr = NULL;
              t_i32_132 = user_std_json__is_hex_digit(ctx, input, t_i32_133);
              if (t_i32_132 != UINT32_C(0)) {
                uint32_t t_i32_136 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_136 = UINT32_C(0);
              } else {
                uint32_t t_i32_137 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_137 = UINT32_C(4294967295);
                return t_i32_137;
              }
              uint32_t t_i32_138 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_138 = UINT32_C(0);
            }
            uint32_t t_i32_139 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_139 = UINT32_C(0);
          }
          uint32_t t_i32_140 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_140 = UINT32_C(0);
        }
        uint32_t t_i32_141 = UINT32_C(0);
        rt_fuel(ctx, 1);
        uint32_t t_i32_142 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_142 = v_3;
        uint32_t t_i32_143 = UINT32_C(0);
        rt_fuel(ctx, 1);
        t_i32_143 = UINT32_C(1);
        t_i32_141 = t_i32_142 + t_i32_143;
        return t_i32_141;
      }
    } else {
      uint32_t t_i32_144 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_144 = UINT32_C(0);
    }
    rt_fuel(ctx, 1);
    out = UINT32_C(4294967295);
  }
  return out;
}

#line 1 "std/json.x07.json"
static bytes_t user_std_json_extract_path_canon_or_err(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    uint32_t v_1 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_2 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_2 = p0;
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_3 = UINT32_C(0);
    v_1 = user_std_json__find_byte(ctx, input, t_view_2, t_i32_3);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = v_1;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_6 = UINT32_C(0);
    t_i32_4 = ((t_i32_5 ^ UINT32_C(0x80000000)) < (t_i32_6 ^ UINT32_C(0x80000000)));
    if (t_i32_4 != UINT32_C(0)) {
      bytes_t t_bytes_7 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_7 = user_std_json__err(ctx, input);
      return t_bytes_7;
    } else {
      uint32_t t_i32_8 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_8 = UINT32_C(0);
    }
    bytes_view_t v_9 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_10 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_10 = p0;
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_11 = UINT32_C(0);
    uint32_t t_i32_12 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_12 = v_1;
    ctx->trap_ptr = "/decls/35/body/3/2";
    v_9 = rt_view_slice(ctx, t_view_10, t_i32_11, t_i32_12);
    ctx->trap_ptr = NULL;
    bytes_view_t v_13 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_14 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_14 = p0;
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_16 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_16 = v_1;
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_17 = UINT32_C(1);
    t_i32_15 = t_i32_16 + t_i32_17;
    uint32_t t_i32_18 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_19 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_20 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_20 = p0;
    t_i32_19 = t_view_20.len;
    uint32_t t_i32_21 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_22 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_22 = v_1;
    uint32_t t_i32_23 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_23 = UINT32_C(1);
    t_i32_21 = t_i32_22 + t_i32_23;
    t_i32_18 = t_i32_19 - t_i32_21;
    ctx->trap_ptr = "/decls/35/body/4/2";
    v_13 = rt_view_slice(ctx, t_view_14, t_i32_15, t_i32_18);
    ctx->trap_ptr = NULL;
    uint32_t v_24 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_25 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_25 = v_13;
    v_24 = t_view_25.len;
    uint32_t t_i32_26 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_27 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_27 = v_24;
    uint32_t t_i32_28 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_28 = UINT32_C(0);
    t_i32_26 = (t_i32_27 == t_i32_28);
    if (t_i32_26 != UINT32_C(0)) {
      bytes_t t_bytes_29 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_29 = user_std_json__err(ctx, input);
      return t_bytes_29;
    } else {
      uint32_t t_i32_30 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_30 = UINT32_C(0);
    }
    vec_u8_t v_31 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_32 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_33 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_33 = v_24;
    uint32_t t_i32_34 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_34 = UINT32_C(4);
    t_i32_32 = t_i32_33 + t_i32_34;
    v_31 = rt_vec_u8_new(ctx, t_i32_32);
    uint32_t v_35 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_35 = UINT32_C(0);
    uint32_t v_36 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_36 = UINT32_C(0);
    uint32_t t_i32_37 = UINT32_C(0);
    uint32_t v_38 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_38 = UINT32_C(0);
    uint32_t t_i32_39 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_39 = v_24;
    for (;;) {
      if (v_38 >= t_i32_39) break;
      {
        {
          uint32_t t_i32_40 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_41 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_42 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_42 = v_13;
          uint32_t t_i32_43 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_43 = v_38;
          ctx->trap_ptr = "/decls/35/body/10/4/1/1/1";
          t_i32_41 = rt_view_get_u8(ctx, t_view_42, t_i32_43);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_44 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_44 = UINT32_C(46);
          t_i32_40 = (t_i32_41 == t_i32_44);
          if (t_i32_40 != UINT32_C(0)) {
            {
              uint32_t v_45 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_46 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_46 = v_38;
              uint32_t t_i32_47 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_47 = v_35;
              v_45 = t_i32_46 - t_i32_47;
              uint32_t t_i32_48 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_49 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_49 = v_45;
              uint32_t t_i32_50 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_50 = UINT32_C(0);
              t_i32_48 = (t_i32_49 == t_i32_50);
              if (t_i32_48 != UINT32_C(0)) {
                bytes_t t_bytes_51 = rt_bytes_empty(ctx);
                rt_fuel(ctx, 1);
                t_bytes_51 = user_std_json__err(ctx, input);
                rt_vec_u8_drop(ctx, &v_31);
                return t_bytes_51;
              } else {
                uint32_t t_i32_52 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_52 = UINT32_C(0);
              }
              vec_u8_t t_set_53 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              bytes_t t_bytes_54 = rt_bytes_empty(ctx);
              rt_fuel(ctx, 1);
              uint32_t t_i32_55 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_55 = v_45;
              t_bytes_54 = rt_codec_write_u32_le(ctx, t_i32_55);
              bytes_view_t t_view_56 = rt_view_empty(ctx);
              t_view_56 = rt_bytes_view(ctx, t_bytes_54);
              v_31 = rt_vec_u8_extend_bytes(ctx, v_31, t_view_56);
              t_set_53 = v_31;
              v_31 = (vec_u8_t){0};
              v_31 = t_set_53;
              t_set_53 = (vec_u8_t){0};
              vec_u8_t t_set_57 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              bytes_view_t t_view_58 = rt_view_empty(ctx);
              rt_fuel(ctx, 1);
              t_view_58 = v_13;
              uint32_t t_i32_59 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_59 = v_35;
              uint32_t t_i32_60 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_60 = v_45;
              v_31 = rt_vec_u8_extend_bytes_range(ctx, v_31, t_view_58, t_i32_59, t_i32_60);
              t_set_57 = v_31;
              v_31 = (vec_u8_t){0};
              v_31 = t_set_57;
              t_set_57 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              uint32_t t_i32_61 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_61 = v_36;
              uint32_t t_i32_62 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_62 = UINT32_C(1);
              v_36 = t_i32_61 + t_i32_62;
              rt_fuel(ctx, 1);
              uint32_t t_i32_63 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_63 = v_38;
              uint32_t t_i32_64 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_64 = UINT32_C(1);
              v_35 = t_i32_63 + t_i32_64;
              rt_bytes_drop(ctx, &t_bytes_54);
            }
          } else {
            uint32_t t_i32_65 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_65 = UINT32_C(0);
          }
          uint32_t t_i32_66 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_66 = UINT32_C(0);
        }
      }
      v_38 = v_38 + UINT32_C(1);
    }
    t_i32_37 = UINT32_C(0);
    uint32_t v_67 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_68 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_68 = v_24;
    uint32_t t_i32_69 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_69 = v_35;
    v_67 = t_i32_68 - t_i32_69;
    uint32_t t_i32_70 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_71 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_71 = v_67;
    uint32_t t_i32_72 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_72 = UINT32_C(0);
    t_i32_70 = (t_i32_71 == t_i32_72);
    if (t_i32_70 != UINT32_C(0)) {
      bytes_t t_bytes_73 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_73 = user_std_json__err(ctx, input);
      rt_vec_u8_drop(ctx, &v_31);
      return t_bytes_73;
    } else {
      uint32_t t_i32_74 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_74 = UINT32_C(0);
    }
    vec_u8_t t_set_75 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    bytes_t t_bytes_76 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    uint32_t t_i32_77 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_77 = v_67;
    t_bytes_76 = rt_codec_write_u32_le(ctx, t_i32_77);
    bytes_view_t t_view_78 = rt_view_empty(ctx);
    t_view_78 = rt_bytes_view(ctx, t_bytes_76);
    v_31 = rt_vec_u8_extend_bytes(ctx, v_31, t_view_78);
    t_set_75 = v_31;
    v_31 = (vec_u8_t){0};
    v_31 = t_set_75;
    t_set_75 = (vec_u8_t){0};
    vec_u8_t t_set_79 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    bytes_view_t t_view_80 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_80 = v_13;
    uint32_t t_i32_81 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_81 = v_35;
    uint32_t t_i32_82 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_82 = v_67;
    v_31 = rt_vec_u8_extend_bytes_range(ctx, v_31, t_view_80, t_i32_81, t_i32_82);
    t_set_79 = v_31;
    v_31 = (vec_u8_t){0};
    v_31 = t_set_79;
    t_set_79 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_83 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_83 = v_36;
    uint32_t t_i32_84 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_84 = UINT32_C(1);
    v_36 = t_i32_83 + t_i32_84;
    bytes_t v_85 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    v_85 = rt_vec_u8_into_bytes(ctx, &v_31);
    bytes_t v_86 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_87 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_87 = v_9;
    uint32_t t_i32_88 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_88 = UINT32_C(0);
    bytes_view_t t_view_89 = rt_view_empty(ctx);
    t_view_89 = rt_bytes_view(ctx, v_85);
    uint32_t t_i32_90 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_90 = UINT32_C(0);
    uint32_t t_i32_91 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_91 = v_36;
    v_86 = user_std_json__extract_from_value(ctx, input, t_view_87, t_i32_88, t_view_89, t_i32_90, t_i32_91);
    uint32_t t_i32_92 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_93 = rt_view_empty(ctx);
    t_view_93 = rt_bytes_view(ctx, v_86);
    bytes_t t_bytes_94 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    t_bytes_94 = user_std_json__err(ctx, input);
    bytes_view_t t_view_95 = rt_view_empty(ctx);
    t_view_95 = rt_bytes_view(ctx, t_bytes_94);
    t_i32_92 = rt_view_eq(ctx, t_view_93, t_view_95);
    if (t_i32_92 != UINT32_C(0)) {
      bytes_t t_bytes_96 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_96 = user_std_json__err(ctx, input);
      rt_bytes_drop(ctx, &t_bytes_76);
      rt_bytes_drop(ctx, &t_bytes_94);
      rt_bytes_drop(ctx, &v_86);
      rt_bytes_drop(ctx, &v_85);
      rt_vec_u8_drop(ctx, &v_31);
      return t_bytes_96;
    } else {
      uint32_t t_i32_97 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_97 = UINT32_C(0);
    }
    uint32_t v_98 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_99 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_99 = v_9;
    uint32_t t_i32_100 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_100 = UINT32_C(0);
    v_98 = user_std_json__skip_value(ctx, input, t_view_99, t_i32_100);
    uint32_t t_i32_101 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_102 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_102 = v_98;
    uint32_t t_i32_103 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_103 = UINT32_C(0);
    t_i32_101 = ((t_i32_102 ^ UINT32_C(0x80000000)) < (t_i32_103 ^ UINT32_C(0x80000000)));
    if (t_i32_101 != UINT32_C(0)) {
      bytes_t t_bytes_104 = rt_bytes_empty(ctx);
      rt_fuel(ctx, 1);
      t_bytes_104 = user_std_json__err(ctx, input);
      rt_bytes_drop(ctx, &t_bytes_76);
      rt_bytes_drop(ctx, &t_bytes_94);
      rt_bytes_drop(ctx, &v_86);
      rt_bytes_drop(ctx, &v_85);
      rt_vec_u8_drop(ctx, &v_31);
      return t_bytes_104;
    } else {
      uint32_t t_i32_105 = UINT32_C(0);
      rt_fuel(ctx, 1);
      t_i32_105 = UINT32_C(0);
    }
    uint32_t v_106 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_107 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_107 = v_9;
    uint32_t t_i32_108 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_108 = v_98;
    v_106 = user_std_json__skip_ws(ctx, input, t_view_107, t_i32_108);
    rt_fuel(ctx, 1);
    uint32_t t_i32_109 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_110 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_110 = v_106;
    uint32_t t_i32_111 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_112 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_112 = v_9;
    t_i32_111 = t_view_112.len;
    t_i32_109 = (t_i32_110 == t_i32_111);
    if (t_i32_109 != UINT32_C(0)) {
      rt_fuel(ctx, 1);
      out = v_86;
      v_86 = rt_bytes_empty(ctx);
    } else {
      rt_fuel(ctx, 1);
      out = user_std_json__err(ctx, input);
    }
    rt_bytes_drop(ctx, &t_bytes_76);
    rt_bytes_drop(ctx, &t_bytes_94);
    rt_bytes_drop(ctx, &v_86);
    rt_bytes_drop(ctx, &v_85);
    rt_vec_u8_drop(ctx, &v_31);
  }
  return out;
}

#line 1 "std/vec.x07.json"
static bytes_t user_std_vec_as_bytes(ctx_t* ctx, bytes_view_t input, vec_u8_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  out = rt_vec_u8_into_bytes(ctx, &p0);
  rt_vec_u8_drop(ctx, &p0);
  return out;
}

#line 1 "std/vec.x07.json"
static vec_u8_t user_std_vec_extend_bytes(ctx_t* ctx, bytes_view_t input, vec_u8_t p0, bytes_view_t p1) {
  vec_u8_t out = (vec_u8_t){0};
  rt_fuel(ctx, 1);
  bytes_view_t t_view_1 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_1 = p1;
  p0 = rt_vec_u8_extend_bytes(ctx, p0, t_view_1);
  out = p0;
  p0 = (vec_u8_t){0};
  rt_vec_u8_drop(ctx, &p0);
  return out;
}

#line 1 "std/vec.x07.json"
static vec_u8_t user_std_vec_push(ctx_t* ctx, bytes_view_t input, vec_u8_t p0, uint32_t p1) {
  vec_u8_t out = (vec_u8_t){0};
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_1 = p1;
  p0 = rt_vec_u8_push(ctx, p0, t_i32_1);
  out = p0;
  p0 = (vec_u8_t){0};
  rt_vec_u8_drop(ctx, &p0);
  return out;
}

#line 1 "std/vec.x07.json"
static vec_u8_t user_std_vec_with_capacity(ctx_t* ctx, bytes_view_t input, uint32_t p0) {
  vec_u8_t out = (vec_u8_t){0};
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  t_i32_1 = p0;
  out = rt_vec_u8_new(ctx, t_i32_1);
  return out;
}

#line 1 "std/web_ui/effects/http.x07.json"
static bytes_t user_std_web_ui_effects_http__json_string_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    vec_u8_t v_1 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_2 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p0;
    t_i32_3 = t_view_4.len;
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_5 = UINT32_C(16);
    t_i32_2 = t_i32_3 + t_i32_5;
    v_1 = user_std_vec_with_capacity(ctx, input, t_i32_2);
    vec_u8_t t_set_6 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_7 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_7 = v_1;
    v_1 = (vec_u8_t){0};
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = UINT32_C(34);
    t_set_6 = user_std_vec_push(ctx, input, t_vec_u8_7, t_i32_8);
    t_vec_u8_7 = (vec_u8_t){0};
    v_1 = t_set_6;
    t_set_6 = (vec_u8_t){0};
    uint32_t t_i32_9 = UINT32_C(0);
    uint32_t v_10 = UINT32_C(0);
    rt_fuel(ctx, 1);
    v_10 = UINT32_C(0);
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p0;
    t_i32_11 = t_view_12.len;
    for (;;) {
      if (v_10 >= t_i32_11) break;
      {
        {
          uint32_t v_13 = UINT32_C(0);
          rt_fuel(ctx, 1);
          bytes_view_t t_view_14 = rt_view_empty(ctx);
          rt_fuel(ctx, 1);
          t_view_14 = p0;
          uint32_t t_i32_15 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_15 = v_10;
          ctx->trap_ptr = "/decls/2/body/3/4/1/2";
          v_13 = rt_view_get_u8(ctx, t_view_14, t_i32_15);
          ctx->trap_ptr = NULL;
          uint32_t t_i32_16 = UINT32_C(0);
          rt_fuel(ctx, 1);
          uint32_t t_i32_17 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_17 = v_13;
          uint32_t t_i32_18 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_18 = UINT32_C(34);
          t_i32_16 = (t_i32_17 == t_i32_18);
          if (t_i32_16 != UINT32_C(0)) {
            {
              vec_u8_t t_set_19 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              vec_u8_t t_vec_u8_20 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              t_vec_u8_20 = v_1;
              v_1 = (vec_u8_t){0};
              uint32_t t_i32_21 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_21 = UINT32_C(92);
              t_set_19 = user_std_vec_push(ctx, input, t_vec_u8_20, t_i32_21);
              t_vec_u8_20 = (vec_u8_t){0};
              v_1 = t_set_19;
              t_set_19 = (vec_u8_t){0};
              vec_u8_t t_set_22 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              vec_u8_t t_vec_u8_23 = (vec_u8_t){0};
              rt_fuel(ctx, 1);
              t_vec_u8_23 = v_1;
              v_1 = (vec_u8_t){0};
              uint32_t t_i32_24 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_24 = UINT32_C(34);
              t_set_22 = user_std_vec_push(ctx, input, t_vec_u8_23, t_i32_24);
              t_vec_u8_23 = (vec_u8_t){0};
              v_1 = t_set_22;
              t_set_22 = (vec_u8_t){0};
              uint32_t t_i32_25 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_25 = UINT32_C(0);
              rt_vec_u8_drop(ctx, &t_vec_u8_20);
              rt_vec_u8_drop(ctx, &t_vec_u8_23);
            }
          } else {
            uint32_t t_i32_26 = UINT32_C(0);
            rt_fuel(ctx, 1);
            uint32_t t_i32_27 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_27 = v_13;
            uint32_t t_i32_28 = UINT32_C(0);
            rt_fuel(ctx, 1);
            t_i32_28 = UINT32_C(92);
            t_i32_26 = (t_i32_27 == t_i32_28);
            if (t_i32_26 != UINT32_C(0)) {
              {
                vec_u8_t t_set_29 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                vec_u8_t t_vec_u8_30 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                t_vec_u8_30 = v_1;
                v_1 = (vec_u8_t){0};
                uint32_t t_i32_31 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_31 = UINT32_C(92);
                t_set_29 = user_std_vec_push(ctx, input, t_vec_u8_30, t_i32_31);
                t_vec_u8_30 = (vec_u8_t){0};
                v_1 = t_set_29;
                t_set_29 = (vec_u8_t){0};
                vec_u8_t t_set_32 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                vec_u8_t t_vec_u8_33 = (vec_u8_t){0};
                rt_fuel(ctx, 1);
                t_vec_u8_33 = v_1;
                v_1 = (vec_u8_t){0};
                uint32_t t_i32_34 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_34 = UINT32_C(92);
                t_set_32 = user_std_vec_push(ctx, input, t_vec_u8_33, t_i32_34);
                t_vec_u8_33 = (vec_u8_t){0};
                v_1 = t_set_32;
                t_set_32 = (vec_u8_t){0};
                uint32_t t_i32_35 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_35 = UINT32_C(0);
                rt_vec_u8_drop(ctx, &t_vec_u8_30);
                rt_vec_u8_drop(ctx, &t_vec_u8_33);
              }
            } else {
              uint32_t t_i32_36 = UINT32_C(0);
              rt_fuel(ctx, 1);
              uint32_t t_i32_37 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_37 = v_13;
              uint32_t t_i32_38 = UINT32_C(0);
              rt_fuel(ctx, 1);
              t_i32_38 = UINT32_C(10);
              t_i32_36 = (t_i32_37 == t_i32_38);
              if (t_i32_36 != UINT32_C(0)) {
                {
                  vec_u8_t t_set_39 = (vec_u8_t){0};
                  rt_fuel(ctx, 1);
                  vec_u8_t t_vec_u8_40 = (vec_u8_t){0};
                  rt_fuel(ctx, 1);
                  t_vec_u8_40 = v_1;
                  v_1 = (vec_u8_t){0};
                  uint32_t t_i32_41 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_41 = UINT32_C(92);
                  t_set_39 = user_std_vec_push(ctx, input, t_vec_u8_40, t_i32_41);
                  t_vec_u8_40 = (vec_u8_t){0};
                  v_1 = t_set_39;
                  t_set_39 = (vec_u8_t){0};
                  vec_u8_t t_set_42 = (vec_u8_t){0};
                  rt_fuel(ctx, 1);
                  vec_u8_t t_vec_u8_43 = (vec_u8_t){0};
                  rt_fuel(ctx, 1);
                  t_vec_u8_43 = v_1;
                  v_1 = (vec_u8_t){0};
                  uint32_t t_i32_44 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_44 = UINT32_C(110);
                  t_set_42 = user_std_vec_push(ctx, input, t_vec_u8_43, t_i32_44);
                  t_vec_u8_43 = (vec_u8_t){0};
                  v_1 = t_set_42;
                  t_set_42 = (vec_u8_t){0};
                  uint32_t t_i32_45 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_45 = UINT32_C(0);
                  rt_vec_u8_drop(ctx, &t_vec_u8_40);
                  rt_vec_u8_drop(ctx, &t_vec_u8_43);
                }
              } else {
                uint32_t t_i32_46 = UINT32_C(0);
                rt_fuel(ctx, 1);
                uint32_t t_i32_47 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_47 = v_13;
                uint32_t t_i32_48 = UINT32_C(0);
                rt_fuel(ctx, 1);
                t_i32_48 = UINT32_C(13);
                t_i32_46 = (t_i32_47 == t_i32_48);
                if (t_i32_46 != UINT32_C(0)) {
                  {
                    vec_u8_t t_set_49 = (vec_u8_t){0};
                    rt_fuel(ctx, 1);
                    vec_u8_t t_vec_u8_50 = (vec_u8_t){0};
                    rt_fuel(ctx, 1);
                    t_vec_u8_50 = v_1;
                    v_1 = (vec_u8_t){0};
                    uint32_t t_i32_51 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_51 = UINT32_C(92);
                    t_set_49 = user_std_vec_push(ctx, input, t_vec_u8_50, t_i32_51);
                    t_vec_u8_50 = (vec_u8_t){0};
                    v_1 = t_set_49;
                    t_set_49 = (vec_u8_t){0};
                    vec_u8_t t_set_52 = (vec_u8_t){0};
                    rt_fuel(ctx, 1);
                    vec_u8_t t_vec_u8_53 = (vec_u8_t){0};
                    rt_fuel(ctx, 1);
                    t_vec_u8_53 = v_1;
                    v_1 = (vec_u8_t){0};
                    uint32_t t_i32_54 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_54 = UINT32_C(114);
                    t_set_52 = user_std_vec_push(ctx, input, t_vec_u8_53, t_i32_54);
                    t_vec_u8_53 = (vec_u8_t){0};
                    v_1 = t_set_52;
                    t_set_52 = (vec_u8_t){0};
                    uint32_t t_i32_55 = UINT32_C(0);
                    rt_fuel(ctx, 1);
                    t_i32_55 = UINT32_C(0);
                    rt_vec_u8_drop(ctx, &t_vec_u8_50);
                    rt_vec_u8_drop(ctx, &t_vec_u8_53);
                  }
                } else {
                  uint32_t t_i32_56 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  uint32_t t_i32_57 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_57 = v_13;
                  uint32_t t_i32_58 = UINT32_C(0);
                  rt_fuel(ctx, 1);
                  t_i32_58 = UINT32_C(9);
                  t_i32_56 = (t_i32_57 == t_i32_58);
                  if (t_i32_56 != UINT32_C(0)) {
                    {
                      vec_u8_t t_set_59 = (vec_u8_t){0};
                      rt_fuel(ctx, 1);
                      vec_u8_t t_vec_u8_60 = (vec_u8_t){0};
                      rt_fuel(ctx, 1);
                      t_vec_u8_60 = v_1;
                      v_1 = (vec_u8_t){0};
                      uint32_t t_i32_61 = UINT32_C(0);
                      rt_fuel(ctx, 1);
                      t_i32_61 = UINT32_C(92);
                      t_set_59 = user_std_vec_push(ctx, input, t_vec_u8_60, t_i32_61);
                      t_vec_u8_60 = (vec_u8_t){0};
                      v_1 = t_set_59;
                      t_set_59 = (vec_u8_t){0};
                      vec_u8_t t_set_62 = (vec_u8_t){0};
                      rt_fuel(ctx, 1);
                      vec_u8_t t_vec_u8_63 = (vec_u8_t){0};
                      rt_fuel(ctx, 1);
                      t_vec_u8_63 = v_1;
                      v_1 = (vec_u8_t){0};
                      uint32_t t_i32_64 = UINT32_C(0);
                      rt_fuel(ctx, 1);
                      t_i32_64 = UINT32_C(116);
                      t_set_62 = user_std_vec_push(ctx, input, t_vec_u8_63, t_i32_64);
                      t_vec_u8_63 = (vec_u8_t){0};
                      v_1 = t_set_62;
                      t_set_62 = (vec_u8_t){0};
                      uint32_t t_i32_65 = UINT32_C(0);
                      rt_fuel(ctx, 1);
                      t_i32_65 = UINT32_C(0);
                      rt_vec_u8_drop(ctx, &t_vec_u8_60);
                      rt_vec_u8_drop(ctx, &t_vec_u8_63);
                    }
                  } else {
                    {
                      vec_u8_t t_set_66 = (vec_u8_t){0};
                      rt_fuel(ctx, 1);
                      vec_u8_t t_vec_u8_67 = (vec_u8_t){0};
                      rt_fuel(ctx, 1);
                      t_vec_u8_67 = v_1;
                      v_1 = (vec_u8_t){0};
                      uint32_t t_i32_68 = UINT32_C(0);
                      rt_fuel(ctx, 1);
                      t_i32_68 = v_13;
                      t_set_66 = user_std_vec_push(ctx, input, t_vec_u8_67, t_i32_68);
                      t_vec_u8_67 = (vec_u8_t){0};
                      v_1 = t_set_66;
                      t_set_66 = (vec_u8_t){0};
                      uint32_t t_i32_69 = UINT32_C(0);
                      rt_fuel(ctx, 1);
                      t_i32_69 = UINT32_C(0);
                      rt_vec_u8_drop(ctx, &t_vec_u8_67);
                    }
                  }
                }
              }
            }
          }
          uint32_t t_i32_70 = UINT32_C(0);
          rt_fuel(ctx, 1);
          t_i32_70 = UINT32_C(0);
        }
      }
      v_10 = v_10 + UINT32_C(1);
    }
    t_i32_9 = UINT32_C(0);
    vec_u8_t t_set_71 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_72 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_72 = v_1;
    v_1 = (vec_u8_t){0};
    uint32_t t_i32_73 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_73 = UINT32_C(34);
    t_set_71 = user_std_vec_push(ctx, input, t_vec_u8_72, t_i32_73);
    t_vec_u8_72 = (vec_u8_t){0};
    v_1 = t_set_71;
    t_set_71 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_74 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_74 = v_1;
    v_1 = (vec_u8_t){0};
    out = user_std_vec_as_bytes(ctx, input, t_vec_u8_74);
    t_vec_u8_74 = (vec_u8_t){0};
    rt_vec_u8_drop(ctx, &t_vec_u8_7);
    rt_vec_u8_drop(ctx, &t_vec_u8_72);
    rt_vec_u8_drop(ctx, &t_vec_u8_74);
    rt_vec_u8_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/effects/http.x07.json"
static bytes_t user_std_web_ui_effects_http__request_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    vec_u8_t v_1 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_2 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_5 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_8 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_8 = p0;
    t_i32_7 = t_view_8.len;
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_10 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_10 = p1;
    t_i32_9 = t_view_10.len;
    t_i32_6 = t_i32_7 + t_i32_9;
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = p2;
    t_i32_11 = t_view_12.len;
    t_i32_5 = t_i32_6 + t_i32_11;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_14 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_14 = p3;
    t_i32_13 = t_view_14.len;
    t_i32_4 = t_i32_5 + t_i32_13;
    uint32_t t_i32_15 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_16 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_16 = p4;
    t_i32_15 = t_view_16.len;
    t_i32_3 = t_i32_4 + t_i32_15;
    uint32_t t_i32_17 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_17 = UINT32_C(128);
    t_i32_2 = t_i32_3 + t_i32_17;
    v_1 = user_std_vec_with_capacity(ctx, input, t_i32_2);
    vec_u8_t t_set_18 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_19 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_19 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_20 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_21[] = "{\"v\":1,\"kind\":\"x07.web_ui.effect.http.request\",\"request\":{\"schema_version\":\"x07.http.request.envelope@0.1.0\",\"id\":";
    t_view_20 = rt_view_from_literal(ctx, (const uint8_t*)lit_21, UINT32_C(114));
    t_set_18 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_19, t_view_20);
    t_vec_u8_19 = (vec_u8_t){0};
    v_1 = t_set_18;
    t_set_18 = (vec_u8_t){0};
    vec_u8_t t_set_22 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_23 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_23 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_24 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_24 = p0;
    t_set_22 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_23, t_view_24);
    t_vec_u8_23 = (vec_u8_t){0};
    v_1 = t_set_22;
    t_set_22 = (vec_u8_t){0};
    vec_u8_t t_set_25 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_26 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_26 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_27 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_28[] = ",\"method\":";
    t_view_27 = rt_view_from_literal(ctx, (const uint8_t*)lit_28, UINT32_C(10));
    t_set_25 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_26, t_view_27);
    t_vec_u8_26 = (vec_u8_t){0};
    v_1 = t_set_25;
    t_set_25 = (vec_u8_t){0};
    vec_u8_t t_set_29 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_30 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_30 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_31 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_31 = p1;
    t_set_29 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_30, t_view_31);
    t_vec_u8_30 = (vec_u8_t){0};
    v_1 = t_set_29;
    t_set_29 = (vec_u8_t){0};
    vec_u8_t t_set_32 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_33 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_33 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_34 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_35[] = ",\"path\":";
    t_view_34 = rt_view_from_literal(ctx, (const uint8_t*)lit_35, UINT32_C(8));
    t_set_32 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_33, t_view_34);
    t_vec_u8_33 = (vec_u8_t){0};
    v_1 = t_set_32;
    t_set_32 = (vec_u8_t){0};
    vec_u8_t t_set_36 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_37 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_37 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_38 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_38 = p2;
    t_set_36 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_37, t_view_38);
    t_vec_u8_37 = (vec_u8_t){0};
    v_1 = t_set_36;
    t_set_36 = (vec_u8_t){0};
    vec_u8_t t_set_39 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_40 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_40 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_41 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_42[] = ",\"headers\":";
    t_view_41 = rt_view_from_literal(ctx, (const uint8_t*)lit_42, UINT32_C(11));
    t_set_39 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_40, t_view_41);
    t_vec_u8_40 = (vec_u8_t){0};
    v_1 = t_set_39;
    t_set_39 = (vec_u8_t){0};
    vec_u8_t t_set_43 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_44 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_44 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_45 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_45 = p3;
    t_set_43 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_44, t_view_45);
    t_vec_u8_44 = (vec_u8_t){0};
    v_1 = t_set_43;
    t_set_43 = (vec_u8_t){0};
    vec_u8_t t_set_46 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_47 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_47 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_48 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_49[] = ",\"body\":";
    t_view_48 = rt_view_from_literal(ctx, (const uint8_t*)lit_49, UINT32_C(8));
    t_set_46 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_47, t_view_48);
    t_vec_u8_47 = (vec_u8_t){0};
    v_1 = t_set_46;
    t_set_46 = (vec_u8_t){0};
    vec_u8_t t_set_50 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_51 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_51 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_52 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_52 = p4;
    t_set_50 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_51, t_view_52);
    t_vec_u8_51 = (vec_u8_t){0};
    v_1 = t_set_50;
    t_set_50 = (vec_u8_t){0};
    vec_u8_t t_set_53 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_54 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_54 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_55 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_56[] = "}}";
    t_view_55 = rt_view_from_literal(ctx, (const uint8_t*)lit_56, UINT32_C(2));
    t_set_53 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_54, t_view_55);
    t_vec_u8_54 = (vec_u8_t){0};
    v_1 = t_set_53;
    t_set_53 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_57 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_57 = v_1;
    v_1 = (vec_u8_t){0};
    out = user_std_vec_as_bytes(ctx, input, t_vec_u8_57);
    t_vec_u8_57 = (vec_u8_t){0};
    rt_vec_u8_drop(ctx, &t_vec_u8_19);
    rt_vec_u8_drop(ctx, &t_vec_u8_23);
    rt_vec_u8_drop(ctx, &t_vec_u8_26);
    rt_vec_u8_drop(ctx, &t_vec_u8_30);
    rt_vec_u8_drop(ctx, &t_vec_u8_33);
    rt_vec_u8_drop(ctx, &t_vec_u8_37);
    rt_vec_u8_drop(ctx, &t_vec_u8_40);
    rt_vec_u8_drop(ctx, &t_vec_u8_44);
    rt_vec_u8_drop(ctx, &t_vec_u8_47);
    rt_vec_u8_drop(ctx, &t_vec_u8_51);
    rt_vec_u8_drop(ctx, &t_vec_u8_54);
    rt_vec_u8_drop(ctx, &t_vec_u8_57);
    rt_vec_u8_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/effects/http.x07.json"
static bytes_t user_std_web_ui_effects_http_dispatch_response_body_text_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_2[] = "state.__x07_http.response.body.text";
    v_1 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_2, UINT32_C(35));
    rt_fuel(ctx, 1);
    bytes_view_t t_view_3 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_3 = p0;
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = rt_bytes_view(ctx, v_1);
    out = user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx, input, t_view_3, t_view_4);
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/effects/http.x07.json"
static bytes_t user_std_web_ui_effects_http_dispatch_response_status_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_2[] = "state.__x07_http.response.status";
    v_1 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_2, UINT32_C(32));
    rt_fuel(ctx, 1);
    bytes_view_t t_view_3 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_3 = p0;
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = rt_bytes_view(ctx, v_1);
    out = user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx, input, t_view_3, t_view_4);
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/effects/http.x07.json"
static bytes_t user_std_web_ui_effects_http_request_get_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_1 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_1 = p0;
  bytes_view_t t_view_2 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "\"GET\"";
  t_view_2 = rt_view_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(5));
  bytes_view_t t_view_4 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_4 = p1;
  bytes_view_t t_view_5 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_6[] = "[]";
  t_view_5 = rt_view_from_literal(ctx, (const uint8_t*)lit_6, UINT32_C(2));
  bytes_view_t t_view_7 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_8[] = "{\"bytes_len\":0}";
  t_view_7 = rt_view_from_literal(ctx, (const uint8_t*)lit_8, UINT32_C(15));
  out = user_std_web_ui_effects_http__request_v1(ctx, input, t_view_1, t_view_2, t_view_4, t_view_5, t_view_7);
  return out;
}

#line 1 "std/web_ui/effects/http.x07.json"
static bytes_t user_std_web_ui_effects_http_request_post_json_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    uint32_t t_i32_2 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_3 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_3 = p2;
    t_i32_2 = t_view_3.len;
    v_1 = user_std_fmt_u32_to_dec(ctx, input, t_i32_2);
    bytes_t v_4 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_5 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_5 = p2;
    v_4 = user_std_web_ui_effects_http__json_string_v1(ctx, input, t_view_5);
    vec_u8_t v_6 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_7 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_9 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_10 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_10 = rt_bytes_view(ctx, v_1);
    t_i32_9 = t_view_10.len;
    uint32_t t_i32_11 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = rt_bytes_view(ctx, v_4);
    t_i32_11 = t_view_12.len;
    t_i32_8 = t_i32_9 + t_i32_11;
    uint32_t t_i32_13 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_13 = UINT32_C(32);
    t_i32_7 = t_i32_8 + t_i32_13;
    v_6 = user_std_vec_with_capacity(ctx, input, t_i32_7);
    vec_u8_t t_set_14 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_15 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_15 = v_6;
    v_6 = (vec_u8_t){0};
    bytes_view_t t_view_16 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_17[] = "{\"bytes_len\":";
    t_view_16 = rt_view_from_literal(ctx, (const uint8_t*)lit_17, UINT32_C(13));
    t_set_14 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_15, t_view_16);
    t_vec_u8_15 = (vec_u8_t){0};
    v_6 = t_set_14;
    t_set_14 = (vec_u8_t){0};
    vec_u8_t t_set_18 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_19 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_19 = v_6;
    v_6 = (vec_u8_t){0};
    bytes_view_t t_view_20 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_20 = rt_bytes_view(ctx, v_1);
    t_set_18 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_19, t_view_20);
    t_vec_u8_19 = (vec_u8_t){0};
    v_6 = t_set_18;
    t_set_18 = (vec_u8_t){0};
    vec_u8_t t_set_21 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_22 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_22 = v_6;
    v_6 = (vec_u8_t){0};
    bytes_view_t t_view_23 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_24[] = ",\"text\":";
    t_view_23 = rt_view_from_literal(ctx, (const uint8_t*)lit_24, UINT32_C(8));
    t_set_21 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_22, t_view_23);
    t_vec_u8_22 = (vec_u8_t){0};
    v_6 = t_set_21;
    t_set_21 = (vec_u8_t){0};
    vec_u8_t t_set_25 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_26 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_26 = v_6;
    v_6 = (vec_u8_t){0};
    bytes_view_t t_view_27 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_27 = rt_bytes_view(ctx, v_4);
    t_set_25 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_26, t_view_27);
    t_vec_u8_26 = (vec_u8_t){0};
    v_6 = t_set_25;
    t_set_25 = (vec_u8_t){0};
    vec_u8_t t_set_28 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_29 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_29 = v_6;
    v_6 = (vec_u8_t){0};
    bytes_view_t t_view_30 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_31[] = "}";
    t_view_30 = rt_view_from_literal(ctx, (const uint8_t*)lit_31, UINT32_C(1));
    t_set_28 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_29, t_view_30);
    t_vec_u8_29 = (vec_u8_t){0};
    v_6 = t_set_28;
    t_set_28 = (vec_u8_t){0};
    bytes_t v_32 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_33 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_33 = v_6;
    v_6 = (vec_u8_t){0};
    v_32 = user_std_vec_as_bytes(ctx, input, t_vec_u8_33);
    t_vec_u8_33 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    bytes_view_t t_view_34 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_34 = p0;
    bytes_view_t t_view_35 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_36[] = "\"POST\"";
    t_view_35 = rt_view_from_literal(ctx, (const uint8_t*)lit_36, UINT32_C(6));
    bytes_view_t t_view_37 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_37 = p1;
    bytes_view_t t_view_38 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_39[] = "[{\"k\":\"Content-Type\",\"v\":\"application/json\"}]";
    t_view_38 = rt_view_from_literal(ctx, (const uint8_t*)lit_39, UINT32_C(45));
    bytes_view_t t_view_40 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_40 = rt_bytes_view(ctx, v_32);
    out = user_std_web_ui_effects_http__request_v1(ctx, input, t_view_34, t_view_35, t_view_37, t_view_38, t_view_40);
    rt_vec_u8_drop(ctx, &t_vec_u8_15);
    rt_vec_u8_drop(ctx, &t_vec_u8_19);
    rt_vec_u8_drop(ctx, &t_vec_u8_22);
    rt_vec_u8_drop(ctx, &t_vec_u8_26);
    rt_vec_u8_drop(ctx, &t_vec_u8_29);
    rt_vec_u8_drop(ctx, &t_vec_u8_33);
    rt_vec_u8_drop(ctx, &v_6);
    rt_bytes_drop(ctx, &v_32);
    rt_bytes_drop(ctx, &v_1);
    rt_bytes_drop(ctx, &v_4);
  }
  return out;
}

#line 1 "std/web_ui/effects/nav.x07.json"
static bytes_t user_std_web_ui_effects_nav_dispatch_href_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_2[] = "state.__x07_nav.href";
    v_1 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_2, UINT32_C(20));
    rt_fuel(ctx, 1);
    bytes_view_t t_view_3 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_3 = p0;
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = rt_bytes_view(ctx, v_1);
    out = user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx, input, t_view_3, t_view_4);
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/effects/nav.x07.json"
static bytes_t user_std_web_ui_effects_nav_push_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "{\"v\":1,\"kind\":\"x07.web_ui.effect.nav.push\",\"path\":";
  t_bytes_2 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(50));
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_5 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_5 = p0;
  t_bytes_4 = rt_view_to_bytes(ctx, t_view_5);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_4);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_7[] = "}";
  t_bytes_6 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  return out;
}

#line 1 "std/web_ui/effects/storage.x07.json"
static bytes_t user_std_web_ui_effects_storage_dispatch_get_value_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_2[] = "state.__x07_storage.get.value";
    v_1 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_2, UINT32_C(29));
    rt_fuel(ctx, 1);
    bytes_view_t t_view_3 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_3 = p0;
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = rt_bytes_view(ctx, v_1);
    out = user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx, input, t_view_3, t_view_4);
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/effects/storage.x07.json"
static bytes_t user_std_web_ui_effects_storage_dispatch_set_ok_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_2[] = "state.__x07_storage.set.ok";
    v_1 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_2, UINT32_C(26));
    rt_fuel(ctx, 1);
    bytes_view_t t_view_3 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_3 = p0;
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = rt_bytes_view(ctx, v_1);
    out = user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx, input, t_view_3, t_view_4);
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/effects/storage.x07.json"
static bytes_t user_std_web_ui_effects_storage_get_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "{\"v\":1,\"kind\":\"x07.web_ui.effect.storage.get\",\"key\":";
  t_bytes_2 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(52));
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_5 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_5 = p0;
  t_bytes_4 = rt_view_to_bytes(ctx, t_view_5);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_4);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_7[] = "}";
  t_bytes_6 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  return out;
}

#line 1 "std/web_ui/effects/storage.x07.json"
static bytes_t user_std_web_ui_effects_storage_set_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_4[] = "{\"v\":1,\"kind\":\"x07.web_ui.effect.storage.set\",\"key\":";
  t_bytes_3 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_4, UINT32_C(52));
  bytes_t t_bytes_5 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_6 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_6 = p0;
  t_bytes_5 = rt_view_to_bytes(ctx, t_view_6);
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_5);
  bytes_t t_bytes_7 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_8[] = ",\"value\":";
  t_bytes_7 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_8, UINT32_C(9));
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_7);
  bytes_t t_bytes_9 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_11 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_11 = p1;
  t_bytes_10 = rt_view_to_bytes(ctx, t_view_11);
  bytes_t t_bytes_12 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_13[] = "}";
  t_bytes_12 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_13, UINT32_C(1));
  t_bytes_9 = rt_bytes_concat(ctx, t_bytes_10, t_bytes_12);
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_9);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_5);
  rt_bytes_drop(ctx, &t_bytes_7);
  rt_bytes_drop(ctx, &t_bytes_9);
  return out;
}

#line 1 "std/web_ui/patch.x07.json"
static bytes_t user_std_web_ui_patch_diff_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  uint32_t t_i32_1 = UINT32_C(0);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_2 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_2 = p0;
  bytes_view_t t_view_3 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_3 = p1;
  t_i32_1 = rt_view_eq(ctx, t_view_2, t_view_3);
  if (t_i32_1 != UINT32_C(0)) {
    rt_fuel(ctx, 1);
    out = user_std_web_ui_patch_empty_v1(ctx, input);
  } else {
    rt_fuel(ctx, 1);
    bytes_view_t t_view_4 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_4 = p1;
    out = user_std_web_ui_patch_replace_root_v1(ctx, input, t_view_4);
  }
  return out;
}

#line 1 "std/web_ui/patch.x07.json"
static bytes_t user_std_web_ui_patch_empty_v1(ctx_t* ctx, bytes_view_t input) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_1[] = "[]";
  out = rt_bytes_from_literal(ctx, (const uint8_t*)lit_1, UINT32_C(2));
  return out;
}

#line 1 "std/web_ui/patch.x07.json"
static bytes_t user_std_web_ui_patch_replace_root_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "[{\"op\":\"replace\",\"path\":\"\",\"value\":";
  t_bytes_2 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(35));
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_5 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_5 = p0;
  t_bytes_4 = rt_view_to_bytes(ctx, t_view_5);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_4);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_7[] = "}]";
  t_bytes_6 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(2));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  return out;
}

#line 1 "std/web_ui/runtime.x07.json"
static bytes_t user_std_web_ui_runtime_dispatch_extract_canon_or_err_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    vec_u8_t v_1 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    uint32_t t_i32_2 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_3 = UINT32_C(0);
    rt_fuel(ctx, 1);
    uint32_t t_i32_4 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_5 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_5 = p0;
    t_i32_4 = t_view_5.len;
    uint32_t t_i32_6 = UINT32_C(0);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_7 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_7 = p1;
    t_i32_6 = t_view_7.len;
    t_i32_3 = t_i32_4 + t_i32_6;
    uint32_t t_i32_8 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_8 = UINT32_C(8);
    t_i32_2 = t_i32_3 + t_i32_8;
    v_1 = user_std_vec_with_capacity(ctx, input, t_i32_2);
    vec_u8_t t_set_9 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_10 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_10 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_11 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_11 = p0;
    t_set_9 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_10, t_view_11);
    t_vec_u8_10 = (vec_u8_t){0};
    v_1 = t_set_9;
    t_set_9 = (vec_u8_t){0};
    vec_u8_t t_set_12 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_13 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_13 = v_1;
    v_1 = (vec_u8_t){0};
    uint32_t t_i32_14 = UINT32_C(0);
    rt_fuel(ctx, 1);
    t_i32_14 = UINT32_C(0);
    t_set_12 = user_std_vec_push(ctx, input, t_vec_u8_13, t_i32_14);
    t_vec_u8_13 = (vec_u8_t){0};
    v_1 = t_set_12;
    t_set_12 = (vec_u8_t){0};
    vec_u8_t t_set_15 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_16 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_16 = v_1;
    v_1 = (vec_u8_t){0};
    bytes_view_t t_view_17 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_17 = p1;
    t_set_15 = user_std_vec_extend_bytes(ctx, input, t_vec_u8_16, t_view_17);
    t_vec_u8_16 = (vec_u8_t){0};
    v_1 = t_set_15;
    t_set_15 = (vec_u8_t){0};
    bytes_t v_18 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    vec_u8_t t_vec_u8_19 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    t_vec_u8_19 = v_1;
    v_1 = (vec_u8_t){0};
    v_18 = user_std_vec_as_bytes(ctx, input, t_vec_u8_19);
    t_vec_u8_19 = (vec_u8_t){0};
    rt_fuel(ctx, 1);
    bytes_view_t t_view_20 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_20 = rt_bytes_view(ctx, v_18);
    out = user_std_json_extract_path_canon_or_err(ctx, input, t_view_20);
    rt_vec_u8_drop(ctx, &t_vec_u8_10);
    rt_vec_u8_drop(ctx, &t_vec_u8_13);
    rt_vec_u8_drop(ctx, &t_vec_u8_16);
    rt_vec_u8_drop(ctx, &t_vec_u8_19);
    rt_bytes_drop(ctx, &v_18);
    rt_vec_u8_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/runtime.x07.json"
static bytes_t user_std_web_ui_runtime_frame_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_5 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_7 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_8 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_9 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_11[] = "{\"v\":1,\"kind\":\"x07.web_ui.frame\",\"state\":";
  t_bytes_10 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_11, UINT32_C(41));
  bytes_t t_bytes_12 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_13 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_13 = p0;
  t_bytes_12 = rt_view_to_bytes(ctx, t_view_13);
  t_bytes_9 = rt_bytes_concat(ctx, t_bytes_10, t_bytes_12);
  bytes_t t_bytes_14 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_15[] = ",\"ui\":";
  t_bytes_14 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_15, UINT32_C(6));
  t_bytes_8 = rt_bytes_concat(ctx, t_bytes_9, t_bytes_14);
  bytes_t t_bytes_16 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_17 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_17 = p1;
  t_bytes_16 = rt_view_to_bytes(ctx, t_view_17);
  t_bytes_7 = rt_bytes_concat(ctx, t_bytes_8, t_bytes_16);
  bytes_t t_bytes_18 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_19[] = ",\"patches\":";
  t_bytes_18 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_19, UINT32_C(11));
  t_bytes_6 = rt_bytes_concat(ctx, t_bytes_7, t_bytes_18);
  bytes_t t_bytes_20 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_21 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_21 = p2;
  t_bytes_20 = rt_view_to_bytes(ctx, t_view_21);
  t_bytes_5 = rt_bytes_concat(ctx, t_bytes_6, t_bytes_20);
  bytes_t t_bytes_22 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_23[] = ",\"effects\":";
  t_bytes_22 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_23, UINT32_C(11));
  t_bytes_4 = rt_bytes_concat(ctx, t_bytes_5, t_bytes_22);
  bytes_t t_bytes_24 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_25 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_25 = p3;
  t_bytes_24 = rt_view_to_bytes(ctx, t_view_25);
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_24);
  bytes_t t_bytes_26 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_27[] = ",\"telemetry\":";
  t_bytes_26 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_27, UINT32_C(13));
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_26);
  bytes_t t_bytes_28 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_29 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_29 = p4;
  t_bytes_28 = rt_view_to_bytes(ctx, t_view_29);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_28);
  bytes_t t_bytes_30 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_31[] = "}\n";
  t_bytes_30 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_31, UINT32_C(2));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_30);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_14);
  rt_bytes_drop(ctx, &t_bytes_16);
  rt_bytes_drop(ctx, &t_bytes_18);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_20);
  rt_bytes_drop(ctx, &t_bytes_22);
  rt_bytes_drop(ctx, &t_bytes_24);
  rt_bytes_drop(ctx, &t_bytes_26);
  rt_bytes_drop(ctx, &t_bytes_28);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_30);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_5);
  rt_bytes_drop(ctx, &t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_7);
  rt_bytes_drop(ctx, &t_bytes_8);
  rt_bytes_drop(ctx, &t_bytes_9);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_children1_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "[";
  t_bytes_2 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(1));
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_5 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_5 = p0;
  t_bytes_4 = rt_view_to_bytes(ctx, t_view_5);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_4);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_7[] = "]";
  t_bytes_6 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_children2_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_5[] = "[";
  t_bytes_4 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_5, UINT32_C(1));
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_7 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_7 = p0;
  t_bytes_6 = rt_view_to_bytes(ctx, t_view_7);
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_6);
  bytes_t t_bytes_8 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_9[] = ",";
  t_bytes_8 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_9, UINT32_C(1));
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_8);
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_11 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_11 = p1;
  t_bytes_10 = rt_view_to_bytes(ctx, t_view_11);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_10);
  bytes_t t_bytes_12 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_13[] = "]";
  t_bytes_12 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_13, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_8);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_children3_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_5 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_7[] = "[";
  t_bytes_6 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(1));
  bytes_t t_bytes_8 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_9 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_9 = p0;
  t_bytes_8 = rt_view_to_bytes(ctx, t_view_9);
  t_bytes_5 = rt_bytes_concat(ctx, t_bytes_6, t_bytes_8);
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_11[] = ",";
  t_bytes_10 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_11, UINT32_C(1));
  t_bytes_4 = rt_bytes_concat(ctx, t_bytes_5, t_bytes_10);
  bytes_t t_bytes_12 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_13 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_13 = p1;
  t_bytes_12 = rt_view_to_bytes(ctx, t_view_13);
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_12);
  bytes_t t_bytes_14 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_15[] = ",";
  t_bytes_14 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_15, UINT32_C(1));
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_14);
  bytes_t t_bytes_16 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_17 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_17 = p2;
  t_bytes_16 = rt_view_to_bytes(ctx, t_view_17);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_16);
  bytes_t t_bytes_18 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_19[] = "]";
  t_bytes_18 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_19, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_18);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_14);
  rt_bytes_drop(ctx, &t_bytes_16);
  rt_bytes_drop(ctx, &t_bytes_18);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_5);
  rt_bytes_drop(ctx, &t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_8);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_el_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2, bytes_view_t p3, bytes_view_t p4) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_5 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_7 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_8 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_9[] = "{\"k\":\"el\",\"tag\":";
  t_bytes_8 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_9, UINT32_C(16));
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_11 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_11 = p0;
  t_bytes_10 = rt_view_to_bytes(ctx, t_view_11);
  t_bytes_7 = rt_bytes_concat(ctx, t_bytes_8, t_bytes_10);
  bytes_t t_bytes_12 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_13[] = ",\"key\":";
  t_bytes_12 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_13, UINT32_C(7));
  t_bytes_6 = rt_bytes_concat(ctx, t_bytes_7, t_bytes_12);
  bytes_t t_bytes_14 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_15 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_15 = p1;
  t_bytes_14 = rt_view_to_bytes(ctx, t_view_15);
  t_bytes_5 = rt_bytes_concat(ctx, t_bytes_6, t_bytes_14);
  bytes_t t_bytes_16 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_17[] = ",\"props\":";
  t_bytes_16 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_17, UINT32_C(9));
  t_bytes_4 = rt_bytes_concat(ctx, t_bytes_5, t_bytes_16);
  bytes_t t_bytes_18 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_19 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_19 = p2;
  t_bytes_18 = rt_view_to_bytes(ctx, t_view_19);
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_18);
  bytes_t t_bytes_20 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_21[] = ",\"on\":";
  t_bytes_20 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_21, UINT32_C(6));
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_20);
  bytes_t t_bytes_22 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_23 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_23 = p3;
  t_bytes_22 = rt_view_to_bytes(ctx, t_view_23);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_22);
  bytes_t t_bytes_24 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_25 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_26 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_27[] = ",\"children\":";
  t_bytes_26 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_27, UINT32_C(12));
  bytes_t t_bytes_28 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_29 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_29 = p4;
  t_bytes_28 = rt_view_to_bytes(ctx, t_view_29);
  t_bytes_25 = rt_bytes_concat(ctx, t_bytes_26, t_bytes_28);
  bytes_t t_bytes_30 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_31[] = "}";
  t_bytes_30 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_31, UINT32_C(1));
  t_bytes_24 = rt_bytes_concat(ctx, t_bytes_25, t_bytes_30);
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_24);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_14);
  rt_bytes_drop(ctx, &t_bytes_16);
  rt_bytes_drop(ctx, &t_bytes_18);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_20);
  rt_bytes_drop(ctx, &t_bytes_22);
  rt_bytes_drop(ctx, &t_bytes_24);
  rt_bytes_drop(ctx, &t_bytes_25);
  rt_bytes_drop(ctx, &t_bytes_26);
  rt_bytes_drop(ctx, &t_bytes_28);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_30);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_5);
  rt_bytes_drop(ctx, &t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_7);
  rt_bytes_drop(ctx, &t_bytes_8);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_fragment_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  {
    bytes_t v_1 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_2[] = "\"x07-fragment\"";
    v_1 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_2, UINT32_C(14));
    bytes_t v_3 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_4[] = "{}";
    v_3 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_4, UINT32_C(2));
    bytes_t v_5 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_6[] = "[]";
    v_5 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_6, UINT32_C(2));
    bytes_t v_7 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    static const char lit_8[] = "{\"display\":\"contents\"}";
    v_7 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_8, UINT32_C(22));
    bytes_t v_9 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_10 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_10 = rt_bytes_view(ctx, v_3);
    bytes_view_t t_view_11 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_11 = rt_bytes_view(ctx, v_5);
    bytes_view_t t_view_12 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_12 = rt_bytes_view(ctx, v_7);
    v_9 = user_std_web_ui_vdom_props_v1(ctx, input, t_view_10, t_view_11, t_view_12);
    bytes_t v_13 = rt_bytes_empty(ctx);
    rt_fuel(ctx, 1);
    v_13 = user_std_web_ui_vdom_on_empty_v1(ctx, input);
    rt_fuel(ctx, 1);
    bytes_view_t t_view_14 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_14 = rt_bytes_view(ctx, v_1);
    bytes_view_t t_view_15 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_15 = p0;
    bytes_view_t t_view_16 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_16 = rt_bytes_view(ctx, v_9);
    bytes_view_t t_view_17 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_17 = rt_bytes_view(ctx, v_13);
    bytes_view_t t_view_18 = rt_view_empty(ctx);
    rt_fuel(ctx, 1);
    t_view_18 = p1;
    out = user_std_web_ui_vdom_el_v1(ctx, input, t_view_14, t_view_15, t_view_16, t_view_17, t_view_18);
    rt_bytes_drop(ctx, &v_3);
    rt_bytes_drop(ctx, &v_5);
    rt_bytes_drop(ctx, &v_13);
    rt_bytes_drop(ctx, &v_9);
    rt_bytes_drop(ctx, &v_7);
    rt_bytes_drop(ctx, &v_1);
  }
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_on_click_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "[{\"type\":\"click\",\"target\":";
  t_bytes_2 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(26));
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_5 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_5 = p0;
  t_bytes_4 = rt_view_to_bytes(ctx, t_view_5);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_4);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_7[] = "}]";
  t_bytes_6 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(2));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_on_empty_v1(ctx_t* ctx, bytes_view_t input) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_1[] = "[]";
  out = rt_bytes_from_literal(ctx, (const uint8_t*)lit_1, UINT32_C(2));
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_props_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1, bytes_view_t p2) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_5 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_6[] = "{\"attrs\":";
  t_bytes_5 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_6, UINT32_C(9));
  bytes_t t_bytes_7 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_8 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_8 = p0;
  t_bytes_7 = rt_view_to_bytes(ctx, t_view_8);
  t_bytes_4 = rt_bytes_concat(ctx, t_bytes_5, t_bytes_7);
  bytes_t t_bytes_9 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_10[] = ",\"class\":";
  t_bytes_9 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_10, UINT32_C(9));
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_9);
  bytes_t t_bytes_11 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_12 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_12 = p1;
  t_bytes_11 = rt_view_to_bytes(ctx, t_view_12);
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_11);
  bytes_t t_bytes_13 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_14[] = ",\"style\":";
  t_bytes_13 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_14, UINT32_C(9));
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_13);
  bytes_t t_bytes_15 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_16 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_17 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_17 = p2;
  t_bytes_16 = rt_view_to_bytes(ctx, t_view_17);
  bytes_t t_bytes_18 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_19[] = "}";
  t_bytes_18 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_19, UINT32_C(1));
  t_bytes_15 = rt_bytes_concat(ctx, t_bytes_16, t_bytes_18);
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_15);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_11);
  rt_bytes_drop(ctx, &t_bytes_13);
  rt_bytes_drop(ctx, &t_bytes_15);
  rt_bytes_drop(ctx, &t_bytes_16);
  rt_bytes_drop(ctx, &t_bytes_18);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_5);
  rt_bytes_drop(ctx, &t_bytes_7);
  rt_bytes_drop(ctx, &t_bytes_9);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_text_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0, bytes_view_t p1) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_3 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_5[] = "{\"k\":\"text\",\"key\":";
  t_bytes_4 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_5, UINT32_C(18));
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_7 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_7 = p0;
  t_bytes_6 = rt_view_to_bytes(ctx, t_view_7);
  t_bytes_3 = rt_bytes_concat(ctx, t_bytes_4, t_bytes_6);
  bytes_t t_bytes_8 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_9[] = ",\"text\":";
  t_bytes_8 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_9, UINT32_C(8));
  t_bytes_2 = rt_bytes_concat(ctx, t_bytes_3, t_bytes_8);
  bytes_t t_bytes_10 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_11 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_11 = p1;
  t_bytes_10 = rt_view_to_bytes(ctx, t_view_11);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_10);
  bytes_t t_bytes_12 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_13[] = "}";
  t_bytes_12 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_13, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_10);
  rt_bytes_drop(ctx, &t_bytes_12);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_3);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_8);
  return out;
}

#line 1 "std/web_ui/vdom.x07.json"
static bytes_t user_std_web_ui_vdom_tree_v1(ctx_t* ctx, bytes_view_t input, bytes_view_t p0) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_1 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_t t_bytes_2 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_3[] = "{\"v\":1,\"kind\":\"x07.web_ui.tree\",\"root\":";
  t_bytes_2 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_3, UINT32_C(39));
  bytes_t t_bytes_4 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_5 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_5 = p0;
  t_bytes_4 = rt_view_to_bytes(ctx, t_view_5);
  t_bytes_1 = rt_bytes_concat(ctx, t_bytes_2, t_bytes_4);
  bytes_t t_bytes_6 = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  static const char lit_7[] = "}";
  t_bytes_6 = rt_bytes_from_literal(ctx, (const uint8_t*)lit_7, UINT32_C(1));
  out = rt_bytes_concat(ctx, t_bytes_1, t_bytes_6);
  rt_bytes_drop(ctx, &t_bytes_1);
  rt_bytes_drop(ctx, &t_bytes_2);
  rt_bytes_drop(ctx, &t_bytes_4);
  rt_bytes_drop(ctx, &t_bytes_6);
  return out;
}

#line 1 "main.x07.json"
static bytes_t solve(ctx_t* ctx, bytes_view_t input) {
  bytes_t out = rt_bytes_empty(ctx);
  rt_fuel(ctx, 1);
  bytes_view_t t_view_1 = rt_view_empty(ctx);
  rt_fuel(ctx, 1);
  t_view_1 = input;
  out = user_app_step_v1(ctx, input, t_view_1);
  return out;
}


static uint8_t rt_dummy_heap_mem[1];
static int32_t rt_last_exit_code = 0;

int32_t x07_exit_code_v1(void) {
  return rt_last_exit_code;
}

bytes_t x07_solve_v2(
    uint8_t* arena_mem,
    uint32_t arena_cap,
    const uint8_t* input_ptr,
    uint32_t input_len
) {
  if (!arena_mem) {
    if (arena_cap != 0) rt_trap("arena_mem is NULL");
    arena_mem = rt_dummy_heap_mem;
  }

  ctx_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.fuel_init = (uint64_t)(X07_FUEL_INIT);
  ctx.fuel = ctx.fuel_init;
  ctx.heap.mem = arena_mem;
  ctx.heap.cap = arena_cap;
  rt_heap_init(&ctx);
  rt_allocator_init(&ctx);
  rt_ext_ctx = &ctx;

#ifdef X07_DEBUG_BORROW
  rt_dbg_init(&ctx);
#endif

  rt_kv_init(&ctx);

  bytes_t input_bytes = rt_bytes_alloc(&ctx, input_len);
  if (input_len) {
    memcpy(input_bytes.ptr, input_ptr, input_len);
    rt_mem_on_memcpy(&ctx, input_len);
  }

  rt_mem_epoch_reset(&ctx);

  bytes_view_t input = rt_bytes_view(&ctx, input_bytes);
  bytes_t out = solve(&ctx, input);
  rt_last_exit_code = ctx.exit_code;
  rt_ext_ctx = NULL;

#ifdef X07_DEBUG_BORROW
  (void)rt_dbg_bytes_check(&ctx, out);
#endif

  rt_bytes_drop(&ctx, &input_bytes);

  return out;
}
