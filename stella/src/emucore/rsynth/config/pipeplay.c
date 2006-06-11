#include <config.h>
#include <useconfig.h>
#include <stdio.h>
#include "getargs.h"
#include "hplay.h"
#include <fcntl.h>

static FILE *dev_fd;
long samp_rate = 8000;

int
audio_init(int argc, char **argv)
{
 dev_fd = popen("/usr/audio/bin/send_sound -l16", "w");
 return (0);
}

void
audio_term(void)
{
 pclose(dev_fd);
}

void
audio_play(int n, short *data)
{
 fwrite(data, 2, n, dev_fd);
 fflush(dev_fd);
}
