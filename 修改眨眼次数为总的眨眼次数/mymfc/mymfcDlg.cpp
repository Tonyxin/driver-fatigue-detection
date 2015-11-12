
// mymfcDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "mymfc.h"
#include "mymfcDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CRITICAL_SECTION mynotation;					//此处是声明全局变量的地方
double blink_frequency = 0;
int blink_count_temp=0;
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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


// CmymfcDlg 对话框



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

//对类中静态变量的初始化
int CmymfcDlg::N_FACE = 20;										//利用20帧得到人脸平均面积
int CmymfcDlg::count_face=0;								//用于统计校验时检测人脸数
int CmymfcDlg::MAX_FACE=0;
int CmymfcDlg::MIN_FACE=0;
float CmymfcDlg::scale=1.3;										//检测人脸时用于缩放，加快检测进度
CvScalar CmymfcDlg::colors={ { 0, 0, 255 } };

bool CmymfcDlg::thread_is_end=0;							//用于线程同步

double CmymfcDlg::t_begin=0;								//与t_end结合，用于统计10s内的眨眼次数
double CmymfcDlg::t_end=0;

CvCapture* CmymfcDlg::capture = 0;
CvRect CmymfcDlg::mark_face;

int CmymfcDlg::LowThresh;
int CmymfcDlg::HighThresh;

//消息响应机制
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


// CmymfcDlg 消息处理程序

BOOL CmymfcDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	m_start.EnableWindow(TRUE);																//激活“开始”按键
	m_end.EnableWindow(FALSE);																//关闭结束按键

	CSliderCtrl* SliderLow = (CSliderCtrl*)GetDlgItem(IDC_LowThresh);		//设置滑动条的范围和初始值
	CSliderCtrl* SliderHigh = (CSliderCtrl*)GetDlgItem(IDC_HighThresh);
	SliderLow->SetRange(0, 200, TRUE);														//低阈值初始值为60
	SliderLow->SetPos(60);
	LowThresh = SliderLow->GetPos();
	SliderHigh->SetRange(0, 200, TRUE);													//高阈值初始值为100	
	SliderHigh->SetPos(100);
	HighThresh = SliderHigh->GetPos();

	CRect rect_face, rect_eye, rect_edge;
	GetDlgItem(IDC_ShowFace)->GetClientRect(&rect_face);								//获取相应的显示区域
	GetDlgItem(IDC_ShowEye)->GetClientRect(&rect_eye);
	GetDlgItem(IDC_ShowEdge)->GetClientRect(&rect_edge);
	int rw_face = rect_face.right - rect_face.left;													 // 求出脸部控件的宽和高
	int rh_face = rect_face.bottom - rect_face.top;
	int rw_eye = rect_eye.right - rect_eye.left;														 // 求出眼部控件的宽和高
	int rh_eye = rect_eye.bottom - rect_eye.top;
	int rw_edge = rect_edge.right - rect_edge.left;												 // 求出边缘控件的宽和高
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
	m_result="未启动";
	UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CmymfcDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
		CDialog::UpdateWindow();                // 更新windows窗口，如果无这步调用，图片显示还会出现问题
		ShowImage(Show_Face, IDC_ShowFace);    // 重绘图片函数
		ShowImage(Show_Eye, IDC_ShowEye);
		ShowImage(Show_Edge, IDC_ShowEdge);
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CmymfcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//单击开始按钮
void CmymfcDlg::OnBnClickedStart()
{
	// TODO:  在此添加控件通知处理程序代码
	m_BlinkCount = 0;
	m_BlinkFreq = 0;
	m_start.EnableWindow(FALSE);							//关闭“开始按键”
	m_end.EnableWindow(TRUE);							//激活结束“按键”
	InitializeCriticalSection(&mynotation);
	int optlen = strlen("--cascade=");
	// 加载分类器
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

	capture = cvCaptureFromCAM(0);										//从摄像头中抓取帧

	t_begin = (double)cvGetTickCount();										//用于计时10s

	if (cascade)
	{
		CvRect mark_eye;
		memset(&mark_eye, 0, sizeof(CvRect));
		bool eye_is_found = 0;
		IplImage *frame, *frame_copy = 0;
		if (!cvGrabFrame(capture))	return;														//用于提取一帧
		frame = cvRetrieveFrame(capture);
		if (!frame)		return;
		if (!frame_copy)
			frame_copy = cvCreateImage(cvGetSize(frame),
			IPL_DEPTH_8U, frame->nChannels);

		if (frame->origin == IPL_ORIGIN_TL)													//将左上角设为原点
			cvCopy(frame, frame_copy, 0);
		else
			cvFlip(frame, frame_copy, 0);

		IplImage*frame_clone = cvCloneImage(frame_copy);

		if (count_face < N_FACE)											//利用前20帧预处理，包括统计人脸大小，对人脸进行锁定
		{
			pre_process(frame_copy, mark_face);
		}

		if (count_face >= N_FACE)
		{
			eye_is_found = detect_and_draw(frame_copy, mark_face, mark_eye);			//寻找人脸、人眼
			if (eye_is_found)
			{
				CvRect src_eye_rect;
				src_eye_rect.height = mark_eye.height*scale;
				src_eye_rect.x = mark_eye.x*scale;
				src_eye_rect.y = mark_eye.y*scale + 2 * src_eye_rect.height / 5;				//取框选区域的下半部分作为人眼的处理区域
				src_eye_rect.height = 3 * src_eye_rect.height / 5;
				src_eye_rect.width = mark_eye.width*scale;

				while (1)																					//等待上一帧的线程结束
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
				
				HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Eye_Process, (LPVOID)message2work, 0, NULL);			//创建线程句柄
	//			cvReleaseImage(&eye_pro);
				CloseHandle(hThread);																											//撤销线程句柄			

			}
		}

		cvResize(frame_copy, Show_Face, 1);
		ShowImage(Show_Face, IDC_ShowFace);
			
		cvReleaseImage(&frame_copy);
		cvReleaseImage(&frame_clone);
	}
	//设置触发器，每隔20s触发一次事件
	SetTimer(1, 20, NULL);
	return;
}

