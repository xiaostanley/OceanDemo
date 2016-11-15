#include "stdafx.h"
#include "TerranLiquid.h"

using namespace Ogre;

TerranLiquid::TerranLiquid()
{
	clList = new std::list<TerranLiquid::CoastLine>;
}

TerranLiquid::~TerranLiquid()
{
	for (auto iter = clVecs.begin(); iter != clVecs.end(); iter++)
		delete *iter;

	delete[] vertices;
	delete[] indices;
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
	_generateOceanGrid();
}

void TerranLiquid::_getCoastLines(void)
{
	entTerra = mSceneMgr->getEntity(nameTerra);
	scale = nodeTerra->getScale();

	// ����������������Ƭ
	// ����ÿ����Ƭ�뺣ƽ��Ľ���
	// ����ӵ�clList������

	_getMeshInfo(
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

	// ������ȡ������ĺ�����
	_getInitialCoastLines();

	// ����������Ϣ��ʹ֮����
	_collectCoastLines();
}

void TerranLiquid::_getInitialCoastLines(void)
{
	// ��ƽ�����
	Vector3 p0(10.f, heightSeaLevel, 10.f);
	Vector3 n = Vector3::UNIT_Y;
	float dp = p0.dotProduct(n);

	for (size_t i = 0; i < index_count; i += 3)
	{
		Vector3 p[3];
		p[0] = vertices[indices[i]];
		p[1] = vertices[indices[i + 1]];
		p[2] = vertices[indices[i + 2]];

		if (!((p[0].y < heightSeaLevel && p[1].y < heightSeaLevel && p[2].y < heightSeaLevel) ||
			(p[0].y > heightSeaLevel && p[1].y > heightSeaLevel && p[2].y > heightSeaLevel)))
		{
			// ���������� �����뺣ƽ�潻��
			// �洢��������
			std::vector<Vector3> pi;

			for (size_t j = 0; j < 3; j++)
			{
				const Vector3& tp0 = p[j];
				const Vector3& tp1 = p[(j + 1) % 3];

				Vector3 d(tp1 - tp0);
				if ((tp0.y - heightSeaLevel) * (tp1.y - heightSeaLevel) < 0.f)	// ���ڽ���
				{
					float t = (dp - tp0.dotProduct(n)) / d.dotProduct(n);
					pi.push_back(tp0 + t * d);
				}
			}

			clList->push_back(TerranLiquid::CoastLine(-1, -1, pi[0], pi[1]));
		}
	}

#if 0
	ManualObject* mobj = mSceneMgr->createManualObject("mobjline");
	mobj->begin("TestDemo", RenderOperation::OT_LINE_LIST);
	for (CoastLineList::iterator iter = clList->begin(); iter != clList->end(); iter++)
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
}

void TerranLiquid::_collectCoastLines(void)
{
	// ȷ��clList��ÿ�����������������
	for (CoastLineList::iterator iter = clList->begin(); iter != clList->end(); iter++)
	{
		const Ogre::Vector3& sv = iter->startVertex;
		const Ogre::Vector3& ev = iter->endVertex;

		auto viter = clPoints.begin();
		for (; viter != clPoints.end(); viter++)
		{
			if (_absValue(sv.x, viter->x) < eps && _absValue(sv.y, viter->y) < eps && _absValue(sv.z, viter->z) < eps)
			{
				iter->startIdx = std::distance(clPoints.begin(), viter);
				break;
			}
		}
		if (viter == clPoints.end())
		{
			iter->startIdx = clPoints.size();
			clPoints.push_back(sv);
		}

		viter = clPoints.begin();
		for (; viter != clPoints.end(); viter++)
		{
			if (_absValue(ev.x, viter->x) < eps && _absValue(ev.y, viter->y) < eps && _absValue(ev.z, viter->z) < eps)
			{
				iter->endIdx = std::distance(clPoints.begin(), viter);
				break;
			}
		}
		if (viter == clPoints.end())
		{
			iter->endIdx = clPoints.size();
			clPoints.push_back(ev);
		}
	}

	// ����clList��ÿ��Ԫ���ڲ����˵�˳��
	for (auto iter = clList->begin(); iter != clList->end(); iter++)
	{
		if (iter->endIdx < iter->startIdx)
		{
			std::swap(iter->endIdx, iter->startIdx);
			iter->startVertex.swap(iter->endVertex);
		}
	}

	while (clList->size() != 0)
	{
		// �����ձ� ��clList����Ԫ�ؼ���ձ�����clList��ɾ��
		CoastLineList* tclList = new CoastLineList;
		tclList->push_back(clList->front());
		clList->erase(clList->begin());

		// ����clList��ʣ���Ԫ�أ�������tclList��ͷ����β�����
		// �����clList��ɾ��������tclListͷ����β������
		while (true)
		{
			bool isExisted = false;	// clList���Ƿ����ܼ��뵽tclList�е��߶Σ�
			for (auto iter = clList->begin(); iter != clList->end();)
			{
				const TerranLiquid::CoastLine& fr = tclList->front();
				const TerranLiquid::CoastLine& bk = tclList->back();

				if (iter->startIdx == fr.startIdx || iter->startIdx == fr.endIdx || iter->endIdx == fr.startIdx || iter->endIdx == fr.endIdx)
				{
					isExisted = true;
					tclList->push_front(*iter);
					iter = clList->erase(iter);
				}
				else if (iter->startIdx == bk.startIdx || iter->startIdx == bk.endIdx || iter->endIdx == bk.startIdx || iter->endIdx == bk.endIdx)
				{
					isExisted = true;
					tclList->push_back(*iter);
					iter = clList->erase(iter);
				}
				else
					iter++;
			}

			// clList���Ѳ������ܼ��뵽tclList�е��߶� �˳�ѭ��
			if (!isExisted) 
				break;
		}

		clVecs.push_back(tclList);
	}
	delete clList;

#if 1
	for (size_t i = 0; i < clVecs.size(); i++)
	{
		ManualObject* mobj = mSceneMgr->createManualObject("mobjline" + Ogre::StringConverter::toString(i));
		mobj->begin("TestDemo", RenderOperation::OT_LINE_LIST);
		for (CoastLineList::iterator iter = clVecs[i]->begin(); iter != clVecs[i]->end(); iter++)
		{
			mobj->position(iter->startVertex);
			if (i % 3 == 0)
				mobj->colour(Ogre::ColourValue::White);
			else if (i % 3 == 1)
				mobj->colour(Ogre::ColourValue::Red);
			else
				mobj->colour(Ogre::ColourValue::Green);
			mobj->position(iter->endVertex);
			if (i % 3 == 0)
				mobj->colour(Ogre::ColourValue::White);
			else if (i % 3 == 1)
				mobj->colour(Ogre::ColourValue::Red);
			else
				mobj->colour(Ogre::ColourValue::Green);
		}
		mobj->end();
		SceneNode* nodeObj = mSceneMgr->createSceneNode("nodeObj" + Ogre::StringConverter::toString(i));
		nodeObj->attachObject(mobj);
		mSceneMgr->getRootSceneNode()->addChild(nodeObj);
	}
#endif
}

void TerranLiquid::_getMeshInfo(
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

	// 2016-11-10 10:56:05 submesh����
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

	// 2016-11-15 12:56:09 ��ֵ
	minx = 1000000000.f;
	minz = minx;
	maxx = 0.f;
	maxz = maxx;

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

				Ogre::Vector3 tvex = ((orient * pt) + position) * scale;	// �˴�orient position scale��˳����ע��
				minx = std::min<float>(minx, tvex.x);
				minz = std::min<float>(minz, tvex.z);
				maxx = std::max<float>(maxx, tvex.x);
				maxz = std::max<float>(maxz, tvex.z);

				vertices[current_offset + j] = tvex;
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

void TerranLiquid::setHeight(float heightSeaLevel)
{
	this->heightSeaLevel = heightSeaLevel;
}

void TerranLiquid::_generateOceanGrid(void)
{
	// ������ɢ�㼯 ��Ϊ��������㼯
	
	// �ĸ��ǵ�
	clPoints.push_back(Ogre::Vector3(minx, heightSeaLevel, minz));
	clPoints.push_back(Ogre::Vector3(maxx, heightSeaLevel, minz));
	clPoints.push_back(Ogre::Vector3(minx, heightSeaLevel, maxz));
	clPoints.push_back(Ogre::Vector3(maxx, heightSeaLevel, maxz));

	// �߽��
	for (float step = minx + density; step < maxx; step += density)
	{
		clPoints.push_back(Ogre::Vector3(step, heightSeaLevel, minz));
		clPoints.push_back(Ogre::Vector3(step, heightSeaLevel, maxz));
	}
	for (float step = minz + density; step < maxz; step += density)
	{
		clPoints.push_back(Ogre::Vector3(minx, heightSeaLevel, step));
		clPoints.push_back(Ogre::Vector3(maxx, heightSeaLevel, step));
	}

	// �ڲ���
	for (float xstep = minx + density; xstep < maxx; xstep += density)
	{
		for (float ystep = minz + density; ystep < maxz; ystep += density)
		{
			clPoints.push_back(Ogre::Vector3(xstep, heightSeaLevel, ystep));
		}
	}

	GeoPoint3D* verticesPtr = new GeoPoint3D[clPoints.size()];
	for (size_t i = 0; i < clPoints.size(); i++)
		verticesPtr[i] = GeoPoint3D(clPoints[i].x, clPoints[i].z, clPoints[i].y);
	
	// Լ����
	int countConstraint = 0;
	for (size_t i = 0; i < clVecs.size(); i++)
		countConstraint += clVecs[i]->size();
	int* constraints = new int[2 * countConstraint];
	for (size_t i = 0, j = 0; i < clVecs.size(); i++)
	{
		const auto plist = clVecs[i];
		for (auto iter = plist->begin(); iter != plist->end(); iter++)
		{
			constraints[j++] = iter->startIdx;
			constraints[j++] = iter->endIdx;
		}
	}

	int* faces = NULL;
	int countFaces = 0;

	// ��������������
	std::unique_ptr<GPUDT> gpuDT(new GPUDT);
	gpuDT->setInputPoints(verticesPtr, clPoints.size());
	gpuDT->setInputConstraints(constraints, countConstraint);
	gpuDT->computeDT(&faces, countFaces);

#if 1
	const char* path = "D:/Ganymede/DynamicOcean/OceanDemo/grid.ply";
	Helper::exportPlyModel(path, verticesPtr, clPoints.size(), faces, countFaces);
#endif

	// ����faces��������Ƭ
	// ����ÿ����Ƭ���ĵ��ڵ���mesh�еĶ�Ӧ�߶�
	// �жϵ�ǰ��Ƭ�Ƿ�Ӧɾ��

	std::vector<bool> isNotOceanMesh(countFaces, false);

	for (size_t i = 0; i < 3 * countFaces; i += 3)
	{
		const auto& p0 = clPoints[faces[i]];
		const auto& p1 = clPoints[faces[i + 1]];
		const auto& p2 = clPoints[faces[i + 2]];

		Ogre::Vector3 cp = (p0 + p1 + p2) / 3.f;

		Ogre::Ray ray(cp + Ogre::Vector3(0.f, 1000000000.f, 0.f), Ogre::Vector3::NEGATIVE_UNIT_Y);
		for (size_t j = 0; j < index_count; j += 3)
		{
			Ogre::Plane pl(vertices[indices[j]], vertices[indices[j + 1]], vertices[indices[j + 2]]);
			auto rs = ray.intersects(pl);
			if (rs.first)
			{
				if (ray.getOrigin().y - rs.second > heightSeaLevel)
				{
					isNotOceanMesh[i / 3] = true;
				}
				break;
			}
		}
	}

	// ɾ����Ч��Ƭ

	std::vector<int> newIndices;
	for (size_t i = 0; i < countFaces; i++)
	{
		if (isNotOceanMesh[i])
		{
			newIndices.push_back(faces[3 * i]);
			newIndices.push_back(faces[3 * i + 1]);
			newIndices.push_back(faces[3 * i + 2]);
		}
	}

#if 1
	const char* path1 = "D:/Ganymede/DynamicOcean/OceanDemo/grid2.ply";
	Helper::exportPlyModel(path1, verticesPtr, clPoints.size(), &newIndices[0], newIndices.size() / 3);
#endif

	gpuDT->releaseMemory();

	delete[] verticesPtr;
	delete[] constraints;
	delete[] faces;
}

void TerranLiquid::setGridDensity(float density)
{
	this->density = density;
}
