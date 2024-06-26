#include <opencv2/opencv.hpp>  // Include OpenCV library
#include <sys/time.h>          // Include sys/time.h for time-related functions
#include "D8MCapture.h"        // Include D8MCapture header
#include "hps_0.h"             // Include hps_0 header
#include "string"              // Include string header
#include "cmath"               // Include cmath header
#include "fstream"             // Include fstream header for file operations

#include <chrono>              // For time related functions
#include <iostream>            // used to read parameter values from a file

using namespace cv;            // Using OpenCV namespace
using namespace std;           // Using standard namespace

#ifndef CAPTURE_RAM_DEVICE
#define CAPTURE_RAM_DEVICE "/dev/f2h-dma-memory"    // Default RAM device if not defined
#endif /* ifndef CAPTURE_RAM_DEVICE */

int main()
{
    Mat src; // Unmodified Camera Feed
    D8MCapture *cap = new D8MCapture(TV_DECODER_TERASIC_STREAM_CAPTURE_BASE, CAPTURE_RAM_DEVICE);
    // Check if the capture object is successfully opened
    if (!cap->isOpened()) {
        return -1;              // Return -1 if capture object failed to open
    }

    // Declare variables for FPS calculation using built in OpenCV funtion
    double fps = 0.0;
    int64 prevTick = 0;

    while (1) {
        // Read frame from the camera feed
        if (!cap->read(src))
            return -1;

        // Read overlay image from the saved file "assignment5_image.png"
        // that the python server obtained from the QT client
        Mat overlayedImage = imread("assignment5_image.png");

        // Declare fstream variable to parse parameter data
        fstream parametersFromFile;

        // Declare variables to store brightness and contrast parameters
        int brightness;
        int contrast;

        // Parameter brightness and contrast string data is
        // parsed from the saved parameter file that the python
        // server obtained from the QT client and is converted
        // back to integers

         // Open parameter file to read brightness and contrast values
        parametersFromFile.open("assignment5_parameters.txt");
        string line;

        // Read brightness value from the file and convert to integer
        getline(parametersFromFile, line);
        brightness = stoi(line);
        // Read contrast value from the file and convert to integer
        getline(parametersFromFile, line);
        contrast = stoi(line);
        // Close the parameter file
        parametersFromFile.close();

        // Create necessary Mats for overlay and brightness/contrast adjustment
        Mat resize1;
        Mat resize2;
        Mat outputImage = src.clone();

        // Resize and convert overlay image to match camera feed format
        resize(overlayedImage, resize1, Size(800, 480), INTER_LINEAR);
        cvtColor(resize1, resize2, COLOR_BGR2BGRA);
   
        // Apply brightness and contrast adjustments to the live camera feed
        double brightnessAdjust = brightness - 50;
        double contrastAdjust = contrast / 255.0;
        outputImage.convertTo(outputImage, -1, contrastAdjust, brightnessAdjust);

        // Calculate region of interest (ROI) and overlay the resized image onto it
        // Overlayed image to cover 25% of output (ROI) in the upper left of output
        int qtrWidth = outputImage.cols/2;
        int qtrHeight = outputImage.rows/2;
        Rect roi(0, 0, qtrWidth, qtrHeight);
        resize(resize2, resize2, Size(qtrWidth, qtrHeight), INTER_LINEAR);
        Mat overlayedRoi = outputImage(roi);
        addWeighted(overlayedRoi, 1.0, resize2, 0.5, 0.0, overlayedRoi);

        // Calculate FPS
        double tickFrequency = getTickFrequency();
        int64 currentTick = getTickCount();
        double timeDiff = (currentTick - prevTick) / tickFrequency;
        fps = 1.0 / timeDiff;
        prevTick = currentTick;

        // Display FPS values on the output image (top right corner)
        std::string fpsString = "FPS: " + std::to_string(fps);
        fpsString = fpsString.substr(0, fpsString.find(".") + 3);
        putText(outputImage, fpsString, Point(outputImage.cols - 150, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);

        // Display the modified camera feed and overlay image
        imshow("Live Output Image", outputImage);

        // Bail out if escape was pressed
        int c = waitKey(10);
        if ((char)c == 27) {
            break;
        }
    }

    // Release capture object and close windows
    delete cap;
    destroyAllWindows();

    return 0;  // Return 0 to indicate successful execution
}