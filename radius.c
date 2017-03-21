/* STD INCLUDE */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/sysctl.h>

/* Network include */

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <radlib.h>
#include <radlib_vs.h>

#include "ipoed.h"
#include "radius.h"

