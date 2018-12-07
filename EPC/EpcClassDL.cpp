#include "EpcClassDL.h"
#include <iostream>
#ifndef WIN32
#include "sys/time.h"
#endif
#include "Filter/dataFilter.h"

#define  CLASS_NAME  "EpcClassDL"

//  Get+CLASS_NAME
#ifdef WIN32
TofClassDL* GetEpcClassDL(void)
{
	return new EpcClassDL;
}
#else
DPtr<TofClassDL> GetEpcClassDL(void)
{
    return DPtr<TofClassDL>(new EpcClassDL);
}
#endif



void  EpcClassDL::LoadConfig(const char *filename)
{
	char EpcCmd[32];
	FILE *fp = fopen(filename, "rb");
	if (fp)
	{
		memset(EpcCmd, 0, sizeof(EpcCmd));

		while (fgets(EpcCmd, sizeof(EpcCmd), fp) != NULL)
		{
			_netRequest.Request(NULL, 0, EpcCmd);
			memset(EpcCmd, 0, sizeof(EpcCmd));

		}
		fclose(fp);
	}

}



void EpcClassDL::getServerConf(char *Addr, int AddrLen, int &port)
{
    if(AddrLen < sizeof(_ipAddr))
        memcpy(Addr, _ipAddr, AddrLen);
    else
        memcpy(Addr, _ipAddr, sizeof(_ipAddr));

    port = _port;
}

void EpcClassDL::LoadOtherConfig(const char *filename)
{
    char EpcCmd[32];
    char strPad[32];
    unsigned int  valuePad = 0;
    int ret = 0;
    int min, max;
    min = max = 0;

    FILE *fp = fopen(filename, "rb");
    
    if (fp)
    {
        memset(EpcCmd, 0, sizeof(EpcCmd));

        while (fgets(EpcCmd, sizeof(EpcCmd), fp) != NULL)
        {
            ret = sscanf(EpcCmd, "%s %d", strPad, &valuePad);
            if(ret == 2)
            {
                if(!strcmp(strPad, "setIntegrationTime3D"))
                {
                    setDeviceByCmd(EpcCmd);
                }
                else if(!strcmp(strPad, "setMinAmplitude"))
                {
                    setDeviceByCmd(EpcCmd); 
                }
                else if(!strcmp(strPad, "setMinExPos"))
                {
                    min = valuePad;
                }
                else if(!strcmp(strPad, "setMaxExPos"))
                {
                    max = valuePad;
                }
                else if(!strcmp(strPad, "setServerAddr"))
                {

                    memset(_ipAddr, 0, sizeof(_ipAddr));
                    unsigned int addr = valuePad;
                    // std::cout << "ipaddr>> \n" << addr << std::endl;

                    unsigned int pad[4];
                    for (int i = (sizeof(pad)/sizeof(int)) - 1; i >= 0; i--)
                    {
                        pad[i] = (addr & 0xff);
                        // std::cout << pad[i] << std::endl;
                        addr = addr >> 8;
                    }
                    snprintf(_ipAddr, sizeof(_ipAddr), "%d.%d.%d.%d", \
                    pad[0], pad[1], pad[2], pad[3]);
                    // std::cout << "ipaddr>> \n" << _ipAddr << std::endl;
                }
                else if(!strcmp(strPad, "setServerPort"))
                {
                    _port = valuePad;
                    // std::cout << "port>> \n" << _port << std::endl;
                }
                else if(!strcmp(strPad, "setRotate"))
                {
                    int isR = valuePad ? 1 : 0;
                    setRotate(isR);
                }
                else if(!strcmp(strPad, "w73"))
                {
                    _w73 = valuePad;
                    char cmd[32] = {0};
                    snprintf(cmd, sizeof(cmd), "w 0x73 %d", _w73);
                    setDeviceByCmd(cmd);
                }
                else if(!strcmp(strPad, "setFov"))
                {
                    _field_view = valuePad;
                }
            }

            memset(EpcCmd, 0, sizeof(EpcCmd));
        }
        fclose(fp);
    }
    
    if(!max)
        max = 1200;
    if(!min)
        min = 300;

    setMinMaxExpos(0, max);
    setMinMaxExpos(1, min);

    setDeviceByCmd("startVideo"); //start Video

    printf("%d %d\n", max, min);
}




