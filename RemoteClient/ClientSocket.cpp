#include "pch.h"
#include "ClientSocket.h"

// 类中静态成员需要使用显式地方式初始化，而非静态成员一般放在构造函数中初始化
// 而静态成员不能在类的构造函数中初始化，因为静态成员是所有这个类实例化的对象或其子类实例化的对象共用的
CClientSocket* CClientSocket::m_instance = NULL; // 初始化 ServerSocket.h 中声明的静态成员变量 m_instance 

CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pserver = CClientSocket::getInstance();

std::string GetErrInfo(int wsaErrCode) // 通过错误代码能够获取相应的人类可读的错误信息 输入 Windows 套接字错误代码，函数将返回与此错误代码相关的错误信息
{
	std::string ret;
	LPVOID lpMsgBuf = NULL; // 定义一个指向消息缓冲区的指针，并初始化为NULL
	FormatMessage( // 将系统错误代码转换为相应的可读字符串
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, // 使用系统错误表中的消息 | 让系统为消息分配内存
		NULL,
		wsaErrCode, // 要获取消息的错误代码
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // 指定语言标识，这里是中性语言和默认子语言
		(LPTSTR)&lpMsgBuf, 0, NULL); //指定输出消息的缓冲区，以及其他一些参数
	ret = (char*)lpMsgBuf; // 将消息缓冲区的内容转换为std::string类型的字符串
	LocalFree(lpMsgBuf); // 释放先前由FormatMessage分配的内存，以避免内存泄漏

	return ret;
}

bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0); // 创建 socket 对象
	if (m_sock == -1) return false;

	// 初始化配置客户端套接字地址信息
	sockaddr_in serv_addr; // 声明一个 sockaddr_in 类型的变量 serv_adr，该结构体用于存储 IPv4 地址和端口信息
	memset(&serv_addr, 0, sizeof(serv_addr)); // 使用 memset 函数将 serv_adr 结构体的内存内容全部设置为零。这是一种初始化结构体的常见方法，以确保结构体的所有字段都被正确设置
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_port = htons(m_nPort);

	if (serv_addr.sin_addr.s_addr == INADDR_NONE)
	{
		AfxMessageBox(_T("指定的IP地址不存在！"));
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1)
	{
		TRACE("连接失败：%d \r\n", errno);
		AfxMessageBox(_T("连接失败！"));
		TRACE("连接失败：%d %s \r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	return true;
}

/* 改消息机制解决冲突之前：事件机制
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{ // para1:客户端发送的命令包  para2：记录服务器的应答包 para3：是否自动关闭套接字通信 用于应答多个包的场景(如远程桌面和文件传输)
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) // 条件2用于避免每次发送命令包时重复开启通信线程
	{
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this); // 启动线程来处理客户端并发的指令
	}
	m_lock.lock(); // 上锁 进行线程间同步	// para2加&代表建立的是一个HANDLE和std::list<CPacket>*
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks)); // 返回的是一个 pair<>, bool
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	m_lstSend.push_back(pack);
	m_lock.unlock(); // 等待一个进程 push_back 之后，再解锁
	
	WaitForSingleObject(pack.hEvent, INFINITE); // 等到 hEvent 为有信号状态，说明远程服务器有了应答，在线程函数中设置了 hEvent
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end()) // 存在应答结果
	{
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
		
	}
	return false;
}
*/

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) // 消息机制解决通信冲突
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut); // 解析命令包
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam); // 打包成 PACKET_DATA 用来给消息响应函数SendPack()传参
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd); // 投递消息以及和消息有关的参数给通信线程 
	if (ret == false) // new出来的命令包，如果消息投递成功，则由通信线程删除，否则自己删除
	{
		delete pData;
	}
	return ret;
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_eventInvoke = ss.m_eventInvoke;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

CClientSocket::CClientSocket():
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true), m_hThread(INVALID_HANDLE_VALUE)
{

	if (InitSockEnv() == FALSE) // 若网络环境初始化失败，显示错误信息，然后调用 'exit(0)' 
	{
		MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误"), MB_OK | MB_ICONERROR);
		exit(0);
	}

	// 通信线程启动：
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL); // 人工重置，必须调用ResetEvent()才能将其恢复为无信号状态，且当其为有信号状态时，等待该事件对象的所有线程均可获得信号变为可调度线程
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID); // 启动线程来处理客户端各个对话框并发的消息
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) // 等待事件超时，即通信线程未启动成功
	{
		TRACE("网络消息处理线程启动失败！\r\n");
	}
	CloseHandle(m_eventInvoke); // 线程启动成功，关闭事件对象

	m_buffer.resize(BUFFER_SIZE); // 设置缓冲区大小
	memset(m_buffer.data(), 0, BUFFER_SIZE); // 初始化缓冲区为全0	

	struct // 设置消息响应函数映射表
	{
		UINT message;
		MSGFUNC func;
	}funcs[] =
	{
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func));
	}
}

