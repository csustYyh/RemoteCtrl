#pragma once
class CEdoyunTool
{

public:
	// 将字节数组中的每个字节以十六进制的形式格式化，并按照每行显示16个字节的格式输出到调试输出。这种格式化的输出通常用于调试目的，方便查看二进制数据的内容。
	static void Dump(BYTE* pData, size_t nSize)
	{
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) // 循环遍历每个字节
		{
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n"; // 每 16 个字节换行
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); // 将每个字节格式化为两位的十六进制字符串
			strOut += buf; // 追加到 strOut
		}

		strOut += "\n";
		OutputDebugStringA(strOut.c_str()); // 将调试信息输出到调试器（如 Visual Studio 的输出窗口）中
	}

	static void ShowError() // 获取最近一次发生的错误信息，并将其输出到调试输出窗口
	{
		LPWSTR lpMessageBuf = NULL; // 声明了一个空的 Unicode 字符串指针 
		// strerror(errno); // 标准C语言库
		FormatMessage( // 格式化一个消息字符串，根据错误代码生成对应的错误消息文本
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf); // 将错误消息输出到调试输出窗口
		LocalFree(lpMessageBuf); // 释放 FormatMessage() 分配的缓冲区，以避免内存泄漏
	}


	static bool IsAdmin() // 检查当前进程是否拥有管理员权限
	{   // 1.拿当前进程的token 2.拿token中的信息 3.根据信息判断当前进程是否拥有管理员权限
		HANDLE hToken = NULL;
		// 打开当前进程的访问令牌，并将其句柄存储在 hToken 变量中
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) // para1:获取当前正在执行的进程的句柄 para2:表示只需要查询访问令牌的信息而不做更改 para3:接收打开的访问令牌的句柄
		{
			ShowError();
			return false;
		}

		// 检查当前进程是否以提权(管理员权限)运行
		TOKEN_ELEVATION eve; // 存储有关令牌提升状态的信息
		DWORD len = 0; // 存储调用 GetTokenInformation 函数后返回的信息长度
		// 获取当前进程的访问令牌信息
		// para1:当前进程的访问令牌句柄 para2:指定要获取的令牌信息类型，表示提升（管理员）权限的信息 para3:存储获取到的提升状态信息 para4:存储实际返回的信息长度
		if (!GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len))
		{
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(hToken)) // 检查返回的信息长度是否与令牌句柄的大小相同。如果相同，说明函数成功返回了提升状态信息
		{
			return eve.TokenIsElevated; // 返回 TOKEN_ELEVATION 结构体中的 TokenIsElevated 成员，该成员表示当前进程是否以提升的权限运行
		}
		// 若不等，则没有成功返回提升状态信息
		printf("length of tokeninformation is %d\r\n", len);
		return false;
	}

	static bool RunAsAdmin() // 获取管理员权限->使用该权限创建一个以管理员权限运行的进程    ps:视频课程实际实现时，是直接调用CreateProcessWithLogonW()函数让管理员用户来执行RemoteCtrl.exe
	{
		// 获取管理员权限
		/*HANDLE hToken = NULL;
		bool ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
		if (!ret)
		{
			ShowError();
			MessageBox(NULL, _T("登录错误！"), _T("程序错误！"), 0);
			::exit(0);
		}
		OutputDebugString(L"Logon adminiastrator sucess!\r\n");
		MessageBox(NULL, _T("登录成功！"), _T("成功"), 0);*/

		// 创建一个以管理员权限运行的进程来运行 RemoteCtrl.exe
		// 使用指定的访问令牌(上面获取的有管理员权限的)创建一个新进程，并等待该进程的完成
		STARTUPINFO si = { 0 }; // STARTUPINFO 结构体包含了一个新进程的一些属性，例如窗口外观、标准输入输出的重定向等信息
		PROCESS_INFORMATION pi = { 0 }; // PROCESS_INFORMATION 结构体包含了一个新进程的一些信息，例如进程的句柄、线程的句柄等
		TCHAR sPath[MAX_PATH] = _T(""); // 用于存储当前进程的目录路径

		// 本地策略组 开启Admin账户 禁止空密码之能登录本地控制台
		// GetCurrentDirectory(MAX_PATH, sPath); // 获取当前进程的目录路径
		// CString strCmd = sPath;
		// strCmd += _T("\\RemoteCtrl.exe"); // 将字符串 strCmd 后面追加上 \RemoteCtrl.exe，构成完整的可执行文件路径
		// para1:访问令牌句柄（有管理员权限） para2:标志，指示创建的进程应该使用令牌的用户配置文件 para3:要执行的可执行文件路径 para4:标志，指示使用 Unicode 环境
		// para5:指定新进程的一些属性 para6:接收新进程的信息
		//ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); // 创建一个新的进程执行RemoteCtrl.exe
		// CloseHandle(hToken);

		GetModuleFileName(NULL, sPath, MAX_PATH); // // 获取当前进程已加载模块的文件的完整路径，该模块必须由当前进程加载
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); // 创建一个新的进程，并使用另一个用户(管理员)的权限来执行指定的可执行文件或命令
		if (!ret)
		{
			ShowError();
			MessageBox(NULL, _T("创建进程失败！"), _T("程序错误！"), 0);
			return false;
		}

		WaitForSingleObject(pi.hProcess, INFINITE); // 等待新进程的主线程完成执行
		CloseHandle(pi.hProcess); // 关闭新进程的句柄，释放资源
		CloseHandle(pi.hThread); // 关闭新进程的主线程句柄，释放资源
		
		return true;
	}

	// 设置开机自动启动 ---0410
	// 开机启动的时候，程序的权限是跟随启动用户的
	// 如果两者权限不一致，则会导致程序启动失败
	// 开机启动对环境变量有影响，如果依赖dll(动态库)，则可能启动失败 
	// 解决方法：
	// 【复制这些dll到system32下面或者sysWOW64下面】
	// system32下面，多是64位程序，syswow64下面多是32位程序
	// 【使用静态库而非动态库】
	static bool WriteRegisterTable(const CString& strPath) // 修改注册表
	{
		// 将"RemoteCtrl.exe"拷贝到系统目录下
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret == FALSE)
		{
			MessageBox(NULL, _T("复制文件失败，是否权限不足？\r\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		// 加到注册表中，让其开机自动启动
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey); // 拿到键
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机失败!是否权限不足？ \r\n程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		// CString strPath = CString(_T("%SystemRoot%\\SysWOW64\\RemoteCtrl.exe"));
		// 设置注册表中指定项的值
		// para1:打开的注册表项的句柄 para2:要设置的注册表项的名称 para3:保留参数，通常为0
		// para4:指定数据类型，这里是 REG_EXPAND_SZ，表示要设置的值是可扩展的字符串(允许在字符串中包含环境变量，如 %SystemRoot% 等，而这些环境变量会在写入时被扩展为相应的实际值) 
		// para5:要写入的值的数据 para6:要写入的值的长度，以字节为单位
		// 返回值：如果函数调用成功，则返回 ERROR_SUCCESS。若调用失败，则返回一个错误代码
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机失败!是否权限不足？ \r\n程序启动失败！"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static BOOL WriteStarupDir(const CString& strPath)
	{
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		// 虚拟机地址：C:\Users\Yyh\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup
		// fopen CFile system(copy) CopyFile OpenFile
		return CopyFile(sPath, strPath, FALSE);
	}

	static bool Init()
	{// 用于mfc命令行项目初始化
		HMODULE hModule = ::GetModuleHandle(nullptr);

		if (hModule == nullptr)
		{
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}

		// 初始化 MFC 并在失败时显示错误
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: 在此处为应用程序的行为编写代码。
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}
};

