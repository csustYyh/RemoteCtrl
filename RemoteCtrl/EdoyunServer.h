#pragma once
#include "EdoyunThread.h"
#include "EdoyunQueue.h"
#include <map>
#include <MSWSock.h>

enum EdoyunOperator{
    ENone, 
    EAccept,
    ERecv,
    ESend,
    EError
};

class CEdoyunServer;
class EdoyunClient;
typedef std::shared_ptr<EdoyunClient> PCLIENT;

// 为什么下面类里面要把 OVERLAPPED 放在最开始： 这样 m_overlapped 和 EdoyunOverlapped的实例 内存起始地址就是一样的
// AcceptEX WSARecv WSASend... 用户态传(m_overlapped)给内核，内核处理完返回的也是(m_overlapped)，这时候在用户态做一次强转(EdoyunServer.cpp line:236)，就可以拿到EdoyunOverlapped类里面的所有成员，也就是定位到了那一次的IO请求
class EdoyunOverlapped { // overlapped 类
public:
    OVERLAPPED m_overlapped; // 重叠结构
    DWORD m_operator; // 操作 参见：EdoyunOperator
    std::vector<char> m_buffer; // 缓冲区
    ThreadWorker m_worker; // 不同操作对应的处理函数   
    CEdoyunServer* m_server; // 服务器对象
    EdoyunClient* m_client; // 对应的客户端
    WSABUF m_wsabuffer; // WSARecv() WSASend() 要用到的参数
    virtual ~EdoyunOverlapped()
    {
        m_buffer.clear();
    }
};

template<EdoyunOperator> class AcceptOverlapped; // 模版类 用于创建与特定操作类型(recv)相关联的重叠结构
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
template<EdoyunOperator> class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<EdoyunOperator> class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class EdoyunClient : public ThreadFuncBase { // 客户端类
public:
    EdoyunClient();
    ~EdoyunClient() {
        m_buffer.clear();
        closesocket(m_sock);
        m_overlapped.reset();
        m_overlapped_recv.reset();
        m_overlapped_send.reset();
        m_vecSend.Clear();
    }

    void SetOverlapped(PCLIENT& ptr);

    operator SOCKET() // 重载 SOCKET 类型转换 主要用于给 AcceptEX() WSARecv() 传递参数
    {
        return m_sock;
    }
    operator PVOID() // 重载 PVOID 类型转换 主要用于给 AcceptEX() WSARecv() 传递参数
    {
        return &m_buffer[0];
    }
    operator LPDWORD() // 重载 LPDWORD 类型转换 主要用于给 AcceptEX() WSARecv() 传递参数
    {
        return &m_received;
    }
    operator LPOVERLAPPED (); // 重载 LPOVERLAPPED 类型转换 主要用于给 AcceptEX()传递参数  // 这里后面应该要改  返回的是 ACCEPTOVERLAPPED 而WSARecv()  WSASend()应该要用另外的参数
    LPWSABUF RecvWSABUFFER(); // 主要用于给 WSARecv() 传递参数
    LPWSAOVERLAPPED RecvOVERLAPPED(); // 主要用于给 WSARecv() 传递参数
    LPWSABUF SendWSABUFFER(); // 主要用于给 WSASend() 传递参数
    LPWSAOVERLAPPED SendOVERLAPPED(); // 主要用于给 WSASend() 传递参数
    
    DWORD& flags() { return m_flags; } // 主要用于给 WSARecv()传递参数
    sockaddr_in* GetLocalAddr() { return &m_laddr;}
    sockaddr_in* GetRemoteAddr() { return &m_raddr;}
    size_t GetBufferSize()const { return m_buffer.size(); }
    int Recv();
    int Send(void* buffer, size_t nSize); // 先投递到一个队列里面去，因为高并发的情况下可能Send()不会立即完成
    int SendData(std::vector<char>& data);
private:
    SOCKET m_sock; // 与客户端通信的套接字
    std::vector<char> m_buffer; // 缓冲区
    size_t m_used; // 已经使用的缓冲区大小
    sockaddr_in m_laddr; // 客户端地址
    sockaddr_in m_raddr; // 远程服务器地址
    bool m_isbusy; // 客户端是否已经连接
    DWORD m_received; // AcceptEX() WSARecv()要用到的参数
    DWORD m_flags; // WSARecv() 要用到的参数 
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped; // AcceptEX()要用到的参数 Accept这一I/O操作对应的重叠结构指针
    std::shared_ptr<RECVOVERLAPPED> m_overlapped_recv; // WSARecv()要用到的参数 Receive这一I/O操作对应的重叠结构指针
    std::shared_ptr<SENDOVERLAPPED> m_overlapped_send; // WSASend()要用到的参数 Send这一I/O操作对应的重叠结构指针
    EdoyunSendQueue<std::vector<char>> m_vecSend; // 发送数据队列
};


template<EdoyunOperator>
class AcceptOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped();
    int AcceptWorker();
};

template<EdoyunOperator>
class RecvOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
    RecvOverlapped();
    int RecvWorker()
    {
        //TODO:recv
        int ret = m_client->Recv();
        return ret;
    }
};


template<EdoyunOperator>
class SendOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
    SendOverlapped();
    int SendWorker()
    {
        //TODO:send
        /*
        * 
        */
        return -1;
    }
};


template<EdoyunOperator>
class ErrorOverlapped :public EdoyunOverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped() :m_operator(EError), m_worker(this, (FUNCTYPE)&ErrorOverlapped::ErrorWorker) {
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }
    int ErrorWorker()
    {
        //TODO:error
        return -1;
    }
};  
typedef ErrorOverlapped<EError> ERROROVERLAPPED;

class CEdoyunServer :
    public ThreadFuncBase
{
public:
    CEdoyunServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10) {
        m_hIOCP = INVALID_HANDLE_VALUE;
        m_sock = INVALID_SOCKET;
        m_addr.sin_family = AF_INET;
        m_addr.sin_port = htons(port);
        m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}
    ~CEdoyunServer();
    bool StartService();

    bool NewAccept();

    void BindNewSocket(SOCKET s); // 绑定新的通信套接字到IOCP
    
private:
    void CreateSocket();
    
    int threadIocp(); // IOCP线程函数

    CEdoyunThreadPool m_pool; // 线程池
    HANDLE m_hIOCP; // 完全端口句柄
    SOCKET m_sock; // 服务器套接字
    sockaddr_in m_addr; // 服务器地址
    std::map<SOCKET, PCLIENT> m_client; // 客户端套接字(通信套接字)与客户端对象的映射表 每连上一个客户端就将其维护进map shared_ptr：智能指针 
};

