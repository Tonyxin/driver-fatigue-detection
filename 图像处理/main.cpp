#include <opencv2\opencv.hpp>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
using namespace cv;
using namespace std;
/*
��������dataָ����ʻ���val[]��0����ʾ��ɫ��255��ʾ��ɫ
*/
void image_pro(IplImage*, IplImage*, IplImage*, IplImage*);									//���ø�˹ģ�ͣ�ת��Ϊ�Ҷ�ͼ����ֵ��
float p_cal(uchar, uchar);																						//������㴦�������ĸ���
void border(IplImage*, IplImage*, IplImage*, int &, int &, int &, int &);					//����ͶӰ����Ѱ�ҿ�ѡ�����ľ����ĸ�����
void mark(IplImage*, int, int, int, int);																		//��ԭͼ�ϻ�����ѡ�����ľ���

int main()
{	
	clock_t begin, end;																								//���ڼ�¼��������ʱ��
	double  cost;
	begin = clock();

	IplImage*src = cvLoadImage("00740_941201_fa.ppm", CV_LOAD_IMAGE_COLOR);					//srcָ��ԭͼ
	IplImage*ycbcr = cvCreateImage(cvGetSize(src), src->depth, src->nChannels);	//dstΪycbcr�µ�ͼ	
	IplImage*y = cvCreateImage(cvGetSize(src), src->depth, 1);									//��ȡCb,Crͨ������
	IplImage*cr = cvCreateImage(cvGetSize(src), src->depth, 1);
	IplImage*cb = cvCreateImage(cvGetSize(src), src->depth, 1);
	IplImage*gray = cvCreateImage(cvGetSize(src), src->depth, 1);							//gray��Ƥ�����ֵĸ��ʻҶ�ͼ
	IplImage*bin = cvCreateImage(cvGetSize(src), src->depth, 1);								//bin�Ƕ�ֵ��ͼ
	IplImage*median_filter = cvCreateImage(cvGetSize(src), src->depth, 1);				//median_filter����ֵ�˲����ͼ
	IplImage*paintx = cvCreateImage(cvGetSize(src), src->depth, 1);							//paintx����ֱͶӰ����ͼ
	IplImage*painty = cvCreateImage(cvGetSize(src), src->depth, 1);							//painty��ˮƽͶӰ����ͼ

	IplImage*gray1 = cvCreateImage(cvGetSize(src), src->depth, 1);							

	

	int Vleft, Vright, Htop, Hlow;																	//VleftΪ��߽磬VrightΪ�ұ߽磬HlowΪ�±߽磬HtopΪ�ϱ߽�
	int height = src->height;																		//��ȡͼ��ĳߴ�
	int width = src->width;


	cout << "width=" << width << endl;
	cout << "height=" << height << endl;

	cvCvtColor(src, ycbcr, CV_BGR2YCrCb);												//��ԭͼת����YCbCr�ռ�

	cvCvtColor(src, gray1, CV_BGR2GRAY);													//�õ������ĻҶ�ͼ

	cvSplit(ycbcr, y, cr, cb, 0);																		//��ȡCr,Cbɫ��ͨ��

	image_pro(cr, cb, bin, gray);																	//���ø�˹ģ��ת��Ϊ�Ҷ�ͼ����ֵ��

	cvSmooth(bin, median_filter, CV_MEDIAN);											//��ֵ�˲�
	
	border(median_filter, paintx, painty, Htop, Hlow, Vleft, Vright);				//ȷ����ѡ�߽�

	CvSize size;																							//ѡ�����������򣬽��зָ�
	size.height = Hlow - Htop;
	size.width = Vright - Vleft;
	cvSetImageROI(gray, cvRect(Vleft, Htop, size.width, size.height));
	IplImage*gray_cut = cvCreateImage(size, src->depth, 1);
	cvCopy(gray, gray_cut, 0);
	cvResetImageROI(gray);
	
	IplImage*sobel = cvCreateImage(size, IPL_DEPTH_16S, 1);					//����sobel����͹�Գ��۾�����
	cvSobel(gray_cut, sobel, 1, 0, 3);
	IplImage *sobel8u = cvCreateImage(size, IPL_DEPTH_8U, 1);
	cvConvertScaleAbs(sobel, sobel8u, 1, 0);


	cout << "Vleft=" << Vleft << endl;
	cout << "Vright=" << Vright << endl;
	cout << "Htop=" << Htop << endl;
	cout << "Hlow=" << Hlow << endl;

	mark(src, Htop, Hlow, Vleft, Vright);														//��ԭͼ����ӿ�
	
	cvShowImage("sobel", sobel8u);
	cvShowImage("gray_cut", gray_cut);
	cvShowImage("gray", gray);
	cvShowImage("gray1", gray1);
	cvShowImage("bin", bin);
	cvShowImage("median_filter", median_filter);
	cvShowImage("paintx", paintx);
	cvShowImage("painty", painty);
	cvShowImage("���", src);

	end = clock();
	cost = (double)(end - begin) / CLOCKS_PER_SEC;
	cout << "����ʱ��=" << cost << "s" << endl;
	waitKey();
	return 0;
}


