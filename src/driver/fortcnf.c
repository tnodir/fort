/* Fort Firewall Configuration */

#define FORT_CONF_POOL_SIZE	(64 * 1024)
#define FORT_CONF_POOL_SIZE_MIN	(FORT_CONF_POOL_SIZE - 256)
#define FORT_CONF_POOL_DATA_OFF	offsetof(FORT_CONF_POOL, data)

/* Synchronize with tommy_node! */
typedef struct fort_conf_pool {
  struct fort_conf_pool *next;
  struct fort_conf_pool *prev;

  char data[4];
} FORT_CONF_POOL, *PFORT_CONF_POOL;

/* Synchronize with tommy_hashdyn_node! */
typedef struct fort_conf_exe_node {
  struct fort_conf_exe_node *next;
  struct fort_conf_exe_node *prev;

  PFORT_APP_ENTRY app_entry;

  tommy_key_t path_hash;
} FORT_CONF_EXE_NODE, *PFORT_CONF_EXE_NODE;

typedef struct fort_conf_ref {
  UINT32 volatile refcount;

  tlsf_t tlsf;
  tommy_list pools;
  tommy_list free_nodes;

  tommy_arrayof exe_nodes;
  tommy_hashdyn exe_map;

  EX_SPIN_LOCK lock;

  FORT_CONF conf;
} FORT_CONF_REF, *PFORT_CONF_REF;

#define FORT_DEVICE_PROV_BOOT		0x01
#define FORT_DEVICE_IS_OPENED		0x02
#define FORT_DEVICE_IS_VALIDATED	0x04
#define FORT_DEVICE_POWER_OFF		0x08
#define FORT_DEVICE_FILTER_TRANSPORT	0x10

typedef struct fort_device_conf {
  UCHAR volatile flags;

  FORT_CONF_FLAGS volatile conf_flags;
  PFORT_CONF_REF volatile ref;

  KSPIN_LOCK lock;
} FORT_DEVICE_CONF, *PFORT_DEVICE_CONF;


static void
fort_device_conf_open (PFORT_DEVICE_CONF device_conf)
{
  KeInitializeSpinLock(&device_conf->lock);
}

static UCHAR
fort_device_flag_set (PFORT_DEVICE_CONF device_conf, UCHAR flag, BOOL on)
{
  return on ? InterlockedOr8(&device_conf->flags, flag)
            : InterlockedAnd8(&device_conf->flags, ~flag);
}

static UCHAR
fort_device_flags (PFORT_DEVICE_CONF device_conf)
{
  return fort_device_flag_set(device_conf, 0, TRUE);
}

static UCHAR
fort_device_flag (PFORT_DEVICE_CONF device_conf, UCHAR flag)
{
  return fort_device_flags(device_conf) & flag;
}

static void
fort_conf_pool_init (PFORT_CONF_REF conf_ref, UINT32 size)
{
  const UINT32 pool_size = (size >= FORT_CONF_POOL_SIZE_MIN)
    ? size * 2 : FORT_CONF_POOL_SIZE;

  tommy_node *pool = tommy_malloc(pool_size);
  if (pool == NULL)
    return;

  tommy_list_insert_first(&conf_ref->pools, pool);

  conf_ref->tlsf = tlsf_create_with_pool(
    (char *) pool + FORT_CONF_POOL_DATA_OFF,
    pool_size - FORT_CONF_POOL_DATA_OFF);
}

static void
fort_conf_pool_done (PFORT_CONF_REF conf_ref)
{
  tommy_node *pool = tommy_list_head(&conf_ref->pools);
  while (pool != NULL) {
    tommy_node *next = pool->next;
    tommy_free(pool);
    pool = next;
  }
}

static void *
fort_conf_pool_malloc (PFORT_CONF_REF conf_ref, UINT32 size)
{
  tommy_node *pool = tommy_list_tail(&conf_ref->pools);
  void *p;

  if (pool == NULL)
    return NULL;

  p = tlsf_malloc(conf_ref->tlsf, size);
  if (p == NULL) {
    const UINT32 pool_size = (size >= FORT_CONF_POOL_SIZE_MIN)
      ? size * 2 : FORT_CONF_POOL_SIZE;

    pool = tommy_malloc(pool_size);
    if (pool == NULL)
      return NULL;

    tommy_list_insert_head_not_empty(&conf_ref->pools, pool);

    tlsf_add_pool(conf_ref->tlsf,
      (char *) pool + FORT_CONF_POOL_DATA_OFF,
      pool_size - FORT_CONF_POOL_DATA_OFF);

    p = tlsf_malloc(conf_ref->tlsf, size);
  }

  return p;
}

