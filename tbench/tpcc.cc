#include "tpcc.h"
#include "util.h"
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>
#include <unistd.h>
#include <vector>
#include <atomic>

#define QUANTITY 1000000

int n_threads = 4;
int duration = 1;
int done = 0;

void initCustomer(int warehouse, struct Hashtable* table){
  long i, j, k;
  // warehouse*10*3000 customers
  CustomerRow* cr = (CustomerRow*)malloc(sizeof(CustomerRow)*warehouse*10*3000);
  for(i = 0; i < warehouse; i++){
    for(j = 0; j < 10; j++){
      for(k = 0; k < 3000; k++){
        int index = i * 30000 + j * 3000 + k;
	cr[index].C_ID = i;
	cr[index].C_DID = rand()%10;
	cr[index].C_SINCE = 0;
 	hash_table_put(table, CUSTOMER_PID(i, j, k), (long)&cr[index]);
      }
    }
  }
}

void initItem(struct Hashtable* table){
  long i;
  // 100k items
  ItemRow* ir = (ItemRow*)malloc(sizeof(ItemRow) * 100000);
  for(i = 0; i < 100000; i++){
    ir[i].I_ID = i;ir[i].I_IM_ID = i;
    ir[i].I_PRICE = rand()%1000 + 10;
    hash_table_put(table, i, (long)&ir[i]);
  }
}

void initStock(int warehouse, struct Hashtable* table){
  long i, j;
  // warehouse * 100000 stock rows
  StockRow* sr = (StockRow*)malloc(sizeof(StockRow) * warehouse * 100000);
  for(i = 0; i < warehouse; i++){
    for(j = 0; j < 100000; j++){
      int index = i*100000 + j;
      sr[index].S_I_ID = i;
      sr[index].S_W_ID = warehouse;
      sr[index].S_QUANTITY = rand()%QUANTITY + QUANTITY;
      sr[index].S_ORDER_CNT = 0;
      sr[index].S_REMOTE_CNT = 0;
      hash_table_put(table, STOCK_PID(i, j), (long)&sr[index]);
    }
  }
}

void initWarehouse(int warehouse, struct Hashtable* table){
  long i;
  WarehouseRow* wr = (WarehouseRow*)malloc(sizeof(WarehouseRow) * warehouse);
  for(i = 0; i < warehouse; i++){
    hash_table_put(table, i, (long)&wr[i]);
  }
}

void initDistrict(int warehouse, struct Hashtable* table){
  long i, j;
  DistrictRow* dr = (DistrictRow*)malloc(sizeof(DistrictRow) * 10 * warehouse);
  for(i = 0; i < warehouse; i++){
    for(j = 0; j < 10; j++){
      int index = i*10 + j;
      dr[index].D_NEXT_O_ID = 0;
      hash_table_put(table, DISTRICT_PID(i, j), (long)&dr[index]);
    }
  }
}

TpccBenchmark* init_tpcc(){
  TpccBenchmark* tpcc = (TpccBenchmark*) malloc(sizeof(TpccBenchmark));

  // prime number to scatter keys to different buckets
  tpcc->warehouse_table = hash_table_init(29);
  tpcc->district_table = hash_table_init(541);
  tpcc->customer_table = hash_table_init(104729);
  tpcc->order_table = hash_table_init(1299709);
  tpcc->orderline_table = hash_table_init(6185863);
  tpcc->item_table = hash_table_init(1299709);
  tpcc->stock_table = hash_table_init(104729);
  tpcc->new_order_table = hash_table_init(1299709);

  initWarehouse(WAREHOUSE_NUM, tpcc->warehouse_table);
  initDistrict(WAREHOUSE_NUM, tpcc->district_table);
  initItem(tpcc->item_table);
  initStock(WAREHOUSE_NUM, tpcc->stock_table);
  initCustomer(WAREHOUSE_NUM, tpcc->customer_table);
  //printf(".. tpcc init finished\n");
  return tpcc;
}

/////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
struct testpara{
    int id;
    barrier_t *barrier;
    TpccBenchmark* tpcc;
    long ops;
};

