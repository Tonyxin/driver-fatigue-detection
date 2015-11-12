
// mymfcDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "mymfc.h"
#include "mymfcDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRITICAL_SECTION mynotation;					//�˴�������ȫ�ֱ����ĵط�
double blink_frequency = 0;
int blink_count_temp=0;
// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CmymfcDlg �Ի���



CmymfcDlg::CmymfcDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CmymfcDlg::IDD, pParent)
	, Show_Face(NULL)
	, m_BlinkCount(0)
	, m_BlinkFreq(0)
	, m_result(_T(""))
	, m_LowThreshValue(0)
	, m_HighThreshValue(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CmymfcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_Number, m_BlinkCount);
	DDX_Text(pDX, IDC_Freq, m_BlinkFreq);
	DDX_Text(pDX, IDC_Reasult, m_result);
	DDX_Control(pDX, IDC_Start, m_start);
	DDX_Control(pDX, IDC_TheEnd, m_end);
	DDX_Text(pDX, IDC_LowThreshValue, m_LowThreshValue);
	DDX_Text(pDX, IDC_HighThreshValue, m_HighThreshValue);
}

//�����о�̬�����ĳ�ʼ��
int CmymfcDlg::N_FACE = 20;										//����20֡�õ�����ƽ�����
int CmymfcDlg::count_face=0;								//����ͳ��У��ʱ���������
int CmymfcDlg::MAX_FACE=0;
int CmymfcDlg::MIN_FACE=0;
float CmymfcDlg::scale=1.3;										//�������ʱ�������ţ��ӿ������
CvScalar CmymfcDlg::colors={ { 0, 0, 255 } };

bool CmymfcDlg::thread_is_end=0;							//�����߳�ͬ��

double CmymfcDlg::t_begin=0;								//��t_end��ϣ�����ͳ��10s�ڵ�գ�۴���
double CmymfcDlg::t_end=0;

CvCapture* CmymfcDlg::capture = 0;
CvRect CmymfcDlg::mark_face;

int CmymfcDlg::LowThresh;
int CmymfcDlg::HighThresh;

//��Ϣ��Ӧ����
BEGIN_MESSAGE_MAP(CmymfcDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_Start, &CmymfcDlg::OnBnClickedStart)
	ON_MESSAGE(WM_RECVDATA, &CmymfcDlg::OnRecvData)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_TheEnd, &CmymfcDlg::OnBnClickedTheend)
	ON_EN_CHANGE(IDC_Number, &CmymfcDlg::OnEnChangeNumber)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CmymfcDlg ��Ϣ�������

