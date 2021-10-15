#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

///////////////////// Images ///////////////////////
int main() {

	//string path = "Resources/cup.mp4";
	VideoCapture cap(0);
	Mat img;
	while (true) {
		cap.read(img);
		imshow("Video",img);
		waitKey(1);

	}

}
