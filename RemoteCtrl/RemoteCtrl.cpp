// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
// 1. 将受控端作为服务器端

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include <conio.h>
#include "EdoyunQueue.h"
#include <MSWSock.h>
#include "EdoyunServer.h"
#include "ESocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 属性配置： 链接器---> 所有选项 ---> 入口点：mainCRTStartup  子系统：窗口(/SUBSYSTEM:WINDOWS)
// 子系统和入口点各有两个选项：
// 注意加空格，+空格+/entry
//1、子系统 窗口 + 入口 窗口入口
// #pragma comment (linker,"/subsystem:windows /entry:wWinMainCRTStartup")// 测试成功 没有控制台   子系统是窗口系统，入口也是窗口入口（开发Windows图像用户界面[GUI]应用程序的启动设置 因为Windows应用程序通常使用 WinMain 或 wWinMain 作为入口点，而不是标准的 main 函数）
//2、子系统 窗口 + 入口 命令行入口
// #pragma comment (linker,"/subsystem:windows /entry:mainCRTStartup")// 测试成功 没有控制台       子系统是窗口系统，入口是命令行(控制台)入口
//3、子系统 控制台 + 入口 命令行入口
// #pragma comment (linker,"/subsystem:console /entry:mainCRTStartup")// 测试成功 有控制台     (纯后台，没有窗口)
//4、子系统 控制台 + 入口 窗口入口
// #pragma comment (linker,"/subsystem:console /entry:wWinMainCRTStartup")// 测试成功 有控制台 （可以在控制台中开启窗口）

// #define INVOKE_PATH _T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")
#define INVOKE_PATH _T("C:\\Users\\Yyh\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe") 

// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

// 项目属性->链接器->清单文件->UAC执行级别改为需要管理员权限：这样每次启动时会提示需要管理员权限运行，避免在远程服务器忘记启动时选择管理员权限  ---0410

bool ChooseAutoInvoke(const CString& strPath) // 开机自动启动：1.通过修改设置注册表 2.复制文件到StartUp
{
	// CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")); // 方法1的路径
	// CString strPath = _T("C:\\Users\\Yyh\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe"); // 方法2的路径
	if (PathFileExists(strPath)) // 如果不是第一次启动，即已设置开机自动启动(注册表有值)，则直接跳过
	{
		return true;
	}
	CString strInfo = _T("该程序只允许用于合法的用途！\n");
	strInfo += _T("继续运行该程序，将是的这台机器处于被监控状态！\n");
	strInfo += _T("如果你不希望这样，请按“取消”按钮，退出程序。\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器上，并随系统启动而自动运行！\n");
	strInfo += _T("按下“否”按钮，该程序只运行一次，不会在系统内留下任何东西！\n");
	int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
	
	if (ret == IDYES) // 用户点击“是”
	{
		// WriteRegisterTable(strPath); // 开机启动方式1：修改注册表
		if (!CEdoyunTool::WriteStarupDir(strPath)) // 开机启动方式2：windows下有一个启动的文件夹，文件夹内的程序开机时都会自动启动，故将RemoteCtrl.exe复制进那个文件夹即可  
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
	}
	else if (ret == IDCANCEL) // 用户点击“取消”
	{
		return false;
	}
	return true;
}

void iocp();

void udp_server();
void udp_client(bool ishost = true);
void initsock()
{
	WSADATA wsa; // 存储关于Winsock的初始化信息 
	WSAStartup(MAKEWORD(2, 2), &wsa); // para1：指定了所需的Winsock库的版本 2.2   para2:接收初始化信息
}