BOOL CmymfcDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	m_start.EnableWindow(TRUE);																//�����ʼ������
	m_end.EnableWindow(FALSE);																//�رս�������

	CSliderCtrl* SliderLow = (CSliderCtrl*)GetDlgItem(IDC_LowThresh);		//���û������ķ�Χ�ͳ�ʼֵ
	CSliderCtrl* SliderHigh = (CSliderCtrl*)GetDlgItem(IDC_HighThresh);
	SliderLow->SetRange(0, 200, TRUE);														//����ֵ��ʼֵΪ60
	SliderLow->SetPos(60);
	LowThresh = SliderLow->GetPos();
	SliderHigh->SetRange(0, 200, TRUE);													//����ֵ��ʼֵΪ100	
	SliderHigh->SetPos(100);
	HighThresh = SliderHigh->GetPos();

	CRect rect_face, rect_eye, rect_edge;
	GetDlgItem(IDC_ShowFace)->GetClientRect(&rect_face);								//��ȡ��Ӧ����ʾ����
	GetDlgItem(IDC_ShowEye)->GetClientRect(&rect_eye);
	GetDlgItem(IDC_ShowEdge)->GetClientRect(&rect_edge);
	int rw_face = rect_face.right - rect_face.left;													 // ��������ؼ��Ŀ�͸�
	int rh_face = rect_face.bottom - rect_face.top;
	int rw_eye = rect_eye.right - rect_eye.left;														 // ����۲��ؼ��Ŀ�͸�
	int rh_eye = rect_eye.bottom - rect_eye.top;
	int rw_edge = rect_edge.right - rect_edge.left;												 // �����Ե�ؼ��Ŀ�͸�
	int rh_edge = rect_edge.bottom - rect_edge.top;
	CvSize size_face, size_eye, size_edge;
	size_face.height = rh_face;
	size_face.width = rw_face;
	size_eye.height = rh_eye;
	size_eye.width = rw_eye;
	size_edge.height = rh_edge;
	size_edge.width = rw_edge;
	Show_Face = cvCreateImage(size_face, IPL_DEPTH_8U, 3);
	Show_Eye = cvCreateImage(size_eye, IPL_DEPTH_8U, 3);
	Show_Edge = cvCreateImage(size_edge, IPL_DEPTH_8U, 1);

	m_LowThreshValue = LowThresh;
	m_HighThreshValue = HighThresh;
	m_BlinkCount=0;
	m_BlinkFreq=0;
	m_result="δ����";
	UpdateData(FALSE);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CmymfcDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CmymfcDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
		CDialog::UpdateWindow();                // ����windows���ڣ�������ⲽ���ã�ͼƬ��ʾ�����������
		ShowImage(Show_Face, IDC_ShowFace);    // �ػ�ͼƬ����
		ShowImage(Show_Eye, IDC_ShowEye);
		ShowImage(Show_Edge, IDC_ShowEdge);
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CmymfcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//������ʼ��ť
void CmymfcDlg::OnBnClickedStart()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	m_BlinkCount = 0;
	m_BlinkFreq = 0;
	m_start.EnableWindow(FALSE);							//�رա���ʼ������
	m_end.EnableWindow(TRUE);							//���������������
	InitializeCriticalSection(&mynotation);
	int optlen = strlen("--cascade=");
	// ���ط�����
	cascade = (CvHaarClassifierCascade*)cvLoad(cascade_name, 0, 0, 0);
	cascade_eye = (CvHaarClassifierCascade*)cvLoad(cascade_eye_name, 0, 0, 0);

	if (!cascade)
	{
		AfxMessageBox(_T("ERROR: Could not load classifier cascade\n"));
		return;
	}

	if (!cascade_eye)
	{
		AfxMessageBox(_T("ERROR: Could not load classifier cascade\n"));
		return;
	}

	storage = cvCreateMemStorage(0);
	storage_eye = cvCreateMemStorage(0);

	capture = cvCaptureFromCAM(0);										//������ͷ��ץȡ֡

	t_begin = (double)cvGetTickCount();										//���ڼ�ʱ10s

	if (cascade)
	{
		CvRect mark_eye;
		memset(&mark_eye, 0, sizeof(CvRect));
		bool eye_is_found = 0;
		IplImage *frame, *frame_copy = 0;
		if (!cvGrabFrame(capture))	return;														//������ȡһ֡
		frame = cvRetrieveFrame(capture);
		if (!frame)		return;
		if (!frame_copy)
			frame_copy = cvCreateImage(cvGetSize(frame),
			IPL_DEPTH_8U, frame->nChannels);

		if (frame->origin == IPL_ORIGIN_TL)													//�����Ͻ���Ϊԭ��
			cvCopy(frame, frame_copy, 0);
		else
			cvFlip(frame, frame_copy, 0);

		IplImage*frame_clone = cvCloneImage(frame_copy);

		if (count_face < N_FACE)											//����ǰ20֡Ԥ��������ͳ��������С����������������
		{
			pre_process(frame_copy, mark_face);
		}

		if (count_face >= N_FACE)
		{
			eye_is_found = detect_and_draw(frame_copy, mark_face, mark_eye);			//Ѱ������������
			if (eye_is_found)
			{
				CvRect src_eye_rect;
				src_eye_rect.height = mark_eye.height*scale;
				src_eye_rect.x = mark_eye.x*scale;
				src_eye_rect.y = mark_eye.y*scale + 2 * src_eye_rect.height / 5;				//ȡ��ѡ������°벿����Ϊ���۵Ĵ�������
				src_eye_rect.height = 3 * src_eye_rect.height / 5;
				src_eye_rect.width = mark_eye.width*scale;

				while (1)																					//�ȴ���һ֡���߳̽���
				{
					int i = 0;
					EnterCriticalSection(&mynotation);
					if (!thread_is_end)	i = 1;
					LeaveCriticalSection(&mynotation);
					if (i)	break;
				}
				IplImage* eye_pro = cvCreateImage(cvSize(src_eye_rect.width, src_eye_rect.height), frame_copy->depth, frame_copy->nChannels);
				cvSetImageROI(frame_clone, src_eye_rect);
				cvCopy(frame_clone, eye_pro, 0);
				cvResetImageROI(frame_clone);

				cvResize(eye_pro, Show_Eye, 1);
				ShowImage(Show_Eye, IDC_ShowEye);		
				
				Mesasge2Work* message2work = new Mesasge2Work;
				message2work->hwnd = m_hWnd;
				message2work->img = eye_pro;			
				
				HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Eye_Process, (LPVOID)message2work, 0, NULL);			//�����߳̾��
	//			cvReleaseImage(&eye_pro);
				CloseHandle(hThread);																											//�����߳̾��			

			}
		}

		cvResize(frame_copy, Show_Face, 1);
		ShowImage(Show_Face, IDC_ShowFace);
			
		cvReleaseImage(&frame_copy);
		cvReleaseImage(&frame_clone);
	}
	//���ô�������ÿ��20s����һ���¼�
	SetTimer(1, 20, NULL);
	return;
}

