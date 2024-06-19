#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc; // .h文件中声明但未初始化的静态成员变量，在.cpp文件中需要定义和初始化

// 单例的解释：https://www.zhihu.com/question/639483371/answer/3383218426
CClientController* CClientController::m_instance = NULL; 

CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) // 静态函数没有 this 指针，所以无法直接访问成员变量
	{
		m_instance = new CClientController();

		// 初始化消息映射表
		struct
		{
			UINT     msg;  // 消息ID
			MSGFUNC  func; // 对应的响应函数
		}MsgFuncs[] =
		{
			//{WM_SEND_PACK, &CClientController::OnSendPack},
			//{WM_SEND_DATA, &CClientController::OnSendData},
			{WM_SHOW_STATUS, &CClientController::OnShowStatus}, 
			{WM_SHOW_WATCH, &CClientController::OnShowWatch},

			{(UINT)-1, NULL}, // 无效
		};
		for (int i = 0; MsgFuncs[i].msg != -1; i++)
		{
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].msg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg); // 创建状态对话框 para1:MFC中的资源ID para2:父窗口的指针
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg; // 将 m_remoteDlg 对象的地址赋值给传入的 pMainWnd 指针
	return m_remoteDlg.DoModal(); // 显示模态对话框，并等待对话框关闭
}

//LRESULT CClientController::SendMessage(MSG msg)
//{
//	MSGINFO info(msg);
//	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // 创建一个事件对象 para2:手动复位 para3：初始状态是非 signaled 的。这意味着任何等待这个事件的线程在事件对象被显式地设置为 signaled 之前都会被阻塞
//	if (hEvent == NULL) return -2;// 事件创建失败
//
//	// 发送一个WM_SEND_MESSAGE消息给别的线程(处理消息循环的线程) 附带参数：MSGINFO结构体(MSG + result)和一个事件
//	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent); // 将一个自定义消息发送给标识为 m_nThreadID 的线程
//	WaitForSingleObject(hEvent, INFINITE); // 在windows下，通过事件来实现线程间的同步
//	CloseHandle(hEvent);
//	return info.result;
//}

