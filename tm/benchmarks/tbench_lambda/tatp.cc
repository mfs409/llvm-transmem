#include <getopt.h>
#include <stdio.h>
#include <unistd.h>

#include "tatp.h"
#include "util.h"

int n_threads = 4;
int opssize = 100000;
int done = 0;
int duration = 5;

void UpdateLocation(struct TatpBenchmark *tatp, long s_id, long n_loc) {
  TX_BEGIN {
    Subscriber *sr =
        (Subscriber *)hash_table_read(tatp->subscriber_table, s_id);
    sr->vlr_location = n_loc;
  }
  TX_END;
}

void initSubscriber(struct Hashtable *table, int population) {
  int i;
  Subscriber *sr = (Subscriber *)malloc(sizeof(Subscriber) * population);
  for (i = 0; i < population; i++) {
    sr[i].s_id = i;
    sr[i].vlr_location = rand();
    hash_table_put(table, i, (long)&sr[i]);
  }
}

struct TatpBenchmark *InitTatp(int population) {
  TatpBenchmark *tatp =
      (struct TatpBenchmark *)malloc(sizeof(struct TatpBenchmark));
  tatp->subscriber_table = hash_table_init(10 * population);
  tatp->population = population;
  initSubscriber(tatp->subscriber_table, population);
  return tatp;
}

struct testpara {
  long id;
  long ops;
  struct TatpBenchmark *tatp;
  barrier_t *barrier;
};

void *test(void *para) {
  unsigned short seed[3];
  unsigned int s = rand();
  /* Initialize seed (use rand48 as rand is poor) */
  seed[0] = (unsigned short)rand_r(&s);
  seed[1] = (unsigned short)rand_r(&s);
  seed[2] = (unsigned short)rand_r(&s);

  struct testpara *data = (struct testpara *)para;
  long i = 0;
  barrier_cross(data->barrier);

  while (done == 0) {
    long s_id = (int)(erand48(seed) * data->tatp->population);
    long n_loc = (int)(erand48(seed) * 0x7fffffff);
    UpdateLocation(data->tatp, s_id, n_loc);
    i++;
  }
  data->ops = i;
  return NULL;
}

int main(int argc, char **argv) {
  int i, c;
  while (1) {
    i = 0;
    c = getopt_long(argc, argv, "n:t:d:", NULL, &i);
    if (c == -1)
      break;

    switch (c) {
    case 0:
      break;
    case 'd':
      duration = atoi(optarg);
      break;
    case 't':
      n_threads = atoi(optarg);
      break;
    case 'n':
      opssize = atoi(optarg);
      break;
    default:
      exit(0);
    }
  }

  struct TatpBenchmark *tatp = InitTatp(opssize);
  pthread_t *pid = (pthread_t *)malloc(sizeof(pthread_t) * n_threads);
  struct testpara *data =
      (struct testpara *)malloc(sizeof(struct testpara) * n_threads);

  barrier_t barrier;
  barrier_init(&barrier, n_threads + 1);

  for (i = 0; i < n_threads; i++) {
    data[i].id = i;
    data[i].ops = 0;
    data[i].tatp = tatp;
    data[i].barrier = &barrier;
    pthread_create(&pid[i], NULL, test, &data[i]);
  }

  printf("START..\n");
  double start = get_wall_time();
  barrier_cross(&barrier);
  sleep(duration);
  done = 1;
  printf("STOPPING..\n");

  for (i = 0; i < n_threads; i++) {
    pthread_join(pid[i], NULL);
  }
  double end = get_wall_time();

  long ops = 0;
  for (i = 0; i < n_threads; i++) {
    ops += data[i].ops;
  }

  double duration = end - start;
  printf("txs: %ld, throughtput: %lf KTps, duration %lf\n", ops,
         ops / duration / 1000, duration);

  return 0;
}