//�����ڶԻ���ָ��λ�����ͼ��
void CmymfcDlg::ShowImage(IplImage* img, UINT ID)
{
	CDC* pDC = GetDlgItem(ID)->GetDC();										//��ȡ��ʾ�ؼ���DC
	HDC hDC = pDC->GetSafeHdc();												//��ȡHDC���豸����������ڻ�ͼ
	CRect rect;
	GetDlgItem(ID)->GetClientRect(&rect);
	int rw = rect.right - rect.left;														// ���ͼƬ�ؼ��Ŀ�͸�
	int rh = rect.bottom - rect.top;
	SetRect(rect, 0, 0, rw, rh);
	CvvImage cimg;
	cimg.CopyOf(img);
	cimg.DrawToHDC(hDC, &rect);
	ReleaseDC(pDC);
}

//����ͼ��Ԥ����
void CmymfcDlg::pre_process(IplImage* img, CvRect& rect)
{
	int i, AVG_FACE;
	static int SUM_FACE=0;
	IplImage* gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	//��������һ�����ţ���ԭ������С��1.3��
	IplImage* small_img = cvCreateImage(cvSize(cvRound(img->width / scale), cvRound(img->height / scale)), 8, 1);		//cvRound()��double���ͽ�����������															
	cvCvtColor(img, gray, CV_BGR2GRAY);				//ת��Ϊ�Ҷ�ͼ
	cvResize(gray, small_img, CV_INTER_LINEAR);	//���Ҷ�ͼ��С
	cvEqualizeHist(small_img, small_img);				//�Ҷ�ͼ��ֱ��ͼ���⻯
	cvClearMemStorage(storage);

	CvSeq* faces = cvHaarDetectObjects(small_img, cascade, storage,
		1.2, 2, CV_HAAR_DO_CANNY_PRUNING,
		cvSize(30, 30));
	// ͳ���沿��������ظ��������ڼ���С���沿Ŀ�꣬���Զ������С���沿�ļ��
	int face_num = faces->total;
	int *area_face = new int[face_num];
	memset(area_face, 0, face_num*sizeof(int));
	int max_face_area = -1;
	int max_face_area_index = -1;
	if (faces->total != 0)
	{
		count_face++;
		for (i = 0; i<faces->total; i++)
		{
			CvRect *face = (CvRect*)cvGetSeqElem(faces, i); // ��ȡ��i������
			area_face[i] = face->height*face->width; // ����������������ظ���
			if (area_face[i]>max_face_area)
			{
				// �޸������沿�����С�Ͷ�Ӧ���沿���������
				max_face_area = area_face[i];
				max_face_area_index = i;
			}
		}
		SUM_FACE = SUM_FACE + max_face_area;  // �ۼ�ͷ������
		CvRect *face = (CvRect*)cvGetSeqElem(faces, max_face_area_index);
		cvRectangle(img, cvPoint(face->x * scale, face->y * scale), cvPoint((face->x + face->width) * scale, (face->y + face->height)  * scale), colors, 3, 8, 0);
		if (count_face == N_FACE - 1)
		{
			rect = *face;
			AVG_FACE = (int)(SUM_FACE / N_FACE);
			MAX_FACE = AVG_FACE * 3;
			MIN_FACE = (int)(AVG_FACE / 3);
		}
	}
	delete[] area_face;
	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);
}