// 改为消息机制后，下面两个用于文件下载的函数不再需要，文件下载转移到了CRemoteDlg(view层)中执行，但是这样不是没有实现MVC架构吗？---0409
//void CClientController::threadDownloadFile()
//{
//	// 创建文件流用于保存待下载文件
//	FILE* pFile = fopen(m_strLocal, "wb+");
//	if (pFile == NULL)
//	{
//		// 在 C++ 中，_T("中文") 是宏定义的一种用法，用于在 Unicode 和 ANSI 字符串之间进行转换，以便在不同的编译模式下（Unicode 或 ANSI）使用相同的代码
//		// _T("中文")会在 Unicode 编译模式下被展开为 L"中文"，在 ANSI 编译模式下被展开为 "中文"。
//		// 在 Unicode 编译模式下，L 前缀表示字符串是 Unicode 编码的，每个字符占两个字节；
//		// 而在 ANSI 编译模式下，字符串不带前缀，表示使用本地的 ANSI 字符集，每个字符占一个字节
//		AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！")); // _T("中文") 的作用是根据编译模式选择正确的字符串类型，以确保代码的兼容性 
//		m_statusDlg.ShowWindow(SW_HIDE);
//		m_remoteDlg.EndWaitCursor();
//		return;
//	}
//	CClientSocket* pClient = CClientSocket::getInstance();
//	do {
//		// 发送文件下载命令给M层(CClientSocket类)
//		int ret = SendCommandPack(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
//		// 03 服务器返回待下载文件的大小
//		// 服务器接收到下载文件命令后返回来的第一个包中 .strData 是下载文件的大小长度
//		long long nLength = *(long long*)CClientSocket::getInstance()->GetPacket().strData.c_str(); // 待下载文件的长度
//		if (nLength == 0)
//		{
//			AfxMessageBox("文件长度为0或者无法读取文件！");
//			break;
//		}
//
//		// 04 接收服务器发送过来的文件，下载保存到GetPathName()路径下面
//		long long nCount = 0; // 记录客户端已接收文件的大小
//		while (nCount < nLength)
//		{
//			ret = pClient->DealCommand();
//			if (ret < 0)
//			{
//				AfxMessageBox("文件长度为0或者无法读取文件！");
//				TRACE("传输失败： ret = %d\r\n", ret);
//				break;
//			}
//			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
//			nCount += pClient->GetPacket().strData.size();
//		}
//	} while (false);
//	fclose(pFile);
//	pClient->CloseSocket();
//	m_statusDlg.ShowWindow(SW_HIDE);
//	m_remoteDlg.EndWaitCursor();
//	m_remoteDlg.MessageBox(_T("下载完成！"), _T("完成"));
//
//}
//
//void CClientController::threadDownloadEntry(void* arg)
//{
//	CClientController* thiz = (CClientController*)arg;
//	thiz->threadDownloadFile();
//	_endthread();
//}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) // 当前窗口还存在时
	{
		if (m_watchDlg.isFull() == false) // 更新服务器发来的截屏数据到缓存 若缓存没被读取数据(即定时器没触发将缓存显示到远程dlg上)，则丢帧
		{
			if (GetTickCount64() - nTick < 200) // 保证远程桌面命令最大发送频率为200ms/次  即1s显示5帧 (5FPS)
			{
				Sleep(200 - DWORD(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();
			int ret = SendCommandPack(m_watchDlg.GetSafeHwnd(), 6, false, NULL, 0); // 发送远程桌面请求
			// TODO:添加WM_SEND_PACK_ACK这个消息的响应函数
			if (ret != 1)
			{
				TRACE("获取图片失败！ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}
}

void CClientController::threadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

bool CClientController::SendCommandPack(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam); // 发送命令数据包
	return ret;
}

/* // 事件机制处理通信冲突
int CClientController::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength,
	std::list<CPacket>* plstPacks)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	// 将指令投入队列
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	std::list<CPacket> lstPacks; // 有的命令关心应答结果，这里用一个list存放应答结果
	if (plstPacks == NULL) // 若传入的plstPacks为NULL，说明该命令不关心应答
		plstPacks = &lstPacks;
	pClient->SendPacket(CPacket(nCmd, pData, nLength, hEvent), *plstPacks, bAutoClose);
	CloseHandle(hEvent); // 回收事件句柄 防止资源耗尽
	if (plstPacks->size() > 0) // 有应答结果
	{
		// 不管客户端发出的命令关不关心应答结果，我们都直接返回指令即可
		// 因为若关心应答结果，在对应的业务逻辑调用SendCommandPacket()时，就会传参plstPacks 
		// 如：threadWatchScreen 远程桌面这个命令就关心应答结果，而一些鼠标操作并不关心应答结果
		return plstPacks->front().sCmd; // 返回应答包的指令
	}
	// int cmd = DealCommand();
	// if (bAutoClose)
		// CloseSocket();
	// return cmd;
	return -1;
}
*/

int CClientController::DownFile(CString strPath)
{
	// strPath = "C:\\Users\\Yyh\\Documents\\RemoteCtrl.pdb";
	// 创建了一个文件对话框
		// para1:表示文件对话框是用来打开文件，而不是保存文件。如果值为 TRUE，则表示用来保存文件
		// para2:表示文件对话框将显示所有类型的文件, 也可以传入特定的文件扩展名，如 "*.txt"，表示只显示 .txt 格式的文件
		// para3:用于指定文件对话框中显示的默认文件名 para4:指定了文件对话框在用户指定的文件已经存在时是否显示覆盖提示,这里的参数表示会弹出提示框询问用户是否覆盖
		// para5:指定了对话框的过滤器，通常用于限制用户可以选择的文件类型,由于 para2 使用 "*" 参数表示显示所有类型的文件，因此过滤器为空，表示不限制文件类型
		// para6:这个参数表示对话框的父窗口，通常是指向当前窗口的指针
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg); // 创建了一个文件对话框
	// 显示文件对话框（模态），并等待用户的操作，直到用户关闭对话框为止
	// DoModal()会暂停当前程序的执行，直到用户完成对话框的操作，比如选择了一个文件并点击了对话框中的打开按钮（或者点击了取消按钮），然后返回用户的操作结果
	// 返回值表示用户的操作结果，常用的返回值有 IDOK（用户点击了确定按钮）和 IDCANCEL（用户点击了取消按钮）。
	if (dlg.DoModal() == IDOK) // 用户点击确定
	{
		m_strRemote = strPath; // 远程下载文件路径
		m_strLocal = dlg.GetPathName(); // 本地保存文件路径

		FILE* pFile = fopen(m_strLocal, "wb+"); // 本地二进制方式打开文件流，用于写入文件到硬盘上
		if (pFile == NULL)
		{
			// 在 C++ 中，_T("中文") 是宏定义的一种用法，用于在 Unicode 和 ANSI 字符串之间进行转换，以便在不同的编译模式下（Unicode 或 ANSI）使用相同的代码
			// _T("中文")会在 Unicode 编译模式下被展开为 L"中文"，在 ANSI 编译模式下被展开为 "中文"。
			// 在 Unicode 编译模式下，L 前缀表示字符串是 Unicode 编码的，每个字符占两个字节；
			// 而在 ANSI 编译模式下，字符串不带前缀，表示使用本地的 ANSI 字符集，每个字符占一个字节
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！")); // _T("中文") 的作用是根据编译模式选择正确的字符串类型，以确保代码的兼容性 
			return -1;
		}
		SendCommandPack(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		//SendCommandPack(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile); // 发送
		// m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this); // 开启线程执行下载文件操作
		/*if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		{
			return -1;
		}*/
		m_remoteDlg.BeginWaitCursor(); // 设置主窗口光标为等待状态(沙漏)
		m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg); // 基于主窗口居中
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！"), _T("完成"));
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg); // 远程桌面dlg parent设置为主窗口
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal(); // 设置为模态，直到用户完成对话框的操作，
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadFunc()
{
	MSG msg;
	// 消息循环是经典的 Windows 消息处理模式，它保持程序的执行，等待并处理用户和系统发来的消息。只有在接收到退出消息（通常是关闭窗口的消息）时，循环才会退出，程序结束执行
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		// 获取消息
		if (msg.message == WM_SEND_MESSAGE)
		{
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			// 在这个消息循环线程中，处理完 WM_SEND_MESSAGE 这个消息后，将返回值填入传进来的结构体MSGINFO中的result
			// 同时设置事件，让发送这个消息的线程(正在Wait)知道消息已处理完毕
			if (it != m_mapFunc.end()) // 如果找到了消息对应的成员函数指针，就通过成员函数指针调用对应的成员函数，并传递 msg 作为参数。这里的 (this->*it->second) 是通过成员函数指针调用成员函数的语法。
			{
				pmsg->result = (this->*it->second) (pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam); // WM_SEND_MESSAGE对应的消息响应函数的返回值
			}
			else
			{
				pmsg->result = -1; // 不存在对应的消息
			}
			SetEvent(hEvent); // 在当前线程执行完WM_SEND_MESSAGE对应的逻辑后，设置事件，用于告诉发送这个消息的线程，消息已处理 这样在SendMessage()中就可以wait这个消息是否完成 即实现线程同步
		}
		else
		{
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) // 如果找到了消息对应的成员函数指针，就通过成员函数指针调用对应的成员函数，并传递 msg 作为参数。这里的 (this->*it->second) 是通过成员函数指针调用成员函数的语法。
			{
				(this->*it->second) (msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}


unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc(); // 启动线程函数
	_endthreadex(0);
	return 0;
}

//LRESULT CClientController::OnSendPack(UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	CPacket* pPacket = (CPacket*)wParam;
//	return pClient->Send(*pPacket); // 发送数据包
//}

//LRESULT CClientController::OnSendData(UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	char* pBuffer = (char*)wParam;
//	return pClient->Send(pBuffer, (int)lParam); // 发送数据
//}

LRESULT CClientController::OnShowStatus(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW); // 显示状态对话框 
}

LRESULT CClientController::OnShowWatch(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal(); // 远程桌面对话框设置为阻塞
}