#include "tpcc.h"
#include "util.h"
#include <assert.h>
#include <atomic>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>

#define QUANTITY 1000000

int n_threads = 4;
int duration = 1;
int done = 0;

void initCustomer(int warehouse, bplus_tree *table) {
  long i, j, k;
  // warehouse*10*3000 customers
  CustomerRow *cr =
      (CustomerRow *)malloc(sizeof(CustomerRow) * warehouse * 10 * 3000);
  for (i = 0; i < warehouse; i++) {
    for (j = 0; j < 10; j++) {
      for (k = 0; k < 3000; k++) {
        int index = i * 30000 + j * 3000 + k;
        cr[index].C_ID = i;
        cr[index].C_DID = rand() % 10;
        cr[index].C_SINCE = 0;
        bplus_tree_put(table, CUSTOMER_PID(i, j, k), (long)&cr[index]);
      }
    }
  }
}

void initItem(bplus_tree *table) {
  long i;
  // 100k items
  ItemRow *ir = (ItemRow *)malloc(sizeof(ItemRow) * 100000);
  for (i = 0; i < 100000; i++) {
    ir[i].I_ID = i;
    ir[i].I_IM_ID = i;
    ir[i].I_PRICE = rand() % 1000 + 10;
    bplus_tree_put(table, i, (long)&ir[i]);
  }
}

void initStock(int warehouse, bplus_tree *table) {
  long i, j;
  // warehouse * 100000 stock rows
  StockRow *sr = (StockRow *)malloc(sizeof(StockRow) * warehouse * 100000);
  for (i = 0; i < warehouse; i++) {
    for (j = 0; j < 100000; j++) {
      int index = i * 100000 + j;
      sr[index].S_I_ID = i;
      sr[index].S_W_ID = warehouse;
      sr[index].S_QUANTITY = rand() % QUANTITY + QUANTITY;
      sr[index].S_ORDER_CNT = 0;
      sr[index].S_REMOTE_CNT = 0;
      bplus_tree_put(table, STOCK_PID(i, j), (long)&sr[index]);
    }
  }
}

void initWarehouse(int warehouse, bplus_tree *table) {
  long i;
  WarehouseRow *wr = (WarehouseRow *)malloc(sizeof(WarehouseRow) * warehouse);
  for (i = 0; i < warehouse; i++) {
    bplus_tree_put(table, i, (long)&wr[i]);
  }
}

void initDistrict(int warehouse, bplus_tree *table) {
  long i, j;
  DistrictRow *dr = (DistrictRow *)malloc(sizeof(DistrictRow) * 10 * warehouse);
  for (i = 0; i < warehouse; i++) {
    for (j = 0; j < 10; j++) {
      int index = i * 10 + j;
      dr[index].D_NEXT_O_ID = 0;
      bplus_tree_put(table, DISTRICT_PID(i, j), (long)&dr[index]);
    }
  }
}

TpccBenchmarkBPlusTree *init_tpcc_tree() {
  TpccBenchmarkBPlusTree *tpcc =
      (TpccBenchmarkBPlusTree *)malloc(sizeof(TpccBenchmarkBPlusTree));

  tpcc->warehouse_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);
  tpcc->district_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);
  tpcc->customer_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);
  tpcc->order_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);
  tpcc->orderline_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);
  tpcc->item_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);
  tpcc->stock_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);
  tpcc->new_order_table = bplus_tree_init(MAX_LEVEL, MAX_ORDER, MAX_ENTRIES);

  initWarehouse(WAREHOUSE_NUM, tpcc->warehouse_table);
  printf(".. init warehouse %d\n", WAREHOUSE_NUM);
  initDistrict(WAREHOUSE_NUM, tpcc->district_table);
  printf(".. init district %d\n", WAREHOUSE_NUM * 10);
  initItem(tpcc->item_table);
  printf(".. init item: %d\n", 100000);
  initStock(WAREHOUSE_NUM, tpcc->stock_table);
  printf(".. init stock %d\n", (WAREHOUSE_NUM * 100000));
  initCustomer(WAREHOUSE_NUM, tpcc->customer_table);
  printf(".. init customer %d\n", (WAREHOUSE_NUM * 10 * 3000));
  printf(".. tpcc init finished\n");

  return tpcc;
}

/////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
struct testpara {
  int id;
  barrier_t *barrier;
  TpccBenchmarkBPlusTree *tpcc;
  long ops;
};

