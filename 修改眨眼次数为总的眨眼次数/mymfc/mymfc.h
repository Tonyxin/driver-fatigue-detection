
// mymfc.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������
#include "CvvImage.h"
#include <opencv2\opencv.hpp>
#include <Windows.h>
#include <process.h>


// CmymfcApp: 
// �йش����ʵ�֣������ mymfc.cpp
//

class CmymfcApp : public CWinApp
{
public:
	CmymfcApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CmymfcApp theApp;