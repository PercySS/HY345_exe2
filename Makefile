CC =gcc
CFLAGS = -Wall -pthread -ansi -pedantic -g

SRCS = exe2.c
OBJS = $(SRCS:.c=.o)
EXECUTABLE = study_hall_simulation

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXECUTABLE)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXECUTABLE)
