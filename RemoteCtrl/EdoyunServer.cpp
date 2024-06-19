#include "pch.h"
#include "EdoyunServer.h"
#include "EdoyunTool.h"
#pragma warning (disable:4407)

template<EdoyunOperator op>
AcceptOverlapped<op>::AcceptOverlapped() {
    m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
    m_operator = EAccept;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024);
    m_server = NULL;
}
template<EdoyunOperator op>
SendOverlapped<op>::SendOverlapped() {
    m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
    m_operator = ESend;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}

template<EdoyunOperator op>
RecvOverlapped<op>::RecvOverlapped() {
    m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
    m_operator = ERecv;
    memset(&m_overlapped, 0, sizeof(m_overlapped));
    m_buffer.resize(1024 * 256);
}


template<EdoyunOperator op>
int AcceptOverlapped<op>::AcceptWorker() // Accept这个操作对应的执行函数
{
    //TODO:accept
    int lLength = 0, rLength = 0;
    if (m_client->GetBufferSize() > 0) // client收到了buffer值
    {
        sockaddr* plocal = NULL, * premote = NULL;
        GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)&plocal, &lLength, // 客户端地址 
            (sockaddr**)&premote, &rLength // 远程服务器地址
        );
        memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
        memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
        m_server->BindNewSocket(*m_client); // 绑定新连接的通信套接字到IOCP
        int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABUFFER(), 1, *m_client, &m_client->flags(), m_client->RecvOVERLAPPED(), NULL); // WSARecv()并不会真的recv() 而是发送请求给IOCP
        if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) // WSA_IO_PENDING：正在操作
        {
            // TODO: 报错
            TRACE("ret = %d error = %d\r\n", ret, WSAGetLastError());
        }
        // 连接到一个客户端就accept一次
        if (m_server->NewAccept())
        {
            return -2; // accept出错
        }
    }
    return -1; // 正常结束
}

EdoyunClient::EdoyunClient() 
    :m_isbusy(false), m_flags(0), 
    m_overlapped(new ACCEPTOVERLAPPED()), m_overlapped_recv(new RECVOVERLAPPED()), m_overlapped_send(new SENDOVERLAPPED()), 
    m_vecSend(this, (SENDCALLBACK)&EdoyunClient::SendData)
{
    // 想要使用重叠I/O的（实现异步通信）话，初始化Socket的时候一定要使用WSASocket并带上WSA_FLAG_OVERLAPPED参数才可以(只有在服务器端需要这么做，在客户端是不需要的)
    // 设置"WSA_FLAG_OVERLAPPED"参数后，得到的 sock 就是非阻塞的
    m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); // 事先建好的，用于接收连接的，等有客户端连接进来直接把这个Socket拿给它用的那个，是AcceptEx高性能的关键所在
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(m_laddr));
    memset(&m_raddr, 0, sizeof(m_raddr));
}
void EdoyunClient::SetOverlapped(PCLIENT& ptr)
{
    m_overlapped->m_client = ptr.get();
    m_overlapped_recv->m_client = ptr.get();
    m_overlapped_send->m_client = ptr.get();
}
EdoyunClient:: operator LPOVERLAPPED ()
{
    return &m_overlapped->m_overlapped;
}

LPWSABUF EdoyunClient::RecvWSABUFFER()
{
    return &m_overlapped_recv->m_wsabuffer;
}

LPWSAOVERLAPPED EdoyunClient::RecvOVERLAPPED()
{
    return &m_overlapped_recv->m_overlapped;
}

LPWSABUF EdoyunClient::SendWSABUFFER()
{
    return &m_overlapped_send->m_wsabuffer;
}

LPWSAOVERLAPPED EdoyunClient::SendOVERLAPPED()
{
    return &m_overlapped_send->m_overlapped;
}

int EdoyunClient::Recv()
{
    int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
    if (ret <= 0) return -1;
    m_used += (size_t)ret;
    // TODO:解析数据
    CEdoyunTool::Dump((BYTE*)m_buffer.data(), ret);
    return 0;
}

int EdoyunClient::Send(void* buffer, size_t nSize)
{
    std::vector<char> data(nSize);
    memcpy(data.data(), buffer, nSize);
    if (m_vecSend.PushBack(data))
    {
        return 0;
    }
    return -1;
}

int EdoyunClient::SendData(std::vector<char>& data)
{
    if (m_vecSend.Size() > 0) // 没有发送完
    {
        int ret = WSASend(m_sock, SendWSABUFFER(), 1, &m_received, m_flags, &m_overlapped_send->m_overlapped, NULL);
        if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING))
        {
            CEdoyunTool::ShowError();
            return -1;
        }
    }
    return 0;
}

CEdoyunServer::~CEdoyunServer()
{
    closesocket(m_sock);
    std::map < SOCKET, PCLIENT >::iterator it = m_client.begin();
    for (; it != m_client.end(); it++)
    {
        it->second.reset();
    }
    m_client.clear();
    CloseHandle(m_hIOCP);
    m_pool.Stop();
    WSACleanup();
}