//���ڼ���������Ƿ�Ϊ���������ĸ���
float p_cal(uchar a, uchar b)														
{
	float p;
	float c[2][2] = { 231.1231, 9.7823, 9.7823, 115.2362 };
	float c_inv[2][2];
	float tmp = c[0][0] * c[1][1] - c[0][1] * c[1][0];										//��ȡc�������
	c_inv[0][0] = c[1][1] / tmp;																
	c_inv[0][1] = -c[1][0] / tmp;
	c_inv[1][0] = -c[0][1] / tmp;
	c_inv[1][1] = c[0][0] / tmp;
	float m[2][1] = { 148.5632, 116.9231 };																
	p = (a - m[0][0])*(a - m[0][0])*c_inv[0][0] + (b - m[1][0])*(a - m[0][0])*c_inv[1][0] + //��Ҫ����ѧ��ʽ�������
		(a - m[0][0])*(b - m[1][0])*c_inv[0][1] + (b - m[1][0])*(b - m[1][0])*c_inv[1][1];
	p = exp((-0.5)*p) * 255;
	return p;
}

//����ͶӰ����Ѱ�ҿ�ѡ�����ľ����Ķ���
// c++�е�������һ�ֺ���Ч�ĵ�ַ���ݷ�ʽ������Ҫָ��
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
	int* v = new int[width];																		//v[i]���ÿһ���Ϻ�ɫ���ص����
	int* h = new int[height];																	//h[j]���ÿһ���Ϻ�ɫ���ص����
	memset(v, 0, width*sizeof(int));															//��ʼ��Ϊ0
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
			if (s == 255)	v[i]++;																	//����ֱ�����ͶӰ����
		}
		if (v[i]>Vmax)																				//Ѱ�ҵ�����������һ��
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
			if (s == 255)	h[j]++;																	//��ˮƽ�����ͶӰ����
		}																									
		if (h[j]>Hmax)																				//Ѱ�ҵ�����������һ��
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
	for (i = 0; i < tmp_V; i++)															//����ֱ��������һ�У�������Ѱ�ҵ�һ�λ���ֵС��0.3*Vmax
	{																										//��Ϊ���ұ߽�
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

	for (j = 0; j < tmp_H; j++)																	//��ˮƽ��������һ�У������ҵ���һ��С��0.3*HmaxΪ�ϱ߽�				
	{
		if (h[j] > 0.3*Hmax)
		{
			Htop = j;
			break;
		}
	}

	for (j = tmp_H; j < height; j++)															//�����ҵ���һ��С��0.5*HmaxΪ�±߽磨��Ҫȥ�����ӵ�Ӱ�죩
	{
		if (h[j] < 0.491*Hmax)
		{
			Hlow = j;
			break;
		}
	}
	delete [] v;																							//�ͷ�����Ķ�̬�ڴ�
	delete [] h;																						//������������ע��ǰ��Ӧ��[]��������������ǰ�ɼӿɲ���
}

//������ԭͼ�ϻ�����
void mark(IplImage*img, int top, int low, int left, int right)
{
	int i, j;
	uchar* data_img = (uchar*)img->imageData;
	int step_img = img->widthStep / sizeof(uchar);
	int channels = img->nChannels;
	for (i = top; i < low; i++)																					//�����ұ߽续Ϊ��ɫ
	{
		data_img[i*step_img + left*channels + 0] = 0;
		data_img[i*step_img + left*channels + 1] = 0;
		data_img[i*step_img + left*channels + 2] = 255;
		data_img[i*step_img + right*channels + 0] = 0;
		data_img[i*step_img + right*channels + 1] = 0;
		data_img[i*step_img + right*channels + 2] = 255;
	}

	for (j = left; j < right; j++)																					//�����±߽续Ϊ��ɫ
	{
		data_img[top*step_img + j*channels + 0] = 0;
		data_img[top*step_img + j*channels + 1] = 0;
		data_img[top*step_img + j*channels + 2] = 255;
		data_img[low*step_img + j*channels + 0] = 0;
		data_img[low*step_img + j*channels + 1] = 0;
		data_img[low*step_img + j*channels + 2] = 255;
	}
}

//���ø�˹ģ��ת��Ϊ�Ҷ�ͼ����ֵ��
void image_pro(IplImage*Cr, IplImage*Cb, IplImage*Bin, IplImage*Gray)				
{
	int i, j;
	uchar tmp_cb, tmp_cr, probablity;
	uchar* data_gray = (uchar *)Gray->imageData;												//����ָ����ʸ�������
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
			tmp_cb = data_cb[i*step + j];																	//��ȡcb�õ������ֵ
			tmp_cr = data_cr[i*step + j];
			probablity = p_cal(tmp_cb, tmp_cr);
			data_gray[i*step + j] = probablity;																//probablityֵԽС��ʾԽ�Ǻ�ɫ������������
			if (probablity < 0.13 * 255)		data_bin[i*step + j] = 255;						//�������Ĳ��ֵ�Ϊ��ɫ
			else    data_bin[i*step + j] = 0;
		}
	}
}