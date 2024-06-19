#pragma once
#include "Resource.h"
#include <map>
#include <list>
#include <direct.h>
#include <io.h>
#include <list>
#include <atlimage.h>
//#include "ServerSocket.h" // 解耦命令处理与通信后，在Command类中不再需要ServerSocket.h
#include "Packet.h"
#include "LockDialog.h"
#include "EdoyunTool.h"
#pragma warning(disable:4966) // fopen sprintf strcpy strstr


class CCommand // 将命令的执行和对应功能函数的调用过程完全抽象出来，ExcuteCommand()里面不在涉及任何具体功能函数的调用，全部通过命令-函数映射表来解决
{
public:
	CCommand(); // 默认构造函数，使用映射表方式对所有远程操作进行映射
	~CCommand() {}

public:
	// 即 CServerSocket 中的 m_callback， 用于执行命令的回调函数，来解耦命令执行与网络通信
	// para1:执行命令的对象，这里即CCommand对象 para2:命令 para3:用于储存需要反馈回客户端的数据的lstPackets容器 para4:输入的数据包
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) // 运行命令， 用于暴露给CServerSocket类使用
	{
		CCommand* thiz = (CCommand*)arg;
		if (status > 0)
		{
			int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
			if (ret != 0)
			{
				TRACE("执行命令失败： %d ret = %d \r\n", status, ret);
			}
		}
		else
		{
			MessageBox(NULL, _T("无法正常接入用户，自动重连..."), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
		}
	}

	// 参数 2 3 的用处：为了解耦命令处理与网络通信过程，将网络通信的相关工作全部交由CServerSocket处理
	// 原来耦合的逻辑：在处理命令时，使用socket接收客户端传来数据包->处理命令->使用socket发送处理后的数据包
	// 解耦后的逻辑：传来的数据包和要发送的数据包全部以参数的形式处理，在执行命令过程中不再接收/发送数据，也就不涉及到网络通信了,只关心数据的处理
	// para1:命令 para2:用于储存需要反馈回客户端的数据的lstPackets容器的引用 para3:输入的数据包，因为某些命令需要用到客户端发来的带有路径值的信息
	// 返回值：成功返回 0 失败返回非 0
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPcaket); // 执行命令

protected:
	// CCommand::* 表示成员函数指针将指向 CCommand 类的成员函数 CMDFUNC：定义的新类型的名称，可以在代码中使用 CMDFUNC 来代表这个特定的成员函数指针类型
	// (para1, para2)：这表示成员函数指针可以接受的参数列表。para1:数据包list表，该表中的数据包用来返回给客户端 para2:数据包
	// int：成员函数指针指向的函数的返回类型
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); // CMDFUNC成员函数指针 
	std::map<int, CMDFUNC> m_mapFunction; // 成员函数映射表 从命令号到对应的功能函数的映射 可以使用整数作为键来检索对应的成员函数指针
	CLockDialog dlg; // 创建了 CLockDialog 类的一个实例对象，命名为 dlg CLockDialog 类继承自 MFC 的对话框类
	unsigned threadid; // 创建一个线程id，用于存放执行锁屏命令的子线程的线程号，以便于主线程在执行解锁命令时才能将解锁指令发送给子线程

