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

// global variables //////////////////////////////////////////////////////////////////////////////
string path = "Resources/highway.mp4"; // selecting a video
int frameCounter = 0;
const Scalar BLACK = Scalar(0, 0, 0);
const Scalar WHITE = Scalar(255, 255, 255);
const Scalar YELLOW = Scalar(0, 255, 255);
const Scalar GREEN = Scalar(0, 200, 0);
const Scalar RED = Scalar(0, 0, 255); // color variables to be used
// double dsit2Threshold; //   it is a variable i would use in case we use createBackgroundSubtractorKNN
double varThreshold; // it is a variable i would use in case we use createBackgroundSubtractorMOG2
int history;         // history lengh of the history when subtracting the background
bool detectShadows;  // detecting shadows during subtracting the background
Mat currentFrame, currentFrameGray, oldFrame, oldFrameGray, mask, imgCurrentCrop, maskThreshold;
Ptr<BackgroundSubtractor> object_detector;
vector<vector<Point>> contours;
vector<Vec4i> hierarchy;
vector<Point2f> p0, p1;
int carCount = 0;

class Blob
{
public:
    // member variables ///////////////////////////////////////////////////////////////////////////
    vector<Point> currentContour;

    Rect currentBoundingRect;

    vector<Point> centerPositions;

    double dblCurrentDiagonalSize;

    double dblCurrentAspectRatio;

    bool blnCurrentMatchFoundOrNewBlob;

    bool blnStillBeingTracked;

    int intNumOfConsecutiveFramesWithoutAMatch;

    cv::Point predictedNextPosition;

    // function prototypes ////////////////////////////////////////////////////////////////////////
    Blob(vector<Point> _contour);
    void predictNextPosition(void);
};

