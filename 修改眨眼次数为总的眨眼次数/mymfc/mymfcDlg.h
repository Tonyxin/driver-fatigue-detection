
// mymfcDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

//�˴���ӵĳ�������ȫ��λ��
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



// CmymfcDlg �Ի���
class CmymfcDlg : public CDialog
{
// ����
public:
	CmymfcDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MYMFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnRecvData(WPARAM wParam, LPARAM lParam);					//������Ϣ��Ӧ����
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
	static DWORD WINAPI Eye_Process(LPVOID lpParameter);			//����Ϊ��̬����������������ĳһ�����󣬶�����һ����
	
	// ϵͳ����
	const char* cascade_name = "haarcascade_frontalface_alt2.xml";
	const char* cascade_eye_name = "haarcascade_lefteye_2splits.xml";
	CvMemStorage* storage;
	CvMemStorage* storage_eye;
	CvHaarClassifierCascade* cascade;
	CvHaarClassifierCascade* cascade_eye;

	//������Ԥ�����õ���������Ĵ������
	static int N_FACE;										//����20֡�õ�����ƽ�����
	static int count_face;								//����ͳ��У��ʱ���������
	static int MAX_FACE;
	static int MIN_FACE;
	static float scale;										//�������ʱ�������ţ��ӿ������
	static CvScalar colors;

	static bool thread_is_end;							//�����߳�ͬ��

	static double t_begin;								//��t_end��ϣ�����ͳ��10s�ڵ�գ�۴���
	static double t_end;
	static void noise_eliminate(IplImage* src);

	static CvCapture* capture;						//���ڲ�����Ƶ
	static CvRect mark_face;							//���ڸ�������
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

