#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif

#include "TofClassDL.h"
#include "../protoServer/DataRecv.h"
#include "D23GuiFrame.h"
#include "meshProc.h"
#include "RawDataIo.h"
//#include "../Filter/splineCubic.h"
#include "../protoServer/ProtoInterface.h"


// 以动态库形式加载
int main(int argc, char **argv)
{ 

	DPtr<Mesh> mesh_ = nullptr;
	DPtr<TofClassDL> tof_ = nullptr;

#ifdef WIN32
	typedef TofClassDL*(*tof_dl_f)(void);

	HINSTANCE  lib_dl = LoadLibrary("./EpcClassDL.dll");
	if (lib_dl == NULL)
	{
		printf("load library EpcClassDL.dll error\n");
		return -1;
	}
	tof_dl_f tof_dl;
	tof_dl  = (tof_dl_f)GetProcAddress(lib_dl, "GetEpcClassDL");
	if (!tof_dl)
	{
		printf("EPCClassDL not found\n");
		FreeLibrary(lib_dl);
		return -1;
	}
	tof_ = DPtr<TofClassDL>((*tof_dl)());


	// using namespace GLPlot3D;
	typedef Mesh*(*mesh_dl_f)(void);

	HINSTANCE  lib_dl2 = LoadLibrary("./mesh.dll");
	if (lib_dl2 == NULL)
	{
		printf("load library mesh.dll error\n");
		return -1;
	} 
	mesh_dl_f mesh_dl;
	mesh_dl = (mesh_dl_f)GetProcAddress(lib_dl2, "GetMeshClassDL");
	if (!mesh_dl)
	{
		printf("mesh not found\n");
		FreeLibrary(lib_dl2);
		return -1;
	}
	mesh_ = DPtr<Mesh>((*mesh_dl)());
	//mesh_->init();

#else
	DPtr<TofClassDL>(*tof_dl)(void);
    void *lib_dl = dlopen("./libEpcClassDL.so", RTLD_LAZY);
	if (NULL == lib_dl)
	{
		printf("load library libtdlc.so error.\nErrmsg:%s\n", dlerror());
		return -1;
	}
	tof_dl = (DPtr<TofClassDL> (*)(void))dlsym(lib_dl, "GetEpcClassDL");
    const char *dlmsg = dlerror();
    if(NULL != dlmsg)
    {
        printf("get class testdl error\nErrmsg:%s\n",dlmsg);
        dlclose(lib_dl);
        return -1;
    }
	tof_ = (*tof_dl)();


	/*add  mesh module*/
	Mesh*(*mesh_dl_f)(void);
    void *lib_dl2 = dlopen("./libmesh.so", RTLD_LAZY);
	if (NULL == lib_dl2)
	{
		printf("load library libtdlc.so error.\nErrmsg:%s\n", dlerror());
		return -1;
	}
	mesh_dl_f = (Mesh*(*)(void))dlsym(lib_dl2, "GetMeshClassDL");
	const char *dlmsg2 = dlerror();
    if(NULL != dlmsg2)
    {
        printf("get class testdl error\nErrmsg:%s\n",dlmsg2);
        dlclose(lib_dl2);
        return -1;
    }
	mesh_ = DPtr<Mesh>((*mesh_dl_f)());

#endif

	DPtr<ProtoInterface> pro;

	pro = DPtr<ProtoInterface>(new ProtoInterface);
	pro->setDisplayAndControl(mesh_, tof_);

	dlib::D23GuiFrame  _gui;
	_gui.getManager()->setDeviceType(1);
	_gui.getManager()->start();

    auto f1 = std::bind(&dlib::D23GuiFrame::feedD23Image, &_gui, std::placeholders::_1);
    auto f2 = std::bind(&dlib::D23GuiFrame::feedD2Image, &_gui, std::placeholders::_1);
    auto f3 = std::bind(&dlib::D23GuiFrame::pcdShow , &_gui, std::placeholders::_1);
    auto f4 = std::bind(&ProtoInterface::setCropRegion, pro,  std::placeholders::_1);

	auto f5 = std::bind(&ProtoInterface::cropObject, pro, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	auto f6 = std::bind(&ProtoInterface::getElipseRegion, pro, std::placeholders::_1 );
	//此处增加复位配置的回调响应
	auto f7 = std::bind(&TofClassDL::resetConf,  tof_, std::placeholders::_1 );
	auto f8 = std::bind(&ProtoInterface::modelSetSave, pro, std::placeholders::_1);

    tof_->registerD23_DataCb(f1);
    tof_->registerD2_DataCb(f2);
    tof_->registerD3_DataCb(f3);
	tof_->registerD3_DataCb2(f5);
    //_gui.getManager()->registerD3FacePosCb(f4);//注册位置的回调接口
	_gui.getManager()->registerD3FaceLandCb(f6);
	_gui.registerClickedCb(f7, f8);
	// _gui.registerNameCb(std::bind(&ProtoInterface::modelSetSaveByname, pro, std::placeholders::_1), 
	// std::bind(&ProtoInterface::modelShowByname, pro, std::placeholders::_1));

	char ipaddr[16];
	if (argc == 2)
		memcpy(ipaddr, argv[1], sizeof(ipaddr));
	else
		memcpy(ipaddr, "192.168.1.243", sizeof(ipaddr));

    DPtr<TofOutput>  initRet = tof_->init(ipaddr);
    std::cout << initRet->className << std::endl;
	mesh_->init(initRet->dataWidth, initRet->dataHeight);
	
	char Addr[16] = {0};
	int port;
	tof_->getServerConf(Addr, sizeof(Addr), port);
	std::string ipAddr = Addr;
	// std::cout << "protoServer: " << ipAddr << " " << port << std::endl;
	// //start proto server
	// std::thread t(&ProtoInterface::initProtoServer, pro, ipAddr, port);
	// t.detach();

    _gui.wait_until_closed();
    _gui.getManager()->stop();
    tof_->uninit();
    tof_.reset();
    tof_ = nullptr;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));//
#ifdef WIN32
	FreeLibrary(lib_dl);
	FreeLibrary(lib_dl2);
#else
    dlclose(lib_dl);
	dlclose(lib_dl2);
#endif
    return 0;
}