static PFORT_CONF_EXE_NODE
fort_conf_ref_exe_find_node (PFORT_CONF_REF conf_ref,
                             const char *path, UINT32 path_len,
                             tommy_key_t path_hash)
{
  PFORT_CONF_EXE_NODE node = (PFORT_CONF_EXE_NODE) tommy_hashdyn_bucket(
    &conf_ref->exe_map, path_hash);

  while (node != NULL) {
    if (node->path_hash == path_hash
        && fort_conf_app_exe_equal(node->app_entry, path, path_len)) {
      return node;
    }

    node = node->next;
  }

  return NULL;
}

static FORT_APP_FLAGS
fort_conf_exe_find (const PFORT_CONF conf,
                    const char *path, UINT32 path_len)
{
  PFORT_CONF_REF conf_ref = (PFORT_CONF_REF) ((char *) conf - offsetof(FORT_CONF_REF, conf));
  const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path, path_len);
  FORT_APP_FLAGS app_flags;

  KIRQL oldIrql = ExAcquireSpinLockShared(&conf_ref->lock);
  {
    const PFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(
      conf_ref, path, path_len, path_hash);

    app_flags.v = node ? node->app_entry->flags.v : 0;
  }
  ExReleaseSpinLockShared(&conf_ref->lock, oldIrql);

  return app_flags;
}

static NTSTATUS
fort_conf_ref_exe_add_path_locked (PFORT_CONF_REF conf_ref,
                                   const char *path, UINT32 path_len,
                                   tommy_key_t path_hash,
                                   FORT_APP_FLAGS flags)
{
  const PFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(
    conf_ref, path, path_len, path_hash);

  if (node == NULL) {
    const UINT16 entry_size = FORT_CONF_APP_ENTRY_SIZE(path_len);
    PFORT_APP_ENTRY entry = fort_conf_pool_malloc(conf_ref, entry_size);

    if (entry == NULL)
      return STATUS_INSUFFICIENT_RESOURCES;

    entry->flags = flags;
    entry->path_len = (UINT16) path_len;

    // Copy path
    {
      char *new_path = (char *) (entry + 1);
      RtlCopyMemory(new_path, path, path_len);
    }

    // Add exe node
    {
      PFORT_CONF conf = &conf_ref->conf;

      tommy_arrayof *exe_nodes = &conf_ref->exe_nodes;
      tommy_hashdyn *exe_map = &conf_ref->exe_map;

      tommy_hashdyn_node *node = tommy_list_tail(&conf_ref->free_nodes);

      if (node != NULL) {
        tommy_list_remove_existing(&conf_ref->free_nodes, node);
      } else {
        const UINT16 index = conf->exe_apps_n;

        tommy_arrayof_grow(exe_nodes, index + 1);

        node = tommy_arrayof_ref(exe_nodes, index);
      }

      tommy_hashdyn_insert(exe_map, node, entry, path_hash);

      ++conf->exe_apps_n;
    }

    return TRUE;
  } else {
    if (flags.is_new)
      return FORT_STATUS_USER_ERROR;

    // Replace flags
    {
      PFORT_APP_ENTRY entry = node->app_entry;

      entry->flags = flags;

      return STATUS_SUCCESS;
    }
  }
}

static NTSTATUS
fort_conf_ref_exe_add_path (PFORT_CONF_REF conf_ref,
                            const char *path, UINT32 path_len,
                            FORT_APP_FLAGS flags)
{
  const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path, path_len);
  NTSTATUS status;

  KIRQL oldIrql = ExAcquireSpinLockExclusive(&conf_ref->lock);
  status = fort_conf_ref_exe_add_path_locked(conf_ref, path, path_len, path_hash, flags);
  ExReleaseSpinLockExclusive(&conf_ref->lock, oldIrql);

  return status;
}

