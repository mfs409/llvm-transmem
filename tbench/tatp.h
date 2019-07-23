#ifndef TATP_H
#define TATP_H

#include "hashtable.h"
struct Subscriber{
  long s_id;
  long sub_nbr[2];  // char[15]
  long bit_x;   // bool[10]   0-9 bit is used;
  long hex_x;   	  // bit[10][4]
  long byte2_x[2];   // char[10]
  long msc_location;  //  int 
  long vlr_location;  //  int
};

struct TatpBenchmark{
  struct Hashtable* subscriber_table;
  long population;
};

void initSubscriber(struct Hashtable*, int);
void UpdateLocation(struct TatpBenchmark*, long, long);
struct TatpBenchmark* InitTatp(int);
#endif
