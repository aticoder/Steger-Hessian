#include<iostream>
#include<opencv2\opencv.hpp>
#include<unordered_map>
#include<math.h>
#include<halconcpp\HalconCpp.h>

using namespace std;
using namespace cv;
using namespace HalconCpp;

struct EV_VAL_VEC {
	double nx, ny;//nx��nyΪ�������ֵ��Ӧ����������
};

HObject Mat2Hobject(Mat &image)
{
	HObject Hobj = HObject();
	int hgt = image.rows;
	int wid = image.cols;
	int i;
	//  CV_8UC3  
	if (image.type() == CV_8UC3)
	{
		vector<Mat> imgchannel;
		split(image, imgchannel);
		Mat imgB = imgchannel[0];
		Mat imgG = imgchannel[1];
		Mat imgR = imgchannel[2];
		uchar* dataR = new uchar[hgt*wid];
		uchar* dataG = new uchar[hgt*wid];
		uchar* dataB = new uchar[hgt*wid];
		for (i = 0; i < hgt; i++)
		{
			memcpy(dataR + wid*i, imgR.data + imgR.step*i, wid);
			memcpy(dataG + wid*i, imgG.data + imgG.step*i, wid);
			memcpy(dataB + wid*i, imgB.data + imgB.step*i, wid);
		}
		GenImage3(&Hobj, "byte", wid, hgt, (Hlong)dataR, (Hlong)dataG, (Hlong)dataB);
		delete[]dataR;
		delete[]dataG;
		delete[]dataB;
		dataR = NULL;
		dataG = NULL;
		dataB = NULL;
	}
	//  CV_8UCU1  
	else if (image.type() == CV_8UC1)
	{
		uchar* data = new uchar[hgt*wid];
		for (i = 0; i < hgt; i++)
			memcpy(data + wid*i, image.data + image.step*i, wid);
		GenImage1(&Hobj, "byte", wid, hgt, (Hlong)data);
		delete[] data;
		data = NULL;
	}
	return Hobj;
}

void SetFilterMat(Mat& Dx, Mat& Dy, Mat&Dxx, Mat&Dyy, Mat&Dxy, int size_m)//radius * 2= size 
{
	if (size_m % 2 == 0 || size_m == 1) cout << "size is not illegal !!" << endl;
	Mat m1 = Mat::zeros(Size(size_m, size_m), CV_64F);//Dx
	Mat m2 = Mat::zeros(Size(size_m, size_m), CV_64F);//Dy
	Mat m3 = Mat::zeros(Size(size_m, size_m), CV_64F);//Dxx
	Mat m4 = Mat::zeros(Size(size_m, size_m), CV_64F);//Dyy
	Mat m5 = Mat::zeros(Size(size_m, size_m), CV_64F);//Dxy

	for (int i = 0; i < size_m / 2; i++)
	{
		m1.row(i) = 1;
		m2.col(i) = 1;
		m3.row(i) = -1;
		m4.col(i) = -1;
	}
	for (int i = size_m / 2 + 1; i < size_m; i++)
	{
		m1.row(i) = -1;
		m2.col(i) = -1;
		m3.row(i) = -1;
		m4.col(i) = -1;
	}
	m3.row(size_m / 2) = (size_m / 2) * 2;
	m4.col(size_m / 2) = (size_m / 2) * 2;
	m5.row(size_m / 2) = 1;
	m5.col(size_m / 2) = 1;
	m5.at<double>(size_m / 2, size_m / 2) = -(size_m / 2) * 4;
	if (size_m == 5) m5 = (Mat_<double>(5,5)<< 0,0,1,0,0,0,1,2,1,0,1,2,-16,2,1,0,1,2,1,0,0,0,1,0,0 );
	Dx = m2;
	Dy = m1;
	Dxx = m4;
	Dyy = m3;
	Dxy = m5;
	cout << Dx << endl;
	cout << Dy << endl;
	cout << Dxx << endl;
	cout << Dyy << endl;
	cout << Dxy << endl;
	return;
}

bool isMax(int i,int j,Mat& img,int dx,int dy)//�ڷ��߷������Ƿ�Ϊ��ֵ
{
	double val = img.at<double>(j, i);
	double max_v = max(img.at<double>(j + dy, i + dx), img.at<double>(j - dy, i - dx));
	if (val>=max_v) return true;
	else return false;
}

