#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

/////////////////////// Images ///////////////////////
float width=210, hight=297;
Mat matrix, warp, rotated;

void main() {
	
	string path = "\Resources/paper.jpg";
	Mat img = imread(path);

	Point2f input[4] = { {492,303},{791,351},{282,490},{610,566} };
	Point2f output[4] = { {0.0f,0.0f},{width,0.0f},{0.0f,hight},{width,hight} };

	matrix = getPerspectiveTransform(input, output);
	warpPerspective(img, warp, matrix, Point(width, hight));
	rotate(warp,rotated,ROTATE_180);

	imshow("original",img);
	imshow("after", rotated);

	waitKey(0);

}
