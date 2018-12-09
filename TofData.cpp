#include "TofData.h"
#include <iostream>


DPtr<DataPacket> TofData::setDevice(const char* com)
{
    _isGetStream = 0;
    DPtr<DataPacket> pData = nullptr;
    if (!strcmp(com, "getDistanceAndAmplitudeSorted"))
    {
        _rows = 480;
        _cols = 320;
        _channel = 2;
        _dataSize = 320 * 480 * 2;
        _isGetStream = 1;

        pData = DPtr<DataPacket>(new DataPacket);
    }
    else if (!strcmp(com, "getDistanceSorted"))
    {
        _rows = 240;
        _cols = 320;
        _channel = 2;
        _dataSize = 320 * 240 * 2;
        _isGetStream = 1;
    }//Todo: add other 
    else 
    {
        _rows = 1;
        _cols = 1;
        _channel = 1;
        _dataSize = 0;
    }

    DPtr<char> stream = setDeviceByCmd(com);
    if (_dataSize) {
        pData = DPtr<DataPacket>(new DataPacket);
        pData->rows = _rows;
        pData->cols = _cols;
        pData->channel = _channel;
        pData->data = stream;
    }
    return pData;
}