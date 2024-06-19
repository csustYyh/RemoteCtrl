#include "pch.h"
#include "Command.h"

CCommand::CCommand():threadid(0) // Ĭ�Ϲ��캯�� ͬʱ�� threadid ��ʼ��Ϊ0
{
	/* ʹ��ӳ���
	1. ���ں�����ӡ��ı�Ͳ���
	2. switch �ķ�ʽ�����������ӣ������ٶȻ����
	3. ʹ�� hash ֵѰ�Ҷ�Ӧ�Ĺ��ܺ���ָ��
	4. switch �� if �ķ�ʽ�о����ԣ���� 255 ��
	*/

	struct
	{
		int      nCmd; // ����
		CMDFUNC  func; // �����Ӧ�Ĺ��ܺ���
	}data[] =
	{
		{1, &CCommand::MakeDriverInfo},		// �鿴�����̷�
		{2, &CCommand::MakeDirectoryInfo},  // �鿴�ļ�
		{3, &CCommand::RunFile},			// �����ļ�
		{4, &CCommand::DownloadFile},		// �����ļ�
		{5, &CCommand::MouseEvent},			// ����¼�
		{6, &CCommand::SendScreen},			// Զ����Ļ
		{7, &CCommand::LockMachine},		// ����
		{8, &CCommand::UnlockMachine},		// ����
		{9, &CCommand::DeleteLocalFile},	// ɾ���ļ�
		{1981, &CCommand::TestConnet},		// ���Ӳ���
		{-1, NULL}, // ��Ч	
	};

	for (int i = 0; data[i].nCmd != -1; i++) // ��ʼ��ӳ���
	{
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

// ���ݸ��������� nCmd �� m_mapFunction �в��Ҷ�Ӧ�ĳ�Ա����ָ�룬��ִ�иó�Ա����
// .h��������int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd); // ͨ��ִ�������ҵ���Ӧ��ҵ������ָ��
	if (it == m_mapFunction.end()) return -1; // û���ҵ�
	// ����ҵ��˶�Ӧ�ĳ�Ա����ָ�룬��ͨ����Ա����ָ����ö�Ӧ�ĳ�Ա������������ lstPacket �� inPacket ��Ϊ������
	// ����� (this->*it->second) ��ͨ����Ա����ָ����ó�Ա�������﷨��
	return (this->*it->second) (lstPacket, inPacket); // ������Ӧ��ҵ������
}
