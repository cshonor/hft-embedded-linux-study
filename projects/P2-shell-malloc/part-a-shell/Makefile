CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -g
SRCS     = shell.c parser.c executor.c builtin.c
OBJS     = $(SRCS:.c=.o)
TARGET   = myshell

.PHONY: all test run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(TARGET)
	@printf 'pwd\n' | ./$(TARGET) | grep -q / && echo "PASS  pwd"
	@printf 'echo hello-p2\n' | ./$(TARGET) | grep -q hello-p2 && echo "PASS  echo"
	@printf 'echo piped | cat\n' | ./$(TARGET) | grep -q piped && echo "PASS  pipe"
	@rm -f /tmp/p2_redir_out.txt
	@printf 'echo redir-ok > /tmp/p2_redir_out.txt\n' | ./$(TARGET) >/dev/null
	@grep -q redir-ok /tmp/p2_redir_out.txt && echo "PASS  redir"
	@rm -f /tmp/p2_redir_out.txt
	@echo "part-a-shell: OK"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
