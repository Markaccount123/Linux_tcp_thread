#pragma once 

#include<iostream>
#include<cstdlib>
#include<cstring>
#include<unistd.h>
#include<string>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>
#define BACKLOG 5 //一般这个的值都设置的比较小，表示的意思是此时底层链接队列中等待链接的长度
class tcpServer{
  private:
    int port;
    int lsock;//表示监听的套接字
  public:

    tcpServer(int _port = 8080,int _lsock = -1)
      :port(_port),lsock(_lsock)
    {}

    void initServer()
    {
      lsock = socket(AF_INET,SOCK_STREAM,0);
      if(lsock < 0){
        std::cerr<<"socket error"<<std::endl;
        exit(2);
      }
      struct sockaddr_in local;
      local.sin_family = AF_INET;
      local.sin_port = htons(port);
      local.sin_addr.s_addr = htonl(INADDR_ANY);

      if(bind(lsock,(struct sockaddr*)&local,sizeof(local)) < 0){
        std::cerr<<"bind error!"<<std::endl;
        exit(3);
      }

      //走到这里说明bind成功了，但是tcp是面向链接的，所以还需要一个接口
      if(listen(lsock,BACKLOG) < 0){
        std::cout<<"listen error!"<<std::endl;
        exit(4);
      } 
      
      //tcp必须要将套接字设置为监听状态  这里可以想夜间你去吃饭，什么时候去都可以吃到，是为什么？是因为店里面一直有人在等待着你
      //任意时间有客户端来连接我
      //成功返回0，失败返回-1
    }

    //echo服务器
    static void service(int sock)
    {
      while(true){
        //打开一个文件，也叫打开一个流，所以这里其实使用read和write也是可以的，但是这里是网络，最好使用tcp的recv和send
        char buf[64];
        ssize_t s = recv(sock,buf,sizeof(buf)-1,0);
        if(s > 0){
          buf[s] = '\0';
          std::cout<<"Client# "<< buf <<std::endl;
          send(sock,buf,strlen(buf),0);//此时的网络就是文件，你往文件中发送东西的格式，可不是以\0作为结束，加入\0会出现乱码的情况
        }
        else if(s == 0){
          std::cout<<"Client quit ..."<<std::endl;
          break;
        }else{
          std::cerr<<"recv error"<<std::endl;
          break;
        }
      }
      close(sock);
    }

    static void *serviceRoutine(void *arg)
    {
      pthread_detach(pthread_self());//采用线程分离，运行完了以后自动释放。
      std::cout<<"create a new thread"<<std::endl;
      int *p = (int*)arg;
      int sock =  *p; 
      service(sock);
      delete p;
    }

    void start()
    {
      struct sockaddr_in endpoint;
      while(true){
        //第一步应该获取链接
        //这里可以理解为 饭店拉客
        //lsock 是负责拉客的   accept的返回值是主要负责服务客户的
        socklen_t len = sizeof(endpoint);
        int sock = accept(lsock,(struct sockaddr*)&endpoint,&len);
        if(sock < 0){
          std::cerr<<"accept error!"<<std::endl;
          continue; //相当于拉客失败了，我需要继续拉客
        }
        //打印获取到的连接的ip和端口号
        std::string cli_info = inet_ntoa(endpoint.sin_addr);
        cli_info += ":";
        cli_info += std::to_string(endpoint.sin_port); //这里获取的端口号是一个整数，需要把该整数转换为字符串,当然这里也可以使用stringstream 比如定义一个对象 ， 此时有一个int变量，一个string 对象 你把int输入stringstream 然后再把stringstream在输入string的对象中，就进行了转换
        //stringstream ss;
        //int a = 100;
        //string b ;
        //ss << a;
        //ss >> b;  这样就进行了整形转字符串的功能
        std::cout<<"get a new link "<< cli_info<<" sock: "<<sock<<std::endl;

        pthread_t tid;
        //得到这个sock之后，给他保存一份，然后就不怕被覆盖了
        int *p = new int(sock); //这样做这个sock就是下面所创建新线程所私有的了
        pthread_create(&tid,nullptr,serviceRoutine,(void *)p); //但是这里取sock的地址可能会出现bug，那是因为有可能正在取的时候，主线程又accept获得了一个新连接
        //pthread_create(&tid,nullptr,serviceRoutine,(void*)&sock); //但是这里取sock的地址可能会出现bug，那是因为有可能正在取的时候，主线程又accept获得了一个新连接
        //那么就有可能，对原先的sock进行覆盖，也就是没有处理前面的那个连接
      
        //主进程还需要等待线程，进行回收
        //pthread_join()但是这里如果主线程进行阻塞等待子线程执行完毕的话，那么主线程酒干不了别的事情了，所以不能这样，所以这里还有种方式就是线程分离
      }
    }

    ~tcpServer()
    {}
};
