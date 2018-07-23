#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#define POLL_BUF_SIZE 65536
#define MAX_READ_PER_POLL 65536
#define POLL_INTERVAL_MS 1
#define POLL_INTERVAL_S ((gfloat) POLL_INTERVAL_MS / 1000.0)

typedef struct
{
  gdouble start_time;
  gdouble end_time;
  gint64 dirty_bytes;
  gint64 swap_bytes;
  gint64 output_bytes;
}
PollData;

typedef struct
{
  GMainLoop *main_loop;
  GTimer *timer;
  GPid child_pid;
  guint child_watch_id;
  guint periodic_timeout_id;
  GIOChannel *stdout_chan;
  GString *stdout_buf;
  PollData poll_last;
  PollData poll;
  gint64 total_output_bytes;

  /* Sum of (time * memory) slices */
  gint64 cumulative_byte_seconds;

  glong clk_tck;  /* System ticks per second */
  gchar *stat_fname;
  gchar *status_fname;
  gchar *smaps_fname;
}
RunContext;

static void
shutdown_child (RunContext *run_context)
{
  g_spawn_close_pid (run_context->child_pid);
  g_main_loop_quit (run_context->main_loop);
}

static void
sum_fields (PollData *pd, const gchar *str)
{
  const gchar *p0;

  pd->dirty_bytes = 0;
  pd->swap_bytes = 0;

  for (p0 = str; *p0; p0++)
  {
    gchar c = *p0;

    if (c == 'P' && !strncmp (p0, "Private_Dirty:", strlen ("Private_Dirty:")))
    {
      p0 += strlen ("Private_Dirty:");
      while (*p0 == ' ')
        p0++;
      pd->dirty_bytes += atoi (p0);
    }
    else if (c == 'S' && *(p0 + 1) == 'h' && !strncmp (p0, "Shared_Dirty:", strlen ("Shared_Dirty:")))
    {
      p0 += strlen ("Shared_Dirty:");
      while (*p0 == ' ')
        p0++;
      pd->dirty_bytes += atoi (p0);
    }
    else if (c == 'S' && *(p0 + 1) == 'w' && !strncmp (p0, "Swap:", strlen ("Swap:")))
    {
      p0 += strlen ("Swap:");
      while (*p0 == ' ')
        p0++;
      pd->swap_bytes += atoi (p0);
    }
  }

  pd->dirty_bytes *= 1024;
  pd->swap_bytes *= 1024;
}

static gint64 last_mem;
static gchar *last_smaps;
static gboolean print_next_smaps;

static gdouble
poll_cpu_time (RunContext *run_context)
{
  gchar *stats = NULL;
  guint64 user_ticks = 0;
  gdouble cpu_time = 0.0;

  if (!g_file_get_contents (run_context->stat_fname, &stats, NULL, NULL))
  {
    /* No stat file. We had one before, so process must be gone */
    shutdown_child (run_context);
    goto out;
  }

#if 0
  sscanf (stats, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu",
          &user_ticks);
#else
  sscanf (stats, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu",
          &user_ticks);
#endif

  cpu_time = (gdouble) user_ticks / (gdouble) run_context->clk_tck;

out:
  if (stats)
    g_free (stats);
  return cpu_time;
}

static gint64
poll_rss (RunContext *run_context)
{
  gchar *status = NULL;
  guint64 user_ticks = 0;
  gint64 rss = 0;
  gchar *p;

  if (!g_file_get_contents (run_context->status_fname, &status, NULL, NULL))
  {
    /* No status file. We had one before, so process must be gone */
    shutdown_child (run_context);
    goto out;
  }

  p = strstr (status, "RssAnon:");
  if (!p)
  {
    goto out;
  }

  p += strlen ("RssAnon:");
  while (*p == ' ')
    p++;
  rss = atoll (p) * 1024LL;

out:
  if (status)
    g_free (status);
  return rss;
}

