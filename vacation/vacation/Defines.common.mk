# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================
# Copyright (c) IBM Corp. 2014, and others.


hostname := $(shell hostname)

CFLAGS += -DUSE_TLH
CFLAGS += -DLIST_NO_DUPLICATES

ifeq ($(enable_IBM_optimizations),yes)
CFLAGS += -DMAP_USE_CONCUREENT_HASHTABLE -DHASHTABLE_SIZE_FIELD -DHASHTABLE_RESIZABLE
else
CFLAGS += -DMAP_USE_RBTREE
endif

PROG := vacation

SRCS += \
	client.c \
	customer.c \
	manager.c \
	reservation.c \
	vacation.c \
	$(LIB)/list.c \
	$(LIB)/pair.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/rbtree.c \
	$(LIB)/hashtable.c \
	$(LIB)/conc_hashtable.c \
	$(LIB)/thread.c \
	$(LIB)/memory.c
#
OBJS := ${SRCS:.c=.o}

#RUNPARAMSLOW := -n2 -q90 -u98 -r16384 -t4096
RUNPARAMSLOW := -n2 -q90 -u98 -r1048576 -t4194304
#RUNPARAMSHIGH := -n4 -q60 -u90 -r16384 -t4096
RUNPARAMSHIGH := -n4 -q60 -u90 -r1048576 -t4194304

.PHONY:	runlow1 runlow2 runlow4 runlow6 runlow8 runlow12 runlow16 runlow32 runlow64 runlow128 runlow-16 runlow-32 runlow-64

runlow1:
	$(PROGRAM) $(RUNPARAMSLOW) -c1

runlow2:
	$(PROGRAM) $(RUNPARAMSLOW) -c2

runlow4:
	$(PROGRAM) $(RUNPARAMSLOW) -c4

runlow6:
	$(PROGRAM) $(RUNPARAMSLOW) -c6

runlow8:
	$(PROGRAM) $(RUNPARAMSLOW) -c8

runlow12:
	$(PROGRAM) $(RUNPARAMSLOW) -c12

runlow16:
	$(PROGRAM) $(RUNPARAMSLOW) -c16

runlow32:
	$(PROGRAM) $(RUNPARAMSLOW) -c32

runlow64:
	$(PROGRAM) $(RUNPARAMSLOW) -c64

runlow128:
	$(PROGRAM) $(RUNPARAMSLOW) -c128

runlow-16:
	$(PROGRAM) $(RUNPARAMSLOW) -c-16

runlow-32:
	$(PROGRAM) $(RUNPARAMSLOW) -c-32

runlow-64:
	$(PROGRAM) $(RUNPARAMSLOW) -c-64


.PHONY:	runhigh1 runhigh2 runhigh4 runhigh6 runhigh8 runhigh12 runhigh16 runhigh32 runhigh64 runhigh128 runhigh-16 runhigh-32 runhigh-64

runhigh1:
	$(PROGRAM) $(RUNPARAMSHIGH) -c1

runhigh2:
	$(PROGRAM) $(RUNPARAMSHIGH) -c2

runhigh4:
	$(PROGRAM) $(RUNPARAMSHIGH) -c4

runhigh6:
	$(PROGRAM) $(RUNPARAMSHIGH) -c6

runhigh8:
	$(PROGRAM) $(RUNPARAMSHIGH) -c8

runhigh12:
	$(PROGRAM) $(RUNPARAMSHIGH) -c12

runhigh16:
	$(PROGRAM) $(RUNPARAMSHIGH) -c16

runhigh32:
	$(PROGRAM) $(RUNPARAMSHIGH) -c32

runhigh64:
	$(PROGRAM) $(RUNPARAMSHIGH) -c64

runhigh128:
	$(PROGRAM) $(RUNPARAMSHIGH) -c128

runhigh-16:
	$(PROGRAM) $(RUNPARAMSHIGH) -c-16

runhigh-32:
	$(PROGRAM) $(RUNPARAMSHIGH) -c-32

runhigh-64:
	$(PROGRAM) $(RUNPARAMSHIGH) -c-64


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
