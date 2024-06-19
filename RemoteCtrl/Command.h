#pragma once
#include "Resource.h"
#include <map>
#include <list>
#include <direct.h>
#include <io.h>
#include <list>
#include <atlimage.h>
//#include "ServerSocket.h" // �����������ͨ�ź���Command���в�����ҪServerSocket.h
#include "Packet.h"
#include "LockDialog.h"
#include "EdoyunTool.h"
#pragma warning(disable:4966) // fopen sprintf strcpy strstr


class CCommand // �������ִ�кͶ�Ӧ���ܺ����ĵ��ù�����ȫ���������ExcuteCommand()���治���漰�κξ��幦�ܺ����ĵ��ã�ȫ��ͨ������-����ӳ��������
{
public:
	CCommand(); // Ĭ�Ϲ��캯����ʹ��ӳ���ʽ������Զ�̲�������ӳ��
	~CCommand() {}

public:
	// �� CServerSocket �е� m_callback�� ����ִ������Ļص�����������������ִ��������ͨ��
	// para1:ִ������Ķ������ＴCCommand���� para2:���� para3:���ڴ�����Ҫ�����ؿͻ��˵����ݵ�lstPackets���� para4:��������ݰ�
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) // ������� ���ڱ�¶��CServerSocket��ʹ��
	{
		CCommand* thiz = (CCommand*)arg;
		if (status > 0)
		{
			int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
			if (ret != 0)
			{
				TRACE("ִ������ʧ�ܣ� %d ret = %d \r\n", status, ret);
			}
		}
		else
		{
			MessageBox(NULL, _T("�޷����������û����Զ�����..."), _T("�����û�ʧ�ܣ�"), MB_OK | MB_ICONERROR);
		}
	}

	// ���� 2 3 ���ô���Ϊ�˽��������������ͨ�Ź��̣�������ͨ�ŵ���ع���ȫ������CServerSocket����
	// ԭ����ϵ��߼����ڴ�������ʱ��ʹ��socket���տͻ��˴������ݰ�->��������->ʹ��socket���ʹ��������ݰ�
	// �������߼������������ݰ���Ҫ���͵����ݰ�ȫ���Բ�������ʽ������ִ����������в��ٽ���/�������ݣ�Ҳ�Ͳ��漰������ͨ����,ֻ�������ݵĴ���
	// para1:���� para2:���ڴ�����Ҫ�����ؿͻ��˵����ݵ�lstPackets���������� para3:��������ݰ�����ΪĳЩ������Ҫ�õ��ͻ��˷����Ĵ���·��ֵ����Ϣ
	// ����ֵ���ɹ����� 0 ʧ�ܷ��ط� 0
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPcaket); // ִ������

protected:
	// CCommand::* ��ʾ��Ա����ָ�뽫ָ�� CCommand ��ĳ�Ա���� CMDFUNC������������͵����ƣ������ڴ�����ʹ�� CMDFUNC ����������ض��ĳ�Ա����ָ������
	// (para1, para2)�����ʾ��Ա����ָ����Խ��ܵĲ����б�para1:���ݰ�list���ñ��е����ݰ��������ظ��ͻ��� para2:���ݰ�
	// int����Ա����ָ��ָ��ĺ����ķ�������
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket); // CMDFUNC��Ա����ָ�� 
	std::map<int, CMDFUNC> m_mapFunction; // ��Ա����ӳ��� ������ŵ���Ӧ�Ĺ��ܺ�����ӳ�� ����ʹ��������Ϊ����������Ӧ�ĳ�Ա����ָ��
	CLockDialog dlg; // ������ CLockDialog ���һ��ʵ����������Ϊ dlg CLockDialog ��̳��� MFC �ĶԻ�����
	unsigned threadid; // ����һ���߳�id�����ڴ��ִ��������������̵߳��̺߳ţ��Ա������߳���ִ�н�������ʱ���ܽ�����ָ��͸����߳�

