#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

int main(int argc, const char* argv[])
{
     while ( 1 )
     {
          sleep(3);
          time_t s = time(NULL);
          printf("%d\n",s);
     }
     return 0;
}
