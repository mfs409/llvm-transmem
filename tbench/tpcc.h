#ifndef TPCC
#define TPCC

#include <stdlib.h>
#include <stdio.h>
#include "hashtable.h"
#include "bplustree.h"

#define WAREHOUSE_NUM 1
#define MAX_WAREHOUSE_NUM 1024
#define CUSTOMER_NUM 3000
#define ITEM_NUM 100000

#define CUSTOMER_PID(wid, did, cid)   ((wid << 16) | (did << 12) | cid)
#define DISTRICT_PID(wid, did)        ((wid << 4) | did)
#define ORDER_PID(wid, did, oid)      ((oid << 14) | (did << 10)| wid) 
#define ORDER_LINE_PID(wid, did, oid, num)   ( (oid << 18) | (did << 14) | (wid << 4) | num)
#define STOCK_PID(wid, iid)   		  ((iid << 10) | wid)

struct DistrictRow{
	long D_NEXT_O_ID;
	char D_NAME[10];
	char D_STREET1[20];
	char D_STREET2[20];
	char D_CITY[20];
	char D_STATE[2];
	char D_ZIP[9];
	double  D_TAX;
	double  D_YTD;
}__attribute__((aligned(32)));

struct WarehouseRow{
	char W_NAME[10];
	char W_STREET1[20];
	char W_STREET2[20];
	char W_CITY[20];
	char W_STATE[2];
	char W_ZIP[9];
	double  W_TAX;
	double  W_YTD;   // year to date balance
}__attribute__((aligned(32)));

// Primary: customer id
struct CustomerRow{
 	long  C_ID;
 	long  C_DID;
 	char C_FIRST[16];
 	char C_MIDDLE[2];
 	char C_LAST[16];
 	char C_STREET1[20];
 	char C_STREET2[20];
 	char C_CITY[20];
 	char C_STATE[2];
 	char C_ZIP[9];
 	char C_PHONE[16];
 	long C_SINCE;
 	char C_CREDIT[2];
 	double  C_CREDIT_LIM;
 	double  C_DISCOUNT;
 	double  C_BALANCE;
 	double  C_YTD_PAYMENT;
 	long  C_PAYMENT_CNT;
 	long  C_DELIVERY_CNT;
 	char  C_DATA[100];
}__attribute__((aligned(32)));

struct HistoryRow{
	long  H_C_ID; // custom id
	long  H_C_D_ID;
	long  H_C_W_ID;
	long  H_D_ID;
	long  H_W_ID;
	long  H_DATE;
	long  H_AMOUNT;
	char H_DATA[24];
}__attribute__((aligned(32)));

// primary: order id
struct NewOrderRow{
	long  NO_O_ID;
	long  NO_D_ID;
	long  NO_W_ID;
}__attribute__((aligned(32)));

// Primary: order id
struct OrderRow{
	long  O_ID;
	long  O_D_ID;
	long  O_W_ID;
	long  O_C_ID;
	long  O_ENTRY_D;
	long  O_CARRIER_ID;
	long  O_OL_CNT;
	long  O_ALL_LOCAL;
}__attribute__((aligned(32)));
 
// Primary: order id + order line number 
struct OrderLineRow{
	long OL_O_ID;
	long OL_D_ID;
	long OL_W_ID;
	long OL_NUMBER;
	long OL_I_ID;     //item id;
	long OL_SUPPLY_W_ID; 
	long OL_DELIVERY_D;
	long OL_QUANTITY;    
	long OL_AMOUNT;
	long OL_DIST_INFO[24];
}__attribute__((aligned(32)));

// Primary: item id
struct ItemRow{
	long I_ID;
	long I_IM_ID;  // image id associated to item
	char I_NAME[24];
	long I_PRICE;
	char I_DATA[50]; // brand information
}__attribute__((aligned(32)));

// Primary: item id
struct StockRow {
	long S_I_ID; // item id;
	long S_W_ID;  // warehouse id, here only one warehouse
	long S_QUANTITY;	
	char S_DIST_01[24];
	char S_DIST_02[24];
	char S_DIST_03[24];
	char S_DIST_04[24];
	char S_DIST_05[24];
	char S_DIST_06[24];
	char S_DIST_07[24];
	char S_DIST_08[24];
	char S_DIST_09[24];
	char S_DIST_10[24];

	long S_YTD;  
	long S_ORDER_CNT;
	long S_REMOTE_CNT;
	char S_DATA[50];
}__attribute__((aligned(32)));

struct TpccBenchmark{
	struct Hashtable*    warehouse_table;  // warehouse + 10 distinct
	struct Hashtable*    district_table;
	struct Hashtable*    customer_table;
	struct Hashtable*    order_table;
	struct Hashtable*    orderline_table;
	struct Hashtable*    item_table;
	struct Hashtable*    stock_table;
	struct Hashtable*    new_order_table;
}__attribute__((aligned(32)));

struct TpccBenchmarkBPlusTree{
        bplus_tree*    warehouse_table;  // warehouse + 10 distinct
        bplus_tree*    district_table;
        bplus_tree*    customer_table;
        bplus_tree*    order_table;
        bplus_tree*    orderline_table;
        bplus_tree*    item_table;
        bplus_tree*    stock_table;
        bplus_tree*    new_order_table;
}__attribute__((aligned(32)));

struct TpccBenchmark* init_tpcc();
struct TpccBenchmarkBPlusTree* init_tpcc_tree();
#endif