bool CEdoyunServer::StartService()
{
    CreateSocket(); // 创建服务器套接字 m_sock
    // 将一个本地地址绑定到套接字上，以指定套接字在网络中的通信地址。这样，其他程序就可以通过该地址与套接字通信
    if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) // 注意IP(m_ip)和端口号(m_port)都包含在 m_addr 里
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
    // 将套接字标记为被动套接字，即用于接受连接请求的套接字(监听套接字)
    if (listen(m_sock, 3) == -1) // para1:套接字文件描述符，即要设置为监听状态的套接字 para2:连接队列的最大长度，即同时可以处理的连接请求数量(当有新的连接请求到达时，最多可以放入 3 个请求到等待队列中，超过这个数量的请求将会被拒绝)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4); // 创建IOCP
    if (m_hIOCP == NULL)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        m_hIOCP = INVALID_HANDLE_VALUE;
        return false;
    }
    BindNewSocket(m_sock); // 绑定服务器套接字(监听)到IOCP
    //CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); // 绑定服务器套接字(监听)到IOCP
    m_pool.Invoke(); // 开启线程池
    m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CEdoyunServer::threadIocp)); // 开启IOCP线程
    if (!NewAccept()) return false;
    return true;
}

bool CEdoyunServer::NewAccept()
{
    PCLIENT pClient(new EdoyunClient);
    pClient->SetOverlapped(pClient);
    m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
    // para1:服务器socket，用于监听连接请求 para2:用于接收连接的socket para3:接收缓冲区 para4：para3中用于存放数据的空间大小。如果此参数=0，则AcceptEx时将不会待数据到来，而直接返回，如果此参数不为0，那么一定得等接收到数据了才会返回
    // para5:存放本地址地址信息的空间大小 para6:存放本远端地址信息的空间大小 para7：接收到的数据大小 para8：本次重叠I/O所要用到的重叠结构
    if (AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient) == FALSE) // 并不会真的accept() 而是发送请求给IOCP
    {
        if (WSAGetLastError() != WSA_IO_PENDING) // 如果错误码是 WSA_IO_PENDING 不能算作出错
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false; // accept出错
        }
    }
    return true;
}

void CEdoyunServer::BindNewSocket(SOCKET s)
{
    CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

void CEdoyunServer::CreateSocket()
{
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 2), &WSAData); // 向操作系统说明，我们要用哪个库文件，让该库文件与当前的应用程序绑定，从而就可以调用该版本的socket的各种函数
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); // 创建一个带有重叠 I/O（Overlapped I/O）特性的套接字
    int opt = 1; // 选项常量，表示允许重用本地地址和端口
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)); // 设置了允许重用本地地址和端口的选项
    // 重叠 I/O 是指在进行异步 I/O 操作时，允许应用程序同时进行其他操作，而不必等待 I/O 操作完成。而允许重用地址和端口则允许在套接字关闭后，立即再次使用相同的地址和端口。
}

int CEdoyunServer::threadIocp()
{
    DWORD transferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
    // 监控完成端口,监视完成端口的队列中是否有完成的网络操作
    // para1:建立的那个唯一的完成端口 para2:操作完成后返回的字节数 para3:自定义结构体参数 para4:重叠结构 para5:等待完成端口的超时时间
    if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
    {   // AcceptEx WSARecv WSASend 都会向相关socket绑定的完全端口投递异步 IO 请求
        if (CompletionKey != 0)
        {
            // 重叠结构我们就可以理解成为是一个网络操作的ID号，也就是说我们要利用重叠I/O提供的异步机制的话，每一个网络操作都要有一个唯一的ID号，（其实就是重叠结构里面的各种成员）
            // 因为进了系统内核，里面黑灯瞎火的，也不了解上面出了什么状况，一看到有重叠I/O的调用进来了，就会使用其异步机制，
            // 并且操作系统就只能靠这个重叠结构带有的ID号来区分是哪一个网络操作了，然后内核里面处理完毕之后，根据这个ID号，把对应的数据传上去。
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped); // 通过重叠结构，拿到了重叠结构的父对象的指针
            pOverlapped->m_server = this;
            switch (pOverlapped->m_operator)
            {
            case EAccept: // 处理连接请求
            {
                ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理连接请求
            }
            break;
            case ERecv:
            {
                RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理发送请求
            }
            break;
            case ESend:
            {
                SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理接收请求
            }
            break;
            case EError:
            {
                ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理报错请求
            }
            break;
            }
        }
        else
        {
            return -1;
        }
    }
    return 0;
}


int CEdoyunServer::threadIocp()
{
    DWORD transferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
    // 监控完成端口,监视完成端口的队列中是否有完成的网络操作
    // para1:建立的那个唯一的完成端口 para2:操作完成后返回的字节数 para3:自定义结构体参数 para4:重叠结构 para5:等待完成端口的超时时间
    if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
    {   // AcceptEx WSARecv WSASend 都会向相关socket绑定的完全端口投递异步 IO 请求
        if (CompletionKey != 0)
        {
            // 重叠结构我们就可以理解成为是一个网络操作的ID号，也就是说我们要利用重叠I/O提供的异步机制的话，每一个网络操作都要有一个唯一的ID号，（其实就是重叠结构里面的各种成员）
            // 因为进了系统内核，里面黑灯瞎火的，也不了解上面出了什么状况，一看到有重叠I/O的调用进来了，就会使用其异步机制，
            // 并且操作系统就只能靠这个重叠结构带有的ID号来区分是哪一个网络操作了，然后内核里面处理完毕之后，根据这个ID号，把对应的数据传上去。
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped); // 通过重叠结构，拿到了重叠结构的父对象的指针
            pOverlapped->m_server = this;
            switch (pOverlapped->m_operator)
            {
            case EAccept: // 处理连接请求
            {
                ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理连接请求
            }
            break;
            case ERecv:
            {
                RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理接收请求
            }
            break;
            case ESend:
            {
                SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理发送请求
            }
            break;
            case EError:
            {
                ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // 分配线程处理报错请求
            }
            break;
            }
        }
        else
        {
            return -1;
        }
    }
    return 0;
}