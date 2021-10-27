<<<<<<< HEAD
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

// global variables ///////////////////////////////////////////////////////////////////////////////
const Scalar BLACK = Scalar(0, 0, 0);
const Scalar WHITE = Scalar(255, 255, 255);
const Scalar YELLOW = Scalar(0, 255, 255);
const Scalar GREEN = Scalar(0, 200, 0);
const Scalar RED = Scalar(0, 0, 255);
double dsit2Threshold, varThreshold;
int history;
bool detectShadows;
Mat img, mask, imgCrop, maskThreshold;
Ptr<BackgroundSubtractor> object_detector;

class Blob {
public:
    // member variables ///////////////////////////////////////////////////////////////////////////
    vector<Point> contour;

    Rect boundingRect;

    Point centerPosition;

    double dblDiagonalSize;

    double dblAspectRatio;

    // function prototypes ////////////////////////////////////////////////////////////////////////
    Blob(vector<Point> _contour);

};

Blob::Blob(vector<Point> _contour) {

    contour = _contour;

    boundingRect = cv::boundingRect(contour);

    centerPosition.x = (boundingRect.x + boundingRect.x + boundingRect.width) / 2;
    centerPosition.y = (boundingRect.y + boundingRect.y + boundingRect.height) / 2;

    dblDiagonalSize = sqrt(pow(boundingRect.width, 2) + pow(boundingRect.height, 2));

    dblAspectRatio = (float)boundingRect.width / (float)boundingRect.height;

}


////////////////////////////////// functions ////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob>& existingBlobs, std::vector<Blob>& currentFrameBlobs);
void addBlobToExistingBlobs(Blob& currentFrameBlob, std::vector<Blob>& existingBlobs, int& intIndex);
void addNewBlob(Blob& currentFrameBlob, std::vector<Blob>& existingBlobs);
double distanceBetweenPoints(cv::Point point1, cv::Point point2);
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName);
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName);
void drawBlobInfoOnImage(std::vector<Blob>& blobs, cv::Mat& imgFrame2Copy);



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
        Rect roi(150, 300, 850, 420);
        imgCrop = img(roi);


        /*object detection */
        //object_detector->apply(img, mask);  //no need for this part any more 
        object_detector->apply(imgCrop, mask);// aplying the object detection only in the roi 
        threshold(mask, maskThreshold, 254, 255, THRESH_BINARY);

        vector<vector<Point> > contours;
        vector<Vec4i> hierarchy;
        //findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); //mask with shadows 
        findContours(maskThreshold, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);//mask without the shadows 
        Mat structuringElement3x3 = getStructuringElement(MORPH_RECT, cv::Size(3, 3)); // setting up a 3x3 kernel 
        Mat opening, closing;
        morphologyEx(maskThreshold, closing, 3, structuringElement3x3,Point(-1, -1),5); // preforming a morphological change closing 5 times 
        morphologyEx(closing, opening, 2, structuringElement3x3, Point(-1, -1), 1); // preforming a morphological change openning once
        findContours(opening, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); // finding the contours in the opening matrix 
        vector<vector<Point> >hull(contours.size());
        for (size_t i = 0; i < contours.size(); i++)
        {
            convexHull(contours[i], hull[i]); // setting the boundries of our blobs 
        }
        vector<Blob> blobs;
        for (auto& hull : hull) {
            Blob possibleBlob(hull);

            if (possibleBlob.boundingRect.area() > 1000 &&
                possibleBlob.dblAspectRatio >= 0.5 &&
                possibleBlob.dblAspectRatio <= 2 &&
                possibleBlob.boundingRect.width > 30 &&
                possibleBlob.boundingRect.height > 40 &&
                possibleBlob.dblDiagonalSize > 80.0  ) {
                blobs.push_back(possibleBlob); // selecting the blobs we want and saving them for later 
            }
        }
        Mat imgHulls(opening.size(), CV_8UC3, BLACK);

        hull.clear();

        for (auto& blob : blobs) {
            hull.push_back(blob.contour);
        }
        drawContours(imgHulls, hull, -1, WHITE, -1);

        imshow("imgHulls", imgHulls);


        for (auto& blob : blobs) {                                                  // for each blob
            rectangle(imgCrop, blob.boundingRect, RED, 2);             // draw a red box around the blob
            int tlx = blob.boundingRect.tl().x;
            int tly = blob.boundingRect.tl().y;
            int brx = blob.boundingRect.br().x;
            int bry = blob.boundingRect.br().y;
            cout << "Top left point : " << blob.boundingRect.tl() << " Buttom right point : " << blob.boundingRect.br() << endl;
            cout << "Top left point X : " << tlx << endl;
            cout << "Top left point Y : " << tly << endl;
            cout << "Buttom right point X : " << brx << endl;
            cout << "Buttom right point Y : " << bry << endl;
            circle(imgCrop, blob.centerPosition, 3, GREEN, -1);  // draw a filled-in green circle at the center
            int cenX = blob.centerPosition.x;
            int cenY = blob.centerPosition.y;
            cout << "Center point X: " << blob.centerPosition.x << " Center point Y: " << blob.centerPosition.y << endl;
        }   

        //show the current frame and the mask
        imshow("video",img);
        imshow("mask", maskThreshold);
        imshow("crop", imgCrop);
        //imshow("Hull demo", drawing);

        //get the input from the keyboard
        int keyboard = waitKey(24);
        if (keyboard == 'q' || keyboard == 27)
            break;
    }
    return 0;
=======
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
>>>>>>> f016217e0421be841d7d0fbb8835bab833c453f5
}