//�����������
bool CmymfcDlg::detect_and_draw(IplImage* img, CvRect& rect_face, CvRect& rect_eye)
{
	bool eye_is_found = 0;
	CvPoint point;
	point.x = rect_face.x + rect_face.width / 2;
	point.y = rect_face.y + rect_face.height / 2;

	IplImage* gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	//��������һ�����ţ���ԭ������С��1.3��
	IplImage* small_img = cvCreateImage(cvSize(cvRound(img->width / scale), cvRound(img->height / scale)), 8, 1);		//cvRound()��double���ͽ�����������															
	int i;
	cvCvtColor(img, gray, CV_BGR2GRAY);				//ת��Ϊ�Ҷ�ͼ
	cvResize(gray, small_img, CV_INTER_LINEAR);	//���Ҷ�ͼ��С
	cvEqualizeHist(small_img, small_img);				//�Ҷ�ͼ��ֱ��ͼ���⻯
	cvClearMemStorage(storage);

	//����ǰһ֡��������Ϊ����Ȥ���򣬱����ں�һ֡���������Ч��
	CvRect ROI;
	ROI.x = 2 * rect_face.x / 3;
	ROI.y = 2 * rect_face.y / 3;
	ROI.height = (small_img->height + 2 * rect_face.height) / 3;
	ROI.width = (small_img->width + 2 * rect_face.width) / 3;
	cvSetImageROI(small_img, ROI);

	//�������������ڸ���Ȥ�ķ�Χ�ڽ��еģ�����˴��������Ч��
	CvSeq* faces = cvHaarDetectObjects(small_img, cascade, storage,
		1.2, 2, CV_HAAR_DO_CANNY_PRUNING,					//ѡȡ��������Ե�ķ�ʽ�ܹ����ͼ��ʱ��
		cvSize(small_img->width / 5, small_img->height / 5));
	cvResetImageROI(small_img);

	for (i = 0; i < (faces ? faces->total : 0); i++)
	{
		//ע�⣺��Ϊ�ڼ��������ʱ�����ڸ���Ȥ�ķ�Χ�ڣ�����������������ROI�����н��м���ģ�
		//Ҳ���ǲ�����ԭͼ��ԭ��Ϊԭ�㣬������ROI�����ԭ��Ϊԭ��
		//��ˣ���ԭͼ������������Ҫ��������任��ԭͼ�У��������������ROI.x,ROI.y
		CvRect* r = (CvRect*)cvGetSeqElem(faces, i);					//��һ��������ѡ����i��
		r->x = r->x + ROI.x;
		r->y = r->y + ROI.y;
		//�þ��ο���  
		if ((r->height*r->width)>MIN_FACE && (r->height*r->width) < MAX_FACE&&point.x > r->x&&
			point.x<(r->x + r->width) && point.y > r->y&&point.y < (r->y + r->height))
		{
			cvRectangle(img, cvPoint(r->x * scale, r->y * scale), cvPoint((r->x + r->width) * scale, (r->y + r->height)  * scale), colors, 3, 8, 0);
			rect_face = *r;
			rect_face.x = r->x;
			rect_face.y = r->y;

			eye_is_found = detect_eye(small_img, rect_face, rect_eye);
			if (eye_is_found)
				cvRectangle(img, cvPoint(rect_eye.x * scale, rect_eye.y * scale), cvPoint((rect_eye.x + rect_eye.width) * scale,
				(rect_eye.y + rect_eye.height)  * scale), colors, 3, 8, 0);
		}
	}
	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);
	return eye_is_found;
}

