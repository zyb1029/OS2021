#include <game.h>

#define FPS 30

struct object obj;
struct baffle player1, player2;

int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args); // make run mainargs=xxx
  puts("\"\n");

  puts("Type 'ESC' to exit\n");
  splash();
  init_location();

	
  int next_frame = 0, t = 0, speed = 3;
  while (1) {
	while(io_read(AM_TIMER_UPTIME).us / 1000 < next_frame);
    next_frame += 1000 / FPS;
	test_hit();
	t++;
	if (t >= 30 && t % speed == 0)update_obj();
	int key = 0;
	while ((key = read_key()) != AM_KEY_NONE) {
		if (key == AM_KEY_ESCAPE) halt(0);
		if (key == AM_KEY_HOME)     update_player2(-1);
		else if (key == AM_KEY_END) update_player2(1);
		else if (key == AM_KEY_A)   update_player1(-1);
		else if (key == AM_KEY_D)   update_player1(1);
		else if (key == AM_KEY_P && speed > 1) speed -= 1;
		else if (key == AM_KEY_L) speed += 1;
		else if (key == AM_KEY_M) {
			speed = 3, t = 0;
		    splash(), init_location();
		}
	}
	update_screen();
	test_bound();
  }
  return 0;
}
