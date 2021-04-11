#include <dirent.h> // directory header
#include <stdio.h> // printf()
#include <stdlib.h> // exit()

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <cmath>
#include <iostream>
#include <filesystem>
using namespace cv;
using namespace std;
using namespace std::filesystem; 

bool collecting;

void get_background(Mat &avg_map, Mat &var_map, vector<Mat> video);
void get_mask(Mat &avg_map, Mat &var_map, Mat &new_frame, Mat &mask, float z, float thresh);
void apply_mask(Mat &mask, Mat &new_frame, Mat &output_frame, Mat &background_frame);
void shut_down();

void mouse_callback(int event, int x, int y, int flags, void* userdata) {
    if  ( event == EVENT_LBUTTONDOWN ) {
        cout << "click "<< endl;

        if (collecting == 0) 
            collecting = 1;
        else
            collecting = 0; 
    }
}

int main(int argc, char ** argv)
{

    if (argc !=2) {
        cout << "usage: " << argv[0] << " directory_of_background_image" << endl;
        return -1;
    }
    
    Mat background_image = imread(argv[1]);

    if (background_image.empty()) {
        cout << argv[1] << " not found" <<endl;
        return -1;
    }

    VideoCapture cap(0);
     
    if (cap.isOpened() == false) {
        cout << "Cannot open the video camera" << endl;
        cin.get(); //wait for any key press
        return -1;
    } 

    cout << "Camera opened" << endl;

    float dWidth = cap.get(CAP_PROP_FRAME_WIDTH);
    float dHeight = cap.get(CAP_PROP_FRAME_HEIGHT);

    cout << "Resolution of the video : " << dWidth << " x " << dHeight << endl;

    Size frame_size = Size((int)dWidth, (int)dHeight);

    resize(background_image, background_image, frame_size);

    namedWindow("Camera feed");
    namedWindow("mask");

    int power = 0;
    bool last_collecting = 0; 
    bool get_vid = false;

    Mat frame = Mat::zeros(frame_size,CV_8UC3);
    vector<Mat> video;

    setMouseCallback("Camera feed", mouse_callback, &collecting);

    cout << "Starting feed" << endl;

    while( cap.isOpened()) {
        //cout << "collecting: " << collecting << endl;
        //cout << "alst collecting: " << last_collecting << endl;
        if(! cap.read(frame)) {
            cout << "capture closed" << endl;
        }
        if (frame.empty()) {
            cout << "frame empty" << endl;
        }
        
        if (last_collecting == 0 && collecting == 1) {
            cout << "starting collection" << endl;
            last_collecting = 1; 
        }
        
        if (collecting == 0 && last_collecting == 1) {
            cout << "ending collection" << endl;
            break;
        }
        cap.set(CAP_PROP_POS_FRAMES, cap.get(CAP_PROP_FRAME_COUNT));
        if (collecting == 1) {
            //cout << "in pushback" << endl;
            video.push_back(frame.clone());
        } //find if frames are different
        
        imshow("Camera feed", frame);
        int k = waitKey(27);
        
        if ( k==27 ) {
            break;
        }
           
    }

    

    cout << "Out of loop... cap is opended "<< cap.isOpened()<< endl;
    cout << "video length: "<< video.size()<< endl;
    float z = 0.4;
    float diff_thresh = 20.0; 

    Mat avg_map = Mat::zeros(frame_size,CV_32FC3);
    Mat var_map = Mat::zeros(frame_size,CV_32FC3);
    Mat mask = Mat::zeros(frame_size, CV_8UC1);
    Mat output = Mat::zeros(frame_size, CV_8UC3);

    get_background(avg_map, var_map, video); 
    //exit(0);
    bool running = true; 

    while (running) {
        cap.set(CAP_PROP_POS_FRAMES, cap.get(CAP_PROP_FRAME_COUNT));
        cap.read(frame);
        
        get_mask(avg_map, var_map, frame, mask, z, diff_thresh);
        apply_mask(mask, frame, output, background_image);
        imshow("mask", mask);
        imshow("Camera feed", output);
        int k = waitKey(33);
        if ( k==27 )
            break;
    }

    return 0;
}

