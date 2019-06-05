#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <event.h>
#include <time.h>

void do_timeout(evutil_socket_t fd, short event, void *arg)
{
    struct event_base *base = (struct event_base *)arg;
    printf("do timeout (time: %ld)!\n", time(NULL));
}

struct event *create_timeout_event(struct event_base *base)
{
    struct event *ev;
    struct timeval timeout;

    //ev = evtimer_new(base, do_timeout, NULL);

    ev = event_new(base, -1, EV_READ, do_timeout, base);
    if (ev) {
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        event_add(ev, &timeout);
    }
    return ev;
}


int main1(int argc, char *argv[])
{
    struct event_base *base;

    base = event_base_new();

    if (!base) {
        printf("Error: Create an event base error!\n");
        return -1;
    }

    struct event * ev = create_timeout_event(base);
    event_base_dispatch(base);

    event_free(ev);
    event_base_free(base);
    return 0;
}