//用于在对话框指定位置输出图像
void CmymfcDlg::ShowImage(IplImage* img, UINT ID)
{
	CDC* pDC = GetDlgItem(ID)->GetDC();										//获取显示控件的DC
	HDC hDC = pDC->GetSafeHdc();												//获取HDC（设备句柄），用于画图
	CRect rect;
	GetDlgItem(ID)->GetClientRect(&rect);
	int rw = rect.right - rect.left;														// 求出图片控件的宽和高
	int rh = rect.bottom - rect.top;
	SetRect(rect, 0, 0, rw, rh);
	CvvImage cimg;
	cimg.CopyOf(img);
	cimg.DrawToHDC(hDC, &rect);
	ReleaseDC(pDC);
}

//用于图像预处理
void CmymfcDlg::pre_process(IplImage* img, CvRect& rect)
{
	int i, AVG_FACE;
	static int SUM_FACE=0;
	IplImage* gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	//这里做了一步缩放，将原画像缩小了1.3倍
	IplImage* small_img = cvCreateImage(cvSize(cvRound(img->width / scale), cvRound(img->height / scale)), 8, 1);		//cvRound()对double类型进行四舍五入															
	cvCvtColor(img, gray, CV_BGR2GRAY);				//转变为灰度图
	cvResize(gray, small_img, CV_INTER_LINEAR);	//将灰度图缩小
	cvEqualizeHist(small_img, small_img);				//灰度图象直方图均衡化
	cvClearMemStorage(storage);

	CvSeq* faces = cvHaarDetectObjects(small_img, cascade, storage,
		1.2, 2, CV_HAAR_DO_CANNY_PRUNING,
		cvSize(30, 30));
	// 统计面部区域的像素个数，用于检测过小的面部目标，忽略对面积过小的面部的检测
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
			CvRect *face = (CvRect*)cvGetSeqElem(faces, i); // 获取第i个人脸
			area_face[i] = face->height*face->width; // 计算人脸区域的像素个数
			if (area_face[i]>max_face_area)
			{
				// 修改最大的面部区域大小和对应的面部区域的索引
				max_face_area = area_face[i];
				max_face_area_index = i;
			}
		}
		SUM_FACE = SUM_FACE + max_face_area;  // 累计头像区域
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