/*
void CClientSocket::threadFunc() 
{
	std::string strBuffer; // 客户端接收缓冲区
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str(); // 缓冲区起始地址
	int index = 0; // index 用于追踪缓冲区中待处理数据的位置
	InitSocket();
	while (m_sock != INVALID_SOCKET)
	{
		if (m_lstSend.size() > 0) // 有待发送的数据
		{
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front(); // 按照队列里面的顺序发送命令包 故取队头 
			m_lock.unlock();
			if (Send(head) == false) // 发送失败
			{
				TRACE("发送失败\r\n");
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end())
			{
				std::map<HANDLE, bool>::iterator it0;
				it0 = m_mapAutoClosed.find(head.hEvent);

				do {
					// 发送成功则等待应答
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					if (length > 0 || (index > 0)) // 缓冲区读到了数据，解包
					{
						index += length; // 缓冲区读取了 len 个字节数据，故 buffer 长度 + len
						size_t size = (size_t)index; // 解析缓冲区的所有数据，故将 index 赋值给 len 作为参数传递给下面 Cpacket 类的构造函数 
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) // CPacket的解包构造函数返回的 len 是解包过程中用掉的数据，> 0 说明解包成功
						{
							//TODO：	通知对应的事件 
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) // 若是自动关闭，设置时间来通知已处理完毕
							{
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (length <= 0 && index <= 0) // 发生错误
					{
						CloseSocket();
						SetEvent(head.hEvent); // 等到远程服务器关闭命令之后，再通知事情完成
						if (it0 != m_mapAutoClosed.end())
						{
							//m_mapAutoClosed.erase(it0);
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
						}
						else
						{
							TRACE("异常的情况，没有对应的pair\r\n");
						}
						break; // 通信完毕 接收不到信息 退出循环
					}
				} while (it0->second == false); // 如果不是自动关闭，则一直recv()直到套接字关闭
			}
			m_lock.lock(); // 上锁 防止两个线程同时对 m_lstSend 进行操作 
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock(); // 解锁
			if (InitSocket() == false)
			{
				InitSocket();
			}
		}
		Sleep(1);
	}
	CloseSocket();
}
*/

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke); // 线程启动时设置 m_eventInvoke 为有信号状态
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) // 用消息机制来处理客户端各个dlg对网络通信的请求，避免冲突
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message : %08X \r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam); // 调用对应的消息响应函数 SendPack()
		}
	}
}

// para1:消息类型 para2:请求消息(PacketData)的指针 para3:发送请求消息的窗口句柄
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{// TODO: 定义一个消息的数据结构（数据和数据长度，接收一个应答就关闭还是接收多个应答再关闭[eg:文件下载]） 回调消息的数据结构（HWND MESSAGE）
	
	PACKET_DATA data = *(PACKET_DATA*)wParam; // 参数解析
	delete (PACKET_DATA*)wParam; // 避免内存泄露
	HWND hWnd = (HWND)lParam; // 拿到发送请求消息的窗口句柄

	if (InitSocket() == true)
	{
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0); // 与远程服务器通信，将客户端的请求发给服务器
		if (ret > 0)
		{
			size_t index = 0; // 用于追踪缓冲区中待处理数据的位置
			std::string strBuffer; // 客户端接收缓冲区
			strBuffer.resize(BUFFER_SIZE); // 设置缓冲区大小
			char* pBuffer = (char*)strBuffer.c_str(); // 缓冲区起始地址

			while (m_sock != INVALID_SOCKET) // 有两种通信模式：1.接收一个应答就关闭套接字 2.接收多个应答再关闭[文件下载] 故用 while 循环来处理
			{
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0); // 发送成功则接收远程服务器的应答
				if (length > 0 || (index > 0)) // 收到应答或缓冲区中还有数据 
				{
					index += (size_t)length; // 缓冲区读取了 len 个字节数据，故 buffer 长度 + len 
					size_t nLen = (size_t)index; // 解析缓冲区的所有数据，故将 index 赋值给 nLen 作为参数传递给下面 Cpacket 类的构造函数 
					CPacket pack((BYTE*)pBuffer, nLen); // 解包，将缓冲区中的数据分配给数据包pack中各个成员 pack即可视为应答包
					if (nLen > 0) // CPacket的解包构造函数返回的 nLen 是解包过程中用掉的数据，> 0 说明解包成功
					{
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM) new CPacket(pack), data.wParam); // 得到应答包后，给对应的窗口(hWnd）发送消息，同时传递应答包给hWnd 这里new了一个pack，故在接收方(hWnd)需要delete pack
						if (data.nMode & CSM_AUTOCLOSE) // 如果是自动关闭模式
						{
							CloseSocket();
							return;
						}
						memmove(pBuffer, pBuffer + nLen, index - nLen); // 清理缓冲区中已被解包的 nLen 个数据
						index -= nLen; // 缓冲区待处理的数据长度 - nLen
					}
				}
				else // 通信结束 套接字关闭或者网络异常
				{
					TRACE("recv failed! length: %d, index: %d\r\n", length, index);
					CloseSocket();
				}
			}
		}
		else
		{
			CloseSocket();
			// 网络终止处理
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1); // 给对应的窗口(hWnd）发送消息 NULL + -1 表示发送失败
		}
	}	
	else
	{
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2); // 给对应的窗口(hWnd）发送消息 NULL + -2 表示初始化套接字失败
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0; // 发送
}