void clearsock()
{
	WSACleanup();
}
int main(int argc, char* argv[])
{
	if (!CEdoyunTool::Init()) return 1; // 初始化失败
	
	// iocp();
	initsock();
	// 简单的控制台应用程序，根据命令行参数的数量来确定程序的执行流程
	if (argc == 1) // 主程序 如果命令行参数数量为1
	{
		char strDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, strDir); // 获取当前目录路径
		STARTUPINFOA si; // 用于启动新进程
		PROCESS_INFORMATION pi; // 用于启动新进程
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		// 将第一个命令行参数（argv[0]，即程序自身路径）添加到字符串 strCmd 中，并附加一个数字 "1"。
		string strCmd = argv[0]; 
		strCmd += " 1"; // 下面启动进程时时 argv = 2 了
		// 创建一个新的进程，传递给它拼接好的命令行参数。这个新进程会在新的控制台窗口中启动 
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, NULL, NULL, strDir, &si, &pi);
		if (bRet)
		{   // 如果创建进程成功，则关闭返回的线程和进程句柄，并打印出新进程的进程ID和线程ID
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID:%d\r\n", pi.dwProcessId);
			TRACE("线程ID:%d\r\n", pi.dwThreadId);
			// 将命令行参数 strCmd 再次修改，附加数字 "2"，并用相同的方式创建第二个新进程
			strCmd += " 2"; // 下面启动进程时时 argv = 3 了
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, NULL, NULL, strDir, &si, &pi);
			if (bRet)
			{ // 如果创建第二个进程成功，则关闭返回的线程和进程句柄，并调用 udp_server() 函数
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID:%d\r\n", pi.dwProcessId);
				TRACE("线程ID:%d\r\n", pi.dwThreadId);
				udp_server(); // udp穿透服务器
			}
		}
	}

	else if (argc == 2) // 如果命令行参数数量为2，表示程序处于客户端状态（子程序）
	{
		udp_client(); // 主机客户端
	}
	else
	{
		udp_client(false); // 从机客户端
	}

	clearsock();
	return 0;
}

class COverlapped {
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator; // 标志这个重叠I/O操作是做什么的，如accpet recv send...
	char m_buffer[4096];
	COverlapped()
	{
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		memset(m_buffer, 0, sizeof(m_buffer));
	}
};

void iocp()
{
	/*
	// SOCKET sock = socket(AF_INET, SOCK_STREAM, 0); // IPv4 TCP 默认选择协议 传统的定义套接字
	// 
	// 想要使用重叠I/O的（实现异步通信）话，初始化Socket的时候一定要使用WSASocket并带上WSA_FLAG_OVERLAPPED参数才可以(只有在服务器端需要这么做，在客户端是不需要的)
	// 设置"WSA_FLAG_OVERLAPPED"参数后，得到的 sock 就是非阻塞的
	SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); // “执行I/O请求的时间与线程执行其他任务的时间是重叠(overlapped)的”
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); // 事先建好的，用于接收连接的，等有客户端连接进来直接把这个Socket拿给它用的那个，是AcceptEx高性能的关键所在

	if (sock == INVALID_SOCKET || client == INVALID_SOCKET)
	{
		CEdoyunTool::ShowError();
		return;
	}
	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4); // 创建完成端口 假定是 2 核 CPU
	if (hIOCP == NULL)
	{
		CEdoyunTool::ShowError();
		return;
	}
	sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	addr.sin_port = htons(9527);
	bind(sock, (sockaddr*)&addr, sizeof(addr));
	listen(sock, 5);

	CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0);  // 将监听套接字与完全端口绑定
	
	DWORD recevied = 0;
	COverlapped overlapped;
	overlapped.m_operator = 1; // 表示accept
	//accept() 返回用于通信的socket
	// para1:服务器socket，用于监听连接请求 para2:用于接收连接的socket para3:接收缓冲区 para4：para3中用于存放数据的空间大小。如果此参数=0，则AcceptEx时将不会待数据到来，而直接返回，如果此参数不为0，那么一定得等接收到数据了才会返回
	// para5:存放本地址地址信息的空间大小 para6:存放本远端地址信息的空间大小 para7：接收到的数据大小 para8：本次重叠I/O所要用到的重叠结构
	if (AcceptEx(sock, client, overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &recevied, &overlapped.m_overlapped) == false)
	{
		CEdoyunTool::ShowError();
	}

	// TODO：开启线程
	while (true) // 代表一个线程(worker)
	{
		LPOVERLAPPED pOverlapped = NULL;
		DWORD transferred = 0;
		DWORD key = 0;
		// 监控完成端口,监视完成端口的队列中是否有完成的网络操作
		// para1:建立的那个唯一的完成端口 para2:操作完成后返回的字节数 para3:自定义结构体参数 para4:重叠结构 para5:等待完成端口的超时时间
		if (GetQueuedCompletionStatus(hIOCP, &transferred, &key, &pOverlapped, INFINITY)) // 函数有返回，说明有需要处理的网络操作   para3:pOverlapped 用于接收发送请求时候重叠结构指针 
		{
			// 重叠结构我们就可以理解成为是一个网络操作的ID号，也就是说我们要利用重叠I/O提供的异步机制的话，每一个网络操作都要有一个唯一的ID号，
			// 因为进了系统内核，里面黑灯瞎火的，也不了解上面出了什么状况，一看到有重叠I/O的调用进来了，就会使用其异步机制，
			// 并且操作系统就只能靠这个重叠结构带有的ID号来区分是哪一个网络操作了，然后内核里面处理完毕之后，根据这个ID号，把对应的数据传上去。
			COverlapped* p0 = CONTAINING_RECORD(pOverlapped, COverlapped, m_overlapped); // 通过重叠结构，拿到了重叠结构的父对象的指针 AcceptEx()中的para8 ---> p0
			switch (p0->m_operator)
			{
				case1: // 处理连接请求
			}
			
		}
	}

	CreateIoCompletionPort((HANDLE)client, hIOCP, 0, 0); // 将连接套接字与完全端口绑定
	*/
	
	CEdoyunServer server;
	server.StartService();
	getchar();
}