bool NewOrder(long wid, TpccBenchmarkBPlusTree *tpcc, unsigned short seed[3],
              int orders) {
  char pad_begin[32];
  pad_begin[0] = 'a';
  int dcids[8];
  dcids[0] = (int)(erand48(seed) * 10);
  dcids[1] = (int)(erand48(seed) * 3000);
  dcids[2] = (int)(erand48(seed) * 100);

  int item_nums[16];
  int item_ids[16];

  for (long i = 0; i < orders; i++) {
    item_nums[i] = (int)(erand48(seed) * 10) + 1;
    item_ids[i] =
        (int)(erand48(seed) * ITEM_NUM / orders) + i * ITEM_NUM / orders;
  }

  TX_BEGIN {
    // read all those variables to transaction-local
    long w = wid;
    long c = dcids[1];
    long d = dcids[0];

    DistrictRow *dr = (DistrictRow *)bplus_tree_get(tpcc->district_table,
                                                    (long)(DISTRICT_PID(w, d)));
    long next_order_id = dr->D_NEXT_O_ID;

    dr->D_NEXT_O_ID = next_order_id + 1;
    NewOrderRow *norow = (NewOrderRow *)aligned_alloc(32, sizeof(NewOrderRow));
    norow->NO_O_ID = next_order_id;
    norow->NO_D_ID = d;
    norow->NO_W_ID = w;

    OrderRow *orow = (OrderRow *)aligned_alloc(32, sizeof(OrderRow));
    orow->O_ID = next_order_id;
    orow->O_D_ID = d;
    orow->O_W_ID = w;
    orow->O_C_ID = c;
    orow->O_OL_CNT = orders;
    orow->O_ALL_LOCAL = 1;

    long key = ORDER_PID(w, d, next_order_id);
    bplus_tree_put(tpcc->new_order_table, (long)key,
                   (long)norow);                              // NewOrder Table
    bplus_tree_put(tpcc->order_table, (long)key, (long)orow); // Order Table
    // Order Line
    for (long i = 0; i < orders; i++) {
      long num = item_nums[i];
      long itemid = item_ids[i];
      StockRow *sr =
          (StockRow *)bplus_tree_get(tpcc->stock_table, STOCK_PID(w, itemid));
      if (sr == NULL || sr->S_QUANTITY < num) {
        dcids[2] = 0;
        return; // get out of the transaction
      }

      sr->S_QUANTITY = sr->S_QUANTITY - num; // update stock
      sr->S_ORDER_CNT = sr->S_ORDER_CNT + 1;
      sr->S_REMOTE_CNT = sr->S_REMOTE_CNT + 1;
      ItemRow *ir = (ItemRow *)bplus_tree_get(tpcc->item_table, (long)itemid);
      OrderLineRow *olr =
          (OrderLineRow *)aligned_alloc(32, sizeof(OrderLineRow));
      olr->OL_O_ID = next_order_id;
      olr->OL_D_ID = d;
      olr->OL_W_ID = w;
      olr->OL_NUMBER = i;
      olr->OL_I_ID = itemid;
      olr->OL_SUPPLY_W_ID = 0; // TODO: more than one warehouse
      olr->OL_QUANTITY = num;
      olr->OL_AMOUNT = num * ir->I_PRICE;

      key = ORDER_LINE_PID(w, d, next_order_id, i);
      if (bplus_tree_put(tpcc->orderline_table, (long)key, (long)olr) < 0) {
        dcids[2] = -1;
        return;
      }
    }
  }
  TX_END;
  if (dcids[2] < 0) {
    printf("[ PANIC ] insert failed\n");
  }
  return dcids[2] > 0;
}

void *test(void *data) {
  struct testpara *para = (struct testpara *)(data);
  unsigned short seed[3];
  unsigned int d = rand();

  /* Initialize seed (use rand48 as rand is poor) */
  seed[0] = (unsigned short)rand_r(&d);
  seed[1] = (unsigned short)rand_r(&d);
  seed[2] = (unsigned short)rand_r(&d);
  int s = 0;
  barrier_cross(para->barrier);

  unsigned long times[10000];
  unsigned long long start, end;
  while (done == 0) {
    /* tiz214, we can adjust the size of tx here */
    int orders = (int)(erand48(seed) * 3) + 2;
    if (s % 100 == 1) {
      start = asm_rdtsc();
    }
    if (NewOrder(0, para->tpcc, seed, orders))
      para->ops++;
    if (s++ % 100 == 1 && s < 1000000) {
      end = asm_rdtsc();
      times[s / 100] = end - start;
    }
  }
  int size = (s < 1000000) ? s / 100 : 10000;
  for (int i = 0; i < size; i++) {
    for (int j = i + 1; j < size; j++) {
      if (times[j] < times[i]) {
        float tmp = times[i];
        times[i] = times[j];
        times[j] = tmp;
      }
    }
  }

  double a[] = {0.5, 0.6, 0.7, 0.8, 0.9, 0.99};
  for (int i = 0; i < 6; i++) {
    printf("%ld ", times[int(size * a[i])] / 2660);
  }
  printf("\n");

  return NULL;
}

int main(int argc, char **argv) {
  int i, c;
  while (1) {
    i = 0;
    c = getopt_long(argc, argv, "d:t:n:", NULL, &i);

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
    default:
      exit(0);
    }
  }

  TpccBenchmarkBPlusTree *tpcc = init_tpcc_tree();

  barrier_t barrier;
  barrier_init(&barrier, n_threads + 1);

  struct testpara *data =
      (struct testpara *)malloc(sizeof(struct testpara) * n_threads);
  pthread_t *pid = (pthread_t *)malloc(sizeof(pthread_t) * n_threads);

  for (i = 0; i < n_threads; i++) {
    data[i].tpcc = tpcc;
    data[i].id = i;
    data[i].barrier = &barrier;
    data[i].ops = 0;
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
  for (i = 0; i < n_threads; i++)
    ops += data[i].ops;

  printf("%ld txs throughput: %lf ktps for %lf s\n", ops,
         ops / (end - start) / 1000, end - start);
  return 0;
}
