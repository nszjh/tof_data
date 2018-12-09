#include "TofUSBChn.h"
#include <iostream>

#define  CLASS_NAME  "TofUSBChn"


DPtr<TofOutput> TofUSBChn::init(void *argv) //todo: add usb id
{
    _width = 0;
    _height = 0;
    _usb_sys_win = DPtr<USBSysWin>(new USBSysWin);
    _isInit = 1;
	return nullptr;
}


void TofUSBChn::uninit()
{
    if (_isInit) {
        _width = 0;
        _height = 0;
        if (_usb_sys_win)
            _usb_sys_win.reset();
        _usb_sys_win = nullptr;
    }
}


TofUSBChn::TofUSBChn()
{
}


TofUSBChn::~TofUSBChn()
{
    uninit();
}


DPtr<char> TofUSBChn::setDeviceByCmd(const char *EpcCmd)
{
    DPtr<char> pResult = nullptr;
    char buffer[128] = {0};
    snprintf(buffer, sizeof(buffer), "%s", EpcCmd);

    if (_usb_sys_win) {

        if (_isGetStream) { //if there is a stream requeset
            if (_height != _rows || _width != _cols) {
                _height = _rows;
                _width = _cols;
                _usb_sys_win->StopCaptureVideo();
                _usb_sys_win->StartCaptureVideo(EpcCmd, _cols, _rows, _dataSize);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        pResult = _usb_sys_win->Set_uvcex(buffer, _dataSize);
    }
    return pResult;
}