//����۲�
bool CmymfcDlg::detect_eye(IplImage* img_eye, CvRect rect_face, CvRect& rect_eye)
{
	bool eye_is_found = 0;
	cvClearMemStorage(storage_eye);
	rect_face.x = rect_face.x + rect_face.width / 2;		//���������
	rect_face.width = rect_face.width / 2;					//������������ϰ벿��λ��
	rect_face.height = 2 * rect_face.height / 3;
	cvSetImageROI(img_eye, rect_face);
	IplImage* eye_area = cvCreateImage(cvSize(rect_face.width, rect_face.height), 8, 1);
	cvCopy(img_eye, eye_area);

	CvSeq* eyes = cvHaarDetectObjects(eye_area, cascade_eye, storage_eye, 1.1,
		2, 0, cvSize(10, 10), cvSize(60, 60));
	cvResetImageROI(img_eye);

	int eye_num = eyes->total;
	int *area_eye = new int[eye_num];
	memset(area_eye, 0, eye_num*sizeof(int));
	int max_eye_area = -1;
	int max_eye_area_index = -1;
	if (eye_num)
	{
		eye_is_found = 1;
		for (int i = 0; i < eyes->total; i++)
		{
			CvRect *eye = (CvRect*)cvGetSeqElem(eyes, i); // ��ȡ��i������
			area_eye[i] = eye->height*eye->width; // ����������������ظ���
			if (area_eye[i]>max_eye_area)
			{
				// �޸������沿�����С�Ͷ�Ӧ���沿���������
				max_eye_area = area_eye[i];
				max_eye_area_index = i;
			}
		}
		rect_eye = *(CvRect*)cvGetSeqElem(eyes, max_eye_area_index);
		rect_eye.x = rect_face.x + rect_eye.x;
		rect_eye.y = rect_face.y + rect_eye.y;
	}
	delete[] area_eye;
	cvReleaseImage(&eye_area);
	return eye_is_found;
}



