// 2016-11-13 15:36:56
// Stanley Xiao
// 水面

#ifndef _TERRAIN_LIQUID_H_
#define _TERRAIN_LIQUID_H_

#include "Ogre.h"
#include "OgreSimpleRenderable.h"
#include "GPUDT_vc10.h"

#include <list>
#include <vector>
#include <set>
#include <map>

#define _SHALLOW_OCEAN_STRIP_
#define _TRANSITION_OCEAN_STRIP_

const float eps = 0.001f;

class TerranLiquid //:
	//public Ogre::SimpleRenderable
{
public:
	TerranLiquid();
	~TerranLiquid();

public:
	// 设置输入参数
	void setInputParas(
		Ogre::Root* mRoot,
		Ogre::SceneManager* mSceneMgr,
		Ogre::SceneNode* nodeTerra,
		const Ogre::String& nameTerra,
		const Ogre::Vector3& transPos
	);

	// 设置海平面高度
	void setHeight(float heightSeaLevel);

#ifdef _SHALLOW_OCEAN_STRIP_
	// 设置浅海水深
	void setDepthShallowOcean(float depthShallowOcean);
#endif

	// 设置海面网格密度
	void setGridDensity(float density);

	// 初始化
	void initialize(void);

// public:
// 	virtual Ogre::uint32 getTypeFlags() const;
// 	virtual Ogre::Real getBoundingRadius(void) const;
// 	virtual Ogre::Real getSquaredViewDepth(const Ogre::Camera* cam) const;

private:
	Ogre::Root* mRoot;
	Ogre::SceneManager* mSceneMgr;
	Ogre::SceneNode* nodeTerra;
	Ogre::Entity* entTerra;
	Ogre::String nameTerra;
	Ogre::Vector3 transPos;	// 位移
	Ogre::Vector3 scale;	// 缩放

	Ogre::AxisAlignedBox mBox;

private:
	// 海岸线
	struct CoastLine
	{
		int startIdx, endIdx;
		Ogre::Vector3 startVertex, endVertex;

		CoastLine(void)
			:startIdx(-1), endIdx(-1), startVertex(Ogre::Vector3::ZERO), endVertex(Ogre::Vector3::ZERO)
		{}

		CoastLine(int _startIdx, int _endIdx, const Ogre::Vector3& _startVertex, const Ogre::Vector3& _endVertex)
			:startIdx(_startIdx), endIdx(_endIdx), startVertex(_startVertex), endVertex(_endVertex)
		{}
	};

	std::vector<Ogre::Vector3> clPoints;

	typedef std::list<TerranLiquid::CoastLine> CoastLineList;
	std::vector<CoastLineList*> clVecs;	// 有序海岸线数据
	std::list<TerranLiquid::CoastLine>* clList;

	float heightSeaLevel;	// 海平面高度

#ifdef _SHALLOW_OCEAN_STRIP_
	float depthShallowOcean;	// 浅海水深

#ifdef _TRANSITION_OCEAN_STRIP_
	// 浅水区域边界 指向clList中的元素
	std::vector<bool> isInSwBoundaries;		// clList中的元素是否为浅水区域边界
	std::vector<CoastLineList*> clVecsSwBoundaries;	// 有序浅水区域边界数据
#endif // _TRANSITION_OCEAN_STRIP_
#endif

	///////////////////////////////////////////////////

	size_t vertex_count, index_count;
	Ogre::Vector3* vertices;
	unsigned int* indices;

	Ogre::Real* depths;	// 深度数据

	float minx, minz, maxx, maxz;
	float density;		// 海面网格密度

	float texScale;		// 动态纹理缩放因子
	float depthScale;	// 深度纹理缩放因子

private:
	
	// 初步提取海岸线（无序）
	void _getInitialCoastLines(void);
	
	// 提取网格信息
	void _getMeshInfo(
		const Ogre::MeshPtr mesh, 
		size_t &vertex_count, 
		Ogre::Vector3* &vertices, 
		size_t &index_count, 
		unsigned int* &indices, 
		const Ogre::Vector3 &position = Ogre::Vector3::ZERO, 
		const Ogre::Quaternion &orient = Ogre::Quaternion::IDENTITY, 
		const Ogre::Vector3 &scale = Ogre::Vector3::UNIT_SCALE
	);
	
	// 整理海岸线信息
	void _collectCoastLines(void);

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_ 
	// 提取过渡区域边界
	void _getTransitionBoundary(void);
#endif
#endif

	// 形成海面网格
	void _generateOceanGrid(void);

	// 提取深度数据
	void _getDepthData(void);

	// 删除无效点和面片
	void _removeInvalidData(
		const std::vector<bool>& isNotOceanMesh,
		int* clFaces,
		int countFaces
	);

// private:
// 	// 创建VertexData
// 	void _createVertexData(void);
// 
// 	// 创建IndexData
// 	void _createIndexData(void);
// 
// 	Ogre::VertexData* pVertex;
// 	Ogre::IndexData* pIndex;

private:
	inline Ogre::Real _absValue(Ogre::Real lhs, Ogre::Real rhs)
	{
		return Ogre::Math::Abs(lhs - rhs);
	}

	inline bool _isPointInTriangle2D(
		const Ogre::Vector3& tp0,
		const Ogre::Vector3& tp1,
		const Ogre::Vector3& tp2,
		const Ogre::Vector3& cp
	)
	{
		Ogre::Vector2 pa(tp0.x - cp.x, tp0.z - cp.z);
		Ogre::Vector2 pb(tp1.x - cp.x, tp1.z - cp.z);
		Ogre::Vector2 pc(tp2.x - cp.x, tp2.z - cp.z);

		double t1 = static_cast<double>(pa.x) * static_cast<double>(pb.y) - static_cast<double>(pa.y)*static_cast<double>(pb.x);
		double t2 = static_cast<double>(pb.x) * static_cast<double>(pc.y) - static_cast<double>(pb.y)*static_cast<double>(pc.x);
		double t3 = static_cast<double>(pc.x) * static_cast<double>(pa.y) - static_cast<double>(pc.y)*static_cast<double>(pa.x);

		return (t1*t2 >= 0.0 && t1*t3 >= 0.0);
	}

	inline bool _isIntersected(const Ogre::Vector3* p, float height)
	{
		return (!((p[0].y < height && p[1].y < height && p[2].y < height) ||
			(p[0].y > height && p[1].y > height && p[2].y > height)));
	}
};

#endif
