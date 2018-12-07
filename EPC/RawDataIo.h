#ifndef RAWDATAIO_H_
#define RAWDATAIO_H_

#include "DPtr.h"
#include "d3type.h"
#include <memory>
#include <functional>
#include <string>

using namespace std;
using namespace GLPlot3D;

class RawDataIo
{
public:
    DPtr<GrayDepthField> ply2RawData(string plyFileName, int &width, int &height, int &key_point);
    ostringstream  rawData2Ply(string plyFileName, int width, int height, DPtr<GrayDepthField> inDataPtr,  int key_point);
    ostringstream getplyDataPtr(DPtr<GrayDepthField> inDataPtr, int width, int height,  int key_point);
    ~RawDataIo(){}
};

#endif // RAWDATAIO_H