// 用于服务器实现的回调函数
int RecvFromCB(void* arg, const CEBuffer& buffer, CESockaddrIn& addr) { // 服务器在受到客户端发来的消息后 将消息回发给客户端  回声(echo)
	CEServer* server = (CEServer*)arg;
	return server->SendTo(addr, buffer);
	
}
int SendToCB(void* arg, const CESockaddrIn& addr, int ret) {
	CEServer* server = (CEServer*)arg;
	printf("sendto done!%p\r\n", server);
	return 0;
}
void udp_server()
{
	std::list<CESockaddrIn> lstclients; // 与udp服务器连接的客户端列表
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	CEServerParameter param("127.0.0.1", 20000, ETYPE::ETypeUDP, NULL, NULL, NULL, RecvFromCB, SendToCB); // UDP 服务器参数设置
	CEServer server(param); // 使用自定义的类来实现UDP服务器
	server.Invoke(&server);
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	getchar();
	return;
	//CESOCKET sock(new CESocket(ETYPE::ETypeUDP)); // 使用自己定义的CESocket类来实例化一个socket

	// para1:IPv4 协议族 para2：套接字的类型，这里是数据报套接字，它是一种面向消息的套接字，使用UDP协议进行通信 para3:使用默认协议
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0); 
	sockaddr_in server, client;
	int len = sizeof(client);
	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	server.sin_family = AF_INET;
	server.sin_port = htons(20000); // UDP端口一般设置大一点 20,000 -> 40,000
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (-1 == bind(sock, (sockaddr*)&server, sizeof(server))) return;

	char buf[4096] = "";
	// recvform()用于从一个数据报套接字接收数据，并且可以指定发送方的地址信息,用于数据报套接字（如UDP）
	// para1: 接收数据的套接字描述符 para2:接收数据的缓冲区 para3:缓冲区的大小 para4:可选的标志，通常设置为 0
	// para5: 指向 sockaddr 结构体的指针，用于存储发送方的地址信息 para6:存储 sockaddr 结构体的长度
	int ret = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&client, &len);
	// recv()用于从连接套接字接收数据，不需要指定发送方的地址信息, 适用于面向连接的套接字（如TCP），因为在TCP通信中，数据是通过连接(socket)传输的，不需要指定发送方的地址信息
	if (ret > 0)
	{
		if (lstclients.size() <= 0) // 第一个连接上来的客户端(主客户端) 做一个 echo 应答(即将主客户端发来的信息发送给主客户端)
		{
			// sendto()用于向指定的目标地址发送数据，适用于数据报套接字（如UDP）可以指定目标地址，而UDP是无连接的协议，每个数据包都需要指定目标地址。
			// para1:发送数据的套接字描述符 para2:发送的数据 para3:发送的数据的长度 para4:可选的标志，通常设置为 0
			// para5:指向 sockaddr 结构体的指针，表示目标地址 // para6:存储 sockaddr 结构体的长度
			ret = sendto(sock, buf, ret, 0, (sockaddr*)&client, len);
			// send()用于向已建立连接的套接字发送数据，适用于面向连接的套接字（如TCP） 因为TCP是面向连接的协议，数据是通过已建立的连接(socket)传输的，不需要指定目标地址
		}
		else // 第二个连接上来的客户端(从客户端)
		{
			memcpy(buf, lstclients.front(), lstclients.front().size()); // 将主客户端的地址信息发送给从客户端
			 ret = sendto(sock, buf, lstclients.front().size(), 0, (sockaddr*)&client, len);
			// send()用于向已建立连接的套接字发送数据，适用于面向连接的套接字（如TCP） 因为TCP是面向连接的协议，数据是通过已建立的连接(socket)传输的，不需要指定目标地址
		}
	}
	else
	{
		printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
	}


	if (*sock == INVALID_SOCKET)
	{
		printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		return;
	}
	
	// std::list<sockaddr_in> lstclients; // 与udp服务器连接的客户端列表
	// memset(&server, 0, sizeof(server));
	// memset(&client, 0, sizeof(client));
	CESockaddrIn server("127.0.0.1", 20000), client; // 使用自己定义的CESockaddrIn类来实例化一个server client 这里在重载bind后 server都可以不要了
	std::list<CESockaddrIn> lstclients; // 与udp服务器连接的客户端列表
	

	// if (-1 == bind(*sock, server, sizeof(server))) // 因为重载了 SOCKET 和  sockaddr*
	if (-1 == sock->bind("127.0.0.1", 20000)) // 因为重载了 SOCKET 和  sockaddr* bind
	{
		printf("%s(%d):%s ERROR(%d)!!!\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError());
		// closesocket(*sock); // 不需要手动close ESocket类对象析构时会调用closesocket()	
		return;
	}
	// char buf[4096] = "";
	CEBuffer buf(1024 * 256);
	int len = sizeof(client);
	int ret = 0;
	while (!_kbhit())
	{
		// recvform()用于从一个数据报套接字接收数据，并且可以指定发送方的地址信息,用于数据报套接字（如UDP）
		// para1: 接收数据的套接字描述符 para2:接收数据的缓冲区 para3:缓冲区的大小 para4:可选的标志，通常设置为 0
		// para5: 指向 sockaddr 结构体的指针，用于存储发送方的地址信息 para6:存储 sockaddr 结构体的长度
		// ret = recvfrom(sock, buf, sizeof(buf), 0, (sockaddr*)&client, &len);
		// ret = recvfrom(*sock, buf, sizeof(buf), 0, client, &len); // 因为重载了 SOCKET 和  sockaddr*
		ret = sock->recvfrom(buf, client);
		// recv()用于从连接套接字接收数据，不需要指定发送方的地址信息, 适用于面向连接的套接字（如TCP），因为在TCP通信中，数据是通过连接(socket)传输的，不需要指定发送方的地址信息
		if (ret > 0)
		{
			if (lstclients.size() <= 0) // 第一个连接上来的客户端(主客户端) 做一个 echo 应答(即将主客户端发来的信息发送给主客户端)
			{
				//client.updata();
				lstclients.push_back(client);
				//CEdoyunTool::Dump((BYTE*)buf, ret);
				// printf("%s(%d):%s ip %08X port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port)); // 打印连接上服务器的客户端ip和端口
				printf("%s(%d):%s ip %s port %d\r\n", __FILE__, __LINE__, __FUNCTION__, client.GetIP().c_str(), client.GetPort()); // 打印连接上服务器的客户端ip和端口
				// sendto()用于向指定的目标地址发送数据，适用于数据报套接字（如UDP）可以指定目标地址，而UDP是无连接的协议，每个数据包都需要指定目标地址。
				// para1:发送数据的套接字描述符 para2:发送的数据 para3:发送的数据的长度 para4:可选的标志，通常设置为 0
				// para5:指向 sockaddr 结构体的指针，表示目标地址 // para6:存储 sockaddr 结构体的长度
				// ret = sendto(*sock, buf, ret, 0, (sockaddr*)&client, len);
				//ret = sendto(*sock, buf, ret, 0, client, len);
				ret = sock->sendto(buf, client);
				// send()用于向已建立连接的套接字发送数据，适用于面向连接的套接字（如TCP） 因为TCP是面向连接的协议，数据是通过已建立的连接(socket)传输的，不需要指定目标地址
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
			else // 第二个连接上来的客户端(从客户端)
			{
				// memcpy(buf, lstclients.front(), lstclients.front().size()); // 将主客户端的地址信息发送给从客户端
				buf.Update((void*)&lstclients.front(), lstclients.front().size());
				// ret = sendto(*sock, buf, lstclients.front().size(), 0, client, client.size());
				ret = sock->sendto(buf, client);
				// send()用于向已建立连接的套接字发送数据，适用于面向连接的套接字（如TCP） 因为TCP是面向连接的协议，数据是通过已建立的连接(socket)传输的，不需要指定目标地址
				printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
			}
		}
		else
		{
			printf("%s(%d):%s ERROR(%d)!!! ret = %d\r\n", __FILE__, __LINE__, __FUNCTION__, WSAGetLastError(), ret);
		}
	}
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__); 
	
}

