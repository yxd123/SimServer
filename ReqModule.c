/*
 * ReqModule.c
 * ===========
 * (c) Copyright XianDa Yu 2013/11/2
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "ReqModule.h"

static int firstHeader = 1;

/*
 * Parse HTTP header
 *
 */
//int ParseHttpHeader(char* buffer, struct ReqInfo* reqInfo, struct pool* m_pool)
int ParseHttpHeader(char* buffer, struct ReqInfo* reqInfo)
{
	char* ptr;
	int   len;
	//static int firstHeader = 1;
	//printf("%s\n", buffer);	
	
	if (strstr(buffer, "cgi-bin") != NULL) {
		reqInfo->pageType = DYNAMIC;
	}

	if (firstHeader == 1) {
	/* Get request method */
	if(!strncmp(buffer, "GET ", 4)){
		reqInfo->method = GET;
		buffer += 4;
	}
	else if (!strncmp(buffer, "HEAD ", 5)) {
		reqInfo->method = HEAD;
		buffer += 5;
	}
	else {
		reqInfo->method = UNSUPPORTED;
		reqInfo->status = 501;
		printf("not supported...\n");
		return -1;
	}

	/* Get request resource length */
	ptr = strchr(buffer, ' ');
	if (NULL == ptr)
		len = strlen(buffer);
	else
		len = ptr - buffer;

	if (0 == len) {
		reqInfo->status = 400;
		return -1;
	}
	
	/* alloc resource space */
	//reqInfo->resource = palloc(m_pool, len+1);
	//reqInfo->resource = calloc(len+1, sizeof(char));
	reqInfo->resource = malloc(len+1*sizeof(char));
	strncpy(reqInfo->resource, buffer, len);
	
	/* Get HTTP version */
	if(strstr(buffer, "HTTP/"))
		reqInfo->type = FULL;
	else
		reqInfo->type = SIMPLE;

	firstHeader = 0;
	}//end of firstHeader

	return 0;
}


/* 
 * Get request content.
 * 
 */
//int GetReqContent(int fd, struct ReqInfo* reqInfo, struct pool* m_pool)
int GetReqContent(int fd, struct ReqInfo* reqInfo)
{
	char buffer[MAX_REQ_LINE];
	int  flag;

	fd_set fds;
	struct timeval tv;

	/* Set timeout*/
	tv.tv_sec  = 2;
	tv.tv_usec = 0;

	do {
		memset(buffer, '\0', MAX_REQ_LINE);

		/* Reset sets */
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		
		flag = select(fd + 1, &fds, NULL, NULL, &tv);
		if (flag < 0) {
			perror("select fail !\n");
			exit(1);
		}
		else if (flag == 0) {
			/* timeout */
			printf("select timeout...\n");
			return -1;
		}
		else{
			ReadLine(fd, buffer, MAX_REQ_LINE);
			Trim(buffer);
            printf("Recv: %s", buffer);

			if ((buffer[0] == '\r')&&(buffer[1] == '\n')) {
				printf("buffer[0] == '0'\n");
				break;
			}
			//if(ParseHttpHeader(buffer, reqInfo, m_pool))
			if (ParseHttpHeader(buffer, reqInfo))
				break;
		}
	} while (reqInfo->type != SIMPLE);
	firstHeader = 1;

	return 0;
}

ssize_t ReadLine(int fd, char* buffer, size_t len)
{
	/* n is the number of chars in buffer */
	ssize_t n;
	ssize_t rc;

	char    c;
	/* the end char of buffer must be '\0'*/
	for (n = 0; n < len-1 ; ++n) {
		if ((rc = read(fd, &c, 1)) == 1) {
			*buffer++ = c;
			/* request header is end with '\r\n' */
			if(c == '\n')
				break;
		}
		else if(rc == 0){
			if(n == 0)
				return 0;
			else
				break;
		}
		else{
			/* The call was interrupted by a signal before any data was read */
			if(errno == EINTR)
				continue;
			perror("ReadLine fail !\n");
			exit(1);	
		}
	}
	*buffer = '\0';
	return n+1;
}
