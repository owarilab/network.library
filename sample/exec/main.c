#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<string.h>

#define CHILD_TO_PARENT 0
#define PARENT_TO_CHILD 1
#define FD_READ 0
#define FD_WRITE 1

int main()
{
	int mypipe[2][2];
	if(pipe(mypipe[CHILD_TO_PARENT])<0){
		perror("mypipe");
		exit(1);
	}
	if(pipe(mypipe[PARENT_TO_CHILD])<0){
		close(mypipe[CHILD_TO_PARENT][FD_READ]);
		close(mypipe[CHILD_TO_PARENT][FD_WRITE]);
		perror("mypipe");
		exit(1);
	}
	pid_t pid = fork();
	if(pid<0){
		close(mypipe[CHILD_TO_PARENT][FD_READ]);
		close(mypipe[CHILD_TO_PARENT][FD_WRITE]);
		close(mypipe[PARENT_TO_CHILD][FD_READ]);
		close(mypipe[PARENT_TO_CHILD][FD_WRITE]);
		perror("fork");
		exit(1);
	}else if(pid==0){
		// child
		close(mypipe[PARENT_TO_CHILD][FD_WRITE]); // parent->child stdout
		close(mypipe[CHILD_TO_PARENT][FD_READ]); // child->parent stdin
		dup2(mypipe[PARENT_TO_CHILD][FD_READ],0);
		dup2(mypipe[CHILD_TO_PARENT][FD_WRITE],1);
		close(mypipe[PARENT_TO_CHILD][FD_READ]);
		close(mypipe[CHILD_TO_PARENT][FD_WRITE]);
		int exec_result = execlp("ls","-a","-l",NULL);
		if(exec_result<0){
			perror("execlp");
			return 1;
		}
		exit(0);
	}
	close(mypipe[PARENT_TO_CHILD][FD_READ]);
	close(mypipe[CHILD_TO_PARENT][FD_WRITE]);
	int status;
	pid_t p = waitpid(pid,&status,0);
	if(0==status){
		char buf[128];
		int size = 0;
		while( sizeof(buf) == (size=read(mypipe[CHILD_TO_PARENT][FD_READ],buf,sizeof(buf) ) ) ){
			printf("%s",buf);
			memset(buf,0,sizeof(buf));
		}
		printf("%s\n",buf);
	}
	else{
		return 1;
	}
	return 0;
}


