/*
姿态预估测试
*/

#include <iostream>
#include <opencv/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp> //kalman filter
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <sys/time.h>

#include "Net.h"
#include "DPtr.h"
#include "HeadPosePrediction.h"

NetRequest _netRequest;

int init()
{
    // _netRequest.Request(NULL, 0, "w 11 FA");
    _netRequest.Request(NULL, 0, "setImageAveraging 0");
    _netRequest.Request(NULL, 0, "correctAmbientLight 1");
    _netRequest.Request(NULL, 0, "setAmbientLightFactor 250");
    _netRequest.Request(NULL, 0, "enableDefaultOffset 1");
    _netRequest.Request(NULL, 0, "correctDRNU 2");
    _netRequest.Request(NULL, 0, "correctTemperature 2");
    // _netRequest.Request(NULL, 0, "enableVerticalBinning 0");
    // _netRequest.Request(NULL, 0, "setRowReduction 0");
    // _netRequest.Request(NULL, 0, "enableHorizontalBinning 0");
    // _netRequest.Request(NULL, 0, "setModulationFrequency 1");
    _netRequest.Request(NULL, 0, "setABS 1");
    _netRequest.Request(NULL, 0, "enableSaturation 1");
    _netRequest.Request(NULL, 0, "enableAdcOverflow 1");
    // _netRequest.Request(NULL, 0, "enablePiDelay 0");
    _netRequest.Request(NULL, 0, "selectMode 0");
    _netRequest.Request(NULL, 0, "loadConfig 1");
    _netRequest.Request(NULL, 0, "setMinAmplitude 100");
    // _netRequest.Request(NULL, 0, "setHysteresis 0");
    _netRequest.Request(NULL, 0, "setHysteresis 20");
    // _netRequest.Request(NULL, 0, "enableDualMGX 0");
    // _netRequest.Request(NULL, 0, "enableHDR 0");
    // _netRequest.Request(NULL, 0, "setROI 4 323 6 125");
    //_netRequest.Request(NULL, 0, "setIntegrationTime3D 1500");
    _netRequest.Request(NULL, 0, "setIntegrationTime3D 550");
    // _netRequest.Request(NULL, 0, "setShutter");

    return 0;
}

int stop()
{
    _netRequest.Request(NULL, 0, "stopShutter");
}

DPtr<cv::Mat> setAmpImage(const char *buffer, int bufferLen)
{
    short *imgBuf = new short[240 * 320];

    DPtr<cv::Mat> ampImg(new cv::Mat(240, 320, CV_32FC1));   //
    DPtr<cv::Mat> ampOutImg(new cv::Mat(240, 320, CV_8UC1)); //

    float *data1 = reinterpret_cast<float *>(ampImg->data);
    unsigned char *data2 = reinterpret_cast<unsigned char *>(ampOutImg->data);
    memcpy(imgBuf, buffer, bufferLen);

    for (int i = 0; i < ampImg->rows * ampImg->cols; i++)
    {
        float depthData = imgBuf[i];
        data1[i] = depthData / 65300;
        data1[i] = log(1 + data1[i]);
    }
    cv::GaussianBlur(*ampImg, *ampImg, cv::Size(3, 3), 0, 0);

    //归一化到0~255
    cv::normalize(*ampImg, *ampImg, 0, 1, cv::NORM_MINMAX);
    ampImg->convertTo(*ampOutImg, CV_8UC1, 255, 0);

    delete imgBuf;

    return ampOutImg;
    // cv::imshow("amp", *ampImg);
    // cv::imshow("depth-test", *depthImg_show);
    // cv::waitKey(10);
}