//�����̺߳���
DWORD  CmymfcDlg::Eye_Process(LPVOID lpParameter)
{
	EnterCriticalSection(&mynotation);
	thread_is_end = 1;																						//��ʾ�߳�����ִ��
	LeaveCriticalSection(&mynotation);
	float ratio=0;																										//��ʾ�۾��Ŀ���
	static int blink_count=0;								//գ�۴���
	static bool blink_symbol=0;							//գ�۱�־��0��ʾ�۾�������1��ʾ�۾��պ�
	static double period=10;								//ͳ��ʱ������Ϊ10s

	IplImage* src = ((Mesasge2Work*)lpParameter)->img;						//�Ӳ�������ȡ��Ϣ
	HWND hwnd = ((Mesasge2Work*)lpParameter)->hwnd;

	IplImage*gray = cvCreateImage(cvGetSize(src), src->depth, 1);													//ת��Ϊ�Ҷ�ͼ
	IplImage*gray_smooth = cvCreateImage(cvGetSize(src), src->depth, 1);									//����һ����ֵ�˲�������
	IplImage*canny = cvCreateImage(cvGetSize(src), src->depth, 1);												//����canny������ȡ��Ե

	cvCvtColor(src, gray, CV_BGR2GRAY);
	cvSmooth(gray, gray_smooth, CV_MEDIAN);												//��ֵ�˲�����ֵ�˲�һ�ξ͹��� ���εĻ�Ч������ô����
	//��ͨ����ͼ����ǿ��Ҳ����������������canny������ȡ��Ե��Ҫ����ͼ����ǿ
	cvCanny(gray_smooth, canny, LowThresh, HighThresh, 3);													//����Canny���ӽ��б�Ե��ȡ

	int width = canny->width;
	int height = canny->height;
	int step = canny->widthStep / sizeof(uchar);
	int i, j;

	IplImage* edge = cvCreateImage(cvGetSize(src), src->depth, 1);												//��ȡ�۾��ϱ�Ե
	cvZero(edge);
	uchar* data_canny = (uchar*)canny->imageData;
	uchar* data_edge = (uchar*)edge->imageData;
	//��ȡ�۾����ϱ�Ե
	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			if (data_canny[j*step + i])
			{
				data_edge[j*step + i] = 255;
				break;
			}
		}
	}

	//�������룬����Ĵ����ɱ�
	for (i = 0; i < 2; i++)
		noise_eliminate(edge);


	//Ѱ���ϱ�Ե���Ҷ˵�
	int count_up = 0;																							//�ϱ�Ե���ص�ĸ���
	CvPoint left_point, right_point;
	left_point.x = 0;
	left_point.y = 0;
	right_point.x = 0;
	right_point.y = 0;
	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			if (data_edge[j*step + i])
			{
				if (left_point.x == 0)
				{
					left_point.x = i;
					left_point.y = j;
				}
				if (i>right_point.x)
				{
					right_point.x = i;
					right_point.y = j;
				}
				count_up++;
			}
		}
	}

	int sum_pixel = 0;																						//�����ж��Ƿ��ڱ���״̬
	int* distance = new int[count_up + 1];														//distance������Ҷ˵����ߵ��ϱ�Ե��Ӧ�������
	memset(distance, 0, (count_up + 1)*sizeof(int));
	int k = 0;
	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			if (data_edge[j*step + i])
			{
				//��ȡ�ϱ�Ե���㵽��׼�߶�Ӧ�������
				distance[k] = (i - left_point.x)*(right_point.y - left_point.y) / (right_point.x - left_point.x) + left_point.y - j;
				sum_pixel += distance[k];
				k++;
			}
		}
	}
	if (sum_pixel < count_up / 2)
		ratio = 0;													//����ȡ�ı�Ե�ڻ�׼�����£�����Ϊ�Ǳ���״̬
	else
	{
		CvPoint* up_point = new CvPoint[count_up + 1];									//���ϱ�Ե���еĵ���������up_point
		CvPoint* down_point = new CvPoint[count_up + 1];								//���ϱ�Ե�������·�ת�����������down_point
		memset(up_point, 0, (count_up + 1)*sizeof(CvPoint));
		memset(down_point, 0, (count_up + 1)*sizeof(CvPoint));
		k = 0;
		for (i = 0; i < width; i++)
		{
			for (j = 0; j < height; j++)
			{
				if (data_edge[j*step + i])
				{
					up_point[k].x = i;
					up_point[k].y = j;
					down_point[k].x = i;
					down_point[k].y = j + distance[k] + distance[count_up - k - 1];
					k++;
				}
			}
		}

		IplImage* ellipse = cvCreateImage(cvGetSize(src), src->depth, 1);		//�������±�Ե���е����ص�
		cvZero(ellipse);																									//����ͼ�����ý��г�ʼ��

		uchar* data_ellipse = (uchar*)ellipse->imageData;
		for (k = 0; k < count_up; k++)
		{
			data_ellipse[up_point[k].y*step + up_point[k].x] = 255;
			if (down_point[k].y<height&&down_point[k].y >= 0 && down_point[k].x<width&&down_point[k].x >= 0)
				data_ellipse[down_point[k].y*step + down_point[k].x] = 255;
		}

		delete[] up_point;
		delete[] down_point;

		int count = 0;
		for (i = 0; i < width; i++)
		{
			for (j = 0; j < height; j++)
			{
				if (data_ellipse[j*step + i])
					count++;
			}
		}
		if (count>5)
		{
			CvPoint2D32f* point2d32f = new CvPoint2D32f[count + 1];
			memset(point2d32f, 0, (count + 1)*sizeof(CvPoint2D32f));

			CvBox2D box;
			k = 0;
			for (i = 0; i < width; i++)
			{
				for (j = 0; j < height; j++)
				{
					if (data_ellipse[j*step + i])
					{
						point2d32f[k].x = (float)i;
						point2d32f[k].y = (float)j;
						k++;
					}
				}
			}
			CvMemStorage* storage_fit = cvCreateMemStorage(0);
			cvClearMemStorage(storage_fit);
			CvSeq* ellipse_point = cvCreateSeq(CV_32FC2, sizeof(CvSeq), sizeof(CvPoint2D32f), storage_fit);
			for (i = 0; i < count; i++)
			{
				cvSeqPush(ellipse_point, &(point2d32f[i]));
			}
			box = cvFitEllipse2(ellipse_point);
			delete[] point2d32f;

			CvSize ellipse_size;
			ellipse_size.width = cvRound(box.size.width*0.5);
			ellipse_size.height = cvRound(box.size.height*0.5);
			ratio = (double)ellipse_size.width / (double)ellipse_size.height;

			cvReleaseMemStorage(&storage_fit);
		}
		
		cvReleaseImage(&ellipse);
	}
	delete[] distance;
	
	if (ratio<0.05)
	{
		if (!blink_symbol)		blink_count++;													//��һ֡Ϊ���ۣ��Ҹ߶�С���ȵ�0.05����գ�۴�����1
		blink_symbol = 1;																				//��״̬����Ϊ����
	}
	else	if (blink_symbol&&ratio>0.1)	 blink_symbol = 0;						//��һ֡Ϊ���ۣ��Ҹ߶ȴ��ڿ�ȵ�0.1��״̬����Ϊ����

	t_end = (double)cvGetTickCount();
	double t = (t_end - t_begin) / ((double)cvGetTickFrequency()*1000000.);				//��10sΪ��������
	
	if (t >= period)
	{
		t_begin = t_end;
		blink_frequency = blink_count / period;
		blink_count_temp = blink_count;
		blink_count = 0;
	}
	
																			
	cvReleaseImage(&canny);
	cvReleaseImage(&gray);
	cvReleaseImage(&gray_smooth);

	Mesage2UI* message2ui = new Mesage2UI;
	message2ui->img = edge;
	message2ui->frequency = blink_frequency;
	message2ui->number = blink_count_temp;
	::PostMessage(hwnd, WM_RECVDATA, 0, (LPARAM)message2ui);
