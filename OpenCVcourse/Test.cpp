#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

/////////////////////// Images ///////////////////////


int main() {
	
	/*Setting variables*/
	string path = "Resources/dog.jpg";
	Mat img = imread(path);
	Mat imgGrey, imgBlur, imgCanny, imgDilate, imgErode, imgResize, imgCrop;
	
	/*Blank image*/
	Mat creation(480, 480, CV_8UC3, Scalar(255, 255, 255));
	circle(creation,Point(240,240),30,Scalar(255,0,0),10/*FILLED*/);
	rectangle(creation, Point(120,120), Point(240,240), Scalar(0, 255, 0), 10/*FILLED*/);
	line(creation,Point(120,120),Point(240,240),Scalar(0,0,255),10);
	putText(creation, "Say Hello", Point(180, 180), FONT_HERSHEY_DUPLEX, 0.5, Scalar(50,100,150),5);
	
	///*Resizing and cropping*/
	cout << img.size() << endl;
	resize(img, imgResize, Size()/*Size(480,480)*/, 0.5, 0.5);
	Rect roi(100, 100, 200, 200);
	imgCrop = img(roi);
	
	/*Filtering*/
	cvtColor(img, imgGrey, COLOR_BGR2GRAY);
	GaussianBlur(img, imgBlur, Size(5, 5), 5, 0);
	Canny(imgBlur, imgCanny, 50, 150);
	
	/*Morphology Transformations*/
	Mat kernel = getStructuringElement(MORPH_RECT, Size(5, 5));
	dilate(imgCanny, imgDilate, kernel);
	erode(imgCanny, imgErode, kernel);

	/*showing images*/
	imshow("dog_gray", imgGrey);
	imshow("dog", img);
	imshow("dog_blur", imgBlur);
	imshow("dog_canny", imgCanny);
	imshow("dog_dilate", imgDilate);
	imshow("dog_erode", imgErode);
	imshow("dog_resize", imgResize);
	imshow("dog_crop", imgCrop);
	imshow("new image", creation);





	waitKey(0);

}