CFLAGS+=--std=c99 -Wall -Werror -O2 -g
CFLAGS+= -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include/
LDFLAGS+= -lpcap -lglib-2.0

ESTIMATOR_CFLAGS=-funit-at-a-time

TARGETS=estimator
SOURCES= \
		dispatch.c \
		estimator.c \
		main.c \
		network_rx.c \
		normalized_rxtx.c \
		openbeacon.c \
		pcap_rx.c \
		readerloc.c \
		util.c \


DEP_SUFFIX=dep
DEPCHECK_TARGET=.depcheck
all: $(TARGETS)

estimator: $(SOURCES) $(DEPCHECK_TARGET)
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
-include $(SOURCES:%.c=%.$(DEP_SUFFIX))
endif

$(DEPCHECK_TARGET):
	touch $@
