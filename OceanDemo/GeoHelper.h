// 2016-3-9 20:07:43
// Stanley Xiao
// Geo Data Processing Helper Functions

#ifndef _GEO_HELPER_H_
#define _GEO_HELPER_H_

#include "Ogre.h"
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
		if (faces != NULL)
		{
			outputFile << "element face " << sizeOfFaces << std::endl;
			outputFile << "property list uchar int vertex_index" << std::endl;
		}
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << std::setprecision(15) << points[3 * pIdx] << " "
				<< points[3 * pIdx + 1] << " "
				<< points[3 * pIdx + 2] << std::endl;
		}
		if (faces != NULL)
		{
			for (int fIdx = 0; fIdx < sizeOfFaces; fIdx++)
			{
				outputFile << "3 " << faces[3 * fIdx + 0]
					<< " " << faces[3 * fIdx + 1]
					<< " " << faces[3 * fIdx + 2] << std::endl;
			}
		}
		outputFile.close();
	}

	static void exportPlyModel(const char* path, float* points, int sizeOfPoints, unsigned int* faces, int sizeOfFaces)
	{
		int* ifaces = new int[3 * sizeOfFaces];
		for (int i = 0; i < sizeOfFaces; i++)
		{
			int tidx = 3 * i;
			ifaces[tidx] = (int)faces[tidx];
			ifaces[tidx + 1] = (int)faces[tidx + 1];
			ifaces[tidx + 2] = (int)faces[tidx + 2];
		}

		exportPlyModel(path, points, sizeOfPoints, ifaces, sizeOfFaces);
		delete[] ifaces;
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
		if (faces != NULL)
		{
			outputFile << "element face " << sizeOfFaces << std::endl;
			outputFile << "property list uchar int vertex_index" << std::endl;
		}
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << std::setprecision(15) << points[pIdx].x << " "
				<< points[pIdx].y << " "
				<< points[pIdx].z << std::endl;
		}
		if (faces != NULL)
		{
			for (int fIdx = 0; fIdx < sizeOfFaces; fIdx++)
			{
				outputFile << "3 " << faces[3 * fIdx + 0]
					<< " " << faces[3 * fIdx + 1]
					<< " " << faces[3 * fIdx + 2] << std::endl;
			}
		}
		outputFile.close();
	}

	static void exportPlyModel(const char* path, Ogre::Vector3* points, int sizeOfPoints, int* faces, int sizeOfFaces)
	{
		std::ofstream outputFile(path);
		outputFile << "ply" << std::endl;
		outputFile << "format ascii 1.0" << std::endl;
		outputFile << "element vertex " << sizeOfPoints << std::endl;
		outputFile << "property float x" << std::endl;
		outputFile << "property float y" << std::endl;
		outputFile << "property float z" << std::endl;
		if (faces != NULL)
		{
			outputFile << "element face " << sizeOfFaces << std::endl;
			outputFile << "property list uchar int vertex_index" << std::endl;
		}
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << std::setprecision(15) << points[pIdx].x << " "
				<< points[pIdx].y << " "
				<< points[pIdx].z << std::endl;
		}
		if (faces != NULL)
		{
			for (int fIdx = 0; fIdx < sizeOfFaces; fIdx++)
			{
				outputFile << "3 " << faces[3 * fIdx + 0]
					<< " " << faces[3 * fIdx + 1]
					<< " " << faces[3 * fIdx + 2] << std::endl;
			}
		}
		outputFile.close();
	}

	static void exportPlyModel(const char* path, Ogre::Vector3* points, int sizeOfPoints, unsigned int* faces, int sizeOfFaces)
	{
		int* ifaces = new int[3 * sizeOfFaces];
		for (int i = 0; i < sizeOfFaces; i++)
		{
			int tidx = 3 * i;
			ifaces[tidx] = (int)faces[tidx];
			ifaces[tidx + 1] = (int)faces[tidx + 1];
			ifaces[tidx + 2] = (int)faces[tidx + 2];
		}

		exportPlyModel(path, points, sizeOfPoints, ifaces, sizeOfFaces);
		delete[] ifaces;
	}

	static void exportPlyModelWithEdges(const char* path, Ogre::Vector3* points, int sizeOfPoints, int* edges, int sizeOfEdges)
	{
		std::ofstream outputFile(path);
		outputFile << "ply" << std::endl;
		outputFile << "format ascii 1.0" << std::endl;
		outputFile << "element vertex " << sizeOfPoints << std::endl;
		outputFile << "property float x" << std::endl;
		outputFile << "property float y" << std::endl;
		outputFile << "property float z" << std::endl;
		outputFile << "element edge " << sizeOfEdges << std::endl;
		outputFile << "property int vertex1" << std::endl;
		outputFile << "property int vertex2" << std::endl;
		outputFile << "end_header" << std::endl;
		for (int pIdx = 0; pIdx < sizeOfPoints; pIdx++)
		{
			outputFile << std::setprecision(15) << points[pIdx].x << " "
				<< points[pIdx].y << " "
				<< points[pIdx].z << std::endl;
		}
		for (int fIdx = 0; fIdx < sizeOfEdges; fIdx++)
		{
			outputFile << edges[2 * fIdx + 0] << " " << edges[2 * fIdx + 1] << std::endl;
		}
		outputFile.close();
	}
};

#endif