#include <stdio.h>
#include <pthread.h>
#include <errno.h>
void *myThread1(void)
{
    int i;
    for (i=0; i<99999999999; i++)
    {
        printf("This is the 1st pthread,created by zieckey.\n");
       // sleep(100);//Let this thread to sleep 1 second,and then continue to run
    }
}

void *myThread2(void)
{
    int i;
    for (i=0; i<99999999999; i++)
    {
        printf("This is the 2st pthread,created by zieckey.\n");
        //sleep(100);
    }
}

int main()
{
    int i=0, ret=0;
    pthread_t id1,id2;
   
    ret = pthread_create(&id1, NULL, (void*)myThread1, NULL);
    if (ret)
    {
        printf("Create pthread error!\n");
        return 1;
    }
   
    ret = pthread_create(&id2, NULL, (void*)myThread2, NULL);
    if (ret)
    {
        printf("Create pthread error!\n");
        return 1;
    }
   
    pthread_join(id1, NULL);//sleep 会让出资源
    pthread_join(id2, NULL);

    for (i=0; i<5; i++)
	printf("hello world!\n");

	int kill_rc = pthread_kill(id2,0);

	if(kill_rc == ESRCH)
	printf("the specified thread did not exists or already quit\n");
	else if(kill_rc == EINVAL)
	printf("signal is invalid\n");
	else
	{
		printf("%d\n",kill_rc);
		printf("the specified thread is alive\n");
	}
	/*
	while(kill_rc==0)
	{
		kill_rc = pthread_kill(id2,0);
	}
	*/
   
    return 0;
}