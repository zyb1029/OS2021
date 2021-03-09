#include <stdio.h>
#include <dirent.h>
#include <assert.h>

char path[256] = {"/proc/"};

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  DIR *dir = opendir(path);
  assert(dir != NULL);
  struct dirent *ptr;
  while ((ptr = readdir(dir)) != NULL) {
	printf("d_name: %s\n", ptr->d_name);
  }
  return 0;
}