void EpcClassDL::initEpcDev()
{
    _netRequest.Request(NULL, 0, "initSystem");
    // _netRequest.Request(NULL, 0, "w 0x73 1");
    _netRequest.Request(NULL, 0, "setIntegrationTime3D 200"); 
    _netRequest.Request(NULL, 0, "setMinAmplitude 400"); //add minAmpValue
    LoadConfig(confName.c_str());

    LoadOtherConfig(othrConfName.c_str());
}


DPtr<TofOutput> EpcClassDL::init(void *argv)
{
    char* ipAddr = (char *) (argv);

	std::cout << "EpcClassDL: init\n";
    if(ipAddr)
    {
        setServer(ipAddr);
        // LoadOtherConfig("Time3D.dat");
        initEpcDev();
        start();
    }


    DPtr<TofOutput> _o(new TofOutput);
    memset(_o->className, 0, sizeof(_o->className));
    strncpy(_o->className, CLASS_NAME, sizeof(_o->className));
    _o->dataHeight = _height;
    _o->dataWidth = _width;
    return _o;
}

// char *EpcClassDL::init(void *argv)
// {
//     char* ipAddr = (char *) (argv);

// 	std::cout << "EpcClassDL: init\n";
//     if(ipAddr)
//     {
//         setServer(ipAddr);
//         // LoadOtherConfig("Time3D.dat");
//         initEpcDev();
//         start();
//     }

//     return (char *)CLASS_NAME;
// }


void EpcClassDL::uninit()
{
    if(_isServerSet)
        _netRequest.Request(NULL, 0, "stopVideo");
    stop();
}

void EpcClassDL::setRotate(int isRotate)
{
    _isRotate = isRotate;
    if(_isRotate)
    {
        _height = 320;
        _width = 240;
    }
    else 
    {
        _height = 240;
        _width = 320;        
    }
}


void EpcClassDL::resetConf(int set)
{
    if(set)
        _isResetConf = 1;
}

EpcClassDL::EpcClassDL()
{ 
    _isServerSet = 0;
    _isPause = 0;
    _isResetConf = 0;
    _w73 = 0;
    _field_view = 12;

    setRotate(0);

    if(!_frameGrabber)
        _frameGrabber = DPtr<DGrabber<DPtr<char>>>(new DGrabber<DPtr<char>>);

    _pcdThreadStop = 0;
    _pcdThread = std::thread(&EpcClassDL::pcdFliterAndShowThread, this);
}


EpcClassDL::~EpcClassDL()
{
    _pcdThreadStop = 1;
    if(_pcdThread.joinable())
        _pcdThread.join();

    uninit();
}


void EpcClassDL::setServer(const char *ipAddr)
{
    _netRequest.SetServer(ipAddr);
    _isServerSet = 1;
}


