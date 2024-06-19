#pragma once
#include "pch.h"
#include "framework.h"

// ���ķ�װ��ƣ�1-2���ֽڵİ�ͷ + 2/4���ֽڴ洢���ĳ���(���ݰ��Ĵ�С��ѡ��) 
//            + ���崫�ݵ�����(ǰ1/2���ֽ���Ϊ�����1/2���ֽ���ΪУ�飬�м伴Ϊ��ʽ������)
// ��ͷ���ݾ���һ������Ϊ��FF FE / FE FF ��Ϊ�������ֽڳ��ֵĸ��ʺ�С
// �����ļ��飺��У�飬�������崫�ݵ��������(�����ֽ�/�����Σ��������)�������ӵĽ����ΪУ�����ڰ���ĩβ
class CPacket // ���ݰ���
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}  // ���캯������Ա������ʼ��Ϊ 0
	CPacket(const CPacket& pack) // �������캯��
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket& operator=(const CPacket& pack) // ���ظ�ֵ�����
	{
		if (this != &pack) // eg: a = b; ���ָ�ֵ���
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this; // eg��a = a; ���ָ�ֵ���
	}
<<<<<<< Updated upstream
	// ���ݰ�������캯�������ڰ����ݷ�װ�����ݰ�  
	// size_t �Ǻ궨�壬�� typedef unsigned int size_t, Ϊ�޷������� 4���ֽ�; BYTE �� unsigned char �ĺ궨��
=======
	
	// 01.��������ڰ�Ҫ���͵����ݷ�װ�ɰ�����
>>>>>>> Stashed changes
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) // para1������  para2������ָ��  para3�����ݴ�С
	{
		sHead = 0xFEFF;
		nLength = nSize + 2 + 2; // ����2 + ������nSize + ��У��2
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize); // ��������
		}
		else
		{
			strData.clear(); // ���������
		}

		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) // ����У��λ
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

<<<<<<< Updated upstream
	// ���ݰ�������캯���������е����ݷ��䵽��Ա������
	CPacket(const BYTE* pData, size_t& nSize) 
	// para1������ָ�� para2�����ݴ�С ����ʱ nSize ����������ж��ٸ��ֽڵ�����
	// ���ʱ���ص� nSize ���������ڽ���������õ��˶��ٸ��ֽ�
=======
	// 02.��������ӿͻ����յ��İ��е����ݷ��䵽��Ա������
	CPacket(const BYTE* pData, size_t& nSize) // para1������ָ�� para2�����ݴ�С ����ʱ nSize ����������ж��ٸ��ֽڵ����ݣ����ʱ���ص� nSize ���������ڽ���������õ��˶��ٸ��ֽ�
>>>>>>> Stashed changes
	{
		size_t i = 0; // �������ݰ�
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // ʶ���ͷ
			{
				sHead = *(WORD*)(pData + i); // ȡ��ͷ
				i += 2; // WORD Ϊ�޷��Ŷ����ͣ�ռ�����ֽ�,ȡ����ͷ�����ָ�� i ������λ 
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) // �����ݿ��ܲ�ȫ������ ��ͷi + ����4 + ����2 + У��2 ����ȫ
		{
			nSize = 0;
			return; // ����ʧ��
		}

		nLength = *(DWORD*)(pData + i); i += 4; // �����ĳ���
		if (nLength + i > nSize) // ��δ��ȫ����
		{
			nSize = 0;
			return; // ����ʧ��
		}

		sCmd = *(WORD*)(pData + i); i += 2; // ��ȡ����

		if (nLength > (2 + 2)) // ���ݵĳ��ȴ��� ����2 + У��2 ˵�����������Ҫ��ȡ������
		{
			strData.resize(nLength - 2 - 2); // ���� string �Ĵ�С
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);  // ��ȡ����
			i += nLength - 2 - 2; // ��ȡ�����ݺ��ƶ�����ָ�� i �����µ�λ��(��ʱ�ƶ���У���뿪ʼ��λ��)
		}

		sSum = *(WORD*)(pData + i); i += 2;  // ��У����
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}

		if (sum == sSum) // У��
		{
			nSize = i; // ͷ + ���� + ���� + ���� + У��   nSize ��������(&)�ķ�ʽ���Σ����ﷵ�ص� nSize �������������õ��˶�������
			return;
		}

		nSize = 0;
	}

	~CPacket() {} // ��������

	int Size() // ����������Ĵ�С
	{
		return nLength + 4 + 2; //  ���������� + ������ + ��У��)nLength + ���nLength�Ĵ�С 4 + ��ͷ 2 
	}
	const char* Data() // ���ڽ��������������뻺���� �������ݰ����������ݸ�ֵ�������Ա strOut
	{
		strOut.resize(nLength + 4 + 2);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;    pData += 2;
		*(DWORD*)pData = nLength;  pData += 4;
		*(WORD*)pData = sCmd;     pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str(); // ����������
	}
public:
	WORD sHead;    // ��ͷ �̶�Ϊ FE FF // WORD �Ǻ궨�壬�� typedef unsigned short WORD, Ϊ�޷��Ŷ����� 2���ֽ�
	DWORD nLength; // �������ݵĳ��� // DWORD �Ǻ궨�壬�� typedef unsigned long WORD, Ϊ�޷��ų����� 4���ֽ�(64λ��8���ֽ�) ������ָ���棺�������� + ������ + ��У�� �ĳ���
	WORD sCmd;     // ��������
	std::string strData; // ������
	WORD sSum;     // ��У��
	std::string strOut; // ������������
};

// ���һ�������Ϣ�����ݽṹ�����ڽ��տͻ�������������
typedef struct MouseEvent
{
	MouseEvent() // �ṹ��Ĺ��캯�� ����ʵ�����ṹ��ʱ�����ʼ��
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0��ʾ������1��ʾ˫����2��ʾ���£�3��ʾ�ſ���4��������
	WORD nButton; // 0��ʾ�����1��ʾ�Ҽ���2��ʾ�м���4û�а���
	POINT ptXY; // ���� x y ���� �ṹ������������� x ����� y ����

}MOUSEEV, * PMOUSEEV;

// ���ڴ洢�ܿط���ȡ���ı����ļ���Ϣ
typedef struct file_info
{
	file_info() // �ṹ��Ĺ��캯�� ����ʵ�����ṹ��ʱ�����ʼ��
	{
		IsInvalid = FALSE; // Ĭ����Ч
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; // ��־�Ƿ���Ч��0-��Ч 1-��Ч
	BOOL IsDirectory; // ��־�Ƿ�ΪĿ¼ 0-����Ŀ¼ 1-��Ŀ¼
	BOOL HasNext; // �Ƿ��к����������ļ��л��ļ� 0-û�� 1-��
	char szFileName[256]; // �ļ�/Ŀ¼ ��

}FILEINFO, * PFILEINFO;

