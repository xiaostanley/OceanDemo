#include "stdafx.h"
#include "TerranLiquid.h"

using namespace Ogre;

TerranLiquid::TerranLiquid()
	:mRoot(NULL),
	mSceneMgr(NULL),
	nodeTerra(NULL),
	entTerra(NULL),
	nameTerra(""),
	transPos(Ogre::Vector3::ZERO),
	scale(Ogre::Vector3::ZERO),
	heightSeaLevel(0.f),
	vertex_count(0),
	index_count(0),
	vertices(NULL),
	indices(NULL),
	depths(NULL),
	texScale(1.f),
	depthScale(1.f),
	density(10.f),
	minx(1000000000.f),
	minz(1000000000.f),
	maxx(0.f),
	maxz(0.f)//,
	//pVertex(NULL),
	//pIndex(NULL)
{
	clList = new std::list<TerranLiquid::CoastLine>;
}

TerranLiquid::~TerranLiquid()
{
	for (auto iter = clVecs.begin(); iter != clVecs.end(); iter++)
		delete *iter;

	delete[] vertices;
	delete[] indices;
	if (depths != NULL)
		delete[] depths;
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
	entTerra = mSceneMgr->getEntity(nameTerra);
	scale = nodeTerra->getScale();

	// 遍历所有三角形面片
	// 查找每个面片与海平面的交线
	// 并添加到clList链表中

	_getMeshInfo(entTerra->getMesh(), vertex_count, vertices,
		index_count, indices, transPos, Quaternion::IDENTITY, scale);

#if 0
	const char* path = "D:/Ganymede/DynamicOcean/OceanDemo/base_terrain_mesh.ply";
	//Helper::exportPlyModel(path, verticesPtr, clPoints.size(), faces, countFaces);
	Helper::exportPlyModel(path, vertices, vertex_count, indices, index_count / 3);
#endif

	// 初步提取出无序的海岸线
	_getInitialCoastLines();

	// 整理海岸线信息（使之有序）
	_collectCoastLines();

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_ 
	// 提取过渡区域边界
	_getTransitionBoundary();
#endif
#endif

	// 生成海面网格
	_generateOceanGrid();

	// 获取深度数据（必须在生成OceanGrid之后才能进行）
	// （只能在剔除无效点面后进行）
	//_getDepthData();

//	_createVertexData();
//	_createIndexData();

#if 0
	ManualObject* mobj = mSceneMgr->createManualObject("mobjoceangrid");
	mobj->begin("OceanShallow", RenderOperation::OT_TRIANGLE_LIST);
	float minx = vertices[0].x, minz = vertices[0].z;
	float maxx = vertices[0].x, maxz = vertices[0].z;
	for (size_t i = 0; i < vertex_count; i++)
	{
		minx = std::min(minx, vertices[i].x); maxx = std::max(maxx, vertices[i].x);
		minz = std::min(minz, vertices[i].z); maxz = std::max(maxz, vertices[i].z);
	}
	for (size_t i = 0; i < vertex_count; i++)
	{
		mobj->position(vertices[i]);
		mobj->textureCoord(Ogre::Vector2((vertices[i].x - minx) / (maxx - minx), (vertices[i].z - minz) / (maxz - minz)));
	}
	for (size_t i = 0; i < index_count; i += 3)
		mobj->triangle(indices[i], indices[i + 2], indices[i + 1]);
	mobj->end();
	SceneNode* nodeMobj = mSceneMgr->createSceneNode("nodeMobjoceangrid");
	nodeMobj->attachObject(mobj);
	nodeMobj->showBoundingBox(true);
	nodeMobj->translate(Vector3(0.f, -2 * 3.8f, 0.f));
	mSceneMgr->getRootSceneNode()->addChild(nodeMobj);
#endif
}

