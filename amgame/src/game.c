#include <game.h>

#define FPS 30

struct object obj;
struct baffle player1, player2;

int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  splash();
  init_location();
  puts("Type 'ESC' to exit\n");

 // int next_frame = 0;

  while (1) {
    int frames = io_read(AM_TIMER_UPTIME).us;
	printf("%d\n", frames);

//	while(current < frames) current++;

//	update_obj();
	
	print_key();
  }
  return 0;
}
