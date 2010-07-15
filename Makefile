CFLAGS+=--std=c99 -Wall -Werror -O2 -g
CFLAGS+= -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include/
LDFLAGS+= -lpcap -lglib-2.0

ESTIMATOR_CFLAGS=-funit-at-a-time

TARGETS=estimator spaceparttree_test

COMMON_SOURCES= \
		dispatch.c \
		network_rx.c \
		normalized_rxtx.c \
		pcap_rx.c \
		util.c \

ESTIMATOR_SOURCES= \
		estimator.c \
		openbeacon.c \
		readerloc.c \

SPACEPARTTREE_TEST_SOURCES= \
		spaceparttree.c \
		spaceparttree_test.c \

DEP_SUFFIX=dep
DEPCHECK_TARGET=.depcheck
all: $(TARGETS)

estimator: $(COMMON_SOURCES) $(ESTIMATOR_SOURCES) $(DEPCHECK_TARGET)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(ESTIMATOR_CFLAGS) -o $@ \
	$(filter-out $(DEPCHECK_TARGET),$^)

spaceparttree_test: $(COMMON_SOURCES) \
					$(SPACEPARTTREE_TEST_SOURCES) \
					$(DEPCHECK_TARGET)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(ESTIMATOR_CFLAGS) -o $@ \
	$(filter-out $(DEPCHECK_TARGET),$^)

.PHONY: clean
clean:
	rm -f *.o
	rm -f *.$(DEP_SUFFIX) $(DEPCHECK_TARGET)
	rm -f $(TARGETS)

%.$(DEP_SUFFIX): %.c
	$(CC) $(CFLAGS) $(INCLUDES) -M -MP -MF $@ -MT $(DEPCHECK_TARGET) $<

ifeq (0,$(words $(filter %clean,$(MAKECMDGOALS))))
DO_INCLUDE_DEPS=1
endif

ifeq (1,$(DO_INCLUDE_DEPS))
-include $(COMMON_SOURCES:%.c=%.$(DEP_SUFFIX))
-include $(ESTIMATOR_SOURCES:%.c=%.$(DEP_SUFFIX))
endif

$(DEPCHECK_TARGET):
	touch $@
