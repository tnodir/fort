/* Fort Firewall Configuration */

typedef struct fort_conf_ref {
  UINT32 volatile refcount;

  FORT_CONF conf;
} FORT_CONF_REF, *PFORT_CONF_REF;

#define FORT_DEVICE_PROV_BOOT		0x01
#define FORT_DEVICE_IS_OPENED		0x02
#define FORT_DEVICE_POWER_OFF		0x04
#define FORT_DEVICE_FILTER_TRANSPORT	0x08

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

static PFORT_CONF_REF
fort_conf_ref_new (const PFORT_CONF conf, ULONG len)
{
  const ULONG ref_len = len + offsetof(FORT_CONF_REF, conf);
  PFORT_CONF_REF conf_ref = fort_mem_alloc(ref_len, FORT_DEVICE_POOL_TAG);

  if (conf_ref != NULL) {
    conf_ref->refcount = 0;

    RtlCopyMemory(&conf_ref->conf, conf, len);
  }

  return conf_ref;
}

static void
fort_conf_ref_put (PFORT_DEVICE_CONF device_conf, PFORT_CONF_REF conf_ref)
{
  KLOCK_QUEUE_HANDLE lock_queue;

  KeAcquireInStackQueuedSpinLock(&device_conf->lock, &lock_queue);
  {
    const UINT32 refcount = --conf_ref->refcount;

    if (refcount == 0 && conf_ref != device_conf->ref) {
      fort_mem_free(conf_ref, FORT_DEVICE_POOL_TAG);
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