//检测人脸区域
bool CmymfcDlg::detect_and_draw(IplImage* img, CvRect& rect_face, CvRect& rect_eye)
{
	bool eye_is_found = 0;
	CvPoint point;
	point.x = rect_face.x + rect_face.width / 2;
	point.y = rect_face.y + rect_face.height / 2;

	IplImage* gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	//这里做了一步缩放，将原画像缩小了1.3倍
	IplImage* small_img = cvCreateImage(cvSize(cvRound(img->width / scale), cvRound(img->height / scale)), 8, 1);		//cvRound()对double类型进行四舍五入															
	int i;
	cvCvtColor(img, gray, CV_BGR2GRAY);				//转变为灰度图
	cvResize(gray, small_img, CV_INTER_LINEAR);	//将灰度图缩小
	cvEqualizeHist(small_img, small_img);				//灰度图象直方图均衡化
	cvClearMemStorage(storage);

	//设置前一帧人脸区域为感兴趣区域，便于在后一帧中提高运算效率
	CvRect ROI;
	ROI.x = 2 * rect_face.x / 3;
	ROI.y = 2 * rect_face.y / 3;
	ROI.height = (small_img->height + 2 * rect_face.height) / 3;
	ROI.width = (small_img->width + 2 * rect_face.width) / 3;
	cvSetImageROI(small_img, ROI);

	//这里检测人脸是在感兴趣的范围内进行的，提高了代码的运算效率
	CvSeq* faces = cvHaarDetectObjects(small_img, cascade, storage,
		1.2, 2, CV_HAAR_DO_CANNY_PRUNING,					//选取先做检测边缘的方式能够降低检测时间
		cvSize(small_img->width / 5, small_img->height / 5));
	cvResetImageROI(small_img);

	for (i = 0; i < (faces ? faces->total : 0); i++)
	{
		//注意：因为在检测人脸的时候是在感兴趣的范围内，所以人脸矩形是以ROI区域中进行计算的，
		//也就是不在以原图的原点为原点，而是以ROI区域的原点为原点
		//因此，在原图中人脸矩形需要进行坐标变换到原图中，即横纵坐标加上ROI.x,ROI.y
		CvRect* r = (CvRect*)cvGetSeqElem(faces, i);					//从一序列中挑选出第i个
		r->x = r->x + ROI.x;
		r->y = r->y + ROI.y;
		//用矩形框框出  
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

//检测眼部
bool CmymfcDlg::detect_eye(IplImage* img_eye, CvRect rect_face, CvRect& rect_eye)
{
	bool eye_is_found = 0;
	cvClearMemStorage(storage_eye);
	rect_face.x = rect_face.x + rect_face.width / 2;		//仅检测左眼
	rect_face.width = rect_face.width / 2;					//仅检测检测人脸上半部分位置
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
			CvRect *eye = (CvRect*)cvGetSeqElem(eyes, i); // 获取第i个人眼
			area_eye[i] = eye->height*eye->width; // 计算人眼区域的像素个数
			if (area_eye[i]>max_eye_area)
			{
				// 修改最大的面部区域大小和对应的面部区域的索引
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



//工作线程函数
DWORD  CmymfcDlg::Eye_Process(LPVOID lpParameter)
{
	EnterCriticalSection(&mynotation);
	thread_is_end = 1;																						//表示线程正在执行
	LeaveCriticalSection(&mynotation);
	float ratio=0;																										//表示眼睛的宽长比
	static int blink_count=0;								//眨眼次数
	static bool blink_symbol=0;							//眨眼标志，0表示眼睛睁开，1表示眼睛闭合
	static double period=10;								//统计时间周期为10s

	IplImage* src = ((Mesasge2Work*)lpParameter)->img;						//从参数中提取信息
	HWND hwnd = ((Mesasge2Work*)lpParameter)->hwnd;

	IplImage*gray = cvCreateImage(cvGetSize(src), src->depth, 1);													//转变为灰度图
	IplImage*gray_smooth = cvCreateImage(cvGetSize(src), src->depth, 1);									//做了一步中值滤波，消噪
	IplImage*canny = cvCreateImage(cvGetSize(src), src->depth, 1);												//利用canny算子提取边缘

	cvCvtColor(src, gray, CV_BGR2GRAY);
	cvSmooth(gray, gray_smooth, CV_MEDIAN);												//中值滤波（中值滤波一次就够了 两次的话效果不怎么样）
	//但通道的图像增强后也会引入噪声，对于canny算子提取边缘不要进行图像增强
	cvCanny(gray_smooth, canny, LowThresh, HighThresh, 3);													//利用Canny算子进行边缘提取

	int width = canny->width;
	int height = canny->height;
	int step = canny->widthStep / sizeof(uchar);
	int i, j;

	IplImage* edge = cvCreateImage(cvGetSize(src), src->depth, 1);												//提取眼睛上边缘
	cvZero(edge);
	uchar* data_canny = (uchar*)canny->imageData;
	uchar* data_edge = (uchar*)edge->imageData;
	//提取眼睛的上边缘
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

	//进行消噪，消噪的次数可变
	for (i = 0; i < 2; i++)
		noise_eliminate(edge);


	//寻找上边缘左右端点
	int count_up = 0;																							//上边缘像素点的个数
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

	int sum_pixel = 0;																						//用于判定是否处于闭眼状态
	int* distance = new int[count_up + 1];														//distance存放左右端点连线到上边缘对应个点距离
	memset(distance, 0, (count_up + 1)*sizeof(int));
	int k = 0;
	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			if (data_edge[j*step + i])
			{
				//获取上边缘个点到基准线对应各点距离
				distance[k] = (i - left_point.x)*(right_point.y - left_point.y) / (right_point.x - left_point.x) + left_point.y - j;
				sum_pixel += distance[k];
				k++;
			}
		}
	}
	if (sum_pixel < count_up / 2)
		ratio = 0;													//若提取的边缘在基准线以下，则认为是闭眼状态
	else
	{
		CvPoint* up_point = new CvPoint[count_up + 1];									//将上边缘所有的点存入点数组up_point
		CvPoint* down_point = new CvPoint[count_up + 1];								//将上边缘各点向下翻转，存入点数组down_point
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

		IplImage* ellipse = cvCreateImage(cvGetSize(src), src->depth, 1);		//保存上下边缘所有的像素点
		cvZero(ellipse);																									//创建图像后最好进行初始化

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
		if (!blink_symbol)
		{
			blink_count++;													//上一帧为睁眼，且高度小与宽度的0.05，则眨眼次数加1
			blink_count_temp++;
		}
		blink_symbol = 1;																				//将状态设置为闭眼
	}
	else	if (blink_symbol&&ratio>0.1)	 blink_symbol = 0;						//上一帧为闭眼，且高度大于宽度的0.1，状态设置为睁眼

	t_end = (double)cvGetTickCount();
	double t = (t_end - t_begin) / ((double)cvGetTickFrequency()*1000000.);				//以10s为计数周期
	
	if (t >= period)
	{
		t_begin = t_end;
		blink_frequency = blink_count / period;
//	    blink_count_temp = blink_count;
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
	thread_is_end = 0;								//标志线程已结束
	LeaveCriticalSection(&mynotation);
	return 0;
}

//工作线程中的消噪函数
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
				if (i == width - 1 || j == height - 1)		data_dst[j*step + i] = 0;
				else
				{
					float count_pixel = data_src[j*step + i] + data_src[(j - 1)*step + i - 1] + data_src[j*step + i - 1] + data_src[(j + 1)*step + i - 1]
						+ data_src[(j - 1)*step + i + 1] + data_src[j*step + i + 1] + data_src[(j + 1)*step + i + 1];
					if (count_pixel <= 255 * 2)
						data_dst[j*step + i] = 0;
				}
			}
		}
	}
	cvCopy(dst, src);
	cvReleaseImage(&dst);
}