void EpcClassDL::pcdFliterAndShowThread()
{
    while(!_pcdThreadStop)
    {
        DPtr<char>  frame = _frameGrabber->DGrabber_GetData();
        _frameGrabber->release();

		if(frame)
			setDepthImage3Point(frame.get(), _height * _width * 2);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}


void EpcClassDL::setDepthImage(const char* buffer, int bufferLen)
{
    uint16_t *imgBuf = new uint16_t[_height*_width];
    
    cv::Mat depthImg(_height, _width, CV_32FC1);//
    DPtr<cv::Mat> depthImg_show(new cv::Mat(_height, _width, CV_8UC1));//

    float *data = reinterpret_cast<float *>(depthImg.data);  
    memcpy(imgBuf, buffer, bufferLen);
    for(int i=0; i<depthImg.rows * depthImg.cols; i++)
    {
        float depthData = imgBuf[i];
        depthData /= 4.8;

        if(depthData > 1800)    ////限制范围
            depthData = 1800;
        else if(depthData < 500)
            depthData = 0;
        
        data[i] = depthData;
    }
    cv::normalize(depthImg, depthImg, 0, 1, cv::NORM_MINMAX);
    depthImg.convertTo(*depthImg_show, CV_8UC1, 255, 0);
    unsigned char *dataU8 = reinterpret_cast<unsigned char *>(depthImg_show->data); 
    for(int i=0; i<depthImg.rows * depthImg.cols; i++)
    {
        if(dataU8[i])   
            dataU8[i] = 255 - dataU8[i];
    }

    if(MatD23Cb)
        MatD23Cb(depthImg_show);

    delete imgBuf;
}

DPtr<cv::Mat> EpcClassDL::rawDataTo32F(const char* buffer, int bufferLen)
{
    uint16_t *imgBuf = (uint16_t *)buffer;
    DPtr<cv::Mat> ampImg(new cv::Mat(_height, _width, CV_32FC1));//

    DPtr<cv::Mat> rotateImg = nullptr;
    float *data1;
    data1 = reinterpret_cast<float *>(ampImg->data);  

    for(int i=0; i<ampImg->rows * ampImg->cols; i++)
    {
        float ampData = imgBuf[i];
        data1[i] = ampData / 65300;
    }

    // cv::GaussianBlur(*ampImg, *ampImg, cv::Size(3, 3), 0, 0);
    return ampImg;
}


DPtr<cv::Mat> EpcClassDL::rawDataTo8U(const char* buffer, int bufferLen)
{
    DPtr<cv::Mat> ampImg = rawDataTo32F(buffer, bufferLen);

    //对数变换，提升对比度
    float *data_amp  = reinterpret_cast<float *>(ampImg->data);
    for (int i = 0; i<ampImg->rows * ampImg->cols; i++)
    {
        data_amp[i] = 0.6 * log(1 + data_amp[i]);
    }
    //归一化到0~255  
    cv::normalize(*ampImg, *ampImg, 0, 1, cv::NORM_MINMAX);

    DPtr<cv::Mat> ampImgU8(new cv::Mat(_height, _width, CV_8UC1));
    ampImg->convertTo(*ampImgU8, CV_8UC1, 255, 0);
    return ampImgU8;
}


void EpcClassDL::setAmpImage(const char* buffer, int bufferLen)
{
    // uint16_t *imgBuf = new uint16_t[_height*_width];
    DPtr<cv::Mat> ampImg = rawDataTo32F(buffer, bufferLen);
	// cv::flip(*ampImg, *ampImg, 1);
    if(MatD2Cb)
        MatD2Cb(ampImg);

    // delete imgBuf;
}

void holefillfilterV2(unsigned short *pixeldata)
{
	cv::Mat sorceImg(240, 320, CV_16U, (unsigned short *)pixeldata);
	cv::Mat dstImg;
	cv::Mat erodeStruct = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(4, 4));
	morphologyEx(sorceImg, sorceImg, cv::MORPH_OPEN, erodeStruct);
	//imshow("before", sorceImg);
	//imshow("after", dstImg);
	//waitKey(10);
}


