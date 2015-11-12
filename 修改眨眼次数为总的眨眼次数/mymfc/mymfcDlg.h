
// mymfcDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

//此处添加的程序属于全局位置
#define WM_RECVDATA WM_USER+1
struct Mesage2UI
{
	IplImage* img;
	double frequency;
	int number;
};

struct Mesasge2Work
{
	HWND hwnd;
	IplImage* img;
};



// CmymfcDlg 对话框
class CmymfcDlg : public CDialog
{
// 构造
public:
	CmymfcDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MYMFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnRecvData(WPARAM wParam, LPARAM lParam);					//定义消息响应函数
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMCustomdrawSlider2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButton1();
	IplImage* Show_Face;
	IplImage* Show_Eye;
	IplImage* Show_Edge;
			
	afx_msg void OnBnClickedStart();
	void ShowImage(IplImage* img, UINT ID);
	void pre_process(IplImage* img, CvRect& rect);
	bool detect_and_draw(IplImage* img, CvRect& rect_face, CvRect& rect_eye);
	bool detect_eye(IplImage* img_eye, CvRect rect_face, CvRect& rect_eye);
	afx_msg void OnBnClickedEnd();
	afx_msg void OnBnClickedOk();
	static DWORD WINAPI Eye_Process(LPVOID lpParameter);			//定义为静态函数，函数不属于某一个对象，而属于一个类
	
	// 系统变量
	const char* cascade_name = "haarcascade_frontalface_alt2.xml";
	const char* cascade_eye_name = "haarcascade_lefteye_2splits.xml";
	CvMemStorage* storage;
	CvMemStorage* storage_eye;
	CvHaarClassifierCascade* cascade;
	CvHaarClassifierCascade* cascade_eye;

	//人脸的预处理，得到人脸区域的大致面积
	static int N_FACE;										//利用20帧得到人脸平均面积
	static int count_face;								//用于统计校验时检测人脸数
	static int MAX_FACE;
	static int MIN_FACE;
	static float scale;										//检测人脸时用于缩放，加快检测进度
	static CvScalar colors;

	static bool thread_is_end;							//用于线程同步

	static double t_begin;								//与t_end结合，用于统计10s内的眨眼次数
	static double t_end;
	static void noise_eliminate(IplImage* src);

	static CvCapture* capture;						//用于捕获视频
	static CvRect mark_face;							//用于跟踪人脸
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedTheend();
	int m_BlinkCount;
	double m_BlinkFreq;
	CString m_result;
	afx_msg void OnEnChangeNumber();
	CButton m_start;
	CButton m_end;
	static int LowThresh;
	static int HighThresh;
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	int m_LowThreshValue;
	int m_HighThreshValue;
};

