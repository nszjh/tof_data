#ifndef  __TOF_CLASS_DL_H
#define  __TOF_CLASS_DL_H
#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "DPtr.h"

#define  USE_PCL   1

#include <pcl/point_types.h>
#include <pcl/common/projection_matrix.h>//pcl::PointCloud
#include "meshProc.h"


using namespace GLPlot3D;
struct TofOutput
{
    char className[64];
    int dataWidth;
    int dataHeight;
};

// tof 设备基类， 此头文件结合每个设备独立的动态库 libxxxClassDL.so，实现支持不同设备的需求
class TofClassDL
{
public:

    //virtual char * init(void *argv) = 0;//返回模块名
    virtual DPtr<TofOutput> init(void *argv) = 0;//返回

    virtual void uninit() = 0;
    virtual void pause(int isPause){};
    virtual void getServerConf(char *Addr, int AddrLen, int &port){};
    virtual void resetConf(int set){};


    typedef Function<void (DPtr<cv::Mat> img)> MatDataCallback;
    typedef Function<void (pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloudPt)> PcdDataCallback;
	typedef Function<void (DPtr<GrayDepthField> cloudPt, int width, int height)>                    GLDataCallback;
    void registerD2_DataCb(MatDataCallback f) { MatD2Cb = f; };//回调函数：幅度图
    void registerD23_DataCb(MatDataCallback f) { MatD23Cb = f; };//回调函数：深度图
    void registerD3_DataCb(PcdDataCallback f) { PcdD3Cb = f; };//回调函数：PCD点云
    void registerD3_DataCb2(GLDataCallback f) { GLD3Cb = f; }
    
protected:
    MatDataCallback MatD23Cb;
    MatDataCallback MatD2Cb;
    PcdDataCallback PcdD3Cb;
	GLDataCallback  GLD3Cb;
};


#endif