#define MAX_DEPTH_VALUE  (7000)
//对图像进行空洞填充(n次迭代扫描)
void Darkness_Filling(unsigned short *pixeldata, int width, int height)
{
	int i,j,count=0;
	unsigned short* ptr1;
	unsigned short* ptr2;
	unsigned short* ptr3;
	unsigned short* ptr_gray;
 
    cv::Mat sourceImg(height, width, CV_16UC1, (unsigned short *)pixeldata);

	//x和y方向的梯度
    cv::Mat gradient_x(height, width, CV_16UC1);
    cv::Mat gradient_y(height, width, CV_16UC1);
 
	unsigned short* gradientx;
	unsigned short* gradienty;
	unsigned short* tempx;
	unsigned short* tempy;
 
 
	//计算x和y方向的梯度图
	Sobel(sourceImg,gradient_x, CV_16UC1, 1, 0,3);
	Sobel(sourceImg,gradient_y, CV_16UC1, 0,1,3);
 
 
	for(i = 1;i < height;i++)
	{

		ptr3 = sourceImg.ptr<unsigned short>(i);
 
		//计算x和y方向上的对应点的梯度
		gradientx = gradient_x.ptr<unsigned short>(i);
		gradienty = gradient_y.ptr<unsigned short>(i);
		tempx = gradient_x.ptr<unsigned short>(i);
		tempy = gradient_y.ptr<unsigned short>(i);
		//在灰度图像上寻找空洞点
		for(j = 1;j < width;j++)
		{
            std::vector<unsigned short> v;

			if(ptr3[j] > 0 && ptr3[j] < MAX_DEPTH_VALUE)//7000
				continue;
			//若纹理在y方向
			if(gradientx[j]>gradienty[j])
			{
				//该点值判定为上方一点的值
                {
                    if(i > 5)
                    {
                        ptr_gray = sourceImg.ptr<unsigned short>(i-5);  //得到灰度图上方像素指针
                        ptr3[j] = ptr_gray[j];
                    }
                    // if(i > 10)
                    // {
                    //     for(int k = 10; k>5; k--)
                    //     {
                    //         ptr_gray = sourceImg.ptr<unsigned short>(i-k);  //得到灰度图上方像素指针
                    //         v.push_back(ptr_gray[j]);
                    //     }
                    //     std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());

                    //     ptr3[j] = v[v.size() / 2];
                    // }
                }
               
				if(gradientx[j+1]==gradienty[j+1])
				{
					tempx = gradient_x.ptr<unsigned short>(i-1); 
					tempy = gradient_y.ptr<unsigned short>(i); 
					gradientx[j+1] = (gradientx[j] + tempx[j])/2;
					gradienty[j+1] = (gradienty[j] + tempy[j])/2;
				}
			}
			//若纹理在x方向
			else 
			{
				//该点值判定为左方一点的值
                {
				    ptr_gray = sourceImg.ptr<unsigned short>(i); //得到灰度图左方像素指针
				    //填灰度图，为下次判断进行填充
                    if(j > 5)
				    { 
                        ptr3[j] = ptr_gray[j-5];
                    }
  
                }
 
				if(gradientx[j+1] == gradienty[j+1])
				{
					tempx = gradient_x.ptr<unsigned short>(i-1); 
					tempy = gradient_y.ptr<unsigned short>(i); 
					gradientx[j+1] = (gradientx[j] + tempx[j])/2;
					gradienty[j+1] = (gradienty[j] + tempy[j])/2;
				}
 
			}
		}
 
	}

}




