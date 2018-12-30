#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define USER_LIMIT 5
#define READ_BUFFER_SIZE 64 //读缓冲区大小
#define FD_LIMIT 65535
//客户端数据封装
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
//服务器主程序
int main(int argc,char* argv){
    if(argc <= 2){
    	printf("usage:%s ip_address port!",basename(argv[0]));
    	return 1; 
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    //定义变量
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));//第二个参数是
    address.sin_family = AF_INET;//填充协议，IPV4
    //将ip地址转为网络字节序，填充
    inet_pton(AF_INET,ip,&address.sin_addr);
    //填充端口
    address.port = htons(port);

    //创建连接套接字
    int socket_fd = socket(AF_INET,SOCK_STREAM,0);//ipv4,TCP流套接字
    //注意对返回的套接字描述符进行校验
    if(socket_fd <= 0){
    	printf("create socket failed!");
    	return -1;
    }
    //开始连接服务器
    printf("Connect to the server:%s,port: %d\n",ip,port);
    ret = connect(socket_fd,(struct sockaddr*)&address,sizeof(address));
    if(ret < 0){
    	printf("connection failed!\n");
    	close(socket_fd);//需要关闭文件描述符
    	return 1;
    }
    //使用poll,需要创建pollfd数组
    pollfd fds[2];
    //监听标准输入描述符和套接字描述符
    fds[0].fd = 0;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    //注册套接字描述符
    fds[1].fd = socket_fd;
    fds[1].events = POLLIN|POLLRDHUP;
    fds[1].revents = 0;
    //定义数据缓冲区
    char ReadBuffer[READ_BUFFER_SIZE];
    int pipefd[2];
    //创建管道
    ret = pipe(pipefd); //创建管道，调用pipe函数，传入数组
    if(ret < 0){
    	printf("create pipe failed!");
    	return -1;
    }

    //循环监听
    while(1){
    	ret = poll(fds,2,-1);//调用poll函数，就绪事件写入到fds数组
    	if(ret < 0){
    		printf("poll failure!\n");
    		break;
    	}
    	//检查返回的数组
    	//处理用户输入
    	if(fds[0].revents & POLLIN){
    		//实现零拷贝
    		ret = splice(0,NULL,pipefd[1],NULL,32768,
    			  SPLICE_F_MORE | SPLICE_F_MOVE); //将用户输入的数据拷贝到管道的写端
    		ret = splice(pipefd[0],NULL,socket_fd,NULL,32768,
    			  SPLICE_F_MORE | SPLICE_F_MOVE);//从管道的读端将数据拷贝到sokcet上面
    	}
    	//处理socket可读事件或者关闭事件
    	if(fds[1].revents & POLLRDHUP){
    		cout << "The server has closed the connection!" << endl;
    		break;
    	}
    	else if(fds[1].revents & POLLIN){
    		//缓冲区清零
            memset(ReadBuffer,'\0',READ_BUFFER_SIZE);
            //利用recv从socket上面接收数据
            recv(fds[1].fd,ReadBuffer,READ_BUFFER_SIZE-1,0);
            //打印到用户端终端
            printf("The client has received:%s\n",ReadBuffer);
    	}
    }
    //关闭套接字
    close(socket_fd);
    printf("The Client is going to close the connection!\n");
    return 0;
}