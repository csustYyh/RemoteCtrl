#pragma once
#include "pch.h"
#include "framework.h"

// 包的封装设计：1-2个字节的包头 + 2/4个字节存储包的长度(根据包的大小来选择) 
//            + 具体传递的数据(前1/2个字节作为命令，后1/2个字节作为校验，中间即为正式的数据)
// 包头根据经验一般设置为：FF FE / FE FF 因为这两个字节出现的概率很小
// 常见的检验：和校验，即将具体传递的数据相加(按照字节/短整形，溢出不管)，最后相加的结果作为校验存放在包的末尾
class CPacket // 数据包类
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}  // 构造函数，成员变量初始化为 0
	CPacket(const CPacket& pack) // 拷贝构造函数
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket& operator=(const CPacket& pack) // 重载赋值运算符
	{
		if (this != &pack) // eg: a = b; 这种赋值情况
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this; // eg：a = a; 这种赋值情况
	}
<<<<<<< Updated upstream
	// 数据包封包构造函数，用于把数据封装成数据包  
	// size_t 是宏定义，即 typedef unsigned int size_t, 为无符号整型 4个字节; BYTE 即 unsigned char 的宏定义
=======
	
	// 01.组包，用于把要发送的数据封装成包发送
>>>>>>> Stashed changes
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) // para1：命令  para2：数据指针  para3：数据大小
	{
		sHead = 0xFEFF;
		nLength = nSize + 2 + 2; // 命令2 + 包数据nSize + 和校验2
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize); // 拷贝数据
		}
		else
		{
			strData.clear(); // 无数据清空
		}

		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) // 计算校验位
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

<<<<<<< Updated upstream
	// 数据包解包构造函数，将包中的数据分配到成员变量中
	CPacket(const BYTE* pData, size_t& nSize) 
	// para1：数据指针 para2：数据大小 输入时 nSize 代表这个包有多少个字节的数据
	// 输出时返回的 nSize 代表我们在解包过程中用掉了多少个字节
=======
	// 02.解包，将从客户端收到的包中的数据分配到成员变量中
	CPacket(const BYTE* pData, size_t& nSize) // para1：数据指针 para2：数据大小 输入时 nSize 代表这个包有多少个字节的数据，输出时返回的 nSize 代表我们在解包过程中用掉了多少个字节
>>>>>>> Stashed changes
	{
		size_t i = 0; // 遍历数据包
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // 识别包头
			{
				sHead = *(WORD*)(pData + i); // 取包头
				i += 2; // WORD 为无符号短整型，占两个字节,取出包头后遍历指针 i 后移两位 
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) // 包数据可能不全，即连 包头i + 长度4 + 命令2 + 校验2 都不全
		{
			nSize = 0;
			return; // 解析失败
		}

		nLength = *(DWORD*)(pData + i); i += 4; // 读包的长度
		if (nLength + i > nSize) // 当未完全接收
		{
			nSize = 0;
			return; // 解析失败
		}

		sCmd = *(WORD*)(pData + i); i += 2; // 读取命令

		if (nLength > (2 + 2)) // 数据的长度大于 命令2 + 校验2 说明这个包中有要读取的数据
		{
			strData.resize(nLength - 2 - 2); // 设置 string 的大小
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);  // 读取数据
			i += nLength - 2 - 2; // 读取完数据后移动遍历指针 i 到最新的位置(此时移动到校验码开始的位置)
		}

		sSum = *(WORD*)(pData + i); i += 2;  // 读校验码
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}

		if (sum == sSum) // 校验
		{
			nSize = i; // 头 + 长度 + 命令 + 数据 + 校验   nSize 是以引用(&)的方式传参，这里返回的 nSize 代表解包过程中用掉了多少数据
			return;
		}

		nSize = 0;
	}

	~CPacket() {} // 析构函数

	int Size() // 获得整个包的大小
	{
		return nLength + 4 + 2; //  （控制命令 + 包数据 + 和校验)nLength + 存放nLength的大小 4 + 包头 2 
	}
	const char* Data() // 用于解析包，将包送入缓冲区 即将数据包的所有数据赋值给包类成员 strOut
	{
		strOut.resize(nLength + 4 + 2);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;    pData += 2;
		*(DWORD*)pData = nLength;  pData += 4;
		*(WORD*)pData = sCmd;     pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str(); // 返回整个包
	}
public:
	WORD sHead;    // 包头 固定为 FE FF // WORD 是宏定义，即 typedef unsigned short WORD, 为无符号短整型 2个字节
	DWORD nLength; // 包中数据的长度 // DWORD 是宏定义，即 typedef unsigned long WORD, 为无符号长整型 4个字节(64位下8个字节) 这里是指下面：控制命令 + 包数据 + 和校验 的长度
	WORD sCmd;     // 控制命令
	std::string strData; // 包数据
	WORD sSum;     // 和校验
	std::string strOut; // 整个包的数据
};

// 设计一个鼠标信息的数据结构，用于接收客户端鼠标操作命令
typedef struct MouseEvent
{
	MouseEvent() // 结构体的构造函数 用于实例化结构体时将其初始化
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0表示单击，1表示双击，2表示按下，3表示放开，4不作处理
	WORD nButton; // 0表示左键，1表示右键，2表示中键，4没有按键
	POINT ptXY; // 鼠标的 x y 坐标 结构体变量，里面有 x 坐标和 y 坐标

}MOUSEEV, * PMOUSEEV;

// 用于存储受控方获取到的本地文件信息
typedef struct file_info
{
	file_info() // 结构体的构造函数 用于实例化结构体时将其初始化
	{
		IsInvalid = FALSE; // 默认有效
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; // 标志是否有效，0-有效 1-无效
	BOOL IsDirectory; // 标志是否为目录 0-不是目录 1-是目录
	BOOL HasNext; // 是否还有后续，即子文件夹或文件 0-没有 1-有
	char szFileName[256]; // 文件/目录 名

}FILEINFO, * PFILEINFO;