protected:
	// �������̷�������Ϣ�����ڲ鿴�ļ� �鿴���ش��̷�����������Ϣ��������ݰ� �ɹ����� 0 ʧ�ܷ��� -1
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string result;

		for (int i = 1; i <= 26; i++) // �������̷�
		{
			if (_chdrive(i) == 0) // �л��ɹ���˵����ǰ���̷���Ӧ�Ĵ��̴��ڣ������л����̷����ķ������������̷� _chdrive()���ڸı䵱ǰ����������������Ҫ�л�������������(1->A 2->B ... 26->Z)���ɹ�����0��ʧ�ܷ���-1
			{
				result += ('A' + i - 1); // �õ��̷�
				if (result.size() > 0)
					result += ','; // ���̷�����Ψһ����','���� ����ÿ���̣����ܺ��滹��û���̣���Ҫ��',' ��Ϊ�ͻ����Ǳߵ��߼��Ǹ���','������
			}
		}
		// ��һ���µ� CPacket ����(���Command����������)��ӵ� lstPacket ��ĩβ
		// �����ͽ������˵�����ͨ�����������ͨ����ص�ʵ��ȫ������CServerSocket
		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size())); 
		//CPacket pack(1, (BYTE*)result.c_str(), result.size()); // ���
		//CEdoyunTool::Dump((BYTE*)pack.Data(), pack.Size());
		//CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	// ���������ļ��������õ����ļ���Ϣ��������ݰ�
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		// std::list<FILEINFO> lstFileInfos; // �ļ���Ϣ�б� �б��е������������Զ���� FILEINFO ���ļ��л��ļ�����ᵼ���б���󣬲�����һ���Դ��͸��ͻ��ˡ�
		// ֮ǰ��ϵ��߼�����Ҫ��ִ������ʱ��������ͨ��
		//if (CServerSocket::getInstance()->GetFilePath(strPath) == false)  // �õ�·�� 
		//{
		//	OutputDebugString(_T("��ǰ��������ǻ�ȡ�ļ��б������������"));
		//	return -1;
		//}

		if (_chdir(strPath.c_str()) != 0) // ͨ���л��������·�����ж��Ƿ���ڸ�·��  _chdir()���ڸı䵱ǰ�Ĺ���Ŀ¼,�ɹ�����0��ʧ�ܷ���-1
		{
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo)); // ����һ���ո��ͻ���
			// CServerSocket::getInstance()->Send(pack);
			OutputDebugString(_T("û��Ȩ�޷��ʴ�Ŀ¼��"));
			return -2;
		}

		_finddata_t fdata; // _finddata_t �ṹ�����ڴ洢�ļ�����Ϣ
		int hfind = 0; // �ļ����
		if ((hfind = _findfirst("*", &fdata)) == -1) // �������ļ�ϵͳ�в����ļ���Ŀ¼�ĺ�����ͨ��������Ŀ¼��ö���ļ� para1:һ����ʾ�ļ�������ƥ�������ַ��� para2:һ��ָ�� _finddata_t �ṹ���ָ�룬���ڴ洢���ҵ����ļ�����Ϣ��
		{ // _findfirst()�᷵��һ���Ǹ�����������Ϊ "�ļ����"��file handle�����������ʧ�ܣ��򷵻� -1��ͨ��������������Ϊ _findnext() �����Ĳ������Ա����Ŀ¼�е�����ƥ����
			OutputDebugString(_T("��ָ���ļ���û���ҵ��κ��ļ���"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo)); // ����һ���ո��ͻ���
			// CServerSocket::getInstance()->Send(pack);
			return -3;
		}
		// ѭ���õ������ļ�
		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0; // fdata.attrib & _A_SUBDIR ��Ϊ true ˵����һ���ļ��У�Ϊ false ˵�����ļ�
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			// lstFileInfos.push_back(finfo); // ����ǰ�ļ���Ϣ׷�ӵ��ļ��б�
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo)); // ÿ��ȡһ���ļ���Ϣֱ�� send ���ͻ���
			// CServerSocket::getInstance()->Send(pack);
		} while (_findnext(hfind, &fdata) == 0); // �������ļ�ϵͳ�в����ļ���Ŀ¼����ȡ��һ��ƥ�������Ϣ para1:��ǰ�ļ���� para2��һ��ָ�� _finddata_t �ṹ���ָ�룬���ڴ洢��һ��ƥ�������Ϣ �ɹ�����0��ʧ�ܷ���-1

		// while ѭ������˵�������ļ���ȡ��ϣ�����һ�� finfo ���ͻ��ˣ�HasNext ����Ϊ false���ͻ��˽��յ��󼴿ɵ�֪�����ٵȴ��Ƿ����ļ���Ϣû�д��͹���
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		// CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		// CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	// ���б����ļ���ͨ�����ݰ��е�·����ִ���ļ�
	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData; // �õ���ִ���ļ���·��(�������߼�)
		//CServerSocket::getInstance()->GetFilePath(strPath); // �õ���ִ���ļ���·��������ǰ���߼���
		// ShellExecute()������������ⲿӦ�ó�����ļ�,'A' ��ʾ ANSI �ַ���
		// para1:���ھ��  ������NULL����ʾ��ָ�������� para2:����ָ�����еĲ�������ΪNULLʱ����ʾĬ�ϴ� para3:��ִ���ļ�����·��
		// para4:��ִ���ļ��������У���������ΪNULL para5:ָ��Ĭ��Ŀ¼ para6����ʼ����ʾ��ʽ,ģ���������ļ�ʹ�� SW_SHOWNORMAL
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

		// ִ�������Ӧ��ؿͻ���
		lstPacket.push_back(CPacket(3, NULL, 0));
		// CPacket pack(3, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	// �����ļ�����ȡ���ݰ��е�·����ʹ���ļ�����������ȡ�ļ�������������������ͻؿͻ���
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData; // �õ��������ļ���·��(�������߼�)
		//CServerSocket::getInstance()->GetFilePath(strPath); // �õ��������ļ���·��(����ǰ���߼�)

		long long data = 0; // �ļ���С
		// fopen()��ĳ�ַ�ʽ��ָ���ļ�,����ļ���ʧ�ܣ����᷵�� NULL,��������Ҫ�ں����Ĵ����м�鷵��ֵ���������
		// fopen_s�� C11 ��׼����İ�ȫ�汾��ּ����߶Դ���ļ��ʹ����������Ҫ���ṩ�ļ�ָ��ĵ�ַ���Ա��ڷ�������ʱ�ܹ�����Ϊ NULL��������ֱ�ӷ����ļ�ָ�롣�������ڱ���Ǳ�ڵĿ�ָ���������⡣
		// para1:�ļ�ָ��out�� para2:�ļ�·�� para3:�򿪷�ʽ�������Զ����ƶ��ķ�ʽ�򿪣�Ϊ  ��rb��
		// �����ɹ�ִ��ʱ������ֵΪ 0����ʱ�ļ��ѳɹ��򿪲����ļ�ָ���ѱ����á����ں���ִ��ʧ��ʱ������ֵ����һ������Ĵ������
		// FILE* pFile = fopen(strPath.c_str(), "rb");
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0) // �ļ���ʧ��
		{
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			//CPacket pack(4, (BYTE*)&data, 8);
			//CServerSocket::getInstance()->Send(pack);
			return -1;
		}

		if (pFile != NULL)
		{
			// �����ļ���С
			fseek(pFile, 0, SEEK_END); // fseek()�����ļ�ָ��stream��λ�� para1:�ļ�ָ�� para2��ƫ�ƣ�null/0 para3:���ļ�ָ��ָ��ĳ��λ�ã�SEEK_SET��ʾָ���ļ�ͷ��SEEK_END��ʾָ���ļ�β
			data = _ftelli64(pFile); // ʹ�� _ftelli64() ��ȡ��ǰ�ļ�λ��ָ���ƫ�������Ӷ���֪�ļ��Ĵ�С	
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			// CPacket head(4, (BYTE*)&data, 8);
			// CServerSocket::getInstance()->Send(head);
			fseek(pFile, 0, SEEK_SET); // ���ļ�ָ�븴ԭ���ļ�ͷ

			// �����ļ�
			char buffer[1024] = ""; // �������ļ�������
			size_t rlen = 0; // ÿ�η����ļ��Ĵ�С
			do
			{
				// ���ļ�ָ��ָ����������������ķ�ʽ��ȡ��Ŀ�껺����
				// para1:ָ���Ļ����� para2:һ�ζ�ȡ�Ŀ�� para3:��ȡ�Ĵ��� para4:�ļ���
				// ����ֵ�����سɹ���ȡ�Ķ�������������ִ���򵽴��ļ�ĩβ�������С��count
				rlen = fread(buffer, 1, 1024, pFile);
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
				// CPacket pack(4, (BYTE*)buffer, rlen);
				// CServerSocket::getInstance()->Send(pack);

			} while (rlen >= 1024);
			fclose(pFile);
		}
		else // �ļ�Ϊ�գ��������޷���Ȩ�޵����
		{
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
		}

		// �ٷ���һ����ֹ�����߿ͻ����ļ��ѷ�����
		lstPacket.push_back(CPacket(4, NULL, 0));
		// CPacket pack(4, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	// ɾ���ļ�
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData; // �õ���ɾ���ļ���·��(�������߼�)
		//CServerSocket::getInstance()->GetFilePath(strPath); // �õ���ɾ���ļ���·��(����ǰ���߼�)
		//DeleteFileA(strPath.c_str()); 

		TCHAR sPath[MAX_PATH] = _T(""); // TCHAR ��һ���ڲ�ͬ����ѡ�����ܹ��Զ�ת��Ϊ�ַ����ͣ�char �� wchar_t�������� ������һ�� TCHAR ���͵��ַ����� sPath�����ڴ洢�ļ�·��
		// 
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath, sizeof(sPath) / sizeof(TCHAR)); // ���ڽ����ֽ��ַ��ļ�·��ת��Ϊ���ַ������洢�� sPath ��
		DeleteFile(sPath); // �� Windows API �е�һ������������ɾ��ָ��·�����ļ�

		// �ٷ���һ����ֹ�����߿ͻ����ļ���ɾ��
		lstPacket.push_back(CPacket(9, NULL, 0));
		// CPacket pack(9, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);

		return 0;
	}

	// �������������ɹ�����0��ʧ�ܷ���-1
	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV)); // // �ɹ���ȡ���ͻ��˷������������¼�(�������߼�)
		// if (CServerSocket::getInstance()->GetMouseEvent(mouse))// �ɹ���ȡ���ͻ��˷������������¼�
		DWORD nFlags = 0; // ��갴�����䶯���ı�־

		switch (mouse.nButton)
		{
		case 0: // ���
			nFlags = 1; // ��λ��ʾ�����м�
			break;
		case 1: // �Ҽ�
			nFlags = 2;
			break;
		case 2: // �м�
			nFlags = 4;
			break;
		case 4: // û�а���������������ƶ�
			nFlags = 8;
			break;
		}
		if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y); // ��������������Ļ����λ��,����ֵ��һ������ֵ����ʾ�����Ƿ�ִ�гɹ�������ɹ������ط���ֵ�����򷵻���)
		switch (mouse.nAction)
		{
		case 0: // ����
			nFlags |= 0x10; // ��λ��ʾ���� �磺0x11 ��ʾ���������
			break;
		case 1: // ˫��
			nFlags |= 0x20;
			break;
		case 2: // ����
			nFlags |= 0x40;
			break;
		case 3: // �ɿ�
			nFlags |= 0x80;
			break;
		default: // û�ж���
			break;
		}
		switch (nFlags) // �������� case ���� 12 �����
		{
			// mouse_event()����ģ������¼������������ͨ�����봥�������ƶ���������ͷŵȲ�����������������ʵ�ʵ���������豸
			// ����ԭ�ͣ� VOID mouse_event(DWORD dwFlags, DWORD dx, DWORD dy, DWORD dwData, ULONG_PTR dwExtraInfo);
			// para1:��������¼������ͣ����������г���֮һ�����ǵ���ϣ�
			//	MOUSEEVENTF_ABSOLUTE: ʹ�þ������꣬���λ�ý���ӳ�䵽������Ļ��
			//	MOUSEEVENTF_MOVE : �ƶ���ꡣ
			//	MOUSEEVENTF_LEFTDOWN : ������¡�
			//	MOUSEEVENTF_LEFTUP : ����ͷš�
			//	MOUSEEVENTF_RIGHTDOWN : �Ҽ����¡�
			//	MOUSEEVENTF_RIGHTUP : �Ҽ��ͷš�
			//	MOUSEEVENTF_MIDDLEDOWN : �м����¡�
			//	MOUSEEVENTF_MIDDLEUP : �м��ͷš�
			// para2 & para3: ��� MOUSEEVENTF_ABSOLUTE �����ã���Щ����ָ�����Ǿ�������λ�ã���������ָ������ڵ�ǰ���λ�õ�ˮƽ�ʹ�ֱ�ƶ�
			// para4: ����¼�������ֹ����¼����������ָ����������������ֵ��ʾ��ǰ��������ֵ��ʾ������
			// para5: һ��ָ��������Ϣ��ֵ��ͨ��Ϊ0
		case 0x21: // ���˫��  �������case�󣬻�ִ��һ����������ɿ�(��Ϊһ�ε������),Ȼ�󻹻����ִ������� case 0x11 ��Ӧ�Ĵ��룬�ʿ���Ϊ˫��
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo()); // GetMessageExtraInfo() ����������ȡ����¼��ĸ�����Ϣ��ͨ����һ���豸��ʶ���������й�����¼�����Ϣ
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x11: // �������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x41: // �������
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81: // ����ɿ�
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x22: // �Ҽ�˫�� 
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x12: // �Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42: // �Ҽ�����
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82: // �Ҽ��ɿ�
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x24: // �м�˫��  
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14: // �м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44: // �м�����
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84: // �м��ɿ�
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x08: // �ް���������������ƶ�
			mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
			break;
		}
		// ������󣬷��������ͻ��˷���Ӧ������������������յ��������������ص���Ϣ
		lstPacket.push_back(CPacket(5, NULL, 0));
		// CPacket pack(5, NULL, 0);
		// CServerSocket::getInstance()->Send(pack);


		return 0;
	}

	// ������ĻͼƬ
	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CImage          screen;                               // CImage ��Ķ��󣬸ö���������ڴ���ͼ��ĸ��ֲ���  GDI(ͼ���豸�ӿ�)
		HDC hScreen = ::GetDC(NULL);                  // ��ȡ������Ļ���豸������(��Ļ������������)��������洢�� hScreen �У�HDC ���豸�����ľ������ ::��ʽָ���˵��õ���ȫ�������ռ��еĺ�����������ߴ���Ŀɶ��Ժ���ȷ��
		int nBitPerPixl = GetDeviceCaps(hScreen, BITSPIXEL);  // ��ȡ�豸���ԣ���ɫ��ȼ�ÿ������ʹ�õ�λ�� ��ɫ��Ⱦ�������Ļ��ÿ�����ؿ��Ա�ʾ����ɫ��������������ɫ��Ȱ��� 8 λ��256 ɫ����16 λ��65,536 ɫ����24 λ��Լ 16.7 ����ɫ���� 32 λ��Լ 16.7 ����ɫ������͸������Ϣ�� RGB8888 32bits  RGB888 ������͸���� 24bits
		int nWidth = GetDeviceCaps(hScreen, HORZRES);    // ��
		int nHeight = GetDeviceCaps(hScreen, VERTRES);    // ��
		screen.Create(nWidth, nHeight, nBitPerPixl);          // ������Ļ ������ʾ���Ŀ��ߺ���ɫ���������

		// para1:��ȡ screen ������豸�����ľ�����Ա����ͼ�λ��� para2&3:Ŀ����ε����Ͻ����� para3&4:Ŀ����εĿ�Ⱥ͸߶ȣ���ʾ��ȡ����Ļ����Ĵ�С
		// para5:Դ�豸�����ľ����ͨ���Ǳ�ʾ������Ļ���豸������ para6&7:Դ���ε����Ͻ����꣬ͨ����������Ļ�����Ͻ� para8:��ʾʹ��Դ���ε����ݸ���Ŀ����ε����ݡ�����һ�����õĿ�������
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  // ��ͼ���Ƶ� screen �У��൱�ڽ�ͼ
		::ReleaseDC(NULL, hScreen);                                          // ��Դ�ͷ�

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);              // ���ڴ��з���һ��ȫ�ֵĶ� para1:ָ��������ڴ���ǿ��ƶ��� para2:������ڴ��Ĵ�СΪ���ֽ�,������̬������С ���أ�������ڷ��ʷ�����ڴ��  
		if (hMem == NULL) return -1;
		IStream* pStream = NULL;                                   // �����ڴ��� ����һ��ָ�� IStream �ӿڵ�ָ�룬��ʼ��Ϊ NULL
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream); // ��ȫ�ֶ����������ڴ��� ��ȫ���ڴ��ľ��ת��Ϊ IStream �ӿڣ������Ϳ���ͨ�� IStream �ӿ�����дȫ���ڴ��

		if (ret == S_OK)
		{
			screen.Save(pStream, Gdiplus::ImageFormatPNG);               // ����ͼ���ڴ�����
			//screen.Save(_T("test2024.png"), Gdiplus::ImageFormatPNG);  // ����ͼ���ļ�png��
			//screen.Save(_T("test2024.jpg"), Gdiplus::ImageFormatJPEG); // ����ͼ���ļ�jpg��
			LARGE_INTEGER  bg = { 0 };					  // LARGE_INTEGER �ṹ�� bg ����Ϊ���ʾ��������Ŀ�ͷ��ƫ��
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);             // ��ͷ��ʼ���ã�����ָ��ָ��ͷ
			PBYTE          pData = (PBYTE)GlobalLock(hMem);   // ���������ڶ�ȡ���� ʹ�� GlobalLock ��������ȫ���ڴ�飬����ָ�� pData ָ������ڴ档����Ϊ���ں����Ĵ����ж�ȡ�ڴ���е�����
			SIZE_T         nSize = GlobalSize(hMem);          // ��ȡȫ���ڴ��Ĵ�С
			lstPacket.push_back(CPacket(6, pData, nSize));
			// CPacket pack(6, pData, nSize);                        // ��װ�ɰ�
			// CServerSocket::getInstance()->Send(pack);             // ����
			GlobalUnlock(hMem);									  // ʹ�� GlobalUnlock ��������ȫ���ڴ�飬���������̻߳����Ը��ڴ����з���
		}

		// ��Դ�ͷ�
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();

		return 0;
	}

	/*************************************����*****************************************************/
	// �������� 
	void threadLockDlgMain()
	{
		dlg.Create(IDD_DIALOG_INFO, NULL); // ����������Ϣ�Ի��� para1:�Ի������ԴID para2:û�и�����
		dlg.ShowWindow(SW_SHOW); // ������ʾ��ǰ�����ĶԻ��� dlg para1:���Ի�������Ϊ�ɼ�״̬

		// ���������Χ
		CRect rect; // ����һ�� CRect ���� rect����ʾ��Ļ�ľ�������
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN); // ��Ļ�Ŀ�
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN); // ��Ļ�ĸ�
		rect.bottom = LONG(rect.bottom * 1.10); // ��Ļ�ĸ�
		dlg.MoveWindow(rect); // �������Ի��� dlg �ƶ���ȫ��

		// �������Ի����е���ʾ���־���
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC); // IDC_STATIC ����ʾ���ֵ�ID
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

		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); // �� dlg ���ڴ�����㣬ʹ���Ϊ���ϲ㴰�� para6:���ı䴰�ڵĴ�С��λ��
		ShowCursor(false); // ��������꣬ʹ������Ļ�ϲ��ɼ�
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE); // ����ϵͳ������ FindWindow(�������ҵ�ϵͳ�������Ĵ��ھ��
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect); // ʹ�� ClipCursor �������������ƶ���Χ��������������Ļ�����Ͻ�һ�����ص������������Է�ֹ����뿪���С���򣬴Ӷ�ʵ����������Ч��

		MSG msg; // ���ڴ洢����Ϣ�����м���������Ϣ 

		// �����Ϣѭ���Ǿ���� Windows ��Ϣ����ģʽ�������ֳ����ִ�У��ȴ��������û���ϵͳ��������Ϣ��ֻ���ڽ��յ��˳���Ϣ��ͨ���ǹرմ��ڵ���Ϣ��ʱ��ѭ���Ż��˳����������ִ��
		while (GetMessage(&msg, NULL, 0, 0)) // ����һ����Ϣѭ����ʹ�� GetMessage ��������Ϣ�����л�ȡ��Ϣ
		{ // MFC �ĶԻ���Ҳ����������Ϣѭ����
			TranslateMessage(&msg); // �Լ�����Ϣ���з��롣����Ϊ��ȷ����ȷ�ļ�����Ϣ�����͵���Ϣ����
			DispatchMessage(&msg); // ����Ϣ���͵����ڹ��̣�Window Procedure�����д��� ���ڹ����Ǵ��ڴ�����Ϣ�Ļص�������ͨ�����ڶԻ���򴰿�����ʵ�ֵġ�ͨ������ DispatchMessage����Ϣ�����ݵ����ڹ����н��д�����ִ����Ӧ�Ĳ���
			if (msg.message == WM_KEYDOWN) // ����Ϣѭ���м���Ƿ��м��̰������£����ڼ�⵽��������ʱ���ٶԻ��򲢽�����Ϣѭ�����Ӷ���ֹ����ִ��
			{
				TRACE("msg:%08x wparam:%08x lparam:%08x\r\n", msg.message, msg.wParam, msg.lParam);
				if (msg.wParam == 0x1B) // �� ESC �˳�
				{
					break;
				}
			}
		}
		ClipCursor(NULL); // �������귶Χ������
		ShowCursor(true); // ��ʾ�����
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW); // ��ʾϵͳ������ FindWindow(�������ҵ�ϵͳ�������Ĵ��ھ��

		dlg.DestroyWindow(); // ����������Ϣ�Ի���
	}

	// �������߳��н���ת����������  ��������Ϊ��̬����������� LockMachine() �����У���Ҫ���뵱ǰ����� this ָ��
	static unsigned _stdcall threadLockDlg(void* arg) 
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}

	// ������ͨ�������̣߳����̴߳���һ������������Ļ�ĶԻ����赲������������������̶���һ��λ�ã�������һ����Ϣѭ�������ڻָ�����
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) // ������Ϣ�Ի����������ڣ�˵��δִ���������ܣ��ʴ����ӽ�����ִ������ָ����
		{
			// �����߳���ִ��������Ļ�Ĺ��ܣ����Ա������̵߳���Ӧ�ԣ�ʹ�û��ܹ�������Ӧ�ó�����н�������Ȼ�������ﲻ�ܽ��տ��Ʒ���������Ϣ��
			// ʹ�����̵߳��������ڱ������̱߳���ʱ��������������Ӷ�����Ӧ�ó���Ľ�����
			// ����һ���µ��߳�ִ�� threadLockDlg ���������̶߳��������߳����У����Դ���������Ļ���������Ӱ�����̵߳���������
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid); 
		}
		// ������Ϣ�Ի������Ѵ��ڣ�˵���Ѿ�ִ���������ܣ��Ͳ����ظ������ӽ�����ִ������ָ����

		lstPacket.push_back(CPacket(7, NULL, 0));
		// CPacket pack(7, NULL, 0); // Ӧ����ͻ���(���Ʒ�)�İ�
		// CServerSocket::getInstance()->Send(pack); // Ӧ��

		return 0;
	}
	/**********************************************************************************************/

	// �������������̷߳���post��Ϣ�����������̵߳���Ϣѭ��
	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		// ����Ϣ���͵�ָ���̵߳���Ϣ������ para1:ָ�����߳�id para2:ָ������Ϣ���� para3:��Ϣw,һ����һЩ����ֵ para4:��Ϣl,һ���Ǵ�ָ��
		// ����ֵ������������óɹ������ط���ֵ�������������ʧ�ܣ�����ֵ����
		PostThreadMessage(threadid, WM_KEYDOWN, 0x1B, 0); // ��ָ���������Ի����ڷ���һ���Զ�����Ϣ ͨ�����뷽ʽģ�ⷢ����һ�����̰��µ���Ϣ��������ģ���˰��� ESC ���Ĳ���
		lstPacket.push_back(CPacket(8, NULL, 0));
		// CPacket pack(8, NULL, 0); // Ӧ����ͻ���(���Ʒ�)�İ�
		// CServerSocket::getInstance()->Send(pack); // Ӧ��

		return 0;
	}

	// ���Ӳ���
	int TestConnet(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		lstPacket.push_back(CPacket(1981, NULL, 0));
		// CPacket pack(1981, NULL, 0);
		// CServerSocket::getInstance()->Send(pack); // Ӧ��

		return 0;
	}
};