static NTSTATUS
fort_conf_ref_exe_add_entry (PFORT_CONF_REF conf_ref, const PFORT_APP_ENTRY entry,
                             BOOL locked)
{
  const char *path = (const char *) (entry + 1);
  const UINT32 path_len = entry->path_len;
  const FORT_APP_FLAGS flags = entry->flags;

  if (locked) {
    const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path, path_len);

    return fort_conf_ref_exe_add_path_locked(conf_ref, path, path_len, path_hash, flags);
  } else {
    return fort_conf_ref_exe_add_path(conf_ref, path, path_len, flags);
  }
}

static void
fort_conf_ref_exe_fill (PFORT_CONF_REF conf_ref, const PFORT_CONF conf)
{
  const char *app_entries = (const char *) (conf->data + conf->exe_apps_off);

  const int count = conf->exe_apps_n;
  int i;

  for (i = 0; i < count; ++i) {
    const PFORT_APP_ENTRY entry = (const PFORT_APP_ENTRY) app_entries;

    fort_conf_ref_exe_add_entry(conf_ref, entry, TRUE);

    app_entries += FORT_CONF_APP_ENTRY_SIZE(entry->path_len);
  }
}

static void
fort_conf_ref_exe_del_path (PFORT_CONF_REF conf_ref,
                            const char *path, UINT32 path_len)
{
  const tommy_key_t path_hash = (tommy_key_t) tommy_hash_u64(0, path, path_len);

  KIRQL oldIrql = ExAcquireSpinLockExclusive(&conf_ref->lock);
  {
    PFORT_CONF_EXE_NODE node = fort_conf_ref_exe_find_node(
      conf_ref, path, path_len, path_hash);

    if (node != NULL) {
      // Delete from conf
      {
        PFORT_CONF conf = &conf_ref->conf;
        --conf->exe_apps_n;
      }

      // Delete from pool
      {
        PFORT_APP_ENTRY entry = node->app_entry;
        tlsf_free(conf_ref->tlsf, entry);
      }

      // Delete from exe map
      tommy_hashdyn_remove_existing(&conf_ref->exe_map, (tommy_hashdyn_node *) node);

      tommy_list_insert_tail_check(&conf_ref->free_nodes, (tommy_node *) node);
    }
  }
  ExReleaseSpinLockExclusive(&conf_ref->lock, oldIrql);
}

static void
fort_conf_ref_exe_del_entry (PFORT_CONF_REF conf_ref, const PFORT_APP_ENTRY entry)
{
  const char *path = (const char *) (entry + 1);
  const UINT32 path_len = entry->path_len;

  fort_conf_ref_exe_del_path(conf_ref, path, path_len);
}

static void
fort_conf_ref_init (PFORT_CONF_REF conf_ref)
{
  conf_ref->refcount = 0;

  tommy_list_init(&conf_ref->pools);
  tommy_list_init(&conf_ref->free_nodes);

  tommy_arrayof_init(&conf_ref->exe_nodes, sizeof(FORT_CONF_EXE_NODE));
  tommy_hashdyn_init(&conf_ref->exe_map);

  conf_ref->lock = 0;
}

static PFORT_CONF_REF
fort_conf_ref_new (const PFORT_CONF conf, ULONG len)
{
  const ULONG conf_len = FORT_CONF_DATA_OFF + conf->exe_apps_off;
  const ULONG ref_len = conf_len + offsetof(FORT_CONF_REF, conf);
  PFORT_CONF_REF conf_ref = tommy_malloc(ref_len);

  if (conf_ref != NULL) {
    RtlCopyMemory(&conf_ref->conf, conf, conf_len);

    fort_conf_ref_init(conf_ref);
    fort_conf_pool_init(conf_ref, len - conf_len);

    fort_conf_ref_exe_fill(conf_ref, conf);
  }

  return conf_ref;
}

static void
fort_conf_ref_del (PFORT_CONF_REF conf_ref)
{
  fort_conf_pool_done(conf_ref);

  tommy_hashdyn_done(&conf_ref->exe_map);
  tommy_arrayof_done(&conf_ref->exe_nodes);

  tommy_free(conf_ref);
}

static void
fort_conf_ref_put (PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&device_conf->lock, &lock_queue);
  {
    const UINT32 refcount = --conf_ref->refcount;

    if (refcount == 0 && conf_ref != device_conf->ref) {
      fort_conf_ref_del(conf_ref);
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);
}

