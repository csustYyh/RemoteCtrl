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

// Ϊʲô����������Ҫ�� OVERLAPPED �����ʼ�� ���� m_overlapped �� EdoyunOverlapped��ʵ�� �ڴ���ʼ��ַ����һ����
// AcceptEX WSARecv WSASend... �û�̬��(m_overlapped)���ںˣ��ں˴����귵�ص�Ҳ��(m_overlapped)����ʱ�����û�̬��һ��ǿת(EdoyunServer.cpp line:236)���Ϳ����õ�EdoyunOverlapped����������г�Ա��Ҳ���Ƕ�λ������һ�ε�IO����
class EdoyunOverlapped { // overlapped ��
public:
    OVERLAPPED m_overlapped; // �ص��ṹ
    DWORD m_operator; // ���� �μ���EdoyunOperator
    std::vector<char> m_buffer; // ������
    ThreadWorker m_worker; // ��ͬ������Ӧ�Ĵ�����   
    CEdoyunServer* m_server; // ����������
    EdoyunClient* m_client; // ��Ӧ�Ŀͻ���
    WSABUF m_wsabuffer; // WSARecv() WSASend() Ҫ�õ��Ĳ���
    virtual ~EdoyunOverlapped()
    {
        m_buffer.clear();
    }
};

template<EdoyunOperator> class AcceptOverlapped; // ģ���� ���ڴ������ض���������(recv)��������ص��ṹ
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
template<EdoyunOperator> class RecvOverlapped;
typedef RecvOverlapped<ERecv> RECVOVERLAPPED;
template<EdoyunOperator> class SendOverlapped;
typedef SendOverlapped<ESend> SENDOVERLAPPED;

class EdoyunClient : public ThreadFuncBase { // �ͻ�����
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

    operator SOCKET() // ���� SOCKET ����ת�� ��Ҫ���ڸ� AcceptEX() WSARecv() ���ݲ���
    {
        return m_sock;
    }
    operator PVOID() // ���� PVOID ����ת�� ��Ҫ���ڸ� AcceptEX() WSARecv() ���ݲ���
    {
        return &m_buffer[0];
    }
    operator LPDWORD() // ���� LPDWORD ����ת�� ��Ҫ���ڸ� AcceptEX() WSARecv() ���ݲ���
    {
        return &m_received;
    }
    operator LPOVERLAPPED (); // ���� LPOVERLAPPED ����ת�� ��Ҫ���ڸ� AcceptEX()���ݲ���  // �������Ӧ��Ҫ��  ���ص��� ACCEPTOVERLAPPED ��WSARecv()  WSASend()Ӧ��Ҫ������Ĳ���
    LPWSABUF RecvWSABUFFER(); // ��Ҫ���ڸ� WSARecv() ���ݲ���
    LPWSAOVERLAPPED RecvOVERLAPPED(); // ��Ҫ���ڸ� WSARecv() ���ݲ���
    LPWSABUF SendWSABUFFER(); // ��Ҫ���ڸ� WSASend() ���ݲ���
    LPWSAOVERLAPPED SendOVERLAPPED(); // ��Ҫ���ڸ� WSASend() ���ݲ���
    
    DWORD& flags() { return m_flags; } // ��Ҫ���ڸ� WSARecv()���ݲ���
    sockaddr_in* GetLocalAddr() { return &m_laddr;}
    sockaddr_in* GetRemoteAddr() { return &m_raddr;}
    size_t GetBufferSize()const { return m_buffer.size(); }
    int Recv();
    int Send(void* buffer, size_t nSize); // ��Ͷ�ݵ�һ����������ȥ����Ϊ�߲���������¿���Send()�����������
    int SendData(std::vector<char>& data);
private:
    SOCKET m_sock; // ��ͻ���ͨ�ŵ��׽���
    std::vector<char> m_buffer; // ������
    size_t m_used; // �Ѿ�ʹ�õĻ�������С
    sockaddr_in m_laddr; // �ͻ��˵�ַ
    sockaddr_in m_raddr; // Զ�̷�������ַ
    bool m_isbusy; // �ͻ����Ƿ��Ѿ�����
    DWORD m_received; // AcceptEX() WSARecv()Ҫ�õ��Ĳ���
    DWORD m_flags; // WSARecv() Ҫ�õ��Ĳ��� 
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped; // AcceptEX()Ҫ�õ��Ĳ��� Accept��һI/O������Ӧ���ص��ṹָ��
    std::shared_ptr<RECVOVERLAPPED> m_overlapped_recv; // WSARecv()Ҫ�õ��Ĳ��� Receive��һI/O������Ӧ���ص��ṹָ��
    std::shared_ptr<SENDOVERLAPPED> m_overlapped_send; // WSASend()Ҫ�õ��Ĳ��� Send��һI/O������Ӧ���ص��ṹָ��
    EdoyunSendQueue<std::vector<char>> m_vecSend; // �������ݶ���
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

    void BindNewSocket(SOCKET s); // ���µ�ͨ���׽��ֵ�IOCP
    
private:
    void CreateSocket();
    
    int threadIocp(); // IOCP�̺߳���

    CEdoyunThreadPool m_pool; // �̳߳�
    HANDLE m_hIOCP; // ��ȫ�˿ھ��
    SOCKET m_sock; // �������׽���
    sockaddr_in m_addr; // ��������ַ
    std::map<SOCKET, PCLIENT> m_client; // �ͻ����׽���(ͨ���׽���)��ͻ��˶����ӳ��� ÿ����һ���ͻ��˾ͽ���ά����map shared_ptr������ָ�� 
};

