#pragma once
#include <Windows.h>
#include <string>
#include <atlimage.h>


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

	// 内存中的数据转化为图片，用以远程监控功能
	static int Bytes2Image(CImage& image, const std::string& strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str(); // 拿到内存中的数据
		// 分配全局内存块并与流关联
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0); // 在内存中分配一个可移动的内存块，并返回一个指向该内存块的句柄 para1:分配的内存块是可移动的 para2:分配内存块的大小,0表示分配的内存块没有任何初始内容
		if (hMem == NULL)
		{
			TRACE("内存不足!");
			AfxMessageBox("内存不足!");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL; // 创建内存流
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream); // 创建流到全局内存区 para1:将哪个内存块与流相关联 para2:创建流时会自动释放内存块 para3:是一个指向IStream指针的指针，用于接收创建的流对象的地址。函数调用完成后，pStream 将指向创建的流对象

		// 将服务器传来的桌面数据写入到流中，然后从流中加载图像数据，并将加载的图像数据保存在位图对象中   服务器端的逻辑：截屏保存到流、流与内存块关联 -> 将内存的数据打包发送
		if (hRet == S_OK) // 创建流对象的操作成功执行了，这意味着流对象已经与全局内存块成功关联        客户端的逻辑：接收到的图像数据写入流中 -> 从流中加载数据
		{
			ULONG length = 0; // 用于记录 _OUT_实际写入的字节数
			pStream->Write(pData, strBuffer.size(), &length); // 将服务器传来的桌面数据写入pStream流 &length 是一个指向 ULONG 类型的指针，用于接收实际写入的字节数
			LARGE_INTEGER bg = { 0 }; // 创建了一个起始位置为0的 LARGE_INTEGER 结构体，表示要将流的位置设置为起始位置
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);  // 跳转到流的头部
			if ((HBITMAP)image != NULL) image.Destroy(); // 在加载前先释放之前的缓存:如果之前已经加载了位图数据，则将其销毁
			image.Load(pStream); // 从pStream流中加载图像数据
		}
		return hRet;
	}
};

