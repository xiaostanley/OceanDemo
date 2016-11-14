#include "stdafx.h"
#include "TerranLiquid.h"

using namespace Ogre;

TerranLiquid::TerranLiquid()
{
	clList = new std::list<TerranLiquid::CoastLine>;
}

TerranLiquid::~TerranLiquid()
{
	delete clList;
}

void TerranLiquid::setInputParas(Ogre::Root * mRoot, Ogre::SceneManager * mSceneMgr, Ogre::SceneNode * nodeTerra, const Ogre::String & nameTerra, const Ogre::Vector3 & transPos)
{
	this->mRoot = mRoot;
	this->mSceneMgr = mSceneMgr;
	this->nodeTerra = nodeTerra;
	this->nameTerra = nameTerra;
	this->transPos = transPos;
}

void TerranLiquid::initialize(void)
{
	_getCoastLines();
}

void TerranLiquid::_getCoastLines(void)
{
	entTerra = mSceneMgr->getEntity(nameTerra);
	scale = nodeTerra->getScale();

	// 遍历所有三角形面片
	// 查找每个面片与海平面的交线
	// 并添加到clList链表中

	size_t vertex_count, index_count;
	Vector3* vertices;
	unsigned int* indices;

	__getMeshInfo(
		entTerra->getMesh(), 
		vertex_count, 
		vertices, 
		index_count, 
		indices, 
		transPos, 
		Quaternion::IDENTITY, 
		scale
	);

	//LogManager::getSingleton().logMessage(LML_NORMAL, "Vertices in mesh: %u", vertex_count);
	//LogManager::getSingleton().logMessage(LML_NORMAL, "Triangles in mesh: %u", index_count / 3);

	// 海平面参数
	Vector3 p0(10.f, height, 10.f);
	Vector3 n = Vector3::UNIT_Y;
	float dp = p0.dotProduct(n);

	for (size_t i = 0; i < index_count; i += 3)
	{
		Vector3 p[3];
		p[0] = vertices[indices[i]];
		p[1] = vertices[indices[i + 1]];
		p[2] = vertices[indices[i + 2]];

		if (!((p[0].y < height && p[1].y < height && p[2].y < height) ||
			  (p[0].y > height && p[1].y > height && p[2].y > height)))
		{
			// 遍历三条边 查找与海平面交点
			// 存储两个交点
			std::vector<Vector3> pi;

			for (size_t j = 0; j < 3; j++)
			{
				const Vector3& tp0 = p[j];
				const Vector3& tp1 = p[(j + 1) % 3];

				Vector3 d(tp1 - tp0);
				if ((tp0.y - height) * (tp1.y - height) < 0.f)	// 存在交点
				{
					float t = (dp - tp0.dotProduct(n)) / d.dotProduct(n);
					pi.push_back(tp0 + t * d);
				}
			}

			clList->push_back(TerranLiquid::CoastLine(-1, -1, pi[0], pi[1]));
		}
	}

#if 1
	ManualObject* mobj = mSceneMgr->createManualObject("mobjline");
	mobj->begin("TestDemo", RenderOperation::OT_LINE_LIST);
	for (std::list<TerranLiquid::CoastLine>::iterator iter = clList->begin(); iter != clList->end(); iter++)
	{
		mobj->position(iter->startVertex);
		mobj->colour(Ogre::ColourValue::White);
		mobj->position(iter->endVertex);
		mobj->colour(Ogre::ColourValue::White);
	}
	mobj->end();
	SceneNode* nodeObj = mSceneMgr->createSceneNode("nodeObj");
	nodeObj->attachObject(mobj);
	mSceneMgr->getRootSceneNode()->addChild(nodeObj);
#endif

	delete[] vertices;
	delete[] indices;
}

void TerranLiquid::__getMeshInfo(
	const Ogre::MeshPtr mesh, 
	size_t & vertex_count, 
	Ogre::Vector3 *& vertices, 
	size_t & index_count, 
	unsigned int *& indices, 
	const Ogre::Vector3 & position, 
	const Ogre::Quaternion & orient, 
	const Ogre::Vector3 & scale
)
{
	bool added_shared = false;
	size_t current_offset = 0;
	size_t shared_offset = 0;
	size_t next_offset = 0;
	size_t index_offset = 0;

	vertex_count = index_count = 0;

	// 2016-11-10 10:56:05 submesh数量
	int countOfSubmeshes = (mesh->getNumSubMeshes() > 1) ? 5 : 1;

	// Calculate how many vertices and indices we're going to need
	for (unsigned short i = 0; i < countOfSubmeshes; ++i)
	{
		Ogre::SubMesh* submesh = mesh->getSubMesh(i);
		// We only need to add the shared vertices once
		if (submesh->useSharedVertices)
		{
			if (!added_shared)
			{
				vertex_count += mesh->sharedVertexData->vertexCount;
				added_shared = true;
			}
		}
		else
		{
			vertex_count += submesh->vertexData->vertexCount;
		}
		// Add the indices
		index_count += submesh->indexData->indexCount;
	}

	// Allocate space for the vertices and indices
	vertices = new Ogre::Vector3[vertex_count];
	indices = new unsigned int[index_count];

	added_shared = false;

	// Run through the submeshes again, adding the data into the arrays
	for (unsigned short i = 0; i < countOfSubmeshes/*mesh->getNumSubMeshes()*/; ++i)
	{
		Ogre::SubMesh* submesh = mesh->getSubMesh(i);

		Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

		if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
		{
			if (submesh->useSharedVertices)
			{
				added_shared = true;
				shared_offset = current_offset;
			}

			const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
			Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
			unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

			// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
			//  as second argument. So make it float, to avoid trouble when Ogre::Real will
			//  be comiled/typedefed as double:
			//Ogre::Real* pReal;
			float* pReal;
			for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
			{
				posElem->baseVertexPointerToElement(vertex, &pReal);
				Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
				vertices[current_offset + j] = ((orient * pt) + position) * scale;
			}

			vbuf->unlock();
			next_offset += vertex_data->vertexCount;
		}

		Ogre::IndexData* index_data = submesh->indexData;
		size_t numTris = index_data->indexCount / 3;
		Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

		bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);
		unsigned long* pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
		unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

		size_t offset = (submesh->useSharedVertices) ? shared_offset : current_offset;
		if (use32bitindexes)
		{
			for (size_t k = 0; k < numTris * 3; ++k)
			{
				indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
			}
		}
		else
		{
			for (size_t k = 0; k < numTris * 3; ++k)
			{
				indices[index_offset++] = static_cast<unsigned long>(pShort[k]) + static_cast<unsigned long>(offset);
			}
		}

		ibuf->unlock();
		current_offset = next_offset;
	}

#if 0
	// debug
	ManualObject* mobj = mSceneMgr->createManualObject("mobj");
	mobj->begin("TestDemo", RenderOperation::OT_TRIANGLE_LIST);
	for (size_t i = 0; i < vertex_count; i++)
		mobj->position(vertices[i]);
	for (size_t i = 0; i < index_count; i += 3)
		mobj->triangle(indices[i], indices[i + 1], indices[i + 2]);
	mobj->end();
	SceneNode* nodeMobj = mSceneMgr->createSceneNode("nodeMobj");
	nodeMobj->attachObject(mobj);
	nodeMobj->showBoundingBox(true);
	mSceneMgr->getRootSceneNode()->addChild(nodeMobj);
#endif
}

void TerranLiquid::setHeight(float height)
{
	this->height = height;
}