Blob::Blob(vector<Point> _contour)
{

    currentContour = _contour;

    currentBoundingRect = cv::boundingRect(currentContour);

    cv::Point currentCenter;

    currentCenter.x = (currentBoundingRect.x + currentBoundingRect.x + currentBoundingRect.width) / 2;
    currentCenter.y = (currentBoundingRect.y + currentBoundingRect.y + currentBoundingRect.height) / 2;

    centerPositions.push_back(currentCenter);

    dblCurrentDiagonalSize = sqrt(pow(currentBoundingRect.width, 2) + pow(currentBoundingRect.height, 2));

    dblCurrentAspectRatio = (float)currentBoundingRect.width / (float)currentBoundingRect.height;

    blnStillBeingTracked = true;
    blnCurrentMatchFoundOrNewBlob = true;

    intNumOfConsecutiveFramesWithoutAMatch = 0;
}
void Blob::predictNextPosition(void) {

    int numPositions = (int)centerPositions.size();

    if (numPositions == 1) {

        predictedNextPosition.x = centerPositions.back().x;
        predictedNextPosition.y = centerPositions.back().y;

    }
    else if (numPositions == 2) {

        int deltaX = centerPositions[1].x - centerPositions[0].x;
        int deltaY = centerPositions[1].y - centerPositions[0].y;

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else if (numPositions == 3) {

        int sumOfXChanges = ((centerPositions[2].x - centerPositions[1].x) * 2) +
            ((centerPositions[1].x - centerPositions[0].x) * 1);

        int deltaX = (int)std::round((float)sumOfXChanges / 3.0);

        int sumOfYChanges = ((centerPositions[2].y - centerPositions[1].y) * 2) +
            ((centerPositions[1].y - centerPositions[0].y) * 1);

        int deltaY = (int)std::round((float)sumOfYChanges / 3.0);

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else if (numPositions == 4) {

        int sumOfXChanges = ((centerPositions[3].x - centerPositions[2].x) * 3) +
            ((centerPositions[2].x - centerPositions[1].x) * 2) +
            ((centerPositions[1].x - centerPositions[0].x) * 1);

        int deltaX = (int)std::round((float)sumOfXChanges / 6.0);

        int sumOfYChanges = ((centerPositions[3].y - centerPositions[2].y) * 3) +
            ((centerPositions[2].y - centerPositions[1].y) * 2) +
            ((centerPositions[1].y - centerPositions[0].y) * 1);

        int deltaY = (int)std::round((float)sumOfYChanges / 6.0);

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else if (numPositions >= 5) {

        int sumOfXChanges = ((centerPositions[numPositions - 1].x - centerPositions[numPositions - 2].x) * 4) +
            ((centerPositions[numPositions - 2].x - centerPositions[numPositions - 3].x) * 3) +
            ((centerPositions[numPositions - 3].x - centerPositions[numPositions - 4].x) * 2) +
            ((centerPositions[numPositions - 4].x - centerPositions[numPositions - 5].x) * 1);

        int deltaX = (int)std::round((float)sumOfXChanges / 10.0);

        int sumOfYChanges = ((centerPositions[numPositions - 1].y - centerPositions[numPositions - 2].y) * 4) +
            ((centerPositions[numPositions - 2].y - centerPositions[numPositions - 3].y) * 3) +
            ((centerPositions[numPositions - 3].y - centerPositions[numPositions - 4].y) * 2) +
            ((centerPositions[numPositions - 4].y - centerPositions[numPositions - 5].y) * 1);

        int deltaY = (int)std::round((float)sumOfYChanges / 10.0);

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else {
        // should never get here
    }

}
// function prototypes ////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob>& existingBlobs, std::vector<Blob>& currentFrameBlobs);
void addBlobToExistingBlobs(Blob& currentFrameBlob, std::vector<Blob>& existingBlobs, int& intIndex);
void addNewBlob(Blob& currentFrameBlob, std::vector<Blob>& existingBlobs);
double distanceBetweenPoints(cv::Point point1, cv::Point point2);
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName);
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName);
bool checkIfBlobsCrossedTheLine(std::vector<Blob>& blobs, int& intHorizontalLinePosition, int& carCount);
void drawBlobInfoOnImage(std::vector<Blob>& blobs, cv::Mat& imgFrame2Copy);
void drawCarCountOnImage(int& carCount, cv::Mat& imgFrame2Copy);
/////////////////////// Images ///////////////////////

int main()
{
    std::vector<Blob> currentFrameBlobs;
    VideoCapture capture(path); // reading the video
    if (!capture.isOpened()) {
        //error in opening the video input
        cerr << "Unable to open file!" << endl;
        return 0;
    }
    // Create some random colors
    vector<Scalar> colors;
    RNG rng;
    for (int i = 0; i < 100; i++)
    {
        int r = rng.uniform(0, 256);
        int g = rng.uniform(0, 256);
        int b = rng.uniform(0, 256);
        colors.push_back(Scalar(r, g, b));
    }

    capture >> oldFrame;
    //cvtColor(oldFrame, oldFrameGray, COLOR_BGR2GRAY);
    //goodFeaturesToTrack(oldFrameGray, p0, 100, 0.3, 7, Mat(), 7, false, 0.04);
    //cout << "the good features are :" << endl;
    //for (auto& f : p0) {
    //    cout << f;
    //}
    int intHorizontalLinePosition = (int)std::round((double)oldFrame.rows * 0.65);
    cv::Point crossingLine[2];
    crossingLine[0].x = 1;
    crossingLine[0].y = intHorizontalLinePosition;

    crossingLine[1].x = oldFrame.cols - 1;
    crossingLine[1].y = intHorizontalLinePosition;

    object_detector = createBackgroundSubtractorMOG2(history = (500), varThreshold = (30.0), detectShadows = true);
    

    bool blnFirstFrame = true;

    int frameCount = 2;
    while (true)
    {
        cvtColor(oldFrame, oldFrameGray, COLOR_BGR2GRAY);
        capture >> currentFrame;
        /*frameCounter +=5;
        capture.set(1, frameCounter);
        capture.read(currentFrame);*/
        if (currentFrame.empty())
            break;

        /*extract region of interest*/
        int width = currentFrame.cols;
        int height = currentFrame.rows;
        cout << "Width : " << width << "Height: " << height << endl;
        Rect roi(150, 300, 850, 420); // in this specific video we needed these specific ROI (X,Y,W,H)
        imgCurrentCrop = currentFrame(roi);
        cvtColor(currentFrame, currentFrameGray, COLOR_BGR2GRAY);
        /*object detection */
        // object_detector->apply(img, mask);  //no need for this part any more
        object_detector->apply(imgCurrentCrop, mask);                   // aplying the object detection only in the roi
        threshold(mask, maskThreshold, 254, 255, THRESH_BINARY); // thresholding to get rid of the shadows
        // findContours(mask, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); //mask with shadows
        findContours(maskThreshold, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); // mask without the shadows
        Mat structuringElement3x3 = getStructuringElement(MORPH_RECT, cv::Size(3, 3));    // setting up a 3x3 kernel
        Mat opening, closing;
        morphologyEx(maskThreshold, closing, 3, structuringElement3x3, Point(-1, -1), 5); // preforming a morphological change closing 5 times
        morphologyEx(closing, opening, 2, structuringElement3x3, Point(-1, -1), 1);       // preforming a morphological change openning once
        findContours(opening, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);       // finding the contours of the cars in the opening matrix
        // every cars is a bit far from the other and the background is NULL so we only have the pixels of the countours
        vector<vector<Point>> hull(contours.size()); // hull takes the number of points assined to each countour point
        for (size_t i = 0; i < contours.size(); i++) // find the convex hull of each given set of points that define a shape
        {
            convexHull(contours[i], hull[i]); // we are creating a Wrapping  a hull to each set of points defining a shape
        }
        // we iterate one point at the time in contours,and hull only takes the points that englobe the shape the furthest ones
        // we print some points to the screen to see it
        // cout << "HULL" << endl;
        // for (auto vec : hull)
        //     for (auto v : vec)
        //         std::cout << v << std::endl;
        // cout << "COUNTOUR" << endl;
        // for (auto vec : contours)
        //     for (auto v : vec)
        //         std::cout << v << std::endl;
        vector<Blob> blobs;
        for (auto& hull : hull) // in hull now we have a vector of diffrent convexs representing lot of shapes 
        {
            Blob possibleBlob(hull); // we put some crateria for the convexs we think are cars 

            if (possibleBlob.currentBoundingRect.area() > 1000 &&
                possibleBlob.dblCurrentDiagonalSize >= 0.5 &&
                possibleBlob.dblCurrentAspectRatio <= 2 &&
                possibleBlob.currentBoundingRect.width > 30 &&
                possibleBlob.currentBoundingRect.height > 40 &&
                possibleBlob.dblCurrentDiagonalSize > 80.0)
            {
                blobs.push_back(possibleBlob); // blobs is now a vector of possible convexs 
            }
        }
        Mat imgHulls(opening.size(), CV_8UC3, BLACK);  // create black image matrix similar to our main image 

        hull.clear();  // we save memorie

        for (auto& blob : blobs)
        {
            hull.push_back(blob.currentContour);  // fill the hull vector one last time with the right shapes countours
        }
        drawContours(imgHulls, hull, -1, WHITE, -1);  // we fill in the shapes with white 

        imshow("imgHulls", imgHulls);

        for (auto& blob : blobs)
        {                                                  // for each blob
            rectangle(imgCurrentCrop, blob.currentBoundingRect, RED, 2); // draw a red box around the blob
            int tlx = blob.currentBoundingRect.tl().x;
            int tly = blob.currentBoundingRect.tl().y;
            int brx = blob.currentBoundingRect.br().x;
            int bry = blob.currentBoundingRect.br().y;
            ///*cout << "Top left point : " << blob.currentBoundingRect.tl() << " Buttom right point : " << blob.currentBoundingRect.br() << endl;
            //cout << "Top left point X : " << tlx << endl;
            //cout << "Top left point Y : " << tly << endl;
            //cout << "Buttom right point X : " << brx << endl;
            //cout << "Buttom right point Y : " << bry << endl;*/
            circle(imgCurrentCrop, blob.centerPositions.back(), 3, GREEN, -1); // draw a filled-in green circle at the center
            int cenX = blob.centerPositions.back().x;
            int cenY = blob.centerPositions.back().y;
            cout << "Center points : " << blob.centerPositions << endl;
            //cout << "Center point X: " << blob.centerPositions.back().x << " Center point Y: " << blob.centerPositions.back().y << endl;
        }
       
        if (blnFirstFrame == true) {
            for (auto& currentFrameBlob : currentFrameBlobs) {
                blobs.push_back(currentFrameBlob);
            }
        }
        else {
            matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs);
        }

        drawAndShowContours(maskThreshold.size(), blobs, "imgBlobs");

        Mat currentFrameCopy = currentFrame.clone();          // get another copy of frame 2 since we changed the previous frame 2 copy in the processing above

        drawBlobInfoOnImage(blobs, currentFrameCopy);

        bool blnAtLeastOneBlobCrossedTheLine = checkIfBlobsCrossedTheLine(blobs, intHorizontalLinePosition, carCount);

        if (blnAtLeastOneBlobCrossedTheLine == true) {
            cv::line(currentFrameCopy, crossingLine[0], crossingLine[1], GREEN, 2);
        }
        else {
            cv::line(currentFrameCopy, crossingLine[0], crossingLine[1], RED, 2);
        }

        drawCarCountOnImage(carCount, currentFrameCopy);

        cv::imshow("imgFrame2Copy", currentFrameCopy);

        //cv::waitKey(0);                 // uncomment this line to go frame by frame for debugging

        // now we prepare for the next iteration

        currentFrameBlobs.clear();

        oldFrame = currentFrame.clone();           // move frame 1 up to where frame 2 is

        blnFirstFrame = false;
        frameCount++;


        // show the current frame and the mask
        imshow("video", currentFrame);
        imshow("mask", maskThreshold);
        imshow("crop", imgCurrentCrop);
        //imshow("Optical flow", img);

    
        // get the input from the keyboard
        int keyboard = waitKey(1);
        if (keyboard == 'q' || keyboard == 27)
            break;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob>& existingBlobs, std::vector<Blob>& currentFrameBlobs) {

    for (auto& existingBlob : existingBlobs) {

        existingBlob.blnCurrentMatchFoundOrNewBlob = false;

        existingBlob.predictNextPosition();
    }

    for (auto& currentFrameBlob : currentFrameBlobs) {

        int intIndexOfLeastDistance = 0;
        double dblLeastDistance = 100000.0;

        for (unsigned int i = 0; i < existingBlobs.size(); i++) {

            if (existingBlobs[i].blnStillBeingTracked == true) {

                double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);

                if (dblDistance < dblLeastDistance) {
                    dblLeastDistance = dblDistance;
                    intIndexOfLeastDistance = i;
                }
            }
        }

        if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize * 0.5) {
            addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance);
        }
        else {
            addNewBlob(currentFrameBlob, existingBlobs);
        }

    }

    for (auto& existingBlob : existingBlobs) {

        if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {
            existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
        }

        if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 3) {
            existingBlob.blnStillBeingTracked = false;
        }

    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addBlobToExistingBlobs(Blob& currentFrameBlob, std::vector<Blob>& existingBlobs, int& intIndex) {

    existingBlobs[intIndex].currentContour = currentFrameBlob.currentContour;
    existingBlobs[intIndex].currentBoundingRect = currentFrameBlob.currentBoundingRect;

    existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());

    existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
    existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;

    existingBlobs[intIndex].blnStillBeingTracked = true;
    existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob& currentFrameBlob, std::vector<Blob>& existingBlobs) {

    currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;

    existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(cv::Point point1, cv::Point point2) {

    int intX = abs(point1.x - point2.x);
    int intY = abs(point1.y - point2.y);

    return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName) {
    cv::Mat image(imageSize, CV_8UC3, BLACK);

    cv::drawContours(image, contours, -1, WHITE, -1);

    cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName) {

    cv::Mat image(imageSize, CV_8UC3, BLACK);

    std::vector<std::vector<cv::Point> > contours;

    for (auto& blob : blobs) {
        if (blob.blnStillBeingTracked == true) {
            contours.push_back(blob.currentContour);
        }
    }

    cv::drawContours(image, contours, -1, WHITE, -1);

    cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLine(std::vector<Blob>& blobs, int& intHorizontalLinePosition, int& carCount) {
    bool blnAtLeastOneBlobCrossedTheLine = false;

    for (auto blob : blobs) {

        if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
            int prevFrameIndex = (int)blob.centerPositions.size() - 2;
            int currFrameIndex = (int)blob.centerPositions.size() - 1;

            if (blob.centerPositions[prevFrameIndex].y > intHorizontalLinePosition && blob.centerPositions[currFrameIndex].y <= intHorizontalLinePosition) {
                carCount++;
                blnAtLeastOneBlobCrossedTheLine = true;
            }
        }

    }

    return blnAtLeastOneBlobCrossedTheLine;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawBlobInfoOnImage(std::vector<Blob>& blobs, cv::Mat& imgFrame2Copy) {

    for (unsigned int i = 0; i < blobs.size(); i++) {

        if (blobs[i].blnStillBeingTracked == true) {
            cv::rectangle(imgFrame2Copy, blobs[i].currentBoundingRect, RED, 2);

            int intFontFace = blobs[i].dblCurrentDiagonalSize / 60.0;
            double dblFontScale = blobs[i].dblCurrentDiagonalSize / 60.0;
            int intFontThickness = (int)std::round(dblFontScale * 1.0);

            cv::putText(imgFrame2Copy, std::to_string(i), blobs[i].centerPositions.back(), intFontFace, dblFontScale, GREEN, intFontThickness);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawCarCountOnImage(int& carCount, cv::Mat& imgFrame2Copy) {

    int intFontFace = (imgFrame2Copy.rows * imgFrame2Copy.cols) / 300000.0;
    double dblFontScale = (imgFrame2Copy.rows * imgFrame2Copy.cols) / 300000.0;
    int intFontThickness = (int)std::round(dblFontScale * 1.5);

    cv::Size textSize = cv::getTextSize(std::to_string(carCount), intFontFace, dblFontScale, intFontThickness, 0);

    cv::Point ptTextBottomLeftPosition;

    ptTextBottomLeftPosition.x = imgFrame2Copy.cols - 1 - (int)((double)textSize.width * 1.25);
    ptTextBottomLeftPosition.y = (int)((double)textSize.height * 1.25);

    cv::putText(imgFrame2Copy, std::to_string(carCount), ptTextBottomLeftPosition, intFontFace, dblFontScale, GREEN, intFontThickness);

}