#define FIELD_VIEW   12
void EpcClassDL::setDepthImage2Point(const char* buffer, int bufferLen)
{
    
    if(PcdD3Cb && buffer && bufferLen > 0)
    {
        uint16_t *imgBuf = new uint16_t[_height*_width];
        memcpy(imgBuf, buffer, bufferLen);

        DPtr<cv::Mat> cloudPoint(new cv::Mat(_height, _width, CV_16UC1, imgBuf));//
        // DPtr<cv::Mat> cloudPointTmp(new cv::Mat(_height, _width, CV_16UC1, imgBufTmp));//
        // cv::medianBlur(*cloudPoint, *cloudPoint, 3);
        // FlypixFilter<uint16_t>(imgBuf, imgBuf, 200000, _width, _height);

        //IIR
        // IIRFilter<uint16_t>(imgBuf, imgBuf, 0.5, _width, _height);
        // IIRFilter<uint16_t>(imgBuf, imgBuf, 0.5, _width, _height);


        uint16_t *ampBuf = new uint16_t[_height* _width];
        memcpy(ampBuf, buffer + bufferLen, bufferLen);
        // BilateralFilter<uint16_t>(imgBuf, ampBuf, imgBuf, _width, _height);
        TemporalMedianFilter<uint16_t>(imgBuf, imgBuf, _width, _height);

        // BilateralFilter<uint16_t>(imgBuf, ampBuf, imgBuf, _width, _height);
		//medianFilter(imgBuf, 320, 240);
        cv::medianBlur(*cloudPoint, *cloudPoint, 5);
		gaussFilter(imgBuf, _width, _height, true, 3);

		holefillfilterV2(imgBuf);
        //cv::GaussianBlur(*cloudPoint, *cloudPoint, cv::Size(3, 3), 0, 0);
 
        // medianFilter(imgBuf, _width, _height);//中值

        pcl::PointCloud<pcl::PointXYZRGB>::Ptr outcloud(new pcl::PointCloud<pcl::PointXYZRGB>);
        outcloud->width = _width;
        outcloud->height = _height;
        outcloud->is_dense = true;
        outcloud->points.resize(_width * _height);

        int cnt = 0;
        double  alfa0 = _field_view * 3.14159 / 180;
        double  step = alfa0 / (_width / 2);
        // double  beta0 = step * (_height / 2);
        double  alfa, beta;


        //add amp show
        DPtr<cv::Mat>  ampBufU8 = rawDataTo8U(buffer + bufferLen, bufferLen);
        uint8_t *ampU8Data = reinterpret_cast<uint8_t *>(ampBufU8->data);  

        for(int i=0; i<_height; i++)
        {    
            beta = (i - _height / 2) * step;
            for(int j=0; j<_width; j++)
            {
                alfa = (j - _width / 2) * step;
                float depthPixel = ((float)imgBuf[cnt]) / 4800;
				// float depthPixel = ((float)imgBuf[cnt]) / 48 - 30;
                // float depthPixel = ((float)imgBuf[cnt]) / 4.8;

                if(depthPixel > 0 && depthPixel < 2)
                {
                    // outcloud->points[cnt].x = (float)j / 1000.0;
                    // outcloud->points[cnt].y = (float)i / 1000.0;
                    outcloud->points[cnt].x = depthPixel * cos(beta) * sin(alfa);
                    outcloud->points[cnt].y = depthPixel * sin(beta);
                    outcloud->points[cnt].z = depthPixel * cos(alfa) * cos(beta);
                    outcloud->points[cnt].r = ampU8Data[cnt];
                    outcloud->points[cnt].g = ampU8Data[cnt];
                    outcloud->points[cnt].b = ampU8Data[cnt];
                }
                else
                {
                    //windows 平台  rgb域只能置0，否则后面点云显示会不正常，而Linux设成std::numeric_limits<float>::quiet_NaN() 也OK
                    outcloud->points[cnt].x = 0;//std::numeric_limits<float>::quiet_NaN();
                    outcloud->points[cnt].y = 0;//std::numeric_limits<float>::quiet_NaN();
                    outcloud->points[cnt].z = 0;//std::numeric_limits<float>::quiet_NaN();
                    outcloud->points[cnt].rgb = 0;//std::numeric_limits<float>::quiet_NaN();
                    // outcloud->points[cnt].r = std::numeric_limits<float>::quiet_NaN();
                    // outcloud->points[cnt].g = std::numeric_limits<float>::quiet_NaN();
                    // outcloud->points[cnt].b = std::numeric_limits<float>::quiet_NaN();
                }
                cnt++;
            }
        }

        if(PcdD3Cb)
            PcdD3Cb(outcloud);

		delete imgBuf;
        // delete imgBufTmp;
        delete ampBuf;
    }
}