void get_background(Mat &avg_map, Mat &var_map, vector<Mat> video) {
    int i, j, k, l;
    Mat frame;
    int length = video.size();
    float sum;
    for (i = 0; i < avg_map.rows; i++) {
        for (j = 0; j < avg_map.cols; j++) {
            for (l = 0; l < avg_map.channels(); l++) {
                sum = 0;
                for (k = 0; k < length; k++) {
                    frame = video[k];
                    //cout << "frame: "<<(float)frame.at<Vec3b>(i,j)(l) <<endl;
                    sum += (float)frame.at<Vec3b>(i,j)(l); 
                    //cout << sum << endl;
                }
                //cout << "avg: "<<(float)sum/(float)length <<endl;
                avg_map.at<Vec3b>(i,j)(l) = (float)sum/(float)length;
            }
        }
    }

    for (i = 0; i < var_map.rows; i++) {
        for (j = 0; j < var_map.cols; j++) {
            for (l = 0; l < var_map.channels(); l++) {
                sum = 0;
                for (k = 0; k < length; k++) {
                    frame = video[k];
                    //cout << "frame: "<< (float)frame.at<Vec3b>(i,j)(l) << (float)avg_map.at<Vec3b>(i,j)(l) <<endl;

                    sum += pow((float)frame.at<Vec3b>(i,j)(l)-(float)avg_map.at<Vec3b>(i,j)(l),2); 
                }
                if ((float)sum/(float)length != 0)
                    //cout << "var: "<<(float)sum/(float)length <<endl;
                var_map.at<Vec3b>(i,j)(l) = (float)sum/length;
            }
        }
    }
    
}

void get_mask(Mat &avg_map, Mat &var_map, Mat &new_frame, Mat &mask, float z, float thresh) {
    int i, j, k, l;
    float abs_val[2];
    unsigned char px_val; 
    int len = avg_map.channels();
    float mag_sqr_err; 
    float sum;
    
    Mat mask_cp = mask.clone();
    Mat element = getStructuringElement(MORPH_RECT, Size(5,5), Point(2,2));

    for (i = 0; i < avg_map.rows; i++) {
        for (j = 0; j < avg_map.cols; j++) {
            sum = 0;
            for (l = 0; l < len; l++) {
                px_val = new_frame.at<Vec3b>(i,j)(l);
                //cout << "avg: " << (float)avg_map.at<Vec3b>(i,j)(l)<< " var: " << (float)var_map.at<Vec3b>(i,j)(l) << " frame_val: "<< (float)new_frame.at<Vec3b>(i,j)(l) <<endl; 
                abs_val[0] = abs((avg_map.at<Vec3b>(i,j)(l)+sqrt((float)var_map.at<Vec3b>(i,j)(l))*z)-(float)new_frame.at<Vec3b>(i,j)(l));
                abs_val[1] = abs((avg_map.at<Vec3b>(i,j)(l)-sqrt((float)var_map.at<Vec3b>(i,j)(l))*z)-(float)new_frame.at<Vec3b>(i,j)(l));
                
                if (abs_val[0] > abs_val[1]) {
                    //cout << "abs_val " << abs_val[0] <<endl; 
                    sum += abs_val[0]*abs_val[0];
                } else {
                    //cout << "abs_val " << abs_val[1] <<endl; 
                    sum += abs_val[1]*abs_val[1];
                }
            }
            mag_sqr_err = sqrt(sum)/len;
            if (mag_sqr_err > thresh) {
                //cout << "sqr err: "<< mag_sqr_err << endl;
                mask.at<uchar>(i,j) = 0;
            } else {
                //cout << "using bckground" << endl;
                mask.at<uchar>(i,j) = 255;
            }
        }
    }

    erode(mask, mask_cp, element);
    dilate(mask_cp, mask, element);
}

void apply_mask(Mat &mask, Mat &new_frame, Mat &output_frame, Mat &background_frame) {
    int i, j, k, l;
    int len = output_frame.channels();

    for (i = 0; i < output_frame.rows; i++) {
        for (j = 0; j < output_frame.cols; j++) {
            if (mask.at<uchar>(i,j) == 0) { //use webcam image
                for (l = 0; l < len; l++) {
                    output_frame.at<Vec3b>(i,j)(l) = new_frame.at<Vec3b>(i,j)(l);
                }
            } else {
                for (l = 0; l < len; l++) {
                    output_frame.at<Vec3b>(i,j)(l) = background_frame.at<Vec3b>(i,j)(l);
                }
            }
        }
    }
}

void shut_down() {
    cout << "Shutting down ..." << endl;
    exit(0);
}