bool NewOrder(long wid, TpccBenchmark* tpcc, unsigned short seed[3], int orders){
  char pad_begin[32]; pad_begin[0] = 'a';
  int dcids[8];
  dcids[0] = (int)(erand48(seed) * 10);
  dcids[1] = (int)(erand48(seed) * 3000);
  dcids[2] = (int)(erand48(seed) * 100);
  
  int item_nums[16];
  int item_ids[16];
  
  for(long i = 0; i < orders; i++){
    item_nums[i] = (int)(erand48(seed) * 10) + 1;
    item_ids[i] = (int)(erand48(seed) * ITEM_NUM / orders) + i * ITEM_NUM / orders;
  }
  
  TX_BEGIN {
    // read all those variables to transaction-local
    long w = wid;
    long c = dcids[1];
    long d = dcids[0];

    DistrictRow* dr = (DistrictRow *)hash_table_read(tpcc->district_table, (uint64_t)DISTRICT_PID(w, d));
    long next_order_id = dr->D_NEXT_O_ID;

    dr->D_NEXT_O_ID = next_order_id + 1;
    NewOrderRow* norow = (NewOrderRow *)aligned_alloc(32, sizeof(NewOrderRow));
    norow->NO_O_ID = next_order_id;
    norow->NO_D_ID = d;
    norow->NO_W_ID = w;
 
    OrderRow* orow = (OrderRow *)aligned_alloc(32, sizeof(OrderRow));
    orow->O_ID = next_order_id;
    orow->O_D_ID = d;
    orow->O_W_ID = w;
    orow->O_C_ID = c;
    orow->O_OL_CNT = orders;
    orow->O_ALL_LOCAL = 1;

    long key = ORDER_PID(w, d, next_order_id);
    hash_table_put(tpcc->new_order_table, key, (long)norow);   // NewOrder Table
    hash_table_put(tpcc->order_table, key, (long)orow);        // Order Table
    // Order Line
    for(long i = 0; i < orders; i++){
      long num = item_nums[i];
      long itemid = item_ids[i];
      StockRow* sr = (StockRow*)hash_table_read(tpcc->stock_table, STOCK_PID(w, itemid));
      if (sr == NULL || sr->S_QUANTITY < num){
	dcids[2] = 0;
        return; // get out of the transaction
      }
                      
      sr->S_QUANTITY = sr->S_QUANTITY - num; // update stock
      sr->S_ORDER_CNT = sr->S_ORDER_CNT + 1;
      sr->S_REMOTE_CNT = sr->S_REMOTE_CNT + 1;
      ItemRow* ir = (ItemRow*)hash_table_read(tpcc->item_table, (long)itemid);
      OrderLineRow* olr = (OrderLineRow*)aligned_alloc(32, sizeof(OrderLineRow));
      olr->OL_O_ID = next_order_id;
      olr->OL_D_ID = d;
      olr->OL_W_ID = w;
      olr->OL_NUMBER = i;
      olr->OL_I_ID = itemid;
      olr->OL_SUPPLY_W_ID = 0;    // TODO: more than one warehouse
      olr->OL_QUANTITY = num;
      olr->OL_AMOUNT = num * ir->I_PRICE;

      key = ORDER_LINE_PID(w, d, next_order_id, i);
      if (hash_table_put(tpcc->orderline_table, key, (long)olr) < 0) {
	dcids[2] = -1;
	return;
      }
    }
  } TX_END;
  if (dcids[2] < 0) {
    /*printf("[ PANIC ] exceed table size, please reduce -d (runtime) \n");*/
  }
  return dcids[2] > 0;
}

void* test(void* data){

  struct testpara* para = (struct testpara*)(data);
  unsigned short seed[3];
  unsigned int d = rand();

  /* Initialize seed (use rand48 as rand is poor) */
  seed[0] = (unsigned short)rand_r(&d);
  seed[1] = (unsigned short)rand_r(&d);
  seed[2] = (unsigned short)rand_r(&d);
  int s = 0;
  barrier_cross(para->barrier);

  unsigned long times[10000];
  unsigned long long start , end;
  while(done == 0){
    /* tiz214, we can adjust the size of tx here */
    int orders = (int)(erand48(seed) * 3)  + 2;
    if (s % 100 == 1){
      start = asm_rdtsc();
    }
    if (NewOrder(0, para->tpcc, seed, orders))
      para->ops++;
      if (s++ % 100 == 1 && s<1000000){
        end = asm_rdtsc();
	times[s/100]= end - start;
      }
    }
 /*   int size = (s<1000000)?s/100:10000;
    for(int i=0; i<size; i++){
      for(int j=i+1; j<size; j++){
    	if (times[j] < times[i]){
    	  float tmp = times[i];
    	  times[i] = times[j];
    	  times[j] = tmp;
    	}
      }
    }

    double a[] = {0.5, 0.6, 0.7, 0.8, 0.9, 0.99};
    for(int i = 0; i < 6; i++){
    	printf("%ld ", times[int(size*a[i])]/2660);
    }
    printf("\n");
 */
    return NULL;
}

int main(int argc, char** argv){
  int i,c;
  while(1){
    i = 0;
    c = getopt_long(argc, argv, "d:t:n:", NULL, &i);

    if(c == -1)
      break;

    switch(c){
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
	
  struct TpccBenchmark* tpcc = init_tpcc();
  barrier_t barrier;
  barrier_init(&barrier, n_threads+1);

  struct testpara* data =(struct testpara*) malloc(sizeof(struct testpara)*n_threads);
  pthread_t* pid = (pthread_t*)malloc(sizeof(pthread_t)*n_threads);
    
  for(i = 0; i < n_threads; i++){
    data[i].tpcc = tpcc; 
    data[i].id = i;
    data[i].barrier = &barrier;
    data[i].ops = 0;
    pthread_create(&pid[i], NULL, test, &data[i]);
  }

  //printf("START..\n");
  double start = get_wall_time();
  barrier_cross(&barrier);
  sleep(duration);
  done = 1;
  for(i=0; i<n_threads; i++){
    pthread_join(pid[i], NULL);
  }
  double end = get_wall_time();
  //printf("STOPPING..\n");
  long ops = 0;
  for(i=0; i<n_threads; i++)
    ops += data[i].ops;
  printf("%lf", ops/(end-start)/1000);
  //printf("%ld txs throughput: %lf ktps for %lf s\n", ops, ops/(end-start)/1000, end-start);
  return 0;	
}
