/*
 * NV Lib component.
 *
 * Copyright (c) 2018 Nephovision Inc.
 * 
 * author: nszjh 
 */
#ifndef  _APP_H
#define  _APP_H


#include <thread>
#include <vector>


//应用
class App
{
public:
    App(){ 
        _width = 320;  
        _height = 240; 
        _isLoop = false; 
    };
    ~App(){ 
        if(_isLoop)
            stop();
    };

    virtual void start(){
        if(_Thread.joinable())
            _Thread.join();

        _isLoop = true;
        _Thread = std::thread(&App::thread_Loop, this);
    };
    virtual void stop(){
        _isLoop = false;
        if(_Thread.joinable())
            _Thread.join();
    };

    virtual void thread_Loop() = 0;  //转换线程

    
protected:

    int _width;
    int _height;
    bool          _isLoop;
    std::thread   _Thread;
    // DPtr<DGrabber<pcl::PointCloud<pcl::PointXYZI>::Ptr>> _grab;

};


#endif