static PFORT_CONF_REF
fort_conf_ref_take (PFORT_DEVICE_CONF device_conf)
{
  PFORT_CONF_REF conf_ref;
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&device_conf->lock, &lock_queue);
  {
    conf_ref = device_conf->ref;
    if (conf_ref) {
      ++conf_ref->refcount;
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return conf_ref;
}

static FORT_CONF_FLAGS
fort_conf_ref_set (PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref)
{
  PFORT_CONF_REF old_conf_ref;
  FORT_CONF_FLAGS old_conf_flags;
  KLOCK_QUEUE_HANDLE lock_queue;

  old_conf_ref = fort_conf_ref_take(device_conf);

  KeAcquireInStackQueuedSpinLock(&device_conf->lock, &lock_queue);
  {
    device_conf->ref = conf_ref;

    if (old_conf_ref == NULL) {
      RtlZeroMemory(&old_conf_flags, sizeof(FORT_CONF_FLAGS));
      old_conf_flags.prov_boot = fort_device_flag(device_conf, FORT_DEVICE_PROV_BOOT) != 0;
    }

    if (conf_ref != NULL) {
      PFORT_CONF conf = &conf_ref->conf;
      const PFORT_CONF_FLAGS conf_flags = &conf->flags;

      fort_device_flag_set(device_conf, FORT_DEVICE_PROV_BOOT, conf_flags->prov_boot);

      device_conf->conf_flags = *conf_flags;
    } else {
      RtlZeroMemory((void *) &device_conf->conf_flags, sizeof(FORT_CONF_FLAGS));

      device_conf->conf_flags.prov_boot = fort_device_flag(device_conf, FORT_DEVICE_PROV_BOOT) != 0;
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  if (old_conf_ref != NULL) {
    old_conf_flags = old_conf_ref->conf.flags;
    fort_conf_ref_put(device_conf, old_conf_ref);
  }

  return old_conf_flags;
}

static FORT_CONF_FLAGS
fort_conf_ref_flags_set (PFORT_DEVICE_CONF device_conf, const PFORT_CONF_FLAGS conf_flags)
{
  FORT_CONF_FLAGS old_conf_flags;
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&device_conf->lock, &lock_queue);
  {
    PFORT_CONF_REF conf_ref = device_conf->ref;

    if (conf_ref != NULL) {
      PFORT_CONF conf = &conf_ref->conf;

      old_conf_flags = conf->flags;
      conf->flags = *conf_flags;

      fort_conf_app_perms_mask_init(conf, conf_flags->group_bits);

      fort_device_flag_set(device_conf, FORT_DEVICE_PROV_BOOT, conf_flags->prov_boot);

      device_conf->conf_flags = *conf_flags;
    } else {
      RtlZeroMemory(&old_conf_flags, sizeof(FORT_CONF_FLAGS));
      old_conf_flags.prov_boot = fort_device_flag(device_conf, FORT_DEVICE_PROV_BOOT) != 0;

      device_conf->conf_flags = old_conf_flags;
    }
  }
  KeReleaseInStackQueuedSpinLock(&lock_queue);

  return old_conf_flags;
}

static BOOL
fort_conf_ref_period_update (PFORT_DEVICE_CONF device_conf, BOOL force, int *periods_n)
{
  PFORT_CONF_REF conf_ref;
  FORT_TIME time;
  BOOL res = FALSE;

  /* Get current time */
  {
    TIME_FIELDS tf;
    LARGE_INTEGER system_time, local_time;

    KeQuerySystemTime(&system_time);
    ExSystemTimeToLocalTime(&system_time, &local_time);
    RtlTimeToTimeFields(&local_time, &tf);

    time.hour = (UCHAR) tf.Hour;
    time.minute = (UCHAR) tf.Minute;
  }

  conf_ref = fort_conf_ref_take(device_conf);

  if (conf_ref != NULL) {
    PFORT_CONF conf = &conf_ref->conf;

    if (conf->app_periods_n != 0) {
      const UINT16 period_bits =
        fort_conf_app_period_bits(conf, time, periods_n);

      if (force || device_conf->conf_flags.group_bits != period_bits) {
        device_conf->conf_flags.group_bits = period_bits;

        fort_conf_app_perms_mask_init(conf, period_bits);

        res = TRUE;
      }
    }

    fort_conf_ref_put(device_conf, conf_ref);
  }

  return res;
}
