// 2016-11-22 22:33:56
// Ogre Mesh生成

#ifndef _MESH_GENERATOR_H_
#define _MESH_GENERATOR_H_

#include "Ogre.h"

class MeshGenerator
{
public:

	// 生成Ogre Mesh
	static void generateMesh(
		float* vertices,				// 顶点三维坐标
		float* normals,					// 顶点法向量（可为NULL）
		float* textures,				// 顶点纹理坐标（可为NULL）
		Ogre::RGBA* colors,				// 顶点颜色值（可为NULL）
		int numVertices,				// 顶点数量
		unsigned int* indices,			// 三角形面片索引
		int numFaces,					// 三角形面片数量
		const Ogre::String& nameMatl,	// 材质名称
		const Ogre::String& filePath,	// 存储路径
		const Ogre::String& nameMesh	// Mesh名称
	)
	{
		Ogre::MeshPtr meshPtr = Ogre::MeshManager::getSingleton().createManual(nameMesh, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		Ogre::SubMesh* submeshPtr = meshPtr->createSubMesh();
		meshPtr->sharedVertexData = new Ogre::VertexData();
		meshPtr->sharedVertexData->vertexCount = numVertices;

		Ogre::VertexDeclaration*   vdecl = meshPtr->sharedVertexData->vertexDeclaration;
		Ogre::VertexBufferBinding* vbind = meshPtr->sharedVertexData->vertexBufferBinding;
		size_t offset = 0;
		unsigned short bindCounter = 0;

		// 点坐标数据
		vdecl->addElement(bindCounter, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		Ogre::HardwareVertexBufferSharedPtr posVertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
			offset, numVertices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		posVertexBuffer->writeData(0, posVertexBuffer->getSizeInBytes(), vertices, true);
		vbind->setBinding(bindCounter++, posVertexBuffer);

		// 顶点法向量数据
		if (normals != NULL)
		{
			offset = 0;
			vdecl->addElement(bindCounter, offset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
			offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
			posVertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
				offset, numVertices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			posVertexBuffer->writeData(0, posVertexBuffer->getSizeInBytes(), normals, true);
			vbind->setBinding(bindCounter++, posVertexBuffer);
		}

		// 纹理坐标数据
		if (textures != NULL)
		{
			offset = 0;
			vdecl->addElement(bindCounter, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);
			offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
			posVertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
				offset, numVertices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			posVertexBuffer->writeData(0, posVertexBuffer->getSizeInBytes(), textures, true);
			vbind->setBinding(bindCounter++, posVertexBuffer);
		}

		// 顶点颜色数据
		if (colors != NULL)
		{
			offset = 0;
			vdecl->addElement(bindCounter, offset, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
			offset += Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR);
			posVertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
				offset, numVertices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
			posVertexBuffer->writeData(0, posVertexBuffer->getSizeInBytes(), colors, true);
			vbind->setBinding(bindCounter++, posVertexBuffer);
		}

		// 三角形面片索引数据
		Ogre::HardwareIndexBufferSharedPtr iBuf = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
			Ogre::HardwareIndexBuffer::IT_32BIT, 3 * numFaces, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, true);
		iBuf->writeData(0, iBuf->getSizeInBytes(), indices, true);

		submeshPtr->useSharedVertices = true;
		submeshPtr->indexData->indexBuffer = iBuf;
		submeshPtr->indexData->indexStart = 0;
		submeshPtr->indexData->indexCount = 3 * numFaces;

		submeshPtr->setMaterialName(nameMatl);

		// 包围盒
		float minx = vertices[0];
		float miny = vertices[1];
		float minz = vertices[2];
		float maxx = minx;
		float maxy = miny;
		float maxz = minz;
		for (size_t i = 0; i < (size_t)numVertices; i += 3)
		{
			minx = std::min(minx, vertices[i]);
			miny = std::min(miny, vertices[i + 1]);
			minz = std::min(minz, vertices[i + 2]);
			maxx = std::max(maxx, vertices[i]);
			maxy = std::max(maxy, vertices[i + 1]);
			maxz = std::max(maxz, vertices[i + 2]);
		}

		meshPtr->_setBounds(Ogre::AxisAlignedBox(minx, miny, minz, maxx, maxy, maxz));
		
		meshPtr->load();
		meshPtr->touch();

		// 显示调试信息
		Ogre::StringStream ss; 
		ss << "*** a mesh is generated ***";
		Ogre::LogManager::getSingleton().logMessage(ss.str());

		// 导出mesh
		Ogre::MeshSerializer meshSer;
		meshSer.exportMesh(meshPtr.getPointer(), filePath + nameMesh + ".mesh");
		Ogre::MeshManager::getSingleton().unload(meshPtr->getHandle());
		Ogre::MeshManager::getSingleton().remove(meshPtr->getHandle());
	}
};

#endif