//#define FIELD_VIEW   15.5
void EpcClassDL::setDepthImage3Point(const char* buffer, int bufferLen)
{
    
    if(GLD3Cb && buffer && bufferLen > 0)
    {
        uint16_t *imgBuf = new uint16_t[_height*_width];
        memcpy(imgBuf, buffer, bufferLen);
		//holefillfilterV2(imgBuf);

        DPtr<cv::Mat> cloudPoint(new cv::Mat(_height, _width, CV_16UC1, imgBuf));//
        // DPtr<cv::Mat> cloudPointTmp(new cv::Mat(_height, _width, CV_16UC1, imgBufTmp));//
        // cv::medianBlur(*cloudPoint, *cloudPoint, 3);
        // FlypixFilter<uint16_t>(imgBuf, imgBuf, 200000, _width, _height);

        //IIR
        // IIRFilter<uint16_t>(imgBuf, imgBuf, 0.5, _width, _height);
        // IIRFilter<uint16_t>(imgBuf, imgBuf, 0.5, _width, _height);


        uint16_t *ampBuf = new uint16_t[_height* _width];
        memcpy(ampBuf, buffer + bufferLen, bufferLen);
        // BilateralFilter<uint16_t>(imgBuf, ampBuf, imgBuf, _width, _height);

        //edgeFilterPixels(imgBuf, _width, _height);

		Darkness_Filling(imgBuf, _width, _height); 

        TemporalMedianFilter<uint16_t>(imgBuf, imgBuf, _width, _height);

        // BilateralFilter<uint16_t>(imgBuf, ampBuf, imgBuf, _width, _height);
		//medianFilter(imgBuf, 320, 240);
        cv::medianBlur(*cloudPoint, *cloudPoint, 5);
		
		gaussFilter(imgBuf, _width, _height, true, 3);
		


		// holefillfilterV2(imgBuf);

		//DPtr<cv::Mat>  ampBufU32 = rawDataTo32F((char *)imgBuf, bufferLen);
		//float *data1 = reinterpret_cast<float *>(ampBufU32->data);
		//// memcpy(imgBuf, buffer, bufferLen);

		//for (int i = 0; i<ampBufU32->rows * ampBufU32->cols; i++)
		//{
		//	imgBuf[i] = data1[i] * 65300;
		//}
        //cv::GaussianBlur(*cloudPoint, *cloudPoint, cv::Size(3, 3), 0, 0);
 
        // medianFilter(imgBuf, _width, _height);//中值

        int cnt = 0;
        double  alfa0 = _field_view * 3.14159 / 180;
        double  step = alfa0 / (_width / 2);
        double  beta0 = step * (_height / 2);
        double  alfa, beta;

 
        //add amp show
        DPtr<cv::Mat>  ampBufU8 = rawDataTo8U(buffer + bufferLen, bufferLen);
        uint8_t *ampU8Data = reinterpret_cast<uint8_t *>(ampBufU8->data);  

        DPtr<GrayDepthField>  outcloud = DPtr<GrayDepthField>(new GrayDepthField);
		outcloud->resize(_width * _height);
        float lastDepth = 0;
		unsigned char lastPixelClr = 0;

        int w = _width;
        int h = _height;

        for(int i=0; i<h; i++)
        {    
            // beta = (i - _height / 2) * step;
            beta = i * step - beta0;
            for(int j=0; j<w; j++)
            {
                // alfa = (j - _width / 2) * step;
                alfa = j * step - alfa0;

				float depthPixel = ((float)imgBuf[cnt]) / 4800 - 0.3 * _w73; // - 0.3;
				// float depthPixel = ((float)imgBuf[cnt]) / 4800;
				// float depthPixel = ((float)imgBuf[cnt]) / 48 - 30;
                // float depthPixel = ((float)imgBuf[cnt]) / 4.8;

                if(depthPixel > 0 && depthPixel < 2)
                {
					(*outcloud)[cnt].x = (float)j / 1000.0;
					(*outcloud)[cnt].y = (float)i / 1000.0;
                    // (*outcloud)[cnt].x = depthPixel * cos(beta) * sin(alfa);
                    // (*outcloud)[cnt].y = depthPixel * sin(beta);
                    (*outcloud)[cnt].z = depthPixel * cos(alfa) * cos(beta);
                    (*outcloud)[cnt].g = ampU8Data[cnt];
                    // lastDepth = depthPixel;
					// lastPixelClr = ampU8Data[cnt];
                }
                else
                {
                    {
                        //windows 平台  rgb域只能置0，否则后面点云显示会不正常，而Linux设成std::numeric_limits<float>::quiet_NaN() 也OK
                        (*outcloud)[cnt].x = 0;//std::numeric_limits<float>::quiet_NaN();
                        (*outcloud)[cnt].y = 0;//std::numeric_limits<float>::quiet_NaN();
                        (*outcloud)[cnt].z = 0;//std::numeric_limits<float>::quiet_NaN();
                        (*outcloud)[cnt].g = ampU8Data[cnt];//std::numeric_limits<float>::quiet_NaN();
                    }
                }
                cnt++;
            }
        }


        if(GLD3Cb)
			GLD3Cb(outcloud, _width, _height);

		delete imgBuf;
        // delete imgBufTmp;
        delete ampBuf;
    }
}




