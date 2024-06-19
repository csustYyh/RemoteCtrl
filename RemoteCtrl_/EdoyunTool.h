#pragma once
class CEdoyunTool
{

public:
	// ���ֽ������е�ÿ���ֽ���ʮ�����Ƶ���ʽ��ʽ����������ÿ����ʾ16���ֽڵĸ�ʽ�����������������ָ�ʽ�������ͨ�����ڵ���Ŀ�ģ�����鿴���������ݵ����ݡ�
	static void Dump(BYTE* pData, size_t nSize)
	{
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) // ѭ������ÿ���ֽ�
		{
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n"; // ÿ 16 ���ֽڻ���
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); // ��ÿ���ֽڸ�ʽ��Ϊ��λ��ʮ�������ַ���
			strOut += buf; // ׷�ӵ� strOut
		}

		strOut += "\n";
		OutputDebugStringA(strOut.c_str()); // ��������Ϣ��������������� Visual Studio ��������ڣ���
	}

	static void ShowError() // ��ȡ���һ�η����Ĵ�����Ϣ������������������������
	{
		LPWSTR lpMessageBuf = NULL; // ������һ���յ� Unicode �ַ���ָ�� 
		// strerror(errno); // ��׼C���Կ�
		FormatMessage( // ��ʽ��һ����Ϣ�ַ��������ݴ���������ɶ�Ӧ�Ĵ�����Ϣ�ı�
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf); // ��������Ϣ����������������
		LocalFree(lpMessageBuf); // �ͷ� FormatMessage() ����Ļ��������Ա����ڴ�й©
	}


	static bool IsAdmin() // ��鵱ǰ�����Ƿ�ӵ�й���ԱȨ��
	{   // 1.�õ�ǰ���̵�token 2.��token�е���Ϣ 3.������Ϣ�жϵ�ǰ�����Ƿ�ӵ�й���ԱȨ��
		HANDLE hToken = NULL;
		// �򿪵�ǰ���̵ķ������ƣ����������洢�� hToken ������
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) // para1:��ȡ��ǰ����ִ�еĽ��̵ľ�� para2:��ʾֻ��Ҫ��ѯ�������Ƶ���Ϣ���������� para3:���մ򿪵ķ������Ƶľ��
		{
			ShowError();
			return false;
		}

		// ��鵱ǰ�����Ƿ�����Ȩ(����ԱȨ��)����
		TOKEN_ELEVATION eve; // �洢�й���������״̬����Ϣ
		DWORD len = 0; // �洢���� GetTokenInformation �����󷵻ص���Ϣ����
		// ��ȡ��ǰ���̵ķ���������Ϣ
		// para1:��ǰ���̵ķ������ƾ�� para2:ָ��Ҫ��ȡ��������Ϣ���ͣ���ʾ����������Ա��Ȩ�޵���Ϣ para3:�洢��ȡ��������״̬��Ϣ para4:�洢ʵ�ʷ��ص���Ϣ����
		if (!GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len))
		{
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(hToken)) // ��鷵�ص���Ϣ�����Ƿ������ƾ���Ĵ�С��ͬ�������ͬ��˵�������ɹ�����������״̬��Ϣ
		{
			return eve.TokenIsElevated; // ���� TOKEN_ELEVATION �ṹ���е� TokenIsElevated ��Ա���ó�Ա��ʾ��ǰ�����Ƿ���������Ȩ������
		}
		// �����ȣ���û�гɹ���������״̬��Ϣ
		printf("length of tokeninformation is %d\r\n", len);
		return false;
	}

	static bool RunAsAdmin() // ��ȡ����ԱȨ��->ʹ�ø�Ȩ�޴���һ���Թ���ԱȨ�����еĽ���    ps:��Ƶ�γ�ʵ��ʵ��ʱ����ֱ�ӵ���CreateProcessWithLogonW()�����ù���Ա�û���ִ��RemoteCtrl.exe
	{
		// ��ȡ����ԱȨ��
		/*HANDLE hToken = NULL;
		bool ret = LogonUser(L"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
		if (!ret)
		{
			ShowError();
			MessageBox(NULL, _T("��¼����"), _T("�������"), 0);
			::exit(0);
		}
		OutputDebugString(L"Logon adminiastrator sucess!\r\n");
		MessageBox(NULL, _T("��¼�ɹ���"), _T("�ɹ�"), 0);*/

		// ����һ���Թ���ԱȨ�����еĽ��������� RemoteCtrl.exe
		// ʹ��ָ���ķ�������(�����ȡ���й���ԱȨ�޵�)����һ���½��̣����ȴ��ý��̵����
		STARTUPINFO si = { 0 }; // STARTUPINFO �ṹ�������һ���½��̵�һЩ���ԣ����細����ۡ���׼����������ض������Ϣ
		PROCESS_INFORMATION pi = { 0 }; // PROCESS_INFORMATION �ṹ�������һ���½��̵�һЩ��Ϣ��������̵ľ�����̵߳ľ����
		TCHAR sPath[MAX_PATH] = _T(""); // ���ڴ洢��ǰ���̵�Ŀ¼·��

		// ���ز����� ����Admin�˻� ��ֹ������֮�ܵ�¼���ؿ���̨
		// GetCurrentDirectory(MAX_PATH, sPath); // ��ȡ��ǰ���̵�Ŀ¼·��
		// CString strCmd = sPath;
		// strCmd += _T("\\RemoteCtrl.exe"); // ���ַ��� strCmd ����׷���� \RemoteCtrl.exe�����������Ŀ�ִ���ļ�·��
		// para1:�������ƾ�����й���ԱȨ�ޣ� para2:��־��ָʾ�����Ľ���Ӧ��ʹ�����Ƶ��û������ļ� para3:Ҫִ�еĿ�ִ���ļ�·�� para4:��־��ָʾʹ�� Unicode ����
		// para5:ָ���½��̵�һЩ���� para6:�����½��̵���Ϣ
		//ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)strCmd, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); // ����һ���µĽ���ִ��RemoteCtrl.exe
		// CloseHandle(hToken);

		GetModuleFileName(NULL, sPath, MAX_PATH); // // ��ȡ��ǰ�����Ѽ���ģ����ļ�������·������ģ������ɵ�ǰ���̼���
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi); // ����һ���µĽ��̣���ʹ����һ���û�(����Ա)��Ȩ����ִ��ָ���Ŀ�ִ���ļ�������
		if (!ret)
		{
			ShowError();
			MessageBox(NULL, _T("��������ʧ�ܣ�"), _T("�������"), 0);
			return false;
		}

		WaitForSingleObject(pi.hProcess, INFINITE); // �ȴ��½��̵����߳����ִ��
		CloseHandle(pi.hProcess); // �ر��½��̵ľ�����ͷ���Դ
		CloseHandle(pi.hThread); // �ر��½��̵����߳̾�����ͷ���Դ
		
		return true;
	}

	// ���ÿ����Զ����� ---0410
	// ����������ʱ�򣬳����Ȩ���Ǹ��������û���
	// �������Ȩ�޲�һ�£���ᵼ�³�������ʧ��
	// ���������Ի���������Ӱ�죬�������dll(��̬��)�����������ʧ�� 
	// ���������
	// ��������Щdll��system32�������sysWOW64���桿
	// system32���棬����64λ����syswow64�������32λ����
	// ��ʹ�þ�̬����Ƕ�̬�⡿
	static bool WriteRegisterTable(const CString& strPath) // �޸�ע���
	{
		// ��"RemoteCtrl.exe"������ϵͳĿ¼��
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CopyFile(sPath, strPath, FALSE);
		if (ret == FALSE)
		{
			MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}

		// �ӵ�ע����У����俪���Զ�����
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey); // �õ���
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ�����ʧ��!�Ƿ�Ȩ�޲��㣿 \r\n��������ʧ�ܣ�"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		// CString strPath = CString(_T("%SystemRoot%\\SysWOW64\\RemoteCtrl.exe"));
		// ����ע�����ָ�����ֵ
		// para1:�򿪵�ע�����ľ�� para2:Ҫ���õ�ע���������� para3:����������ͨ��Ϊ0
		// para4:ָ���������ͣ������� REG_EXPAND_SZ����ʾҪ���õ�ֵ�ǿ���չ���ַ���(�������ַ����а��������������� %SystemRoot% �ȣ�����Щ������������д��ʱ����չΪ��Ӧ��ʵ��ֵ) 
		// para5:Ҫд���ֵ������ para6:Ҫд���ֵ�ĳ��ȣ����ֽ�Ϊ��λ
		// ����ֵ������������óɹ����򷵻� ERROR_SUCCESS��������ʧ�ܣ��򷵻�һ���������
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ�����ʧ��!�Ƿ�Ȩ�޲��㣿 \r\n��������ʧ�ܣ�"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
		return true;
	}

	static BOOL WriteStarupDir(const CString& strPath)
	{
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		// �������ַ��C:\Users\Yyh\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup
		// fopen CFile system(copy) CopyFile OpenFile
		return CopyFile(sPath, strPath, FALSE);
	}

	static bool Init()
	{// ����mfc��������Ŀ��ʼ��
		HMODULE hModule = ::GetModuleHandle(nullptr);

		if (hModule == nullptr)
		{
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}

		// ��ʼ�� MFC ����ʧ��ʱ��ʾ����
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}
};

