#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>


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

	// �ڴ��е�����ת��ΪͼƬ������Զ�̼�ع���
	static int Bytes2Image(CImage& image, const std::string& strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str(); // �õ��ڴ��е�����
		// ����ȫ���ڴ�鲢��������
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); // ���ڴ��з���һ�����ƶ����ڴ�飬������һ��ָ����ڴ��ľ�� para1:������ڴ���ǿ��ƶ��� para2:�����ڴ��Ĵ�С,0��ʾ������ڴ��û���κγ�ʼ����
		if (hMem == NULL)
		{
			TRACE("�ڴ治��!");
			AfxMessageBox("�ڴ治��!");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL; // �����ڴ���
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream); // ��������ȫ���ڴ��� para1:���ĸ��ڴ����������� para2:������ʱ���Զ��ͷ��ڴ�� para3:��һ��ָ��IStreamָ���ָ�룬���ڽ��մ�����������ĵ�ַ������������ɺ�pStream ��ָ�򴴽���������

		// ����������������������д�뵽���У�Ȼ������м���ͼ�����ݣ��������ص�ͼ�����ݱ�����λͼ������   �������˵��߼����������浽���������ڴ����� -> ���ڴ�����ݴ������
		if (hRet == S_OK) // ����������Ĳ����ɹ�ִ���ˣ�����ζ���������Ѿ���ȫ���ڴ��ɹ�����        �ͻ��˵��߼������յ���ͼ������д������ -> �����м�������
		{
			ULONG length = 0; // ���ڼ�¼ _OUT_ʵ��д����ֽ���
			pStream->Write(pData, strBuffer.size(), &length); // ����������������������д��pStream�� &length ��һ��ָ�� ULONG ���͵�ָ�룬���ڽ���ʵ��д����ֽ���
			LARGE_INTEGER bg = { 0 }; // ������һ����ʼλ��Ϊ0�� LARGE_INTEGER �ṹ�壬��ʾҪ������λ������Ϊ��ʼλ��
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // ��ת������ͷ��
			if ((HBITMAP)image != NULL) image.Destroy(); // �ڼ���ǰ���ͷ�֮ǰ�Ļ���:���֮ǰ�Ѿ�������λͼ���ݣ���������
			image.Load(pStream); // ��pStream���м���ͼ������
		}
		return hRet;
	}
};

