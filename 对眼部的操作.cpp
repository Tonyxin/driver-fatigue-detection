#include <opencv2\opencv.hpp>
#include <iostream>
using namespace cv;
using namespace std;

void noise_eliminate(IplImage*);
int ImageStretchByHistogram(IplImage *src, IplImage *dst);

int main()
{
	IplImage*src = cvLoadImage("..\\image\\image1.jpg", CV_LOAD_IMAGE_COLOR);					//src指向原图
	IplImage*gray = cvCreateImage(cvGetSize(src), src->depth, 1);												//转变为灰度图
	IplImage*gray_smooth = cvCreateImage(cvGetSize(src), src->depth, 1);									//做了一步中值滤波，消噪
	IplImage*canny = cvCreateImage(cvGetSize(src), src->depth, 1);												//利用canny算子提取边缘
	IplImage*edge = cvCreateImage(cvGetSize(src), src->depth, 1);												//提取眼睛上边缘
	
	float scale = 0;																								//表示眼睛的宽长比

	cvCvtColor(src, gray, CV_BGR2GRAY);
	cvSmooth(gray, gray_smooth, CV_MEDIAN);												//中值滤波（中值滤波一次就够了 两次的话效果不怎么样）
	//但通道的图像增强后也会引入噪声，对于canny算子提取边缘不要进行图像增强
	cvCanny(gray_smooth, canny, 120, 200, 3);													//利用Canny算子进行边缘提取
	
	int width = canny->width;
	int height = canny->height;
	int step = canny->widthStep / sizeof(uchar);
	int i, j;

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
	//进行多次消噪，消噪的次数可变
	for (i = 0; i < 1; i++)
		noise_eliminate(edge);

	int count_up = 0;						//上边缘像素点的个数

	//寻找上边缘左右端点
	CvPoint left_point, right_point;
	left_point.x = 0;
	right_point.x = 0;
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
				break;
			}
		}
	}

	int sum_pixel = 0;										//用于判定是否处于闭眼状态
	int* distance = new int[count_up];														//distance存放左右端点连线到上边缘对应个点距离
	memset(distance, 0, count_up*sizeof(int));
	int k = 0;
	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			if (data_edge[j*step + i])
			{
				distance[k] = (i - left_point.x)*(right_point.y - left_point.y) / (right_point.x - left_point.x) + left_point.y - j;
				sum_pixel += distance[k];
				k++;
			}
		}
	}
	if (sum_pixel < 20)		scale = 0;															//若提取的边缘在基准线以下，则认为是闭眼状态
	else
	{
		CvPoint* up_point = new CvPoint[count_up];									//将上边缘所有的点存入点数组up_point
		CvPoint* down_point = new CvPoint[count_up];								//将上边缘各点向下翻转，存入点数组down_point
		memset(up_point, 0, count_up*sizeof(CvPoint));
		memset(up_point, 0, count_up*sizeof(CvPoint));
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

		IplImage* ellipse = cvCreateImage(cvGetSize(src), src->depth, 1);
		IplImage* ellipse_fit = cvCreateImage(cvGetSize(src), src->depth, 1);
		cvZero(ellipse);																					//创建图像后最好进行初始化
		cvZero(ellipse_fit);

		uchar* data_ellipse = (uchar*)ellipse->imageData;
		for (k = 0; k < count_up; k++)
		{
			data_ellipse[up_point[k].y*step + up_point[k].x] = 255;
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

		CvPoint2D32f* point2d32f = new CvPoint2D32f[count];
		memset(point2d32f, 0, count*sizeof(CvPoint2D32f));

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

		CvPoint ellipse_center;
		CvSize ellipse_size;
		ellipse_center.x = cvRound(box.center.x);
		ellipse_center.y = cvRound(box.center.y);
		ellipse_size.width = cvRound(box.size.width*0.5);
		ellipse_size.height = cvRound(box.size.height*0.5);
		cvEllipse(ellipse_fit, ellipse_center, ellipse_size, box.angle, 0, 360, CV_RGB(255, 255, 255), 1, 8, 0);

		scale = (double)ellipse_size.width / (double)ellipse_size.height;
		cvShowImage("ellipse", ellipse);
		cvShowImage("ellipse_fit", ellipse_fit);
	}

	delete[] distance;
	cout << "scale=" << scale << endl;

	cvShowImage("src", src);
	cvShowImage("gray", gray);
	cvShowImage("gray_smooth", gray_smooth);
	cvShowImage("canny", canny);
	cvShowImage("edge", edge);

	cvWaitKey(-1);
	return 0;
}

void noise_eliminate(IplImage* src)
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