static gboolean
poll_smaps (RunContext *run_context, PollData *poll_out)
{
  PollData poll_temp;
  gchar *smaps = NULL;
  gboolean result = FALSE;

  if (!g_file_get_contents (run_context->smaps_fname, &smaps, NULL, NULL))
  {
    /* No smaps file. We had one before, so process must be gone */
    shutdown_child (run_context);
    goto out;
  }

  sum_fields (&poll_temp, smaps);

  if (poll_temp.dirty_bytes + poll_temp.swap_bytes == 0)
  {
    /* Process likely exited, or bad poll; don't store results */
    goto out;
  }

  poll_out->dirty_bytes = poll_temp.dirty_bytes;
  poll_out->swap_bytes = poll_temp.swap_bytes;

#if 0
  if (print_next_smaps)
  {
    g_print ("\n--- NEXT SMAPS ---\n\n");
    fwrite (smaps, 1, strlen (smaps), stdout);
    print_next_smaps = FALSE;
  }
  else if (poll_temp.dirty_bytes + poll_temp.swap_bytes < (last_mem * 0.80))
  {
    g_print ("\n--- LAST SMAPS ---\n\n");
    fwrite (last_smaps, 1, strlen (last_smaps), stdout);
    g_print ("\n--- WEIRD SMAPS ---\n\n");
    fwrite (smaps, 1, strlen (smaps), stdout);
    print_next_smaps = TRUE;
  }

  last_mem = poll_temp.dirty_bytes + poll_temp.swap_bytes;
  g_free (last_smaps);
  last_smaps = smaps;
#endif

  result = TRUE;

out:
  if (smaps)
    g_free (smaps);
  return result;
}

static void
print_poll_data (RunContext *run_context)
{
  gdouble elapsed = run_context->poll_last.end_time - run_context->poll_last.start_time;
  gint64 mem_bytes;
  gint64 avg_mem_bytes;
  gdouble bps;

#if 1
  if (elapsed < POLL_INTERVAL_S / 4.0)
    return;
#endif

#if 0
  run_context->poll.end_time = g_timer_elapsed (run_context->timer, NULL);
#endif

  mem_bytes = run_context->poll_last.dirty_bytes + run_context->poll_last.swap_bytes;
  avg_mem_bytes = (mem_bytes + run_context->poll_last.dirty_bytes + run_context->poll_last.swap_bytes) / 2;

  run_context->total_output_bytes += run_context->poll_last.output_bytes;
  run_context->cumulative_byte_seconds += avg_mem_bytes * (run_context->poll_last.end_time - run_context->poll_last.start_time);

  bps = run_context->poll_last.output_bytes / elapsed;

  g_print ("%.4lf %ld %ld %.2lf %ld %ld\n",
           run_context->poll_last.end_time,
           run_context->poll_last.dirty_bytes + run_context->poll_last.swap_bytes,
           run_context->poll_last.output_bytes,
           bps,
           run_context->total_output_bytes,
           run_context->cumulative_byte_seconds);
}

static void
process_stdout (RunContext *run_context)
{
  gchar *p0;

  while ((p0 = strchr (run_context->stdout_buf->str, '\n')))
  {
    print_poll_data (run_context);
    fwrite (run_context->stdout_buf->str, sizeof (char), p0 - run_context->stdout_buf->str, stdout);
    fputc ('\n', stdout);
    g_string_erase (run_context->stdout_buf, 0, p0 - run_context->stdout_buf->str + 1);
  }
}

static gint
read_child_stdout (RunContext *run_context, gint n_max)
{
  gchar buf [POLL_BUF_SIZE];
  gint n_total = 0;
  GIOStatus status;

  while (n_total < n_max)
  {
    gsize n_read;

    status = g_io_channel_read_chars (run_context->stdout_chan,
                                      buf,
                                      POLL_BUF_SIZE,
                                      &n_read,
                                      NULL);

    if (status != G_IO_STATUS_NORMAL)
      break;

    n_total += (gint) n_read;
    run_context->poll.output_bytes += (gint) n_read;
  }

  if (status == G_IO_STATUS_ERROR || status == G_IO_STATUS_EOF)
    return -1;

  return n_total;
}

static gboolean
have_child_stdout (GIOChannel *source,
                   GIOCondition condition,
                   gpointer data)
{
  RunContext *run_context = data;

  if (read_child_stdout (run_context, MAX_READ_PER_POLL) < 0)
  {
    shutdown_child (run_context);
    return FALSE;
  }

  return TRUE;
}

static void
child_exited (GPid pid,
              gint status,
              gpointer data)
{
  RunContext *run_context = data;
  GError *error = NULL;

  read_child_stdout (run_context, G_MAXINT);
  shutdown_child (run_context);

  g_spawn_check_exit_status (status, &error);

  if (error)
  {
    g_printerr ("%s\n", error->message);
    g_clear_error (&error);
  }  

  run_context->child_watch_id = 0;
}

static void
begin_interval (RunContext *run_context)
{
  memset (&run_context->poll, 0, sizeof (run_context->poll));
  run_context->poll.start_time = run_context->poll_last.end_time;
}