// usage: ./head_pose 192.168.1.237
int main(int argc, char **argv)
{
    // //open cam
    // cv::VideoCapture cap(0);
    // if (!cap.isOpened())
    //     {
    //     std::cout << "Unable to connect to camera" << std::endl;
    //     return EXIT_FAILURE;
    //     }
    //Load face detection and pose estimation models (dlib).
    if (argc != 2)
    {
        std::cout << "usage: ./head_pose 192.168.1.237\n";
        return -1;
    }
    _netRequest.SetServer(argv[1]);
    init();

    dlib::frontal_face_detector detector = dlib::get_frontal_face_detector();
    dlib::shape_predictor predictor;
    dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> predictor;

    //text on screen
    std::ostringstream outtext;

    std::cout << argv[1] << std::endl;
    char *receivebuf = new char[240 * 320 * 4];

    HeadPosePrediction headPose;
    std::map<int, double> matchMap;

    // dlib::D23GuiFrame  gui;

    //main loop
    while (1)//1
    {
        // Grab a frame
        // cv::Mat srcImg;
        cv::Mat temp;
        DPtr<cv::Mat> srcImg = nullptr;

        int ret = _netRequest.Request(receivebuf, 240 * 320 * 4, "getDistanceAndAmplitudeSorted");
        if (ret == 240 * 320 * 4)
        {
            srcImg = setAmpImage((char *)(receivebuf + 240 * 320 * 2), 240 * 320 * 2);
        }

        // srcImg = cv::imread(argv[1], -1);
        // cap >> temp;
        // dlib::cv_image<dlib::bgr_pixel> cimg(temp);
        // dlib::matrix<dlib::rgb_pixel> cimg;
        // dlib::load_image(cimg, argv[1]);
        if (!srcImg)
            continue;

        dlib::cv_image<unsigned char> cimg(*srcImg);

        cvtColor(*srcImg, temp, CV_GRAY2RGB);

        // Detect faces
        std::vector<dlib::rectangle> faces = detector(cimg);

        // Find the pose of each face
        if (faces.size() > 0)
        {
            //track features
            dlib::full_object_detection shape = predictor(cimg, faces[0]);

            //draw features
            for (unsigned int i = 0; i < 68; ++i)
            {
                cv::circle(temp, cv::Point(shape.part(i).x(), shape.part(i).y()), 2, cv::Scalar(0, 0, 255), -1);
            }

            struct EulerianAngle angle;
            MatchHeadPose matchP;
            headPose.headPosePredict(shape, angle);

            static int beginFlag = -1;
            int matchPos = matchP.matchPose(angle);
            if (matchPos == 0 && beginFlag == -1)
            {
                beginFlag = 0;
            }

            static cv::Point origin_p(100, 100);
            static cv::Point2f center(100, 100);
            if (!beginFlag)
            {
                cv::circle(temp, origin_p, 3, cv::Scalar(255, 0, 0), -1); //中心点
                if (matchPos > 0)
                {
                    const double pi = std::acos(-1);
                    matchMap[matchPos] = matchPos * pi / 4;

                    for (auto &_m : matchMap)
                        cv::line(temp, matchP.calcPoint(center, 10, _m.second - pi * 3 / 4), matchP.calcPoint(center, 10, _m.second - pi * 2 / 4), cv::Scalar(0, 255, 255), 2, 8, 0);
                }
            }

            outtext << "match: " << std::setprecision(1) << matchPos;
            cv::putText(temp, outtext.str(), cv::Point(50, 20), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 255, 0));
            outtext.str("");
            //show angle result
            outtext << "X: " << std::setprecision(3) << angle.x;
            cv::putText(temp, outtext.str(), cv::Point(50, 40), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 255, 0));
            outtext.str("");
            outtext << "Y: " << std::setprecision(3) << angle.y;
            cv::putText(temp, outtext.str(), cv::Point(50, 60), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 255, 0));
            outtext.str("");
            outtext << "Z: " << std::setprecision(3) << angle.z;
            cv::putText(temp, outtext.str(), cv::Point(50, 80), cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(0, 255, 0));
            outtext.str("");
        }

        //press esc to end
        cv::namedWindow("EPC_pose", CV_WINDOW_NORMAL);
        cv::resizeWindow("EPC_pose",  temp.cols * 1.5, temp.rows * 1.5);
        cv::imshow("EPC_pose", temp);
        char key = (char)cv::waitKey(10);
        if ('q' == key)
            break;
        // cv::Mat imgset;
        // cv::hconcat(temp, temp, imgset);
        // cv::Mat reImgSet(temp.rows * 1.5, temp.cols * 1.5, CV_8UC3);
        // cv::resize(temp, reImgSet, reImgSet.size());
        // dlib::cv_image<dlib::bgr_pixel> cimg2(reImgSet);
        // dlib::cv_image<dlib::bgr_pixel> cimg3(reImgSet);

        // // win.clear_overlay();
        // gui.set_image_1(cimg2);
        // gui.set_image_2(cimg3);
        
    }

    stop();
    delete receivebuf;
    return 0;
}