void StegerLine(Mat&img0, vector<Point2d>&sub_pixel, int size_m)
{
	// Local iconic variables
	HObject  ho_Image, ho_Region;

	// Local control variables
	HTuple  hv_WindowHandle, hv_Rows, hv_Columns;

	Mat img;
	cvtColor(img0, img0, CV_BGR2GRAY);
	img = img0.clone();

	//��˹�˲�
	img.convertTo(img, CV_64FC1);
	GaussianBlur(img, img, Size(3, 3), 0.9, 0.9);
	imwrite("gauss_blur_src.bmp", img);

	//ho_Image = Mat2Hobject(img);
	//Threshold(ho_Image, &ho_Region, 80, 255);
	//GetRegionPoints(ho_Region, &hv_Rows, &hv_Columns);

	//һ��ƫ����
	Mat m1, m2;
	//����ƫ����
	Mat m3, m4, m5;
	SetFilterMat(m1, m2, m3, m4, m5, size_m);

	cout << m5 << endl;

	Mat dx, dy;
	filter2D(img, dx, CV_64FC1, m1);
	filter2D(img, dy, CV_64FC1, m2);

	Mat dxx, dyy, dxy;
	filter2D(img, dxx, CV_64FC1, m3);
	filter2D(img, dyy, CV_64FC1, m4);
	filter2D(img, dxy, CV_64FC1, m5);

	//hessian����
	int imgcol = img.cols;
	int imgrow = img.rows;
	vector<double> Pt;

	for (int i = 0; i < imgcol; i++)
	{
		for (int j = 0; j < imgrow; j++)
		{
			double pixel_val = img.at<double>(j, i);
			if (img.at<double>(j, i) >50)            //��Ҫȷ��ROI������
			{
				Mat hessian(2, 2, CV_64FC1);
				hessian.at<double>(0, 0) = dxx.at<double>(j, i);
				hessian.at<double>(0, 1) = dxy.at<double>(j, i);
				hessian.at<double>(1, 0) = dxy.at<double>(j, i);
				hessian.at<double>(1, 1) = dyy.at<double>(j, i);

				Mat eValue;
				Mat eVectors;
				eigen(hessian, eValue, eVectors);

				double nx, ny;
				double EV_max = 0;//����ֵ
				double EV_min = 0;
				if (fabs(eValue.at<double>(0, 0)) >= fabs(eValue.at<double>(1, 0)))  //������ֵ���ʱ��Ӧ����������
				{
					nx = eVectors.at<double>(0, 0);//�������ֵ��Ӧ����������
					ny = eVectors.at<double>(0, 1);
					EV_max = max(eValue.at<double>(0, 0), eValue.at<double>(1, 0));
					EV_min = min(eValue.at<double>(0, 0), eValue.at<double>(1, 0));
				}
				else
				{
					nx = eVectors.at<double>(1, 0);//�������ֵ��Ӧ����������
					ny = eVectors.at<double>(1, 1);
					EV_max = max(eValue.at<double>(0, 0), eValue.at<double>(1, 0));
					EV_min = min(eValue.at<double>(0, 0), eValue.at<double>(1, 0));
				}

				//t ��ʽ ̩��չ�������׵���
				double a = nx*nx*dxx.at<double>(j, i) + 2 * nx*ny*dxy.at<double>(j, i) + ny*ny*dyy.at<double>(j, i);
				double b = nx*dx.at<double>(j, i) + ny*dy.at<double>(j, i);
				double t = -b / a;

				int dx ;
				int dy ;

				if (abs(nx) >= 2*abs(ny)) dx = 1, dy = 0;//��ֱ
				else if (abs(ny) >= 2*abs(nx)) dx = 0, dy = 1;//ˮƽ
				else if (nx>0) dx = 1,dy = 1;
				else if (ny>0) dx = -1, dy = -1;

				if (isMax(i,j,img,dx,dy))
				//if(EV_min<-120)
				{
					double temp1 = t*nx;
					double temp2 = t*ny;
					if (fabs(t*nx) <= 0.5 && fabs(t*ny) <= 0.5)//(x + t * Nx, y + t * Ny)Ϊ����������
					{
						Pt.push_back(i + t*nx);
						Pt.push_back(j + t*ny);
					}
				}
			}
		}
	}

	for (int k = 0; k < Pt.size() / 2; k++)
	{
		Point2d rpt;
		rpt.x = Pt[2 * k + 0];
		rpt.y = Pt[2 * k + 1];
		sub_pixel.push_back(rpt);
	}

}

//E:/workplace/Line_Dtection/Project1/1/1-0.bmp

int main()//E:/workplace/get_line_width/��ͷ����ԭͼ/��ͷ����/1300us.bmp
{
	Mat src = imread("E:/workplace/Line_Dtection/Project1/1/1-9.bmp");
	vector<Point2d>sub_pixel;
	StegerLine(src, sub_pixel, 5);
	ofstream outfile_r, outfile_c;
	outfile_r.open("subpixel_row.txt");
	outfile_c.open("subpixel_col.txt");
	outfile_r << sub_pixel.size() << endl;
	outfile_c << sub_pixel.size() << endl;
	for (int i = 0; i < sub_pixel.size(); i++)
	{
		outfile_r << setprecision(15) << "2 " << sub_pixel[i].y << endl;
		outfile_c << setprecision(15) << "2 " << sub_pixel[i].x << endl;
	}
	outfile_c.close();
	outfile_r.close();
	return 0;
}