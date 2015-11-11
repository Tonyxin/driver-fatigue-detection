#include <opencv2\opencv.hpp>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
using namespace cv;
using namespace std;
/*
不管是用data指针访问还是val[]，0均表示黑色，255表示白色
*/
void image_pro(IplImage*, IplImage*, IplImage*, IplImage*);									//利用高斯模型，转化为灰度图并二值化
float p_cal(uchar, uchar);																						//计算各点处于人脸的概率
void border(IplImage*, IplImage*, IplImage*, int &, int &, int &, int &);					//利用投影积分寻找框选人脸的矩形四个顶点
void mark(IplImage*, int, int, int, int);																		//在原图上画出框选人脸的矩形

int main()
{	
	clock_t begin, end;																								//用于记录程序运行时间
	double  cost;
	begin = clock();

	IplImage*src = cvLoadImage("00740_941201_fa.ppm", CV_LOAD_IMAGE_COLOR);					//src指向原图
	IplImage*ycbcr = cvCreateImage(cvGetSize(src), src->depth, src->nChannels);	//dst为ycbcr下的图	
	IplImage*y = cvCreateImage(cvGetSize(src), src->depth, 1);									//获取Cb,Cr通道分量
	IplImage*cr = cvCreateImage(cvGetSize(src), src->depth, 1);
	IplImage*cb = cvCreateImage(cvGetSize(src), src->depth, 1);
	IplImage*gray = cvCreateImage(cvGetSize(src), src->depth, 1);							//gray是皮肤部分的概率灰度图
	IplImage*bin = cvCreateImage(cvGetSize(src), src->depth, 1);								//bin是二值化图
	IplImage*median_filter = cvCreateImage(cvGetSize(src), src->depth, 1);				//median_filter是中值滤波后的图
	IplImage*paintx = cvCreateImage(cvGetSize(src), src->depth, 1);							//paintx是竖直投影积分图
	IplImage*painty = cvCreateImage(cvGetSize(src), src->depth, 1);							//painty是水平投影积分图

	IplImage*gray1 = cvCreateImage(cvGetSize(src), src->depth, 1);							

	

	int Vleft, Vright, Htop, Hlow;																	//Vleft为左边界，Vright为右边界，Hlow为下边界，Htop为上边界
	int height = src->height;																		//获取图像的尺寸
	int width = src->width;


	cout << "width=" << width << endl;
	cout << "height=" << height << endl;

	cvCvtColor(src, ycbcr, CV_BGR2YCrCb);												//将原图转换到YCbCr空间

	cvCvtColor(src, gray1, CV_BGR2GRAY);													//得到正常的灰度图

	cvSplit(ycbcr, y, cr, cb, 0);																		//提取Cr,Cb色彩通道

	image_pro(cr, cb, bin, gray);																	//利用高斯模型转化为灰度图并二值化

	cvSmooth(bin, median_filter, CV_MEDIAN);											//中值滤波
	
	border(median_filter, paintx, painty, Htop, Hlow, Vleft, Vright);				//确定框选边界

	CvSize size;																							//选出人脸的区域，进行分割
	size.height = Hlow - Htop;
	size.width = Vright - Vleft;
	cvSetImageROI(gray, cvRect(Vleft, Htop, size.width, size.height));
	IplImage*gray_cut = cvCreateImage(size, src->depth, 1);
	cvCopy(gray, gray_cut, 0);
	cvResetImageROI(gray);
	
	IplImage*sobel = cvCreateImage(size, IPL_DEPTH_16S, 1);					//利用sobel算子凸显出眼睛区域
	cvSobel(gray_cut, sobel, 1, 0, 3);
	IplImage *sobel8u = cvCreateImage(size, IPL_DEPTH_8U, 1);
	cvConvertScaleAbs(sobel, sobel8u, 1, 0);


	cout << "Vleft=" << Vleft << endl;
	cout << "Vright=" << Vright << endl;
	cout << "Htop=" << Htop << endl;
	cout << "Hlow=" << Hlow << endl;

	mark(src, Htop, Hlow, Vleft, Vright);														//在原图上添加框
	
	cvShowImage("sobel", sobel8u);
	cvShowImage("gray_cut", gray_cut);
	cvShowImage("gray", gray);
	cvShowImage("gray1", gray1);
	cvShowImage("bin", bin);
	cvShowImage("median_filter", median_filter);
	cvShowImage("paintx", paintx);
	cvShowImage("painty", painty);
	cvShowImage("结果", src);

	end = clock();
	cost = (double)(end - begin) / CLOCKS_PER_SEC;
	cout << "花费时间=" << cost << "s" << endl;
	waitKey();
	return 0;
}


//用于计算各个点是否为处于人脸的概率
float p_cal(uchar a, uchar b)														
{
	float p;
	float c[2][2] = { 231.1231, 9.7823, 9.7823, 115.2362 };
	float c_inv[2][2];
	float tmp = c[0][0] * c[1][1] - c[0][1] * c[1][0];										//求取c的逆矩阵
	c_inv[0][0] = c[1][1] / tmp;																
	c_inv[0][1] = -c[1][0] / tmp;
	c_inv[1][0] = -c[0][1] / tmp;
	c_inv[1][1] = c[0][0] / tmp;
	float m[2][1] = { 148.5632, 116.9231 };																
	p = (a - m[0][0])*(a - m[0][0])*c_inv[0][0] + (b - m[1][0])*(a - m[0][0])*c_inv[1][0] + //主要是数学公式计算概率
		(a - m[0][0])*(b - m[1][0])*c_inv[0][1] + (b - m[1][0])*(b - m[1][0])*c_inv[1][1];
	p = exp((-0.5)*p) * 255;
	return p;
}

