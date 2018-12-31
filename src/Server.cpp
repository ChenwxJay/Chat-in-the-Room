#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define USER_LIMIT 5
#define READ_BUFFER_SIZE 64 //读缓冲区大小
#define FD_LIMIT 65535

//客户端数据结构体封装
struct ClientData{
	sockaddr_in address;
	char* write_buf;
	char buf[READ_BUFFER_SIZE];//读缓冲区定义
};
//设置非阻塞描述符
int SetNonBlocking(int fd){
   int old_options = fcntl(fd,F_GETFL);
   int new_options = old_options |  O_NONBLOCK;
   fcntl(fd,F_SETFL,new_options);
   return old_options;//返回原来的描述符属性
}
int main(){
	if(argc <= 2){

	}
	//事件循环，使用poll对网络连接事件和可读事件进行监听
	while(1)
	{
		ret = poll(fds,user_counter+1,-1);//poll函数，同步等待就绪事件
		if(ret < 0){
			printf("poll failed!\n");
			break;
		}
		//循环遍历fds数组，根据就绪事件类型执行操作
		for(int i = 0;i < user_counter + 1;++i)
		{
			if((fds[i].fd == listen_fd) && (fds[i].revents & EPOLLIN))
			{
				struct sockaddr_in ClientAddress;
				socklen_t client_len = sizeof(ClientAddress);//客户端地址套接字结构体长度
			    //进行accept
			    int connfd = accept(listen_fd,static_cast<struct sockaddr*>(&ClientAddress),
			    	           &client_len);//获取客户端连接
			    if(connfd < 0){
			    	printf("Accept ERROR!\n");
			    	//跳过当次循环，继续监听
			    	continue;
			    }
			    //如果并发请求数太多，则关闭新建立的连接
			    if(user_counter >= USER_LIMIT){
			    	const char* close_info = "too many users!";
			    	printf("%s\n",close_info);
			    	//发送超量信息给用户
			    	send(connfd,close_info,strlen(close_info),0);
			    	//关闭客户端连接
			    	close(connfd);
			    }
			    user_counter++;//客户端数量+1
			    users[connfd].address = ClientAddress;//填充地址
			    fds[user_counter].fd = connfd;
			    //设置非阻塞fd
			    SetNonBlocking(connfd);
			    //填充需要监听的事件
			    //可读事件，带外数据，错误事件
			    fds[user_counter].events = POLLIN|POLLRDHUP|POLLERR;
                fds[user_counter].revents = 0;//就绪事件清零
                //打印日志
                printf("A user comes!Now Threre are %d users connecting to the server!",user_counter);
			}
			else if(fds[i].revents & POLLERR)
			{
				printf("get a error from the client: %d\n",fds[i].fd);
				char errors[100];
				memset(errors,0,sizeof(errors));//缓冲区清零

			}
			else if(fds[i].revents & POLLRDHUP)
			{
               users[fds[i].fd] = users[fds[user_counter].fd];//设置结构体
               close(fds[i].fd);//关闭客户端连接
               //将最后一个描述符pollfd填充到当前位置
               fds[i] = fds[user_counter];
               i--;//减一，下一次循环检测
               //用户总数减一
               user_counter--;
               printf("A client left!");
            }
            else if(fds[i].revents & POLLIN)
            {
            	int connFd = fds[i].fd;//从pollfd数组获取当前客户端的socket fd
            	//清除缓冲区数据
            	memset(users[connFd].buf,0,READ_BUFFER_SIZE);
            	ret = recv(connFd,users[connFd].buf,READ_BUFFER_SIZE-1,0);
            	if(ret < 0)
            	{
            		//接收错误，关闭连接
            		if(errno != EAGAIN)
            		{
            			close(connFd);
            			users[fds[i].fd] = users[fds[user_counter].fd];//设置结构体
            			fds[i] = fds[user_counter];
            			i--;//撤销操作，回到前面的描述符
            			user_counter--; //用户总数减一
            		}
            	}
            	else if(ret == 0){
            		printf("read %d bytes data!",ret);
            	}
            	else 
                {   //聊天室，需要将数据发给其他客户端，这里接收到数据，通知其他客户端socket写
                	for(int j = 1;i < user_counter;i++){
                        if(fds[j].fd = connFd){
                        	//跳过收到消息的客户端本身
                        	continue;
                        }
                        fds[j].events |= ~POLLIN;
                        FDS[j].events |=  POLLOUT;//设置为可写，通知其他客户端可以写数据 
                        //将当前用户缓冲区的数据拷贝到其他客户端缓冲区
                        users[fds[j].fd].write_buf = users[connFd].buf;	
                	}
                }
            }
            else if(fds[i].revents & POLLOUT)
            {
                int connfd = fds[i].fd;//获取当前事件的fd
                if(users[connfd].write_buf == NULL){
                    printf("The buffer is empty!\n");
                    continue;
                }
                //将写缓冲区的数据写到描述符上
                ret = send(connfd,users[connfd].write_buf,strlen(users[connfd].write_buf),0);
                //将写缓冲区置空
                users[connfd].write_buf = NULL;
                //发送完数据立即切换到监听读
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
            else
            {   //不是所监听的事件
            	printf("Some unexpected events have happend!");
                continue;//跳过本次处理
            }
		}
	}
	//释放资源
	delete [] users;
	close(listen_fd);//关闭监听描述符
	printf("The server will close!......");
	return 0;
}