protected:
	// 创建磁盘分区的信息，用于查看文件 查看本地磁盘分区，并把信息打包成数据包 成功返回 0 失败返回 -1
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string result;

		for (int i = 1; i <= 26; i++) // 遍历磁盘符
		{
			if (_chdrive(i) == 0) // 切换成功，说明当前磁盘符对应的磁盘存在，利用切换磁盘分区的方法来遍历磁盘符 _chdrive()用于改变当前的驱动器，参数是要切换到的驱动器号(1->A 2->B ... 26->Z)，成功返回0，失败返回-1
			{
				result += ('A' + i - 1); // 得到盘符
				if (result.size() > 0)
					result += ','; // 磁盘分区不唯一，用','隔开 对于每个盘，不管后面还有没有盘，都要加',' 因为客户端那边的逻辑是根据','来解析
			}
		}
		// 将一个新的 CPacket 对象(打包Command处理后的数据)添加到 lstPacket 的末尾
		// 这样就解耦服务端的数据通信与命令处理，将通信相关的实现全部交给CServerSocket
		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size())); 
		//CPacket pack(1, (BYTE*)result.c_str(), result.size()); // 打包
		//CEdoyunTool::Dump((BYTE*)pack.Data(), pack.Size());
		//CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	// 遍历磁盘文件，并将得到的文件信息打包成数据包
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		// std::list<FILEINFO> lstFileInfos; // 文件信息列表 列表中的数据类型是自定义的 FILEINFO 【文件夹或文件过多会导致列表过大，不便于一次性传送给客户端】
		// 之前耦合的逻辑，还要在执行命令时进行网络通信
		//if (CServerSocket::getInstance()->GetFilePath(strPath) == false)  // 拿到路径 
		//{
		//	OutputDebugString(_T("当前的命令，不是获取文件列表，命令解析错误！"));
		//	return -1;
		//}

		if (_chdir(strPath.c_str()) != 0) // 通过切换到输入的路径来判断是否存在该路径  _chdir()用于改变当前的工作目录,成功返回0，失败返回-1
		{
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo)); // 返回一个空给客户端
			// CServerSocket::getInstance()->Send(pack);
			OutputDebugString(_T("没有权限访问此目录！"));
			return -2;
		}

		_finddata_t fdata; // _finddata_t 结构体用于存储文件的信息
		int hfind = 0; // 文件句柄
		if ((hfind = _findfirst("*", &fdata)) == -1) // 用于在文件系统中查找文件或目录的函数，通常用于在目录中枚举文件 para1:一个表示文件名或者匹配规则的字符串 para2:一个指向 _finddata_t 结构体的指针，用于存储查找到的文件的信息。
		{ // _findfirst()会返回一个非负整数，被称为 "文件句柄"（file handle）。如果查找失败，则返回 -1。通常会用这个句柄作为 _findnext() 函数的参数，以便遍历目录中的所有匹配项
			OutputDebugString(_T("在指定文件夹没有找到任何文件！"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo)); // 返回一个空给客户端
			// CServerSocket::getInstance()->Send(pack);
			return -3;
		}
		// 循环得到遍历文件
		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; // fdata.attrib & _A_SUBDIR 若为 true 说明是一个文件夹，为 false 说明是文件
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			// lstFileInfos.push_back(finfo); // 将当前文件信息追加到文件列表
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo)); // 每获取一个文件信息直接 send 给客户端
			// CServerSocket::getInstance()->Send(pack);
		} while (_findnext(hfind, &fdata) == 0); // 用于在文件系统中查找文件或目录并获取下一个匹配项的信息 para1:当前文件句柄 para2：一个指向 _finddata_t 结构体的指针，用于存储下一个匹配项的信息 成功返回0，失败返回-1

		// while 循环过后说明所有文件读取完毕，发送一个 finfo 给客户端，HasNext 设置为 false，客户端接收到后即可得知不用再等待是否还有文件信息没有传送过来
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		// CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	// 运行本地文件，通过数据包中的路径，执行文件
	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData; // 拿到待执行文件的路径(解耦后的逻辑)
		//CServerSocket::getInstance()->GetFilePath(strPath); // 拿到待执行文件的路径（解耦前的逻辑）
		// ShellExecute()用于启动或打开外部应用程序或文件,'A' 表示 ANSI 字符集
		// para1:窗口句柄  这里填NULL，表示不指定父窗口 para2:用于指定进行的操作，当为NULL时，表示默认打开 para3:可执行文件完整路径
		// para4:可执行文件的命令行，这里设置为NULL para5:指定默认目录 para6：初始化显示方式,模拟正常打开文件使用 SW_SHOWNORMAL
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

		// 执行命令后应答回客户端
		lstPacket.push_back(CPacket(3, NULL, 0));
		// CPacket pack(3, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	// 下载文件，获取数据包中的路径，使用文件操作函数读取文件到缓冲区，并打包发送回客户端
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData; // 拿到待下载文件的路径(解耦后的逻辑)
		//CServerSocket::getInstance()->GetFilePath(strPath); // 拿到待下载文件的路径(解耦前的逻辑)

		long long data = 0; // 文件大小
		// fopen()以某种方式打开指定文件,如果文件打开失败，它会返回 NULL,开发者需要在后续的代码中检查返回值来处理错误
		// fopen_s是 C11 标准引入的安全版本，旨在提高对错误的检测和处理。这个函数要求提供文件指针的地址，以便在发生错误时能够设置为 NULL，而不是直接返回文件指针。这有助于避免潜在的空指针引用问题。
		// para1:文件指针out型 para2:文件路径 para3:打开方式，这里以二进制读的方式打开，为  ”rb”
		// 函数成功执行时，返回值为 0，此时文件已成功打开并且文件指针已被设置。而在函数执行失败时，返回值将是一个非零的错误代码
		// FILE* pFile = fopen(strPath.c_str(), "rb");
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0) // 文件打开失败
		{
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			//CPacket pack(4, (BYTE*)&data, 8);
			//CServerSocket::getInstance()->Send(pack);
			return -1;
		}

		if (pFile != NULL)
		{
			// 计算文件大小
			fseek(pFile, 0, SEEK_END); // fseek()设置文件指针stream的位置 para1:文件指针 para2：偏移，null/0 para3:将文件指针指向某个位置，SEEK_SET表示指向文件头，SEEK_END表示指向文件尾
			data = _ftelli64(pFile); // 使用 _ftelli64() 获取当前文件位置指针的偏移量，从而得知文件的大小	
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			// CPacket head(4, (BYTE*)&data, 8);
			// CServerSocket::getInstance()->Send(head);
			fseek(pFile, 0, SEEK_SET); // 将文件指针复原回文件头

			// 发送文件
			char buffer[1024] = ""; // 待发送文件缓冲区
			size_t rlen = 0; // 每次发送文件的大小
			do
			{
				// 将文件指针指向的数据以数据流的方式读取到目标缓冲区
				// para1:指定的缓冲区 para2:一次读取的宽度 para3:读取的次数 para4:文件流
				// 返回值：返回成功读取的对象个数，若出现错误或到达文件末尾，则可能小于count
				rlen = fread(buffer, 1, 1024, pFile);
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
				// CPacket pack(4, (BYTE*)buffer, rlen);
				// CServerSocket::getInstance()->Send(pack);

			} while (rlen >= 1024);
			fclose(pFile);
		}
		else // 文件为空，可能是无访问权限的情况
		{
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
		}

		// 再发送一个终止包告诉客户端文件已发送完
		lstPacket.push_back(CPacket(4, NULL, 0));
		// CPacket pack(4, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	// 删除文件
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData; // 拿到待删除文件的路径(解耦后的逻辑)
		//CServerSocket::getInstance()->GetFilePath(strPath); // 拿到待删除文件的路径(解耦前的逻辑)
		//DeleteFileA(strPath.c_str()); 

		TCHAR sPath[MAX_PATH] = _T(""); // TCHAR 是一个在不同编译选项下能够自动转换为字符类型（char 或 wchar_t）的类型 声明了一个 TCHAR 类型的字符数组 sPath，用于存储文件路径
		// 
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR)); // 用于将多字节字符文件路径转换为宽字符，并存储在 sPath 中
		DeleteFile(sPath); // 是 Windows API 中的一个函数，用于删除指定路径的文件

		// 再发送一个终止包告诉客户端文件已删除
		lstPacket.push_back(CPacket(9, NULL, 0));
		// CPacket pack(9, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	// 处理鼠标操作，成功返回0，失败返回-1
	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV)); // // 成功获取到客户端发来的鼠标操作事件(解耦后的逻辑)
		// if (CServerSocket::getInstance()->GetMouseEvent(mouse))// 成功获取到客户端发来的鼠标操作事件
		DWORD nFlags = 0; // 鼠标按键及其动作的标志

		switch (mouse.nButton)
		{
		case 0: // 左键
			nFlags = 1; // 低位表示左右中键
			break;
		case 1: // 右键
			nFlags = 2;
			break;
		case 2: // 中键
			nFlags = 4;
			break;
		case 4: // 没有按键，单纯的鼠标移动
			nFlags = 8;
			break;
		}
		if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); // 用于设置鼠标的屏幕坐标位置,返回值是一个布尔值，表示函数是否执行成功。如果成功，返回非零值；否则返回零)
		switch (mouse.nAction)
		{
		case 0: // 单击
			nFlags |= 0x10; // 高位表示动作 如：0x11 表示左键单单击
			break;
		case 1: // 双击
			nFlags |= 0x20;
			break;
		case 2: // 按下
			nFlags |= 0x40;
			break;
		case 3: // 松开
			nFlags |= 0x80;
			break;
		default: // 没有动作
			break;
		}
		switch (nFlags) // 上面两种 case 共有 12 种组合
		{
			// mouse_event()用于模拟鼠标事件。它允许程序通过代码触发鼠标的移动、点击、释放等操作，而不是依赖于实际的物理鼠标设备
			// 函数原型： VOID mouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);
			// para1:定义鼠标事件的类型，可以是下列常量之一或它们的组合：
			//	MOUSEEVENTF_ABSOLUTE: 使用绝对坐标，鼠标位置将被映射到整个屏幕。
			//	MOUSEEVENTF_MOVE : 移动鼠标。
			//	MOUSEEVENTF_LEFTDOWN : 左键按下。
			//	MOUSEEVENTF_LEFTUP : 左键释放。
			//	MOUSEEVENTF_RIGHTDOWN : 右键按下。
			//	MOUSEEVENTF_RIGHTUP : 右键释放。
			//	MOUSEEVENTF_MIDDLEDOWN : 中键按下。
			//	MOUSEEVENTF_MIDDLEUP : 中键释放。
			// para2 & para3: 如果 MOUSEEVENTF_ABSOLUTE 被设置，这些参数指定的是绝对坐标位置；否则，它们指定相对于当前鼠标位置的水平和垂直移动
			// para4: 如果事件是鼠标轮滚动事件，这个参数指定滚动的数量。正值表示向前滚动，负值表示向后滚动
			// para5: 一个指定附加信息的值，通常为0
		case 0x21: // 左键双击  进入这个case后，会执行一次左键按下松开(视为一次点击操作),然后还会继续执行下面的 case 0x11 对应的代码，故可视为双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo()); // GetMessageExtraInfo() 可以用来获取鼠标事件的附加信息，通常是一个设备标识符或其他有关鼠标事件的信息
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11: // 左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41: // 左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81: // 左键松开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22: // 右键双击 
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12: // 右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42: // 右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82: // 右键松开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24: // 中键双击  
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14: // 中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44: // 中键按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84: // 中键松开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08: // 无按键，单纯的鼠标移动
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		// 操作完后，服务器给客户端发送应答包，表明服务器已收到并处理了鼠标相关的消息
		lstPacket.push_back(CPacket(5, NULL, 0));
		// CPacket pack(5, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);


		return 0;
	}

	// 发送屏幕图片
	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CImage          screen;                               // CImage 类的对象，该对象可以用于处理图像的各种操作  GDI(图形设备接口)
		HDC hScreen = ::GetDC(NULL);                  // 获取整个屏幕的设备上下文(屏幕及其所有设置)句柄，并存储在 hScreen 中，HDC 是设备上下文句柄的类 ::显式指定了调用的是全局命名空间中的函数，可以提高代码的可读性和明确性
		int nBitPerPixl = GetDeviceCaps(hScreen, BITSPIXEL);  // 获取设备属性，颜色深度即每个像素使用的位数 颜色深度决定了屏幕上每个像素可以表示的颜色数量。常见的颜色深度包括 8 位（256 色）、16 位（65,536 色）、24 位（约 16.7 百万色）和 32 位（约 16.7 百万色，包括透明度信息） RGB8888 32bits  RGB888 不考虑透明度 24bits
		int nWidth = GetDeviceCaps(hScreen, HORZRES);    // 宽
		int nHeight = GetDeviceCaps(hScreen, VERTRES);    // 高
		screen.Create(nWidth, nHeight, nBitPerPixl);          // 创建屏幕 按照显示器的宽、高和颜色深度来创建

		// para1:获取 screen 对象的设备上下文句柄，以便进行图形绘制 para2&3:目标矩形的左上角坐标 para3&4:目标矩形的宽度和高度，表示截取的屏幕区域的大小
		// para5:源设备上下文句柄，通常是表示整个屏幕的设备上下文 para6&7:源矩形的左上角坐标，通常是整个屏幕的左上角 para8:表示使用源矩形的内容覆盖目标矩形的内容。这是一个常用的拷贝操作
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  // 把图像复制到 screen 中，相当于截图
		::ReleaseDC(NULL, hScreen);                                          // 资源释放

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);              // 在内存中分配一个全局的堆 para1:指定分配的内存块是可移动的 para2:分配的内存块的大小为零字节,后续动态调整大小 返回：句柄用于访问分配的内存块  
		if (hMem == NULL) return -1;
		IStream* pStream = NULL;                                   // 建立内存流 声明一个指向 IStream 接口的指针，初始化为 NULL
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream); // 在全局对象上设置内存流 将全局内存块的句柄转换为 IStream 接口，这样就可以通过 IStream 接口来读写全局内存块

		if (ret == S_OK)
		{
			screen.Save(pStream, Gdiplus::ImageFormatPNG);               // 保存图像到内存流中
			//screen.Save(_T("test2024.png"), Gdiplus::ImageFormatPNG);  // 保存图像到文件png中
			//screen.Save(_T("test2024.jpg"), Gdiplus::ImageFormatJPEG); // 保存图像到文件jpg中
			LARGE_INTEGER  bg = { 0 };					  // LARGE_INTEGER 结构体 bg 设置为零表示相对于流的开头的偏移
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);             // 从头开始设置，把流指针指向头
			PBYTE          pData = (PBYTE)GlobalLock(hMem);   // 上锁，用于读取数据 使用 GlobalLock 函数锁定全局内存块，并将指针 pData 指向这块内存。这是为了在后续的代码中读取内存块中的数据
			SIZE_T         nSize = GlobalSize(hMem);          // 获取全局内存块的大小
			lstPacket.push_back(CPacket(6, pData, nSize));
			// CPacket pack(6, pData, nSize);                        // 封装成包
			// CServerSocket::getInstance()->Send(pack);             // 发送
			GlobalUnlock(hMem);									  // 使用 GlobalUnlock 函数解锁全局内存块，允许其他线程或程序对该内存块进行访问
		}

		// 资源释放
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();

		return 0;
	}

	/*************************************锁机*****************************************************/
	// 锁机函数 
	void threadLockDlgMain()
	{
		dlg.Create(IDD_DIALOG_INFO, NULL); // 创建锁机信息对话框 para1:对话框的资源ID para2:没有父窗口
		dlg.ShowWindow(SW_SHOW); // 用于显示先前创建的对话框 dlg para1:将对话框设置为可见状态

		// 限制鼠标活动范围
		CRect rect; // 创建一个 CRect 对象 rect，表示屏幕的矩形区域
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN); // 屏幕的宽
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN); // 屏幕的高
		rect.bottom = LONG(rect.bottom * 1.10); // 屏幕的高
		dlg.MoveWindow(rect); // 将锁屏对话框 dlg 移动到全屏

		// 将锁机对话框中的提示文字居中
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC); // IDC_STATIC 是提示文字的ID
		if (pText)
		{
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width();
			int x = (rect.right - nWidth) / 2;
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}

		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); // 将 dlg 置于窗口最顶层，使其成为最上层窗口 para6:不改变窗口的大小和位置
		ShowCursor(false); // 隐藏鼠标光标，使其在屏幕上不可见
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE); // 隐藏系统任务栏 FindWindow(）用于找到系统任务栏的窗口句柄
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect); // 使用 ClipCursor 函数限制鼠标的移动范围，将其限制在屏幕的左上角一个像素的区域，这样可以防止鼠标离开这个小区域，从而实现锁定鼠标的效果

		MSG msg; // 用于存储从消息队列中检索到的消息 

		// 这个消息循环是经典的 Windows 消息处理模式，它保持程序的执行，等待并处理用户和系统发来的消息。只有在接收到退出消息（通常是关闭窗口的消息）时，循环才会退出，程序结束执行
		while (GetMessage(&msg, NULL, 0, 0)) // 进入一个消息循环，使用 GetMessage 函数从消息队列中获取消息
		{ // MFC 的对话框也是依赖于消息循环的
			TranslateMessage(&msg); // 对键盘消息进行翻译。这是为了确保正确的键盘消息被发送到消息队列
			DispatchMessage(&msg); // 将消息发送到窗口过程（Window Procedure）进行处理 窗口过程是窗口处理消息的回调函数，通常是在对话框或窗口类中实现的。通过调用 DispatchMessage，消息被传递到窗口过程中进行处理，以执行相应的操作
			if (msg.message == WM_KEYDOWN) // 在消息循环中检测是否有键盘按键按下，并在检测到按键按下时销毁对话框并结束消息循环，从而终止程序执行
			{
				TRACE("msg:%08x wparam:%08x lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x1B) // 按 ESC 退出
				{
					break;
				}
			}
		}
		ClipCursor(NULL); // 解除对鼠标范围的限制
		ShowCursor(true); // 显示鼠标光标
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); // 显示系统任务栏 FindWindow(）用于找到系统任务栏的窗口句柄

		dlg.DestroyWindow(); // 销毁锁机信息对话框
	}

	// 启动子线程中介跳转，用于锁机  这里声明为静态，故在下面的 LockMachine() 函数中，需要传入当前对象的 this 指针
	static unsigned _stdcall threadLockDlg(void* arg) 
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}

	// 锁机，通过创建线程，让线程创建一个大于整个屏幕的对话框，阻挡鼠标点击操作，并将鼠标固定在一个位置，另外做一个消息循环，用于恢复锁机
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) // 锁屏信息对话框若不存在，说明未执行锁屏功能，故创建子进程来执行锁屏指令了
		{
			// 在新线程中执行锁定屏幕的功能，可以保持主线程的响应性，使用户能够继续与应用程序进行交互，不然卡在这里不能接收控制方传来的消息了
			// 使用新线程的优势在于避免主线程被长时间的任务阻塞，从而保持应用程序的交互性
			// 启动一个新的线程执行 threadLockDlg 函数。新线程独立于主线程运行，可以处理锁定屏幕的任务而不影响主线程的正常运行
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid); 
		}
		// 锁屏信息对话框若已存在，说明已经执行锁屏功能，就不用重复创建子进程来执行锁屏指令了

		lstPacket.push_back(CPacket(7, NULL, 0));
		// CPacket pack(7, NULL, 0); // 应答给客户端(控制方)的包
		// CServerSocket::getInstance()->Send(pack); // 应答

		return 0;
	}
	/**********************************************************************************************/

	// 解锁，向锁机线程发送post消息结束锁机子线程的消息循环
	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		// 将消息发送到指定线程的消息队列里 para1:指定的线程id para2:指定的消息类型 para3:消息w,一般是一些常量值 para4:消息l,一般是传指针
		// 返回值：如果函数调用成功，返回非零值。如果函数调用失败，返回值是零
		PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0); // 向指定的锁屏对话窗口发送一个自定义消息 通过代码方式模拟发送了一个键盘按下的消息，具体是模拟了按下 ESC 键的操作
		lstPacket.push_back(CPacket(8, NULL, 0));
		// CPacket pack(8, NULL, 0); // 应答给客户端(控制方)的包
		// CServerSocket::getInstance()->Send(pack); // 应答

		return 0;
	}

	// 连接测试
	int TestConnet(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		lstPacket.push_back(CPacket(1981, NULL, 0));
		// CPacket pack(1981, NULL, 0);
		// CServerSocket::getInstance()->Send(pack); // 应答

		return 0;
	}
};