//	cvReleaseImage(&edge);
	EnterCriticalSection(&mynotation);
	thread_is_end = 0;								//��־�߳��ѽ���
	LeaveCriticalSection(&mynotation);
	return 0;
}

//�����߳��е����뺯��
void CmymfcDlg::noise_eliminate(IplImage* src)
{
	IplImage* dst = cvCloneImage(src);
	uchar* data_src = (uchar*)src->imageData;
	uchar* data_dst = (uchar*)dst->imageData;
	int step = src->widthStep / sizeof(uchar);
	int width = src->width;
	int height = src->height;
	int i, j;
	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			if (data_src[j*step + i])
			{
				float count_pixel = data_src[j*step + i] + data_src[(j - 1)*step + i - 1] + data_src[j*step + i - 1] + data_src[(j + 1)*step + i - 1]
					+ data_src[(j - 1)*step + i + 1] + data_src[j*step + i + 1] + data_src[(j + 1)*step + i + 1];
				if (count_pixel <= 255 * 2)
					data_dst[j*step + i] = 0;
			}
		}
	}
	cvCopy(dst, src);
	cvReleaseImage(&dst);
}

//UI�Թ����̵߳���Ϣ����
LRESULT CmymfcDlg::OnRecvData(WPARAM wParam, LPARAM lParam)
{
	IplImage* img = ((Mesage2UI*)lParam)->img;
	cvResize(img, Show_Edge, 1);
	ShowImage(Show_Edge, IDC_ShowEdge);
	m_BlinkCount = ((Mesage2UI*)lParam)->number;
	m_BlinkFreq = ((Mesage2UI*)lParam)->frequency;
	if (m_BlinkFreq > 0.5)	m_result = "Σ��";
	else    m_result = "��ȫ";
	UpdateData(FALSE);
	return TRUE;
}

