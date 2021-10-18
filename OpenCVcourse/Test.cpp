#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/video.hpp>
#include <iostream>
#include <sstream>


using namespace cv;
using namespace std;

double dsit2Threshold, varThreshold;
int history;
bool detectShadows;
Mat img, mask, imgCrop, maskThreshold;
Ptr<BackgroundSubtractor> object_detector;
/////////////////////// Images ///////////////////////

int main(){

    string path = "Resources/highway.mp4";
    VideoCapture cap(path);


    /*object detection from stable camera */
    //object_detector = createBackgroundSubtractorKNN(history = (500) , dsit2Threshold  =(400.0), detectShadows = true);
    object_detector = createBackgroundSubtractorMOG2(history = (500) , varThreshold=(30.0), detectShadows = true);

    while (true) {
        cap.read(img);

        /*extract region of interest*/
        int width = img.cols;
        int height = img.rows;
        cout << "Width : " << width << "Height: " << height << endl;
        Rect roi(150, 400, 850, 220);
        imgCrop = img(roi);


        /*object detection */
        //object_detector->apply(img, mask);  //no need for this part any more 
        object_detector->apply(imgCrop, mask);// aplying the object detection only in the roi 
        threshold(mask, maskThreshold, 254, 255, THRESH_BINARY);

        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        //findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); //mask with shadows 
        findContours(maskThreshold, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);//mask without the shadows 
        //int detection[];


        /*calculate area and remove small objects */
        for (int cnt = 0; cnt < contours.size(); cnt++) {
            int area = contourArea(contours[cnt]);
            if (area > 2000) {
                cout << area << endl;

                /*draw the contours of the objects*/
                //drawContours(img, contours, (int)cnt, Scalar(255, 0, 0), 2, LINE_8, hierarchy, 0);// draw the contour in all the image 
                //drawContours(imgCrop, contours, (int)cnt, Scalar(255, 0, 0), 2, LINE_8, hierarchy, 0);// draw the contour only in the roi 
                
                /*draw the bounding box of the objects*/
                vector<Rect> boundRect(contours.size());
                boundRect[cnt] = boundingRect(contours[cnt]);
                rectangle(imgCrop, boundRect[cnt].tl(), boundRect[cnt].br(), Scalar(0,255,0), 2);//bounding box in the roi
                //rectangle(img, boundRect[cnt].tl(), boundRect[cnt].br(), Scalar(0, 255, 0), 2);// bouding box on the roi but in the whole image 
                int tlx = boundRect[cnt].tl().x;
                int tly = boundRect[cnt].tl().y;
                int brx = boundRect[cnt].br().x;
                int bry = boundRect[cnt].br().y;
                cout << "Top left point : " << boundRect[cnt].tl() << " Buttom right point : " << boundRect[cnt].br() << endl;
                cout << "Top left point X : " << tlx << endl;
                cout << "Top left point Y : " << tly << endl;
                cout << "Buttom right point X : " << brx << endl;
                cout << "Buttom right point Y : " << bry << endl;
                //detection.
            }
        }
        //show the current frame and the mask
        imshow("video",img);
        imshow("mask", maskThreshold);
        imshow("crop", imgCrop);


        //get the input from the keyboard
        int keyboard = waitKey(24);
        if (keyboard == 'q' || keyboard == 27)
            break;
    }
    return 0;
}