void TerranLiquid::_getInitialCoastLines(void)
{
	// 海平面参数
	Vector3 p0(10.f, heightSeaLevel, 10.f);
	Vector3 n = Vector3::UNIT_Y;
	float dp = p0.dotProduct(n);

#ifdef _SHALLOW_OCEAN_STRIP_
	p0.y = depthShallowOcean;
	float dpsw = p0.dotProduct(n);
#endif // _SHALLOW_OCEAN_STRIP_


	for (size_t i = 0; i < index_count; i += 3)
	{
		Vector3 p[3];
		p[0] = vertices[indices[i]];
		p[1] = vertices[indices[i + 1]];
		p[2] = vertices[indices[i + 2]];

		if (_isIntersected(p, heightSeaLevel))
		{
			// 遍历三条边 查找与海平面交点
			// 存储两个交点
			std::vector<Vector3> pi;
			for (size_t j = 0; j < 3; j++)
			{
				const Vector3& tp0 = p[j];  const Vector3& tp1 = p[(j + 1) % 3];
				Vector3 d(tp1 - tp0);
				if ((tp0.y - heightSeaLevel) * (tp1.y - heightSeaLevel) < 0.f)	// 存在交点
					pi.push_back(tp0 + (dp - tp0.dotProduct(n)) / d.dotProduct(n) * d);
			}
			clList->push_back(TerranLiquid::CoastLine(-1, -1, pi[0], pi[1]));

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_
			isInSwBoundaries.push_back(false);
#endif
#endif
		}

#ifdef _SHALLOW_OCEAN_STRIP_
		// 浅海水深
		if (_isIntersected(p, depthShallowOcean))
		{
			// 遍历三条边 查找与海平面交点
			// 存储两个交点
			std::vector<Vector3> pi;
			for (size_t j = 0; j < 3; j++)
			{
				const Vector3& tp0 = p[j];  const Vector3& tp1 = p[(j + 1) % 3];
				Vector3 d(tp1 - tp0);
				if ((tp0.y - depthShallowOcean) * (tp1.y - depthShallowOcean) < 0.f)	// 存在交点
					pi.push_back(tp0 + (dpsw - tp0.dotProduct(n)) / d.dotProduct(n) * d);
			}
			pi[0].y = pi[1].y = heightSeaLevel;
			clList->push_back(TerranLiquid::CoastLine(-1, -1, pi[0], pi[1]));

#ifdef _TRANSITION_OCEAN_STRIP_
			isInSwBoundaries.push_back(true);
#endif
		}
#endif
	}

#if 0
	ManualObject* mobj = mSceneMgr->createManualObject("mobjline");
	mobj->begin("TestDemo", RenderOperation::OT_LINE_LIST);
	for (auto iter = clList->begin(); iter != clList->end(); iter++)
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
	// 确定clList中每条边两个顶点的索引(startIdx和endIdx)
	for (auto iter = clList->begin(); iter != clList->end(); iter++)
	{
		const Ogre::Vector3& sv = iter->startVertex;
		const Ogre::Vector3& ev = iter->endVertex;

		auto viter = clPoints.begin();
		for (; viter != clPoints.end(); viter++)
		{
			if (viter->positionEquals(sv))
			{
				iter->startIdx = std::distance(clPoints.begin(), viter);
				break;
			}
		}
		if (viter == clPoints.end())	// 当前clPoints中没有sv点
		{
			iter->startIdx = clPoints.size();
			clPoints.push_back(sv);
		}

		viter = clPoints.begin();
		for (; viter != clPoints.end(); viter++)
		{
			if (viter->positionEquals(ev))
			{
				iter->endIdx = std::distance(clPoints.begin(), viter);
				break;
			}
		}
		if (viter == clPoints.end())	// 当前clPoints中没有ev点
		{
			iter->endIdx = clPoints.size();
			clPoints.push_back(ev);
		}
	}

	// 调整clList中每个元素内部两端点顺序
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
		// 建立空表 将clList中首元素加入空表，并在clList中删除
		CoastLineList* tclList = new CoastLineList;
		tclList->push_back(clList->front());
		clList->erase(clList->begin());

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_
		bool isSwBoundaries = false;
		if (isInSwBoundaries.front())
			isSwBoundaries = true;
		isInSwBoundaries.erase(isInSwBoundaries.begin());
#endif // _TRANSITION_OCEAN_STRIP_
#endif // _SHALLOW_OCEAN_STRIP_

		// 遍历clList中剩余的元素，若能与tclList的头部或尾部相接
		// 则将其从clList中删除，并在tclList头部或尾部插入
		while (true)
		{
			bool isExisted = false;	// clList中是否还有能加入到tclList中的线段？

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_
			auto isSwBdIter = isInSwBoundaries.begin();
#endif
#endif
			for (auto iter = clList->begin(); iter != clList->end();)
			{
				const TerranLiquid::CoastLine& fr = tclList->front();
				const TerranLiquid::CoastLine& bk = tclList->back();

				if (iter->startIdx == fr.startIdx || iter->startIdx == fr.endIdx || iter->endIdx == fr.startIdx || iter->endIdx == fr.endIdx)
				{
					isExisted = true;
					tclList->push_front(*iter);
					iter = clList->erase(iter);
#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_
					isSwBdIter = isInSwBoundaries.erase(isSwBdIter);
#endif
#endif
				}
				else if (iter->startIdx == bk.startIdx || iter->startIdx == bk.endIdx || iter->endIdx == bk.startIdx || iter->endIdx == bk.endIdx)
				{
					isExisted = true;
					tclList->push_back(*iter);
					iter = clList->erase(iter);
#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_
					isSwBdIter = isInSwBoundaries.erase(isSwBdIter);
#endif
#endif
				}
				else
				{
					iter++;
#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_
					isSwBdIter++;
#endif
#endif
				}
			}

			// clList中已不存在能加入到tclList中的线段 退出循环
			if (!isExisted) 
				break;
		}

		clVecs.push_back(tclList);

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_
		if (isSwBoundaries)
			clVecsSwBoundaries.push_back(tclList);
#endif // _TRANSITION_OCEAN_STRIP_
#endif // _SHALLOW_OCEAN_STRIP_
	}
	delete clList;

#if 0
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
#if 0
	for (size_t i = 0; i < clVecsSwBoundaries.size(); i++)
	{
		ManualObject* mobj = mSceneMgr->createManualObject("mobjlinets" + Ogre::StringConverter::toString(i));
		mobj->begin("TestDemo", RenderOperation::OT_LINE_LIST);
		for (CoastLineList::iterator iter = clVecsSwBoundaries[i]->begin(); iter != clVecsSwBoundaries[i]->end(); iter++)
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
		SceneNode* nodeObj = mSceneMgr->createSceneNode("nodeObjTs" + Ogre::StringConverter::toString(i));
		nodeObj->attachObject(mobj);
		mSceneMgr->getRootSceneNode()->addChild(nodeObj);
	}
#endif
}

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_ 

//static std::vector<std::vector<bool>> rasterTr(0);
static std::vector<std::vector<bool>> raster(0);
static std::vector<Ogre::Vector3> clSwBdPoints(0);
static int radiusDl = 0;