//����ˢ����ʾ��
void CmymfcDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CvRect mark_eye;
	//		memset(&mark_eye, 0, sizeof(CvRect));
	bool eye_is_found = 0;
	IplImage *frame, *frame_copy = 0;
	if (!cvGrabFrame(capture))	return;														//������ȡһ֡
	frame = cvRetrieveFrame(capture);
	if (!frame)		return;
	if (!frame_copy)
		frame_copy = cvCreateImage(cvGetSize(frame),
		IPL_DEPTH_8U, frame->nChannels);

	if (frame->origin == IPL_ORIGIN_TL)													//�����Ͻ���Ϊԭ��
		cvCopy(frame, frame_copy, 0);
	else
		cvFlip(frame, frame_copy, 0);

	IplImage*frame_clone = cvCloneImage(frame_copy);

	if (count_face < N_FACE)											//����ǰ20֡Ԥ��������ͳ��������С����������������
	{
		pre_process(frame_copy, mark_face);
	}

	if (count_face >= N_FACE)
	{
		eye_is_found = detect_and_draw(frame_copy, mark_face, mark_eye);			//Ѱ������������
		if (eye_is_found)
		{
			CvRect src_eye_rect;
			src_eye_rect.height = mark_eye.height*scale;
			src_eye_rect.x = mark_eye.x*scale;
			src_eye_rect.y = mark_eye.y*scale + 2 * src_eye_rect.height / 5;				//ȡ��ѡ������°벿����Ϊ���۵Ĵ�������
			src_eye_rect.height = 3 * src_eye_rect.height / 5;
			src_eye_rect.width = mark_eye.width*scale;

			while (1)																					//�ȴ���һ֡���߳̽���
			{
				int i = 0;
				EnterCriticalSection(&mynotation);
				if (!thread_is_end)	i = 1;
				LeaveCriticalSection(&mynotation);
				if(i)	break;
			}
			IplImage* eye_pro = cvCreateImage(cvSize(src_eye_rect.width, src_eye_rect.height), frame_copy->depth, frame_copy->nChannels);
			cvSetImageROI(frame_clone, src_eye_rect);
			cvCopy(frame_clone, eye_pro, 0);
			cvResetImageROI(frame_clone);

			cvResize(eye_pro, Show_Eye, 1);
			ShowImage(Show_Eye, IDC_ShowEye);

			Mesasge2Work* message2work = new Mesasge2Work;
			message2work->hwnd = m_hWnd;
			message2work->img = eye_pro;

			HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Eye_Process, (LPVOID)message2work, 0, NULL);			//�����߳̾��
			//			cvReleaseImage(&eye_pro);
			CloseHandle(hThread);																											//�����߳̾��			
		}
	}
	cvResize(frame_copy, Show_Face, 1);
	ShowImage(Show_Face, IDC_ShowFace);

	cvReleaseImage(&frame_copy);
	cvReleaseImage(&frame_clone);

	CDialog::OnTimer(nIDEvent);
}


void CmymfcDlg::OnBnClickedTheend()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	m_start.EnableWindow(TRUE);
	m_end.EnableWindow(FALSE);																//�رս�������

	cvReleaseCapture(&capture);
	cvReleaseHaarClassifierCascade(&cascade);
	cvReleaseHaarClassifierCascade(&cascade_eye);

	//��ͼ����������
	if (Show_Face)
	{
		cvZero(Show_Face);
		ShowImage(Show_Face, IDC_ShowFace);
	}
	if (Show_Eye)
	{
		cvZero(Show_Eye);
		ShowImage(Show_Eye, IDC_ShowEye);
	}
	if (Show_Edge)
	{
		cvZero(Show_Edge);
		ShowImage(Show_Edge, IDC_ShowEdge);
	}
	blink_frequency=0;
	blink_count_temp=0;
	m_BlinkCount = 0;
	m_BlinkFreq = 0;
	m_result = "δ����";
	UpdateData(FALSE);
}


void CmymfcDlg::OnEnChangeNumber()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialog::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void CmymfcDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CSliderCtrl* SliderLow = (CSliderCtrl*)GetDlgItem(IDC_LowThresh);		//���û������ķ�Χ�ͳ�ʼֵ
	CSliderCtrl* SliderHigh = (CSliderCtrl*)GetDlgItem(IDC_HighThresh);		//���û������ķ�Χ�ͳ�ʼֵ
	LowThresh = SliderLow->GetPos();
	HighThresh = SliderHigh->GetPos();
	m_LowThreshValue = LowThresh;
	m_HighThreshValue = HighThresh;
	UpdateData(FALSE);
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}