void udp_client(bool ishost)
{
	Sleep(2000); // 确保服务器先启动
	sockaddr_in server, client; // udp服务器 客户端
	int len = sizeof(client);
	server.sin_family = AF_INET;
	server.sin_port = htons(20000); // UDP端口一般设置大一点 20,000 -> 40,000
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	// para1:IPv4 协议族 para2：套接字的类型，这里是数据报套接字，它是一种面向消息的套接字，使用UDP协议进行通信 para3:使用默认协议
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0); 
	if (-1 == bind(sock, (sockaddr*)&server, sizeof(server))) return;
	if (ishost) { // 主机客户端
		std::string msg = "hello world!\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server)); // 发送消息给 udp 服务器
		if (ret > 0) { // 发送成功 说明此时主客户端已经与udp服务器成功连接
			msg.resize(1024);
			memset((char*)msg.c_str(), msg.size(), 0); // 用于接收udp服务器发来的数据(第一次是echo应答)
			// 注意para5: 指向 sockaddr 结构体的指针，用于存储发送方(这里即udp服务器)的地址信息
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len); 
			// 第二次recv 接收到的是从客户端发来的消息 这时候主客户端就拿到了从客户端的地址信息(存放在client中)，至此主从客户端就可以通信了
			// 注意para5: 指向 sockaddr 结构体的指针，用于存储发送方(这里即udp服务器)的地址信息
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len); 
		}
	}
	else {// 从机客户端
		std::string msg = "hello world!\n";
		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server)); // 发送消息给 udp 服务器
		if (ret > 0) { // 发送成功 说明此时从客户端已经与udp服务器成功连接
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size()); // 用于接收udp服务器发来的数据(另外的客户端的信息)
			// 注意para5: 指向 sockaddr 结构体的指针，用于存储发送方(这里即udp服务器)的地址信息
			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len); 
			if (ret > 0){
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(addr));
				sockaddr_in* paddr = (sockaddr_in*)&addr;
				// 拿到了主客户端的地址信息，就可以基于UDP给他发消息了
				msg = "hello, I'm no.2 client, i got your address from udp server!";
				ret = sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));  
			}
		}
	}
	closesocket(sock);
}


// UDP打洞大致流程：
// 1.建立UDP穿透服务器
// 2.主客户端给UDP服务器发消息，UDP服务器接收到后就有了主客户端的地址信息
// 3.主客户端给UDP服务器发消息，UDP服务器接收到后就有了从客户端的地址信息
// 4.UDP服务器将主客户端的地址信息发送给从客户端,从客户端接收到后就有了主客户端的地址信息
// 5.从客户端给主客户端发消息，主客户端接收到后就有了从客户端的地址信息
// 6.主从客户端都有了对方的地址信息，可以开始双向通信