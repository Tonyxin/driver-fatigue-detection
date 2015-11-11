#include<opencv2\opencv.hpp>
#include<stdio.h>
#include<iostream>

using namespace std;
using namespace cv;

#ifdef _EIC														//如果定义了_EiC，则将_EiC定义为WIN32
#define WIN32
#endif

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;

//系统变量
const char* cascade_name;

void detect_and_draw(IplImage*);

int main(int argc, char** argv)
{
	CvCapture*	capture;
	IplImage *frame, *frame_copy = 0;
	int optlen = strlen("--cascade=");
	const char* input_name;

	if (argc > 1 && strncmp(argv[1], "--cascde=", optlen) == 0)
	{
		cascade_name = argv[1] + optlen;
		input_name = argc > 2 ? argv[2] : 0;
	}
	else
	{
		cascade_name = ".\\haarcascade_frontalface_alt2.xml";
		input_name = argc > 1 ? argv[1] : 0;
	}
	// 加载分类器
	cascade = (CvHaarClassifierCascade*)cvLoad(cascade_name, 0, 0, 0);

	if (!cascade)
	{
		fprintf(stderr, "Error: Could not load classfier cascade\n");
		fprintf(stderr,
			"Usage:facedetect --cascade=\"<cascade_path>\"[filename|camera_index]\n");
		return -1;
	}

	

	if (!input_name || (isdigit(input_name[0]) && input_name[1] == '\0'))				//从摄像头中提取
		capture = cvCaptureFromCAM(!input_name ? 0 : input_name[0] - '\0');
	else		capture = cvCaptureFromAVI(input_name);

	cvNamedWindow("result", 1);

	while (capture)
	{
		storage = cvCreateMemStorage(0);
		if (!cvGrabFrame(capture))						//抓取一帧
			break;
		frame = cvRetrieveFrame(capture);			//从帧中抽取图像，这两步的作用相当于cvQueryFrame
		if (!frame)													
			break;
		if (!frame_copy)
			frame_copy = cvCreateImage(cvSize(frame->width, frame->height),
			IPL_DEPTH_8U, frame->nChannels);

		if (frame->origin == IPL_ORIGIN_TL)		//调整图像原点为左上角
			cvCopy(frame, frame_copy, 0);
		else
			cvCopy(frame, frame_copy, 0);

		detect_and_draw(frame_copy);

		if (cvWaitKey(10) >= 0)							//10ms内按任意键退出
			break;													//做视频显示，每一帧show完后，一定要等待一段时间
																		//不然无法显示
	}

	cvReleaseImage(&frame_copy);
	cvReleaseCapture(&capture);
	cvReleaseHaarClassifierCascade(&cascade);

	cvDestroyWindow("result");
	return 0;
}

void detect_and_draw(IplImage* img)
{
	double scale = 1.3;
	static CvScalar colors[] =
	{
		{ { 0, 0, 255 } },
		{ { 0, 128, 255 } },
		{ { 0, 255, 255 } },
		{ { 0, 255, 0 } },
		{ { 255, 128, 0 } },
		{ { 255, 255, 0 } },
		{ { 255, 0, 0 } },
		{ { 255, 0, 255 } }
	};

	IplImage* gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	IplImage* small_img = cvCreateImage(cvSize(cvRound(img->width / scale),
		cvRound(img->height / scale)), 8, 1);

	cvCvtColor(img, gray, CV_BGR2GRAY);					//转变为灰度图
	cvResize(gray, small_img, CV_INTER_LINEAR);		//将灰度图缩小
	cvEqualizeHist(small_img, small_img);					//灰度图象直方图均衡化
	cvClearMemStorage(storage);								//存储前先清空内存

	if (cascade)
	{
		double t = (double)cvGetTickCount();
		CvSeq* faces = cvHaarDetectObjects(small_img, cascade, storage,
			1.2, 2, CV_HAAR_DO_CANNY_PRUNING, cvSize(30, 30));			//选取先做检测边缘的方式能够降低检测时间
		t = (double)cvGetTickCount() - t;
		cout << "detection time=" << t / ((double)cvGetTickFrequency()*1000.) << "ms" << endl;

		for (int i = 0; i < (faces ? faces->total : 0); i++)
		{
			CvRect* r = (CvRect*)cvGetSeqElem(faces, i);
			cvRectangle(img, cvPoint(r->x * scale, r->y * scale), cvPoint((r->x + r->width) * scale, 
				(r->y + r->height)  * scale), colors[i % 8], 3, 8, 0);
		}
	}
	cvShowImage("result", img);
	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);
}


	