//利用投影积分寻找框选人脸的矩形四顶点
// c++中的引用是一种很有效的地址传递方式，不需要指针
void border(IplImage* img, IplImage* paintx, IplImage* painty, int &Htop, int &Hlow, int &Vleft, int &Vright)		
{
	int tmp_V, tmp_H, Vmax, Hmax, i, j;
	int width = img->width;
	int height = img->height;
	uchar s;
	uchar*data = (uchar*)img->imageData;
	uchar*data_x = (uchar*)paintx->imageData;
	uchar*data_y = (uchar*)painty->imageData;
	int step = img->widthStep / sizeof(uchar);
	int* v = new int[width];																		//v[i]存放每一列上黑色像素点个数
	int* h = new int[height];																	//h[j]存放每一行上黑色像素点个数
	memset(v, 0, width*sizeof(int));															//初始化为0
	memset(h, 0, height*sizeof(int));
	tmp_V = 0;
	tmp_H = 0;
	Vmax = v[0];
	Hmax = h[0];

	for(i = 0; i<width; i++)
	{
		for (j = 0; j<height; j++)
		{
			s = data[j*step + i];
			if (s == 255)	v[i]++;																	//做竖直方向的投影积分
		}
		if (v[i]>Vmax)																				//寻找到积分最大的那一列
		{
			Vmax = v[i];
			tmp_V = i;
		}
	}

	for (i = 0; i < width; i++)
	{
		for (j = 0; j < height; j++)
		{
			if (j < v[i])	data_x[j*step + i] = 255;
			else    data_x[j*step + i] = 0;
		}
	}

	for (j = 0; j<height; j++)
	{
		for (i = 0; i<width; i++)
		{
			s = data[j*step + i];
			if (s == 255)	h[j]++;																	//做水平方向地投影积分
		}																									
		if (h[j]>Hmax)																				//寻找到积分最大的那一行
		{
			Hmax = h[j];
			tmp_H = j;
		}
	}
	for (j = 0; j < height; j++)
	{
		for (i = 0; i < width; i++)
		{
			if (i < h[j])	data_y[j*step + i] = 255;
			else    data_y[j*step + i] = 0;
		}
	}
	for (i = 0; i < tmp_V; i++)															//从竖直积分最大的一列，向两边寻找第一次积分值小于0.3*Vmax
	{																										//作为左右边界
		if (v[i] > 0.3*Vmax)
		{
			Vleft = i;
			break;
		}
	}

	for (i = width; i > tmp_V; i--)
	{
		if (v[i] > 0.3*Vmax)
		{
			Vright = i;
			break;
		}
	}

	for (j = 0; j < tmp_H; j++)																	//从水平积分最大的一行，向上找到第一次小于0.3*Hmax为上边界				
	{
		if (h[j] > 0.3*Hmax)
		{
			Htop = j;
			break;
		}
	}

	for (j = tmp_H; j < height; j++)															//向下找到第一次小于0.5*Hmax为下边界（主要去除脖子的影响）
	{
		if (h[j] < 0.491*Hmax)
		{
			Hlow = j;
			break;
		}
	}
	delete [] v;																							//释放申请的动态内存
	delete [] h;																						//类对象数组回收注意前面应加[]，基本类型数组前可加可不加
}

//用于在原图上画出框
void mark(IplImage*img, int top, int low, int left, int right)
{
	int i, j;
	uchar* data_img = (uchar*)img->imageData;
	int step_img = img->widthStep / sizeof(uchar);
	int channels = img->nChannels;
	for (i = top; i < low; i++)																					//将左右边界画为红色
	{
		data_img[i*step_img + left*channels + 0] = 0;
		data_img[i*step_img + left*channels + 1] = 0;
		data_img[i*step_img + left*channels + 2] = 255;
		data_img[i*step_img + right*channels + 0] = 0;
		data_img[i*step_img + right*channels + 1] = 0;
		data_img[i*step_img + right*channels + 2] = 255;
	}

	for (j = left; j < right; j++)																					//将上下边界画为红色
	{
		data_img[top*step_img + j*channels + 0] = 0;
		data_img[top*step_img + j*channels + 1] = 0;
		data_img[top*step_img + j*channels + 2] = 255;
		data_img[low*step_img + j*channels + 0] = 0;
		data_img[low*step_img + j*channels + 1] = 0;
		data_img[low*step_img + j*channels + 2] = 255;
	}
}

//利用高斯模型转化为灰度图并二值化
void image_pro(IplImage*Cr, IplImage*Cb, IplImage*Bin, IplImage*Gray)				
{
	int i, j;
	uchar tmp_cb, tmp_cr, probablity;
	uchar* data_gray = (uchar *)Gray->imageData;												//利用指针访问各点像素
	uchar* data_bin = (uchar *)Bin->imageData;
	uchar* data_cb = (uchar *)Cb->imageData;
	uchar* data_cr = (uchar *)Cr->imageData;
	int step = Cr->widthStep / sizeof(uchar);
	int height = Cr->height;
	int width = Cr->width;
	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			tmp_cb = data_cb[i*step + j];																	//读取cb该点的像素值
			tmp_cr = data_cr[i*step + j];
			probablity = p_cal(tmp_cb, tmp_cr);
			data_gray[i*step + j] = probablity;																//probablity值越小表示越是黑色（人脸）部分
			if (probablity < 0.13 * 255)		data_bin[i*step + j] = 255;						//将人脸的部分点为白色
			else    data_bin[i*step + j] = 0;
		}
	}
}