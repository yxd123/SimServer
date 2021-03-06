/*
 * epoll_module.c
 * ================
 * (c) Copyright XianDa Yu 2013/12/7
 * the epoll module
 *
 */
 
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>


#include "event.h"
#include "../ReqModule.h"
#include "../RespModule.h"
#include "../log.h"
#include "../Util.h"
 
void setnonblocking(int sock)
{
	int opts;
	opts = fcntl(sock, F_GETFL);
	 if(opts < 0){
		perror("fcntl(sock,GETFL)");
		exit(1);
	}
	opts = opts | O_NONBLOCK;
	if(fcntl(sock, F_SETFL, opts)<0){
		perror("fcntl(sock,SETFL,opts)");
		exit(1);
	}
}
 
int epoll_process(int fd)
{
	int i;
	int n;
	int nfds;
	int epfd;
	int len;
	int flag;
	int newfd;
	int conn;
    pid_t pid;
	
	char buffer[1024];
	struct pool *m_pool;
	struct epoll_event ev;
	struct epoll_event events[1024];
	struct sockaddr_in client_addr;

	//m_pool = createPool(2048);
	epfd = epoll_create(1024);
	
	//close(fd);
	ev.data.fd = fd;
	ev.events = EPOLLIN|EPOLLET;
	flag = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    if(flag < 0){
		perror("epoll_ctl error !\n");
		return -1;
	}
	
	len = sizeof(struct sockaddr);
	while(1){
		nfds = epoll_wait(epfd, events, 512, 500);
		for(i = 0; i < nfds; ++i){
            if(events[i].data.fd == fd){
				conn = accept(fd, (struct sockaddr*)&client_addr, (unsigned int*)&len);
                if(conn < 0){
                    perror("accept error!\n");
                        continue;
                }
				setnonblocking(conn);
                ev.data.fd = conn;
                ev.events  = EPOLLIN|EPOLLET;                        
                epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev);
            }
			else if((events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP)){
				close(events[i].data.fd);
				epoll_ctl(epfd, EPOLL_CTL_ADD, conn, &ev);
				continue;	
			}
			else if(events[i].events&EPOLLIN){
            	/* receive request */                        
                if((newfd = events[i].data.fd) < 0)
                    continue; 
	    	    pid = fork();
	    	    if(pid == 0){
	    	    	close(fd);
                    printf("process %d forked\n", getpid());
	    	    	handleRequest(newfd);
	    	    	exit(0);
	    	    }
	    	    close(newfd);
	    	    waitpid(-1, NULL, WNOHANG);
				
				ev.data.fd = -1;
				epoll_ctl(epfd, EPOLL_CTL_DEL, newfd, &ev);
            }

		}
	}
	//destroyPool(m_pool);
	return 0;
}
