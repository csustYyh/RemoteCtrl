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
int AcceptOverlapped<op>::AcceptWorker() // Accept���������Ӧ��ִ�к���
{
    //TODO:accept
    int lLength = 0, rLength = 0;
    if (m_client->GetBufferSize() > 0) // client�յ���bufferֵ
    {
        sockaddr* plocal = NULL, * premote = NULL;
        GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
            (sockaddr**)&plocal, &lLength, // �ͻ��˵�ַ 
            (sockaddr**)&premote, &rLength // Զ�̷�������ַ
        );
        memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
        memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
        m_server->BindNewSocket(*m_client); // �������ӵ�ͨ���׽��ֵ�IOCP
        int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABUFFER(), 1, *m_client, &m_client->flags(), m_client->RecvOVERLAPPED(), NULL); // WSARecv()���������recv() ���Ƿ��������IOCP
        if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) // WSA_IO_PENDING�����ڲ���
        {
            // TODO: ����
            TRACE("ret = %d error = %d\r\n", ret, WSAGetLastError());
        }
        // ���ӵ�һ���ͻ��˾�acceptһ��
        if (m_server->NewAccept())
        {
            return -2; // accept����
        }
    }
    return -1; // ��������
}

EdoyunClient::EdoyunClient() 
    :m_isbusy(false), m_flags(0), 
    m_overlapped(new ACCEPTOVERLAPPED()), m_overlapped_recv(new RECVOVERLAPPED()), m_overlapped_send(new SENDOVERLAPPED()), 
    m_vecSend(this, (SENDCALLBACK)&EdoyunClient::SendData)
{
    // ��Ҫʹ���ص�I/O�ģ�ʵ���첽ͨ�ţ�������ʼ��Socket��ʱ��һ��Ҫʹ��WSASocket������WSA_FLAG_OVERLAPPED�����ſ���(ֻ���ڷ���������Ҫ��ô�����ڿͻ����ǲ���Ҫ��)
    // ����"WSA_FLAG_OVERLAPPED"�����󣬵õ��� sock ���Ƿ�������
    m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); // ���Ƚ��õģ����ڽ������ӵģ����пͻ������ӽ���ֱ�Ӱ����Socket�ø����õ��Ǹ�����AcceptEx�����ܵĹؼ�����
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
    // TODO:��������
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
    if (m_vecSend.Size() > 0) // û�з�����
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
    CreateSocket(); // �����������׽��� m_sock
    // ��һ�����ص�ַ�󶨵��׽����ϣ���ָ���׽����������е�ͨ�ŵ�ַ����������������Ϳ���ͨ���õ�ַ���׽���ͨ��
    if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) // ע��IP(m_ip)�Ͷ˿ں�(m_port)�������� m_addr ��
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
    // ���׽��ֱ��Ϊ�����׽��֣������ڽ�������������׽���(�����׽���)
    if (listen(m_sock, 3) == -1) // para1:�׽����ļ�����������Ҫ����Ϊ����״̬���׽��� para2:���Ӷ��е���󳤶ȣ���ͬʱ���Դ����������������(�����µ��������󵽴�ʱ�������Է��� 3 �����󵽵ȴ������У�����������������󽫻ᱻ�ܾ�)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }

    m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4); // ����IOCP
    if (m_hIOCP == NULL)
    {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        m_hIOCP = INVALID_HANDLE_VALUE;
        return false;
    }
    BindNewSocket(m_sock); // �󶨷������׽���(����)��IOCP
    //CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0); // �󶨷������׽���(����)��IOCP
    m_pool.Invoke(); // �����̳߳�
    m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&CEdoyunServer::threadIocp)); // ����IOCP�߳�
    if (!NewAccept()) return false;
    return true;
}

bool CEdoyunServer::NewAccept()
{
    PCLIENT pClient(new EdoyunClient);
    pClient->SetOverlapped(pClient);
    m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
    // para1:������socket�����ڼ����������� para2:���ڽ������ӵ�socket para3:���ջ����� para4��para3�����ڴ�����ݵĿռ��С������˲���=0����AcceptExʱ����������ݵ�������ֱ�ӷ��أ�����˲�����Ϊ0����ôһ���õȽ��յ������˲Ż᷵��
    // para5:��ű���ַ��ַ��Ϣ�Ŀռ��С para6:��ű�Զ�˵�ַ��Ϣ�Ŀռ��С para7�����յ������ݴ�С para8�������ص�I/O��Ҫ�õ����ص��ṹ
    if (AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient) == FALSE) // ���������accept() ���Ƿ��������IOCP
    {
        if (WSAGetLastError() != WSA_IO_PENDING) // ����������� WSA_IO_PENDING ������������
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false; // accept����
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
    WSAStartup(MAKEWORD(2, 2), &WSAData); // �����ϵͳ˵��������Ҫ���ĸ����ļ����øÿ��ļ��뵱ǰ��Ӧ�ó���󶨣��Ӷ��Ϳ��Ե��øð汾��socket�ĸ��ֺ���
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); // ����һ�������ص� I/O��Overlapped I/O�����Ե��׽���
    int opt = 1; // ѡ�������ʾ�������ñ��ص�ַ�Ͷ˿�
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)); // �������������ñ��ص�ַ�Ͷ˿ڵ�ѡ��
    // �ص� I/O ��ָ�ڽ����첽 I/O ����ʱ������Ӧ�ó���ͬʱ�������������������صȴ� I/O ������ɡ����������õ�ַ�Ͷ˿����������׽��ֹرպ������ٴ�ʹ����ͬ�ĵ�ַ�Ͷ˿ڡ�
}

