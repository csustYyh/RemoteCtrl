#include "pch.h"
#include "Command.h"

CCommand::CCommand():threadid(0) // 默认构造函数 同时将 threadid 初始化为0
{
	/* 使用映射表
	1. 利于后面添加、改变和插入
	2. switch 的方式会随数量增加，查找速度会减慢
	3. 使用 hash 值寻找对应的功能函数指针
	4. switch 和 if 的方式有局限性，最多 255 个
	*/

	struct
	{
		int      nCmd; // 命令
		CMDFUNC  func; // 命令对应的功能函数
	}data[] =
	{
		{1, &CCommand::MakeDriverInfo},		// 查看驱动盘符
		{2, &CCommand::MakeDirectoryInfo},  // 查看文件
		{3, &CCommand::RunFile},			// 运行文件
		{4, &CCommand::DownloadFile},		// 下载文件
		{5, &CCommand::MouseEvent},			// 鼠标事件
		{6, &CCommand::SendScreen},			// 远程屏幕
		{7, &CCommand::LockMachine},		// 锁机
		{8, &CCommand::UnlockMachine},		// 解锁
		{9, &CCommand::DeleteLocalFile},	// 删除文件
		{1981, &CCommand::TestConnet},		// 连接测试
		{-1, NULL}, // 无效	
	};

	for (int i = 0; data[i].nCmd != -1; i++) // 初始化映射表
	{
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

// 根据给定的命令 nCmd 在 m_mapFunction 中查找对应的成员函数指针，并执行该成员函数
// .h中声明：int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd); // 通过执行命令找到对应的业务处理函数指针
	if (it == m_mapFunction.end()) return -1; // 没有找到
	// 如果找到了对应的成员函数指针，就通过成员函数指针调用对应的成员函数，并传递 lstPacket 和 inPacket 作为参数。
	// 这里的 (this->*it->second) 是通过成员函数指针调用成员函数的语法。
	return (this->*it->second) (lstPacket, inPacket); // 调用相应的业务处理函数
}
