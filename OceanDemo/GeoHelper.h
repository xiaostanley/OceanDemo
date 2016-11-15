// 2016-3-9 20:07:43
// Stanley Xiao
// Geo Data Processing Helper Functions

#ifndef _GEO_HELPER_H_
#define _GEO_HELPER_H_

#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <time.h>

// 二维点
struct GeoPoint 
{
	GeoPoint(void) {}
	GeoPoint(double tmpx, double tmpy)
		:x(tmpx), y(tmpy)
	{}

	double x;
	double y;
};

// 三维点
struct GeoPoint3D 
{
	GeoPoint3D(void) {}
	GeoPoint3D(float _x, float _y, float _z)
		:x(_x), y(_y), z(_z) {}

	float x;
	float y;
	float z;
};

class Helper
{
public:
	static void exportPlyModel(const char* path, float* points, int sizeOfPoints, int* faces, int sizeOfFaces)
	{
		std::ofstream outputFile(path);
		outputFile << "ply" << std::endl;
		outputFile << "format ascii 1.0" << std::endl;
		outputFile << "element vertex " << sizeOfPoints << std::endl;
		outputFile << "property float x" << std::endl;
		outputFile << "property float y" << std::endl;
		outputFile << "property float z" << std::endl;
		outputFile << "element face " << sizeOfFaces << std::endl;
		outputFile << "property list uchar int vertex_index" << std::endl;
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << std::setprecision(15) << points[3 * pIdx] << " "
				<< points[3 * pIdx + 1] << " "
				<< points[3 * pIdx + 2] << std::endl;
		}
		for (int fIdx = 0; fIdx < sizeOfFaces; fIdx++)
		{
			outputFile << "3 " << faces[3 * fIdx + 0]
				<< " " << faces[3 * fIdx + 1]
				<< " " << faces[3 * fIdx + 2] << std::endl;
		}
		outputFile.close();
	}

	static void exportPlyModel(const char* path, float* points, int sizeOfPoints)
	{
		std::ofstream outputFile(path);
		outputFile << "ply" << std::endl;
		outputFile << "format ascii 1.0" << std::endl;
		outputFile << "element vertex " << sizeOfPoints << std::endl;
		outputFile << "property float x" << std::endl;
		outputFile << "property float y" << std::endl;
		outputFile << "property float z" << std::endl;
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << std::setprecision(15) << points[3 * pIdx] << " "
				<< points[3 * pIdx + 1] << " "
				<< points[3 * pIdx + 2] << std::endl;
		}
		outputFile.close();
	}

	static void exportPlyModel(const char* path, GeoPoint3D* points, int sizeOfPoints, int* faces, int sizeOfFaces)
	{
		std::ofstream outputFile(path);
		outputFile << "ply" << std::endl;
		outputFile << "format ascii 1.0" << std::endl;
		outputFile << "element vertex " << sizeOfPoints << std::endl;
		outputFile << "property float x" << std::endl;
		outputFile << "property float y" << std::endl;
		outputFile << "property float z" << std::endl;
		outputFile << "element face " << sizeOfFaces << std::endl;
		outputFile << "property list uchar int vertex_index" << std::endl;
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << std::setprecision(15) << points[pIdx].x << " "
				<< points[pIdx].y << " "
				<< points[pIdx].z << std::endl;
		}
		for (int fIdx = 0; fIdx < sizeOfFaces; fIdx++)
		{
			outputFile << "3 " << faces[3 * fIdx + 0]
				<< " " << faces[3 * fIdx + 1]
				<< " " << faces[3 * fIdx + 2] << std::endl;
		}
		outputFile.close();
	}

	static void exportPlyModel(const char* path, GeoPoint3D* points, int sizeOfPoints)
	{
		std::ofstream outputFile(path);
		outputFile << "ply" << std::endl;
		outputFile << "format ascii 1.0" << std::endl;
		outputFile << "element vertex " << sizeOfPoints << std::endl;
		outputFile << "property float x" << std::endl;
		outputFile << "property float y" << std::endl;
		outputFile << "property float z" << std::endl;
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << points[pIdx].x << " "
				<< points[pIdx].y << " "
				<< points[pIdx].z << std::endl;
		}
		outputFile.close();
	}
};

#endif