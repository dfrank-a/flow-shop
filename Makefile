CC=gcc
SOURCES= main.c dominance.c optimizer.c schedule.c metaheuristics.c job.c schedule_pool.c
OBJS=$(SOURCES:%.c=%.o)
OUTP=flowshop

CFLAGS := -O2 -Wall $(CFLAGS) 
BUILDDIR := build/$(MAKECMDGOALS)
TARGET := $(addprefix $(BUILDDIR)/,$(OBJS))
EXEC   := $(addprefix $(BUILDDIR)/,$(OUTP))

all: $(TARGET)
	@echo $(CFLAGS)
	$(CC) -o $(EXEC) $(TARGET) -lrt -lm

clean:
	rm -rf *.o $(OUTP)

$(BUILDDIR)/%.o: %.c 
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

