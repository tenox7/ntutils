#include "sysdep.h"
#include "c_config.h"
#include "console.h"
#include "gui.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

//#include <time.h>

#define MAX_PIPES 20

struct GPipe {
    int fd;
    int pid;
    int stopped;
    EModel *notify;

    GPipe() : fd(-1), pid(0), stopped(0), notify(NULL) {}
};

static GPipe Pipes[MAX_PIPES];
/*
 * Returns WaitFdPipe status of event.
 */
WaitFdPipe WaitFdPipeEvent(TEvent *Event, int fd, int fd2, int WaitTime)
{
    struct timeval timeout;
    fd_set readfds;
    int rc, maxfd = (fd > fd2) ? fd : fd2;

    Event->What = evNone;

    FD_ZERO(&readfds);

    if (fd >= 0)
	FD_SET(fd, &readfds);

    if (fd2 >= 0)
	FD_SET(fd2, &readfds);

#ifndef NO_PIPES
    for (int p = 0; p < MAX_PIPES; ++p)
	if (Pipes[p].fd != -1) {
	    FD_SET(Pipes[p].fd, &readfds);
	    if (Pipes[p].fd > maxfd)
                maxfd = Pipes[p].fd;
	}
#endif
    //printf("EVENT %d:  %d   %d  %d\n", (int)time(NULL), fd, fd2, WaitTime);
    timeout.tv_sec = WaitTime / 1000;
    timeout.tv_usec = (WaitTime % 1000) * 1000;

    if ((rc = select(maxfd + 1, &readfds, 0, 0,
		     (WaitTime < 0) ? 0 : &timeout)) <= 0)
	return (!rc) ? FD_PIPE_TIMEOUT : FD_PIPE_ERROR;

    if ((fd >= 0) && FD_ISSET(fd, &readfds))
	return FD_PIPE_1;

    if ((fd2 >= 0) && FD_ISSET(fd2, &readfds))
	return FD_PIPE_2;

#ifndef NO_PIPES
    for (int pp = 0; pp < MAX_PIPES; ++pp) {
	if (Pipes[pp].fd != -1
	    && FD_ISSET(Pipes[pp].fd, &readfds)
	    && Pipes[pp].notify) {
	    Event->What = evNotify;
	    Event->Msg.View = 0;
	    Event->Msg.Model = Pipes[pp].notify;
	    Event->Msg.Command = cmPipeRead;
	    Event->Msg.Param1 = pp;
	    Pipes[pp].stopped = 0;
	    return FD_PIPE_EVENT;
	}
	//fprintf(stderr, "Pipe %d\n", Pipes[pp].fd);
    }
#endif

    return FD_PIPE_ERROR;
}

int GUI::OpenPipe(const char *Command, EModel * notify)
{
    //fprintf(stderr, "PIPE  %s   \n", Command);
#ifndef NO_PIPES
    for (int i = 0; i < MAX_PIPES; ++i) {
	if (Pipes[i].fd == -1) {
	    int pfd[2];

	    Pipes[i].notify = notify;
	    Pipes[i].stopped = 1;

	    if (pipe((int *) pfd) == -1) {
		perror("pipe");
		return -1;
	    }

	    switch (Pipes[i].pid = fork()) {
	    case -1:		/* fail */
                perror("fork");
		return -1;
	    case 0:		/* child */
		// FIXME: close other opened descriptor
		signal(SIGPIPE, SIG_DFL);
		close(pfd[0]);
		close(0);
                assert(open("/dev/null", O_RDONLY) == 0);
		dup2(pfd[1], 1);
		dup2(pfd[1], 2);
		close(pfd[1]);
		exit(system(Command));
	    default:
		close(pfd[1]);
		fcntl(pfd[0], F_SETFL, O_NONBLOCK);
		Pipes[i].fd = pfd[0];
	    }
	    return i;
	}
    }
#endif
    return -1;
}

int GUI::SetPipeView(int id, EModel * notify)
{
#ifndef NO_PIPES
    if (id < 0 || id >= MAX_PIPES || Pipes[id].fd == -1)
	return 0;

    Pipes[id].notify = notify;
#endif

    return 1;
}

/*
 * returns read size value from pipe reading
 */
ssize_t GUI::ReadPipe(int id, void *buffer, size_t len)
{
#ifndef NO_PIPES
    if (id < 0 || id >= MAX_PIPES || Pipes[id].fd == -1)
	return -1;

    ssize_t rc;
    if (!(rc = read(Pipes[id].fd, buffer, len))) {
	close(Pipes[id].fd);
	Pipes[id].fd = -1;
	return -1;
    } else if (rc == -1)
	Pipes[id].stopped = 1;

    return rc;
#else
    return -1;
#endif
}

int GUI::ClosePipe(int id)
{
#ifndef NO_PIPES
    int status = -1;

    if (id < 0 || id >= MAX_PIPES || Pipes[id].fd == -1)
	return 0;

    close(Pipes[id].fd);
    Pipes[id].fd = -1;
    kill(Pipes[id].pid, SIGHUP);
    alarm(1);
    waitpid(Pipes[id].pid, &status, 0);
    alarm(0);

    return (WEXITSTATUS(status) == 0);
#endif

    return 0;
}