static void
end_interval (RunContext *run_context, gboolean is_final)
{
  gint64 mem_bytes;

#if 0
  run_context->poll.end_time = poll_cpu_time (run_context);

  if (run_context->poll.end_time < run_context->poll_last.end_time)
    run_context->poll.end_time = run_context->poll_last.end_time;
#else
  run_context->poll.end_time = g_timer_elapsed (run_context->timer, NULL);
#endif

  mem_bytes = run_context->poll.dirty_bytes + run_context->poll.swap_bytes;
  if (mem_bytes == 0)
  {
    /* If we got a zero memory reading, it probably means the process was
     * dead at the time of smaps poll. Just extrapolate from the last one. */
    run_context->poll.dirty_bytes = run_context->poll_last.dirty_bytes;
    run_context->poll.swap_bytes = run_context->poll_last.swap_bytes;
    mem_bytes = run_context->poll.dirty_bytes + run_context->poll.swap_bytes;
  }

  /* If we got a short interval, just add it to the last one to avoid
   * precision issues causing huge spikes. */
#if 1
  if (run_context->poll.end_time - run_context->poll.start_time < (POLL_INTERVAL_S / 4.0))
  {
    run_context->poll_last.end_time = run_context->poll.end_time;
    run_context->poll_last.output_bytes += run_context->poll.output_bytes;

    run_context->poll.start_time = run_context->poll.end_time;
  }
  else
#endif
  {
    print_poll_data (run_context);
    run_context->poll_last = run_context->poll;
  }

  if (is_final)
    print_poll_data (run_context);
}

static gint
periodic_timeout (gpointer data)
{
  RunContext *run_context = data;

  read_child_stdout (run_context, MAX_READ_PER_POLL);

#if 0
  if (!poll_smaps (run_context, &run_context->poll))
    goto out;
#else
  run_context->poll.dirty_bytes = poll_rss (run_context);
#endif

  end_interval (run_context, FALSE);
  begin_interval (run_context);

out:
  return TRUE;
}

static gboolean
run (gint argc, gchar *argv [])
{
  RunContext run_context = { 0 };
  gint stdout_fd;
  gboolean result;
  GError *error = NULL;

  run_context.clk_tck = sysconf (_SC_CLK_TCK);

  run_context.timer = g_timer_new ();
  run_context.main_loop = g_main_loop_new (NULL, FALSE);

  result = g_spawn_async_with_pipes (NULL,  /* Working dir */
                                     &argv [1],
                                     NULL,  /* Env */
                                     G_SPAWN_SEARCH_PATH
                                     | G_SPAWN_CHILD_INHERITS_STDIN
                                     | G_SPAWN_DO_NOT_REAP_CHILD,  /* Flags */
                                     NULL,  /* child_setup */
                                     NULL,  /* user_data */
                                     &run_context.child_pid,
                                     NULL,  /* &stdin_fd */
                                     &stdout_fd,
                                     NULL,  /* &stderr_fd */
                                     NULL);  /* GError ** */
  if (!result)
    goto out;

  run_context.stdout_buf = g_string_new ("");
  run_context.stat_fname = g_strdup_printf ("/proc/%d/stat", (gint) run_context.child_pid);
  run_context.status_fname = g_strdup_printf ("/proc/%d/status", (gint) run_context.child_pid);
  run_context.smaps_fname = g_strdup_printf ("/proc/%d/smaps", (gint) run_context.child_pid);

  run_context.child_watch_id = g_child_watch_add (run_context.child_pid, child_exited, &run_context);
  run_context.periodic_timeout_id = g_timeout_add (POLL_INTERVAL_MS, periodic_timeout, &run_context);

  run_context.stdout_chan = g_io_channel_unix_new (stdout_fd);
  g_io_channel_set_encoding (run_context.stdout_chan, NULL, NULL);
  g_io_channel_set_buffered (run_context.stdout_chan, FALSE);
  g_io_channel_set_flags (run_context.stdout_chan, G_IO_FLAG_NONBLOCK, NULL);
  g_io_add_watch (run_context.stdout_chan, G_IO_IN | G_IO_ERR, have_child_stdout, &run_context);

  g_timer_start (run_context.timer);

  begin_interval (&run_context);
  g_main_loop_run (run_context.main_loop);

#if 1
  /* The last data point may be incomplete, throwing off our calculations and
   * causing ugly artifacts when plotted. Just discard it. */

  end_interval (&run_context, TRUE);
#endif

out:
  if (error)
  {
    g_printerr ("%s\n", error->message);
    g_clear_error (&error);
  }

  return result;
}

gint
main (gint argc, gchar *argv [])
{
  return run (argc, argv) ? 0 : 1;
}
