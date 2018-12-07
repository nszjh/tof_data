#include "RawDataIo.h"
#include "DPtr.h"
#include "memory.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace GLPlot3D;

//Ply format data De serialization.
DPtr<GrayDepthField> RawDataIo::ply2RawData(string plyFileName, int &width, int &height, int &key_point)
{
	char lineStr[64] = { 0 };
    int str2Int = 0;
    float str2Float = 0.000000;
    DataGrayDepth _dataGrayDepthOut;
    DPtr<GrayDepthField> _grayDepthFieldOut = DPtr<GrayDepthField>(new GrayDepthField);
    const char *p = plyFileName.data();
    FILE *fp2 = NULL;
    fp2 = fopen(p, "r");
    if (fp2 == NULL)
    {
        fprintf(stderr, "Cannot open plyFile.\n");
		return nullptr;
    }

	int w = 320, h = 240;
	int point = 1;
	int obj;
	int isHeader = 1;
	int w1 = 0;
	float w2 = 0, w3 = 0, w4 = 0;

    while (fgets(lineStr, sizeof(lineStr), fp2) != NULL)
    {
		if (strlen(lineStr))
		{
			if (isHeader && sscanf(lineStr, "obj_info num_cols %d", &obj) == 1)
				w = obj;
			else if (isHeader && sscanf(lineStr, "obj_info num_rows %d", &obj) == 1)
			{
				h = obj;
				isHeader = 0;
			}
			else if (isHeader && sscanf(lineStr, "obj_info key_point %d", &obj) == 1)
			{
				point = obj;
			}
			else
			{
				if (4 == sscanf(lineStr, "%d %f %f %f", &w1, &w2, &w3, &w4))
				{
					_dataGrayDepthOut.g = w1;
					_dataGrayDepthOut.x = w2;
					_dataGrayDepthOut.y = w3;
					_dataGrayDepthOut.z = w4;
					_grayDepthFieldOut->push_back(_dataGrayDepthOut);
				}
				else
				{
					_dataGrayDepthOut.g = 0;
					_dataGrayDepthOut.x = 0;
					_dataGrayDepthOut.y = 0;
					_dataGrayDepthOut.z = 0;
				}
			}
		}
		memset(lineStr, 0, sizeof(lineStr));
    }
    fclose(fp2);

	width = w;
	height = h;
	key_point = point;
    return _grayDepthFieldOut;
}

//Serialize the original data into ply format.
ostringstream RawDataIo::rawData2Ply(string plyFileName, int width, int height, DPtr<GrayDepthField> inDataPtr,  int key_point)
{
    ostringstream buffer;
    const char *p = plyFileName.data();
    FILE *fp = NULL;
    fp = fopen(p, "w+");
    if (fp == NULL)
    {
        fprintf(stderr, "Cannot open plyFile.\n");
    }
    buffer = getplyDataPtr(inDataPtr, width, height, key_point);
    string s = buffer.str();
    char *buf = (char *)s.c_str();
    fprintf(fp, buf);
    fclose(fp);
    return buffer;
}

//Get the ostringstream of ply data.
ostringstream RawDataIo::getplyDataPtr(DPtr<GrayDepthField> inDataPtr, int width, int height, int key_point)
{
    ostringstream buffer;
    buffer << "ply" << endl;
    buffer << "format ascii 1.0" << endl;
    buffer << "comment PCL generated" << endl;
    buffer << "obj_info is_cyberware_data 0" << endl;
    buffer << "obj_info is_mesh 0" << endl;
    buffer << "obj_info is_warped 0" << endl;
    buffer << "obj_info is_interlaced 0" << endl;
    buffer << "obj_info num_cols " << width << endl;
    buffer << "obj_info num_rows " << height << endl;
    buffer << "obj_info key_point " << key_point << endl;
    buffer << "obj_info echo_rgb_offset_x 0" << endl;
    buffer << "obj_info echo_rgb_offset_y 0" << endl;
    buffer << "obj_info echo_rgb_offset_z 0" << endl;
    buffer << "obj_info echo_rgb_frontfocus 0.0" << endl;
    buffer << "obj_info echo_rgb_backfocus 0.0" << endl;
    buffer << "obj_info echo_rgb_pixelsize 0.0" << endl;
    buffer << "obj_info echo_rgb_centerpixel 0" << endl;
    buffer << "obj_info echo_frames 1" << endl;
    buffer << "obj_info echo_lgincr 0.0" << endl;
    buffer << "element vertex " << inDataPtr->size() << endl;
    buffer << "property unsigned int gray" << endl;
    buffer << "property float x" << endl;
    buffer << "property float y" << endl;
    buffer << "property float z" << endl;
    buffer << "end_header" << endl;
    for (int j = 0; j < inDataPtr->size(); j++)
    {
        buffer << (*inDataPtr)[j].g << " ";
        buffer << (*inDataPtr)[j].x << " ";
        buffer << (*inDataPtr)[j].y << " ";
        buffer << (*inDataPtr)[j].z << endl;
    }
    return buffer;
}