void TerranLiquid::_extractBoundaries(const BvMat& raster, BvMat& rasterBd)
{
	const size_t rows = raster.size();
	const size_t cols = raster[0].size();

	// 单像素腐蚀
	rasterBd.resize(rows, std::vector<bool>(cols, false));
	std::copy(raster.begin(), raster.end(), rasterBd.begin());
	
	// 角点
	if (!raster[0][0])				 rasterBd[0][1] = rasterBd[1][0] = false;
	if (!raster[0][cols - 1])		 rasterBd[0][cols - 2] = rasterBd[1][cols - 1] = false;
	if (!raster[rows - 1][0])		 rasterBd[rows - 2][0] = rasterBd[rows - 1][1] = false;
	if (!raster[rows - 1][cols - 1]) rasterBd[rows - 1][cols - 2] = rasterBd[rows - 2][cols - 1] = false;
	// 边界点
	for (size_t row = 1; row < rows - 1; row++)
	{
		if (!raster[row][0])		rasterBd[row - 1][0] = rasterBd[row][1] = rasterBd[row + 1][0] = false;
		if (!raster[row][cols - 1])	rasterBd[row - 1][cols - 1] = rasterBd[row][cols - 2] = rasterBd[row + 1][cols - 1] = false;
	}
	for (size_t col = 1; col < cols - 1; col++)
	{
		if (!raster[0][col])		rasterBd[0][col - 1] = rasterBd[1][col] = rasterBd[0][col + 1] = false;
		if (!raster[rows - 1][col])	rasterBd[rows - 1][col - 1] = rasterBd[rows - 2][col] = rasterBd[rows - 1][col + 1] = false;
	}
	// 内部点
#pragma omp parallel for
	for (int row = 1; row < (int)(rows - 1); row++)
		for (int col = 1; col < (int)(cols - 1); col++)
			if (!raster[row][col])
				rasterBd[row - 1][col] = rasterBd[row][col - 1] = rasterBd[row][col + 1] = rasterBd[row + 1][col] = false;
	// 边界 = 原始栅格 - 单像素腐蚀栅格
	for (size_t row = 0; row < rows; row++)
		for (size_t col = 0; col < cols; col++)
			if (!rasterBd[row][col] && raster[row][col])
				rasterBd[row][col] = true;
			else if (rasterBd[row][col] && raster[row][col])
				rasterBd[row][col] = false;
}