void EpcClassDL::thread_Loop()
{
    // char *receivebuf = new  char[_height * _width * 4];

    struct timeval start;
    struct timeval end;
    while(_isLoop)
    {
        // int ret = _netRequest.Request(receivebuf, 320 * 240 * 2, "getDistanceSorted"); 
        // int ret = _netRequest.Request(receivebuf, 320 * 240 * 2, "getAmplitudeSorted");
        if(_isServerSet)
        { 
#ifndef WIN32
            gettimeofday(&start, NULL);
#endif
            if(_isPause == 1)
                setIntegrationTime(_maxExpos);
            else if(_isPause == 2)
            {
                _isPause = 0;
                setIntegrationTime(_minExpos);
            }

            DPtr<char>  sharedReceiveBuf(new  char[_height * _width * 4]);

			int ret = _netRequest.Request(sharedReceiveBuf.get(), _height * _width * 4, "getDistanceAndAmplitudeSorted"); 
            if(ret == _height * _width * 4)
            {
                //add cloudpoint show
                // setDepthImage2Point((char *)receivebuf, _height * _width * 2);
                uint16_t *imgBuf;
                if(_isRotate)
                {//旋转两组数据
                    imgBuf = (uint16_t *)sharedReceiveBuf.get();
                    DPtr<char>  ReceiveBuf(new  char[_height * _width * 2]);
                    memcpy(ReceiveBuf.get(), sharedReceiveBuf.get(), _height * _width * 2);
                    DPtr<cv::Mat> srcImage(new cv::Mat(_width, _height, CV_16UC1, (uint16_t *)ReceiveBuf.get()));//注意这里顺序
                    DPtr<cv::Mat> transImage(new cv::Mat(_height, _width, CV_16UC1, imgBuf));
                    cv::transpose(*srcImage, *transImage);
                    cv::flip(*transImage, *transImage, 1);

                    imgBuf = (uint16_t *)(sharedReceiveBuf.get() + _height * _width * 2);
                    memcpy(ReceiveBuf.get(), imgBuf, _height * _width * 2);
                    DPtr<cv::Mat> srcImage2(new cv::Mat(_width, _height, CV_16UC1, (uint16_t *)ReceiveBuf.get()));//注意这里顺序
                    DPtr<cv::Mat> transImage2(new cv::Mat(_height, _width, CV_16UC1, imgBuf));
                    cv::transpose(*srcImage2, *transImage2);
                    cv::flip(*transImage2, *transImage2, 1);
                }

                _frameGrabber->DGrabber_SetData(sharedReceiveBuf);
                setDepthImage((char *)sharedReceiveBuf.get(), _height * _width * 2);
                setAmpImage((char *)(sharedReceiveBuf.get() + _height * _width * 2), _height * _width * 2);
#ifndef WIN32
				gettimeofday(&end, NULL);
				long diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
				//std::cout << "take times: " << diff << std::endl;
#endif
                
                //std::cout << std::endl;
            }
            sharedReceiveBuf.reset();
            sharedReceiveBuf = nullptr;
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        
        if(_isResetConf)
        {
            _isResetConf = 0;
            LoadConfig(confName.c_str());
            LoadOtherConfig(othrConfName.c_str());
        }


    }
    // delete receivebuf;
}

void EpcClassDL::setDeviceByCmd(char *EpcCmd)
{
    if(_isServerSet)
        _netRequest.Request(NULL, 0, EpcCmd); 
}

void EpcClassDL::setIntegrationTime(int value)
{
    char EpcCmd[32] = {0};
    snprintf(EpcCmd, sizeof(EpcCmd), "setIntegrationTime3D %d", value);
    if(_isServerSet)
        _netRequest.Request(NULL, 0, EpcCmd); 
}

void EpcClassDL::pause(int isPause)
{
    _isPause = isPause;
}


void EpcClassDL::setMinMaxExpos(int isMin, int value)
{
    isMin ? _minExpos = value : _maxExpos = value;
}