int CEdoyunServer::threadIocp()
{
    DWORD transferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* lpOverlapped = NULL;
    // �����ɶ˿�,������ɶ˿ڵĶ������Ƿ�����ɵ��������
    // para1:�������Ǹ�Ψһ����ɶ˿� para2:������ɺ󷵻ص��ֽ��� para3:�Զ���ṹ����� para4:�ص��ṹ para5:�ȴ���ɶ˿ڵĳ�ʱʱ��
    if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
    {   // AcceptEx WSARecv WSASend ���������socket�󶨵���ȫ�˿�Ͷ���첽 IO ����
        if (CompletionKey != 0)
        {
            // �ص��ṹ���ǾͿ�������Ϊ��һ�����������ID�ţ�Ҳ����˵����Ҫ�����ص�I/O�ṩ���첽���ƵĻ���ÿһ�����������Ҫ��һ��Ψһ��ID�ţ�����ʵ�����ص��ṹ����ĸ��ֳ�Ա��
            // ��Ϊ����ϵͳ�ںˣ�����ڵ�Ϲ��ģ�Ҳ���˽��������ʲô״����һ�������ص�I/O�ĵ��ý����ˣ��ͻ�ʹ�����첽���ƣ�
            // ���Ҳ���ϵͳ��ֻ�ܿ�����ص��ṹ���е�ID������������һ����������ˣ�Ȼ���ں����洦�����֮�󣬸������ID�ţ��Ѷ�Ӧ�����ݴ���ȥ��
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped); // ͨ���ص��ṹ���õ����ص��ṹ�ĸ������ָ��
            pOverlapped->m_server = this;
            switch (pOverlapped->m_operator)
            {
            case EAccept: // ������������
            {
                ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳�����������
            }
            break;
            case ERecv:
            {
                RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳���������
            }
            break;
            case ESend:
            {
                SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳����������
            }
            break;
            case EError:
            {
                ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳���������
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
    // �����ɶ˿�,������ɶ˿ڵĶ������Ƿ�����ɵ��������
    // para1:�������Ǹ�Ψһ����ɶ˿� para2:������ɺ󷵻ص��ֽ��� para3:�Զ���ṹ����� para4:�ص��ṹ para5:�ȴ���ɶ˿ڵĳ�ʱʱ��
    if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
    {   // AcceptEx WSARecv WSASend ���������socket�󶨵���ȫ�˿�Ͷ���첽 IO ����
        if (CompletionKey != 0)
        {
            // �ص��ṹ���ǾͿ�������Ϊ��һ�����������ID�ţ�Ҳ����˵����Ҫ�����ص�I/O�ṩ���첽���ƵĻ���ÿһ�����������Ҫ��һ��Ψһ��ID�ţ�����ʵ�����ص��ṹ����ĸ��ֳ�Ա��
            // ��Ϊ����ϵͳ�ںˣ�����ڵ�Ϲ��ģ�Ҳ���˽��������ʲô״����һ�������ص�I/O�ĵ��ý����ˣ��ͻ�ʹ�����첽���ƣ�
            // ���Ҳ���ϵͳ��ֻ�ܿ�����ص��ṹ���е�ID������������һ����������ˣ�Ȼ���ں����洦�����֮�󣬸������ID�ţ��Ѷ�Ӧ�����ݴ���ȥ��
            EdoyunOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EdoyunOverlapped, m_overlapped); // ͨ���ص��ṹ���õ����ص��ṹ�ĸ������ָ��
            pOverlapped->m_server = this;
            switch (pOverlapped->m_operator)
            {
            case EAccept: // ������������
            {
                ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳�����������
            }
            break;
            case ERecv:
            {
                RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳����������
            }
            break;
            case ESend:
            {
                SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳���������
            }
            break;
            case EError:
            {
                ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
                m_pool.DispatchWorker(pOver->m_worker); // �����̴߳���������
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