void TerranLiquid::_getTransitionBoundary()
{
	// 调整clVecsSwBoundaries中所有区域边界线中每条边两个端点的顺序
	// 使呈现（前后边的）链式连接关系
	for (auto clvsbiter : clVecsSwBoundaries)
	{
		for (auto citer = clvsbiter->begin(); citer != clvsbiter->end(); citer++)
		{
			if (((++citer)--) == clvsbiter->end())		// 最后一条边
			{
				auto piter = citer; piter--;
				if (citer->endIdx == piter->endIdx)
				{
					std::swap(citer->startIdx, citer->endIdx);
					citer->startVertex.swap(citer->endVertex);
				}
			}
			else  // 非最后一条边
			{
				auto niter = citer; niter++;
				if (citer->startIdx == niter->startIdx || citer->startIdx == niter->endIdx)
				{
					std::swap(citer->startIdx, citer->endIdx);
					citer->startVertex.swap(citer->endVertex);
				}
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////

	// 以图像膨胀算法提取过渡区域边界
	const size_t cols = static_cast<size_t>(mBox.getSize().x / (0.2f * density));
	const size_t rows = static_cast<size_t>(mBox.getSize().z / (0.2f * density));
	float ratioX = mBox.getSize().x / (float)(cols - 1);
	float ratioZ = mBox.getSize().z / (float)(rows - 1);

	std::vector<Vector3> clRasterPoints(rows * cols, Vector3::ZERO);
	//std::vector<std::vector<bool>> raster(rows, std::vector<bool>(cols, false));
	raster.resize(rows, std::vector<bool>(cols, false));

	radiusDl = static_cast<int>(widthTrStrip + 0.5f);	// 以一定半径进行膨胀

	// 浅水区域范围
	std::vector<std::vector<bool>> rasterSW(rows, std::vector<bool>(cols, false));

#pragma omp parallel for
	for (int row = 0; row < (int)rows; row++)
	{
		for (int col = 0; col < (int)cols; col++)
		{
			int index = row * cols + col;
			clRasterPoints[index] = Vector3(col * ratioX + mBox.getMinimum().x, heightSeaLevel, row * ratioZ + mBox.getMinimum().z);
			auto rs = _pointsIntersect(clRasterPoints[index] + Vector3(0.f, 10000.f, 0.f)
				+ ((row == 0)? Vector3(0.f, 0.f, 0.5f) : Vector3::ZERO)
				+ ((row == rows - 1)? Vector3(0.f, 0.f, -0.5f) : Vector3::ZERO)
				+ ((col == 0)? Vector3(0.5f, 0.f, 0.f) : Vector3::ZERO)
				+ ((col == cols - 1)? Vector3(-0.5f, 0.f, 0.f) : Vector3::ZERO)
			);

			if (rs.first && (clRasterPoints[index].y + 10000.f - rs.second > depthShallowOcean))
			{
				rasterSW[row][col] = true;	// debug

				size_t colLf = 0, colRt = 0, rowTp = 0, rowBn = 0;
				int offset = col - radiusDl; colLf = (offset < 0) ? 0 : (size_t)offset;
				offset = col + radiusDl;     colRt = (offset > (int)cols - 1) ? (cols - 1) : (size_t)offset;
				offset = row - radiusDl;     rowTp = (offset < 0) ? 0 : (size_t)offset;
				offset = row + radiusDl;     rowBn = (offset > (int)rows - 1) ? (rows - 1) : (size_t)offset;

				for (size_t trow = rowTp; trow <= rowBn; trow++)
					for (size_t tcol = colLf; tcol <= colRt; tcol++)
						if (Math::Pow((float)trow - (float)row, 2.f) + Math::Pow((float)tcol - (float)col, 2.f) <= Math::Pow(radiusDl, 2.f))
							raster[trow][tcol] = true;
			}
		}
	}

#if 1
	std::vector<Ogre::Vector3> pointsraster;
	for (size_t row = 0; row < rows; row++)
		for (size_t col = 0; col < cols; col++)
			if (raster[row][col])
				pointsraster.push_back(clRasterPoints[row * cols + col]); 
	const char* path = "D:/Ganymede/DynamicOcean/OceanDemo/rasterTrSw.ply";
	Helper::exportPlyModel(path, &pointsraster[0], pointsraster.size(), (int*)NULL, 0);
	std::vector<Ogre::Vector3> pointsw;
	for (size_t row = 0; row < rows; row++)
		for (size_t col = 0; col < cols; col++)
			if (rasterSW[row][col])
				pointsw.push_back(clRasterPoints[row * cols + col]);
	const char* path3 = "D:/Ganymede/DynamicOcean/OceanDemo/rasterSw.ply";
	Helper::exportPlyModel(path3, &pointsw[0], pointsw.size(), (int*)NULL, 0);
#endif

	// 提取过渡区域边界
	std::vector<std::vector<bool>> rasterBd;
	_extractBoundaries(raster, rasterBd);

	for (size_t row = 0; row < rows; row++)
		for (size_t col = 0; col < cols; col++)
			if (rasterBd[row][col])
				clPoints.push_back(clRasterPoints[row * cols + col]);

#if 1
	std::vector<Ogre::Vector3> pointsrasterDl;
	for (size_t row = 0; row < rows; row++)
		for (size_t col = 0; col < cols; col++)
			if (rasterBd[row][col])
				pointsrasterDl.push_back(clRasterPoints[row * cols + col]);
	const char* path2 = "D:/Ganymede/DynamicOcean/OceanDemo/rasterTrBd.ply";
	Helper::exportPlyModel(path2, &pointsrasterDl[0], pointsrasterDl.size(), (int*)NULL, 0);
#endif

	// 提取浅水区域边界
	raster.swap(BvMat());
	raster.clear();
	_extractBoundaries(rasterSW, raster);

	for (size_t row = 0; row < rows; row++)
		for (size_t col = 0; col < cols; col++)
			if (raster[row][col])
				clSwBdPoints.push_back(clRasterPoints[row * cols + col]);

#if 1
	const char* path233 = "D:/Ganymede/DynamicOcean/OceanDemo/rasterSwBd.ply";
	Helper::exportPlyModel(path233, &clSwBdPoints[0], clSwBdPoints.size(), (int*)NULL, 0);
#endif
}

// static std::vector<int> clFacesDeepLeft(0);
// static std::vector<int> clFacesShallowLeft(0);
// static std::vector<int> clFacesTransitionLeft(0);

void TerranLiquid::_removeInvalidData(
	const std::vector<bool>& isOceanMesh, 
	const std::vector<bool>& isShallowMesh, 
	const std::vector<bool>& isTransitionMesh, 
	int* clFaces, 
	int countFaces
)
{
	std::vector<int> clFacesDeepLeft;
	std::vector<int> clFacesShallowLeft;
	std::vector<int> clFacesTransitionLeft;
	for (size_t i = 0; i < static_cast<unsigned int>(countFaces); i++)
	{
		if (isOceanMesh[i])
		{
			if (isShallowMesh[i])
			{
				clFacesShallowLeft.push_back(clFaces[3 * i]);
				clFacesShallowLeft.push_back(clFaces[3 * i + 1]);
				clFacesShallowLeft.push_back(clFaces[3 * i + 2]);
			}
			else if (isTransitionMesh[i])
			{
				clFacesTransitionLeft.push_back(clFaces[3 * i]);
				clFacesTransitionLeft.push_back(clFaces[3 * i + 1]);
				clFacesTransitionLeft.push_back(clFaces[3 * i + 2]);
			}
			else
			{
				clFacesDeepLeft.push_back(clFaces[3 * i]);
				clFacesDeepLeft.push_back(clFaces[3 * i + 1]);
				clFacesDeepLeft.push_back(clFaces[3 * i + 2]);
			}
		}
	}

	// clPoints中每个点所共享的面片
	std::vector<std::vector<int>> nrClFacesPerVertex(clPoints.size());
	for (size_t i = 0; i < clFacesShallowLeft.size(); i++)
		nrClFacesPerVertex[clFacesShallowLeft[i]].push_back(i / 3);
	for (size_t i = 0; i < clFacesTransitionLeft.size(); i++)
		nrClFacesPerVertex[clFacesTransitionLeft[i]].push_back(i / 3);
	for (size_t i = 0; i < clFacesDeepLeft.size(); i++)
		nrClFacesPerVertex[clFacesDeepLeft[i]].push_back(i / 3);

	// 更新面片对应顶点索引

	std::vector<Ogre::Vector3> clPointsLeft;

	// key-value: clPoints索引-clPointsLeft索引
	std::map<size_t, size_t> vertexIndicesUd;
	for (size_t i = 0; i < clPoints.size(); i++)
	{
		if (nrClFacesPerVertex[i].size() != 0)
		{
			vertexIndicesUd.insert(std::map<size_t, size_t>::value_type(i, vertexIndicesUd.size()));
			clPointsLeft.push_back(clPoints[i]);
		}
	}

	// 更新面片顶点索引
	for (auto iter = clFacesShallowLeft.begin(); iter != clFacesShallowLeft.end(); iter++)
	{
		auto viter = vertexIndicesUd.find(*iter);
		if (viter != vertexIndicesUd.end())
			*iter = viter->second;
	}
	for (auto iter = clFacesTransitionLeft.begin(); iter != clFacesTransitionLeft.end(); iter++)
	{
		auto viter = vertexIndicesUd.find(*iter);
		if (viter != vertexIndicesUd.end())
			*iter = viter->second;
	}
	for (auto iter = clFacesDeepLeft.begin(); iter != clFacesDeepLeft.end(); iter++)
	{
		auto viter = vertexIndicesUd.find(*iter);
		if (viter != vertexIndicesUd.end())
			*iter = viter->second;
	}

	delete[] vertices;
	delete[] indices;

	// 删除掉无效点之后的点集
	vertices = new Ogre::Vector3[clPointsLeft.size()];
	indices = new unsigned int[isOceanMesh.size() * 3];
	std::copy(clPointsLeft.begin(), clPointsLeft.end(), vertices);
	std::copy(clFacesShallowLeft.begin(), clFacesShallowLeft.end(), indices);
	std::copy(clFacesTransitionLeft.begin(), clFacesTransitionLeft.end(), indices + clFacesShallowLeft.size());
	std::copy(clFacesDeepLeft.begin(), clFacesDeepLeft.end(), indices + clFacesShallowLeft.size() + clFacesTransitionLeft.size());
	vertex_count = clPointsLeft.size();
	index_count = isOceanMesh.size() * 3;

#if 1
	const char* path = "D:/Ganymede/DynamicOcean/OceanDemo/OceanMesh.ply";
	Helper::exportPlyModel(path, vertices, vertex_count, indices, index_count / 3);
	const char* path1 = "D:/Ganymede/DynamicOcean/OceanDemo/OceanShallowMesh.ply";
	Helper::exportPlyModel(path1, &clPointsLeft[0], clPointsLeft.size(), &clFacesShallowLeft[0], clFacesShallowLeft.size() / 3);
	const char* path2 = "D:/Ganymede/DynamicOcean/OceanDemo/OceanTransitionMesh.ply";
	Helper::exportPlyModel(path2, &clPointsLeft[0], clPointsLeft.size(), &clFacesTransitionLeft[0], clFacesTransitionLeft.size() / 3);
	const char* path3 = "D:/Ganymede/DynamicOcean/OceanDemo/OceanDeepMesh.ply";
	Helper::exportPlyModel(path3, &clPointsLeft[0], clPointsLeft.size(), &clFacesDeepLeft[0], clFacesDeepLeft.size() / 3);
#endif

	/////////////////////////////////////////////////////////////////////////////

	Ogre::String matNameSw = "OceanShallow";
	Ogre::String matNameTr = "OceanTransition";
	Ogre::String matNameDp = "OceanDeep";

	size_t numVertices = vertex_count;

	// 三维坐标 法向量 纹理坐标
	float* coordinates = new float[3 * vertex_count];
	float* normals = new float[3 * vertex_count];
	float* textures = new float[2 * vertex_count];
	for (size_t i = 0; i < vertex_count; i++)
	{
		coordinates[3 * i] = vertices[i].x;
		coordinates[3 * i + 1] = vertices[i].y;
		coordinates[3 * i + 2] = vertices[i].z;

		normals[3 * i] = normals[3 * i + 2] = 0.f;
		normals[3 * i + 1] = 1.f;

		textures[2 * i] = (coordinates[3 * i] - mBox.getMinimum().x) / mBox.getSize().x;
		textures[2 * i + 1] = (coordinates[3 * i + 2] - mBox.getMinimum().z) / mBox.getSize().z;
	}
	
	// 权重
	float* tangents = new float[3 * vertex_count];
	for (size_t i = 0; i < vertex_count; i++)
	{
		tangents[3 * i] = tangents[3 * i + 1] = tangents[3 * i + 2] = 1.f;
	}

	// 生成Ogre Mesh

	Ogre::MeshPtr meshPtr = Ogre::MeshManager::getSingleton().createManual("OceanMesh", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	meshPtr->sharedVertexData = new Ogre::VertexData();
	meshPtr->sharedVertexData->vertexCount = numVertices;

	Ogre::SubMesh* smshSwPtr = meshPtr->createSubMesh("SubmeshSw");
	Ogre::SubMesh* smshTrPtr = meshPtr->createSubMesh("SubmeshTr");
	Ogre::SubMesh* smshDpPtr = meshPtr->createSubMesh("SubmeshDp");

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

	// 顶点切向量数据
	if (tangents != NULL)
	{
		offset = 0;
		vdecl->addElement(bindCounter, offset, Ogre::VET_FLOAT3, Ogre::VES_TANGENT);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		posVertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
			offset, numVertices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		posVertexBuffer->writeData(0, posVertexBuffer->getSizeInBytes(), tangents, true);
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

	// 设置面片顶点索引

	smshSwPtr->indexData->indexCount = clFacesShallowLeft.size();
	smshSwPtr->indexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
		HardwareIndexBuffer::IT_32BIT, smshSwPtr->indexData->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, true);
	smshSwPtr->setMaterialName(matNameSw);
	smshSwPtr->indexData->indexBuffer->writeData(0, smshSwPtr->indexData->indexBuffer->getSizeInBytes(), &clFacesShallowLeft[0], true);
	smshSwPtr->useSharedVertices = true;

	smshTrPtr->indexData->indexCount = clFacesTransitionLeft.size();
	smshTrPtr->indexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
		HardwareIndexBuffer::IT_32BIT, smshTrPtr->indexData->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, true);
	smshTrPtr->setMaterialName(matNameTr);
	smshTrPtr->indexData->indexBuffer->writeData(0, smshTrPtr->indexData->indexBuffer->getSizeInBytes(), &clFacesTransitionLeft[0], true);
	smshTrPtr->useSharedVertices = true;

	smshDpPtr->indexData->indexCount = clFacesDeepLeft.size();
	smshDpPtr->indexData->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
		HardwareIndexBuffer::IT_32BIT, smshDpPtr->indexData->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, true);
	smshDpPtr->setMaterialName(matNameDp);
	smshDpPtr->indexData->indexBuffer->writeData(0, smshDpPtr->indexData->indexBuffer->getSizeInBytes(), &clFacesDeepLeft[0], true);
	smshDpPtr->useSharedVertices = true;

	meshPtr->_setBounds(mBox);

	meshPtr->load();
	meshPtr->touch();

	delete[] coordinates;
	delete[] normals;
	delete[] tangents;
	delete[] textures;
}

std::pair<bool, Ogre::Real> TerranLiquid::_pointsIntersect(const Ogre::Vector3& p)
{
	Ogre::Ray ray(p, Ogre::Vector3::NEGATIVE_UNIT_Y);
	for (size_t j = 0; j < index_count; j += 3)
	{
		const auto& tp0 = vertices[indices[j]];
		const auto& tp1 = vertices[indices[j + 1]];
		const auto& tp2 = vertices[indices[j + 2]];

		if (_isPointInTriangle2D(tp0, tp1, tp2, p))
		{
			auto rs = ray.intersects(Ogre::Plane(tp0, tp1, tp2));
			if (rs.first)
				return rs;
		}
	}
	return std::make_pair(false, 0.f);
}

bool TerranLiquid::_addPointToGrid(const Ogre::Vector3& npl, const Ogre::Vector3& p, std::vector<Ogre::Vector3>& points)
{
	Vector3 pr = p + widthTrStrip * npl;
	auto rs = _pointsIntersect(pr + Vector3(0.f, 10000.f, 0.f));
	if (rs.first && (pr.y + 10000.f - rs.second < depthShallowOcean))
	{
		auto viter = points.begin();
		for (; viter != points.end(); viter++)
			if (viter->positionEquals(pr))
				break;
		if (points.end() == viter)	// 当前clPoints中没有p点
			points.push_back(pr);
		return true;
	}
	return false;
}

#endif
#endif

void TerranLiquid::_generateOceanGrid(void)
{
	// 建立离散点集 作为海面网格点集

	// 四个角点
	clPoints.push_back(Ogre::Vector3(minx, heightSeaLevel, minz));
	clPoints.push_back(Ogre::Vector3(maxx, heightSeaLevel, minz));
	clPoints.push_back(Ogre::Vector3(minx, heightSeaLevel, maxz));
	clPoints.push_back(Ogre::Vector3(maxx, heightSeaLevel, maxz));
	// 边界点
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
	// 内部点
	for (float xstep = minx + density; xstep < maxx; xstep += density)
		for (float ystep = minz + density; ystep < maxz; ystep += density)
			clPoints.push_back(Ogre::Vector3(xstep, heightSeaLevel, ystep));

	GeoPoint3D* verticesPtr = new GeoPoint3D[clPoints.size()];
	for (size_t i = 0; i < clPoints.size(); i++)
		verticesPtr[i] = GeoPoint3D(clPoints[i].x, clPoints[i].z, clPoints[i].y);

	// 约束边
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

	int* clFaces = NULL;
	int countFaces = 0;

	// 建立三角形网格
	std::unique_ptr<GPUDT> gpuDT(new GPUDT);
	gpuDT->setInputPoints(verticesPtr, clPoints.size());
	gpuDT->setInputConstraints(constraints, countConstraint);
	gpuDT->computeDT(&clFaces, countFaces);

	delete[] verticesPtr;
	delete[] constraints;

	// 调整面片顺序
	for (int i = 0; i < countFaces; i++)
		std::swap(clFaces[3 * i + 1], clFaces[3 * i + 2]);

#if 1
	const char* path = "D:/Ganymede/DynamicOcean/OceanDemo/grid.ply";
	//Helper::exportPlyModel(path, verticesPtr, clPoints.size(), faces, countFaces);
	Helper::exportPlyModel(path, &clPoints[0], clPoints.size(), clFaces, countFaces);
#endif

	// 遍历faces中所有面片
	// 计算每个面片中心点在地形mesh中的对应高度
	// 判断当前面片是否应删除

	std::vector<bool> isOceanMesh(countFaces, false);		// 面片是否属于海面网格
	
#ifdef _SHALLOW_OCEAN_STRIP_
	std::vector<bool> isShallowMesh(countFaces, false);		// 面片是否应保留在浅水网格中
#ifdef _TRANSITION_OCEAN_STRIP_ 
	std::vector<bool> isTransitionMesh(countFaces, false);	// 面片是否应保留在过渡区域网格中

	const size_t rows = raster.size();
	const size_t cols = raster[0].size();
	float ratioX = mBox.getSize().x / (float)(cols - 1);
	float ratioZ = mBox.getSize().z / (float)(rows - 1);
#endif
#endif

#pragma omp parallel for
	for (int i = 0; i < countFaces; i++)
	{
		int tidx = 3 * i;
		Ogre::Vector3 p[3];
		p[0] = clPoints[clFaces[tidx]];
		p[1] = clPoints[clFaces[tidx + 1]];
		p[2] = clPoints[clFaces[tidx + 2]];

		Ogre::Vector3 cp = (p[0] + p[1] + p[2]) / 3.f;
		cp.y = 10000.f;

		Ogre::Ray ray(cp, Ogre::Vector3::NEGATIVE_UNIT_Y);
		for (size_t j = 0; j < index_count; j += 3)
		{
			Ogre::Vector3 tp[3];
			tp[0] = vertices[indices[j]];
			tp[1] = vertices[indices[j + 1]];
			tp[2] = vertices[indices[j + 2]];

			if (_isPointInTriangle2D(tp[0], tp[1], tp[2], cp))
			{
				auto rs = ray.intersects(Ogre::Plane(tp[0], tp[1], tp[2]));
				if (rs.first)
				{
					float th = cp.y - rs.second;
					if (th < heightSeaLevel)	// 当前海面面片重心高度高于其对应的垂直地形点高程
					{
						isOceanMesh[i] = true;
#ifdef _SHALLOW_OCEAN_STRIP_
						if (th > depthShallowOcean)
							isShallowMesh[i] = true;
#ifdef _TRANSITION_OCEAN_STRIP_
						else   // 判断是否处于过渡区域
						{
							bool isAllWithin = true;
							for (size_t k = 0; k < 3; k++)
							{
								float mindist = 100000000.f;
								for (auto iter = clSwBdPoints.begin(); iter != clSwBdPoints.end(); iter++)
								{
									float dist = Ogre::Math::Sqrt(Ogre::Math::Pow(p[k].x - iter->x, 2.f) + Ogre::Math::Pow(p[k].z - iter->z, 2.f));
									mindist = (dist < mindist) ? dist : mindist;
								}
								if (mindist > radiusDl * ratioX)
								{
									isAllWithin = false;
									break;
								}
							}
							if (isAllWithin)
								isTransitionMesh[i] = true;
						}
#endif
#endif
					}
					break;
				}
			}
		}
	}

	// 删除无效点和无效面片
	// 根据isOceanMesh剔除无效点和无效面片
	// 同时调整isShallowMesh和isTransitionMesh中的面片索引
	_removeInvalidData(isOceanMesh, isShallowMesh, isTransitionMesh, clFaces, countFaces);

#if 0
	// 删除无效点和无效面片
	_removeInvalidData(isShallowMesh, clFaces, countFaces);
#if 1
	const char* path2 = "D:/Ganymede/DynamicOcean/OceanDemo/ocean_grid_simp.ply";
	//Helper::exportPlyModel(path2, &clPointsLeft[0], clPointsLeft.size(), /*(int*)NULL*/&clFacesLeft[0], clFacesLeft.size() / 3);
	Helper::exportPlyModel(path2, vertices, vertex_count, indices, index_count / 3);
#endif

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_ 
	_removeInvalidData(isTransitionMesh, clFaces, countFaces);
#endif
#endif
#if 1
	const char* path3 = "D:/Ganymede/DynamicOcean/OceanDemo/ocean_grid_tr.ply";
	//Helper::exportPlyModel(path2, &clPointsLeft[0], clPointsLeft.size(), /*(int*)NULL*/&clFacesLeft[0], clFacesLeft.size() / 3);
	Helper::exportPlyModel(path3, vertices, vertex_count, indices, index_count / 3);
#endif
#endif

	gpuDT->releaseMemory();
}

void TerranLiquid::_removeInvalidData(
	const std::vector<bool>& facesDeleted, 
	int* clFaces, 
	int countFaces
)
{
	std::vector<int> clFacesLeft;
	for (size_t i = 0; i < static_cast<unsigned int>(countFaces); i++)
	{
		if (facesDeleted[i])
		{
			clFacesLeft.push_back(clFaces[3 * i]);
			clFacesLeft.push_back(clFaces[3 * i + 1]);
			clFacesLeft.push_back(clFaces[3 * i + 2]);
		}
	}

	// clPoints中每个点所共享的面片
	std::vector<std::vector<int>> nrClFacesPerVertex(clPoints.size());
	for (size_t i = 0; i < clFacesLeft.size(); i++)
		nrClFacesPerVertex[clFacesLeft[i]].push_back(i / 3);

	// 更新面片对应顶点索引

	std::vector<Ogre::Vector3> clPointsLeft;

	// key-value: clPoints索引-clPointsLeft索引
	std::map<size_t, size_t> vertexIndicesUd;

	for (size_t i = 0; i < clPoints.size(); i++)
	{
		if (nrClFacesPerVertex[i].size() != 0)
		{
			vertexIndicesUd.insert(std::map<size_t, size_t>::value_type(i, vertexIndicesUd.size()));
			clPointsLeft.push_back(clPoints[i]);
		}
	}

	for (auto iter = clFacesLeft.begin(); iter != clFacesLeft.end(); iter++)
	{
		auto viter = vertexIndicesUd.find(*iter);
		if (viter != vertexIndicesUd.end())
			*iter = viter->second;
	}

	// 更新
	delete[] vertices;
	delete[] indices;

	// 删除掉无效点之后的点集
	vertices = new Ogre::Vector3[clPointsLeft.size()];
	indices = new unsigned int[clFacesLeft.size() * 3];
	std::copy(clPointsLeft.begin(), clPointsLeft.end(), vertices);
	std::copy(clFacesLeft.begin(), clFacesLeft.end(), indices);
	vertex_count = clPointsLeft.size();
	index_count = clFacesLeft.size();
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
	size_t current_offset = 0, shared_offset = 0;
	size_t next_offset = 0, index_offset = 0;
	vertex_count = index_count = 0;

	// 2016-11-10 10:56:05 submesh数量
	int countOfSubmeshes = (mesh->getNumSubMeshes() > 1) ? 5 : 1;
	// Calculate how many vertices and indices we're going to need
	for (unsigned short i = 0; i < countOfSubmeshes; ++i)
	{
		Ogre::SubMesh* submesh = mesh->getSubMesh(i);
		if (submesh->useSharedVertices)// We only need to add the shared vertices once
		{
			if (!added_shared)
			{
				vertex_count += mesh->sharedVertexData->vertexCount;
				added_shared = true;
			}
		}
		else
			vertex_count += submesh->vertexData->vertexCount;
		
		// Add the indices
		index_count += submesh->indexData->indexCount;
	}

	// Allocate space for the vertices and indices
	vertices = new Ogre::Vector3[vertex_count];
	indices = new unsigned int[index_count];

	added_shared = false;

	// 2016-11-15 12:56:09 最值
	minx = minz = 1000000000.f;
	maxx = maxz = 0.f;

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

			float* pReal;
			for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
			{
				posElem->baseVertexPointerToElement(vertex, &pReal);
				Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

				Ogre::Vector3 tvex = ((orient * pt) + position) * scale;	// 此处orient position scale的顺序需注意
				minx = std::min<float>(minx, tvex.x);  minz = std::min<float>(minz, tvex.z);
				maxx = std::max<float>(maxx, tvex.x);  maxz = std::max<float>(maxz, tvex.z);
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
				indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
		}
		else
		{
			for (size_t k = 0; k < numTris * 3; ++k)
				indices[index_offset++] = static_cast<unsigned long>(pShort[k]) + static_cast<unsigned long>(offset);
		}

		ibuf->unlock();
		current_offset = next_offset;
	}

	float ampitude = 10.f;
	mBox = Ogre::AxisAlignedBox(minx, heightSeaLevel - ampitude, minz, maxx, heightSeaLevel + ampitude, maxz);

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

void TerranLiquid::setGridDensity(float density)
{
	this->density = density;
}

/*void TerranLiquid::_createVertexData(void)
{
	Ogre::HardwareVertexBufferSharedPtr pVertexBuffer;
	pVertex = new Ogre::VertexData;
	pVertex->vertexStart = 0;
	pVertex->vertexCount = vertex_count * 4;

	Ogre::VertexDeclaration* decl = pVertex->vertexDeclaration;
	Ogre::VertexBufferBinding* bind = pVertex->vertexBufferBinding;
	size_t offset = 0;

	decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
	offset += Ogre::VertexElement::getTypeSize(VET_FLOAT3);

	decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
	offset += Ogre::VertexElement::getTypeSize(VET_FLOAT3);

	decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);	// 动态纹理
	offset += Ogre::VertexElement::getTypeSize(VET_FLOAT2);

	decl->addElement(0, offset, VET_FLOAT1, VES_TEXTURE_COORDINATES, 1);	// 一维深度图
	
	pVertexBuffer = HardwareBufferManager::getSingleton().createVertexBuffer(
		decl->getVertexSize(0), pVertex->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
	bind->setBinding(0, pVertexBuffer);

	const Ogre::VertexElement* positionElem = decl->findElementBySemantic(VES_POSITION);
	const Ogre::VertexElement* normalElem = decl->findElementBySemantic(VES_NORMAL);
	const Ogre::VertexElement* textureElem0 = decl->findElementBySemantic(VES_TEXTURE_COORDINATES, 0);
	const Ogre::VertexElement* textureElem1 = decl->findElementBySemantic(VES_TEXTURE_COORDINATES, 1);

	unsigned char* pBase = static_cast<unsigned char*>(pVertexBuffer->lock(HardwareBuffer::HBL_DISCARD));
	void *pPosition, *pNormal, *pTexture0, *pTexture1;

	// 填充顶点数据
	for (size_t i = 0; i < vertex_count; i++)
	{
		Ogre::Vector3 position = vertices[i];
		Ogre::Vector3 normal(Ogre::Vector3::UNIT_Y);
		Ogre::Vector2 texture0 = Ogre::Vector2((vertices[i].x - minx) / (maxx - minx), (vertices[i].z - minz) / (maxz - minz)) * texScale;
		Ogre::Real texture1 = depths[i] * depthScale;

		positionElem->baseVertexPointerToElement(pBase, &pPosition);
		normalElem->baseVertexPointerToElement(pBase, &pNormal);
		textureElem0->baseVertexPointerToElement(pBase, &pTexture0);
		textureElem1->baseVertexPointerToElement(pBase, &pTexture1);

		memcpy(pPosition, &position, sizeof(Ogre::Vector3));
		memcpy(pNormal, &normal, sizeof(Ogre::Vector3));
		memcpy(pTexture0, &texture0, sizeof(Ogre::Vector2));
		memcpy(pTexture1, &texture1, sizeof(Ogre::Real));

		pBase += pVertexBuffer->getVertexSize();
	}

	pVertexBuffer->unlock();
	//mRenderOp.vertexData = pVertex;
}

void TerranLiquid::_createIndexData(void)
{
	pIndex = new Ogre::IndexData;
	pIndex->indexStart = 0;
	pIndex->indexCount = index_count;

	pIndex->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(
		HardwareIndexBuffer::IT_32BIT, pIndex->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	unsigned short* pIdx = static_cast<unsigned short*>(pIndex->indexBuffer->lock(HardwareBuffer::HBL_DISCARD));
	for (size_t i = 0; i < index_count; i++)
	{
		*pIdx++ = static_cast<unsigned short>(indices[i]);
	}

	pIndex->indexBuffer->unlock();
	// 	mRenderOp.indexData = pIndex;
	// 	mRenderOp.operationType = RenderOperation::OT_TRIANGLE_LIST;
	// 	mRenderOp.useIndexes = true;
}*/

/*Ogre::uint32 TerranLiquid::getTypeFlags() const
{
return Ogre::SceneManager::WORLD_GEOMETRY_TYPE_MASK;
}

Ogre::Real TerranLiquid::getBoundingRadius(void) const
{
	//return Math::Sqrt(std::max(mBox.getMaximum().squaredLength(), mBox.getMinimum().squaredLength()));
	return 0.f;
}

Ogre::Real TerranLiquid::getSquaredViewDepth(const Ogre::Camera * cam) const
{
	// 	const Vector3 vMin = mBox.getMinimum();
	// 	const Vector3 vMax = mBox.getMaximum();
	// 	const Vector3 vMid = ((vMin - vMax) * 0.5) + vMin;
	// 	const Vector3 vDist = cam->getDerivedPosition() - vMid;
	//
	// 	return vDist.squaredLength();
	return 0.f;
}*/

void TerranLiquid::_getDepthData(void)
{
	depths = new Ogre::Real[vertex_count];

	for (size_t i = 0; i < vertex_count; i++)
	{
		Ogre::Vector3 cp(vertices[i]);
		cp.y = 1000.f;

		Ogre::Ray ray(cp, Ogre::Vector3::NEGATIVE_UNIT_Y);
		for (size_t j = 0; j < index_count; j += 3)
		{
			const auto& tp0 = vertices[indices[j]];
			const auto& tp1 = vertices[indices[j + 1]];
			const auto& tp2 = vertices[indices[j + 2]];

			if (_isPointInTriangle2D(tp0, tp1, tp2, cp))
			{
				auto rs = ray.intersects(Ogre::Plane(tp0, tp1, tp2));
				if (rs.first)
				{
					depths[i] = cp.y - rs.second - heightSeaLevel;
					if (depths[i] < 0.f)
						depths[i] = 0.f;
					break;
				}
			}
		}
	}
}

#ifdef _SHALLOW_OCEAN_STRIP_
void TerranLiquid::setDepthShallowOcean(float depthShallowOcean)
{
	this->depthShallowOcean = depthShallowOcean;
}
#endif