//UI接收工作线程的消息函数
LRESULT CmymfcDlg::OnRecvData(WPARAM wParam, LPARAM lParam)
{
	IplImage* img = ((Mesage2UI*)lParam)->img;
	cvResize(img, Show_Edge, 1);
	ShowImage(Show_Edge, IDC_ShowEdge);
	m_BlinkCount = ((Mesage2UI*)lParam)->number;
	m_BlinkFreq = ((Mesage2UI*)lParam)->frequency;
	if (m_BlinkFreq > 0.5)	m_result = "危险";
	else    m_result = "安全";
	UpdateData(FALSE);
	return TRUE;
}

//不断刷新显示屏，
void CmymfcDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CvRect mark_eye;
	//		memset(&mark_eye, 0, sizeof(CvRect));
	bool eye_is_found = 0;
	IplImage *frame, *frame_copy = 0;
	if (!cvGrabFrame(capture))	return;														//用于提取一帧
	frame = cvRetrieveFrame(capture);
	if (!frame)		return;
	if (!frame_copy)
		frame_copy = cvCreateImage(cvGetSize(frame),
		IPL_DEPTH_8U, frame->nChannels);

	if (frame->origin == IPL_ORIGIN_TL)													//将左上角设为原点
		cvCopy(frame, frame_copy, 0);
	else
		cvFlip(frame, frame_copy, 0);

	IplImage*frame_clone = cvCloneImage(frame_copy);

	if (count_face < N_FACE)											//利用前20帧预处理，包括统计人脸大小，对人脸进行锁定
	{
		pre_process(frame_copy, mark_face);
	}

	if (count_face >= N_FACE)
	{
		eye_is_found = detect_and_draw(frame_copy, mark_face, mark_eye);			//寻找人脸、人眼
		if (eye_is_found)
		{
			CvRect src_eye_rect;
			src_eye_rect.height = mark_eye.height*scale;
			src_eye_rect.x = mark_eye.x*scale;
			src_eye_rect.y = mark_eye.y*scale + 2 * src_eye_rect.height / 5;				//取框选区域的下半部分作为人眼的处理区域
			src_eye_rect.height = 3 * src_eye_rect.height / 5;
			src_eye_rect.width = mark_eye.width*scale;

			while (1)																					//等待上一帧的线程结束
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

			HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Eye_Process, (LPVOID)message2work, 0, NULL);			//创建线程句柄
			//			cvReleaseImage(&eye_pro);
			CloseHandle(hThread);																											//撤销线程句柄			
		}
	}
	cvResize(frame_copy, Show_Face, 1);
	ShowImage(Show_Face, IDC_ShowFace);

	cvReleaseImage(&frame_copy);
	cvReleaseImage(&frame_clone);

	CDialog::OnTimer(nIDEvent);
}

//按下"结束"按钮
void CmymfcDlg::OnBnClickedTheend()
{
	// TODO:  在此添加控件通知处理程序代码
	m_start.EnableWindow(TRUE);
	m_end.EnableWindow(FALSE);																//关闭结束按键

	cvReleaseCapture(&capture);
	cvReleaseHaarClassifierCascade(&cascade);
	cvReleaseHaarClassifierCascade(&cascade_eye);

	//对图像数据清零
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
	m_result = "未启动";
	UpdateData(FALSE);
}


void CmymfcDlg::OnEnChangeNumber()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialog::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


//滑动条控件的操作
void CmymfcDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CSliderCtrl* SliderLow = (CSliderCtrl*)GetDlgItem(IDC_LowThresh);		//设置滑动条的范围和初始值
	CSliderCtrl* SliderHigh = (CSliderCtrl*)GetDlgItem(IDC_HighThresh);		//设置滑动条的范围和初始值
	LowThresh = SliderLow->GetPos();
	HighThresh = SliderHigh->GetPos();
	m_LowThreshValue = LowThresh;
	m_HighThreshValue = HighThresh;
	UpdateData(FALSE);
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}
