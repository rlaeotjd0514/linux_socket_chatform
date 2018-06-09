#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <hacking.h>
#include <hacking_network.h>
#include <pthread.h>
#include <semaphore.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void * handle_clnt(void *arg);
void send_msg(char * msg, int len);
void error_handling(char * msg);

int clnt_cnt = 0;
int clnt_sock[MAX_CLNT];
pthread_mutex_t mutx;

int main() {
	int serv_sock;
	int clnt_sock2;
	struct sockaddr_in serv_addr, clnt_addr;
	char * server_ip = "192.168.10.19";
	char * server_port = "4404";
	int clnt_addr_sz;
	int yes = 1;
	pthread_t t_id;
	
	pthread_mutex_init(&mutx, NULL);
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);	
	serv_addr.sin_port = htons(atoi(server_port));
	
	printf("IP : %s\n", inet_ntoa(serv_addr.sin_addr));
	
	bind(serv_sock , (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	listen(serv_sock, 20);
	

	while(1) {
		clnt_addr_sz = sizeof(clnt_addr);
		clnt_sock2 = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_sz);
		pthread_mutex_lock(&mutx);
		clnt_sock[clnt_cnt++] = clnt_sock2;

		pthread_mutex_unlock(&mutx);
		pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock2);
		pthread_detach(t_id);

		printf("Connected client IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
	}

	close(serv_sock);
	return 0;
}

void * handle_clnt(void * arg)
{
	int clnt_sock1 = *((int*)arg);
	int str_len = 0, i;
	char msg[BUF_SIZE];

	while((str_len = read(clnt_sock1, msg, sizeof(msg))) != 0) {
		send_msg(msg, str_len);
	}

	pthread_mutex_lock(&mutx);

	for(i=0;i<clnt_cnt;i++) {
		if(clnt_sock1 == clnt_sock[i]) {
			while(i++<clnt_cnt-1) {
				clnt_sock[i] = clnt_sock[i+1];
			}
			break;
		}
	}
	clnt_cnt--;
	pthread_mutex_unlock(&mutx);
	close(clnt_sock1);
	return NULL;
}

void send_msg(char * msg, int len) {
	int i;
	pthread_mutex_lock(&mutx);
	for(i=0;i<clnt_cnt;i++) {
		write(clnt_sock[i], msg, len);
	}
	pthread_mutex_unlock(&mutx);
}



