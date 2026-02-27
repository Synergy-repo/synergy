
#include <rte_ethdev.h>

#include "timer.h"
#include "debug.h"
#include "globals.h"
#include "stats.h"

static _Atomic uint64_t tot_rx_count, tot_rx_dropped, tot_interrupts, tot_steal,
        tot_invalid_interrupts, tot_starvation;

/* per worker stats */
__thread uint64_t rx_count, rx_dropped, interrupts, invalid_interrupts,
        stealing, avoid_starvation, worker_memory, worker_lock, worker_free,
        yield;

/* timer stats no need be atomic or per thread*/
uint64_t timer_starvation_avoid, timer_wait_queue_processed, timer_processed;

#ifdef INT_STATS
uint64_t stats_interrupt_targed;
uint64_t stats_interrupt_fired;
#endif

#ifdef CPU_STATS
double cpu_utilization[MAX_WORKERS];
__thread uint64_t start_runtime;
__thread uint64_t total_app_time;
__thread uint64_t app_start_time;
#endif

static void
dpdk_stats ( uint32_t portid )
{
  struct rte_eth_stats eth_stats;
  int retval = rte_eth_stats_get ( portid, &eth_stats );
  if ( retval != 0 )
    {
      rte_exit ( EXIT_FAILURE, "Unable to get stats from portid\n" );
    }

  INFO ( "DPDK RX Stats:\n"
         "  ipackets: %lu\n"
         "  ibytes: %lu\n"
         "  ierror: %lu\n"
         "  imissed: %lu\n"
         "  rxnombuf: %lu\n",
         eth_stats.ipackets,
         eth_stats.ibytes,
         eth_stats.ierrors,
         eth_stats.imissed,
         eth_stats.rx_nombuf );

  INFO ( "DPDK TX Stats:\n"
         "  opackets: %lu\n"
         "  obytes: %lu\n"
         "  oerror: %lu\n",
         eth_stats.opackets,
         eth_stats.obytes,
         eth_stats.oerrors );

  struct rte_eth_xstat *xstats;
  struct rte_eth_xstat_name *xstats_names;

  int len = rte_eth_xstats_get ( portid, NULL, 0 );
  if ( len < 0 )
    {
      rte_exit (
              EXIT_FAILURE, "rte_eth_xstats_get(%u) failed: %d", portid, len );
    }

  xstats = calloc ( len, sizeof ( *xstats ) );
  if ( xstats == NULL )
    {
      rte_exit ( EXIT_FAILURE, "Failed to calloc memory for xstats" );
    }

  int ret = rte_eth_xstats_get ( portid, xstats, len );
  if ( ret < 0 || ret > len )
    {
      free ( xstats );
      rte_exit ( EXIT_FAILURE,
                 "rte_eth_xstats_get(%u) len%i failed: %d",
                 portid,
                 len,
                 ret );
    }

  xstats_names = calloc ( len, sizeof ( *xstats_names ) );
  if ( xstats_names == NULL )
    {
      free ( xstats );
      rte_exit ( EXIT_FAILURE, "Failed to calloc memory for xstats_names" );
    }

  ret = rte_eth_xstats_get_names ( portid, xstats_names, len );
  if ( ret < 0 || ret > len )
    {
      free ( xstats );
      free ( xstats_names );
      rte_exit ( EXIT_FAILURE,
                 "rte_eth_xstats_get_names(%u) len%i failed: %d",
                 portid,
                 len,
                 ret );
    }

  INFO ( "PORT %u statistics:\n", portid );
  for ( int i = 0; i < len; i++ )
    {
      if ( xstats[i].value > 0 )
        printf ( "  %-18s: %" PRIu64 "\n",
                 xstats_names[i].name,
                 xstats[i].value );
    }

  free ( xstats );
  free ( xstats_names );
}

void
show_stats ( int port_id )
{
  INFO ( "Gerenal stats:\n"
         "  Tot RX count:           %lu\n"
         "  Tot RX dropped:         %lu\n"
         "  Tot Steal:              %lu\n"
         "  Tot starvation:         %lu\n"
         "  Tot interrupts:         %lu\n"
         "  Tot invalid interrupts: %lu\n"
         "  Timer wait queue:       %lu\n"
         "  Timer processed:        %lu\n"
         "  Avg quanta:    %lu ns\n",
         tot_rx_count,
         tot_rx_dropped,
         tot_steal,
         tot_starvation,
         tot_interrupts,
         tot_invalid_interrupts,
         timer_wait_queue_processed,
         timer_processed,
         timer_get_avg_us_quantum () );

  dpdk_stats ( port_id );
}

void
show_stats_per_worker ( void )
{
  tot_rx_count += rx_count;
  tot_rx_dropped += rx_dropped;
  tot_steal += stealing;
  tot_interrupts += interrupts;
  tot_invalid_interrupts += invalid_interrupts;
  tot_starvation += avoid_starvation;

  INFO ( "Worker %u stats:\n"
         " RX count: %lu"
         " RX dropped: %lu"
         " Interruptions: %lu"
         " Invalid interruptions: %lu"
         " yield: %lu"
         " Work stealing: %lu"
         " Starvation: %lu\n"
#ifdef CPU_STATS
         "  CPU: %.2lf%%\n"
#endif
         ,
         worker_id,
         rx_count,
         rx_dropped,
         interrupts,
         invalid_interrupts,
         yield,
         stealing,
         avoid_starvation
#ifdef CPU_STATS
         ,
         cpu_utilization[worker_id]
#endif
  );
}

#ifdef CPU_STATS
void
get_cpu_usage ( double *avg, double *min, double *max )
{
  double sum = 0.0;
  *min = 1.0;
  *max = 0.0;

  for ( int i = 0; i < tot_workers; i++ )
    {
      sum += cpu_utilization[i];
      *min = ( cpu_utilization[i] < *min ) ? cpu_utilization[i] : *min;
      *max = ( cpu_utilization[i] > *max ) ? cpu_utilization[i] : *max;
    }

  *avg = sum / tot_workers;
}

#endif

void
show_stats_timer ( void )
{
#ifdef INT_STATS
  printf ( "Interrupts targed: %lu\n", stats_interrupt_targed );
  printf ( "Interrupts fired: %lu\n", stats_interrupt_fired );
#endif

#ifdef CPU_STATS
  double avg, min, max;
  get_cpu_usage ( &avg, &min, &max );
  printf ( "CPU: %.2lf %.2lf %.2lf\n", avg, min, max );
#endif
}
