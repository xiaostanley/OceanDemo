// 2016-11-13 15:36:56
// Stanley Xiao
// ˮ��

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
	// �����������
	void setInputParas(
		Ogre::Root* mRoot,
		Ogre::SceneManager* mSceneMgr,
		Ogre::SceneNode* nodeTerra,
		const Ogre::String& nameTerra,
		const Ogre::Vector3& transPos
	);

	// ���ú�ƽ��߶�
	void setHeight(float heightSeaLevel);

#ifdef _SHALLOW_OCEAN_STRIP_
	// ����ǳ��ˮ��
	void setDepthShallowOcean(float depthShallowOcean);
#endif

	// ���ú��������ܶ�
	void setGridDensity(float density);

	// ��ʼ��
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
	Ogre::Vector3 transPos;	// λ��
	Ogre::Vector3 scale;	// ����

	Ogre::AxisAlignedBox mBox;

private:
	// ������
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
	std::vector<CoastLineList*> clVecs;	// ���򺣰�������
	std::list<TerranLiquid::CoastLine>* clList;

	float heightSeaLevel;	// ��ƽ��߶�

#ifdef _SHALLOW_OCEAN_STRIP_
	float depthShallowOcean;	// ǳ��ˮ��

#ifdef _TRANSITION_OCEAN_STRIP_
	// ǳˮ����߽� ָ��clList�е�Ԫ��
	std::vector<bool> isInSwBoundaries;		// clList�е�Ԫ���Ƿ�Ϊǳˮ����߽�
	std::vector<CoastLineList*> clVecsSwBoundaries;	// ����ǳˮ����߽�����
#endif // _TRANSITION_OCEAN_STRIP_
#endif

	///////////////////////////////////////////////////

	size_t vertex_count, index_count;
	Ogre::Vector3* vertices;
	unsigned int* indices;

	Ogre::Real* depths;	// �������

	float minx, minz, maxx, maxz;
	float density;		// ���������ܶ�

	float texScale;		// ��̬������������
	float depthScale;	// ���������������

private:
	
	// ������ȡ�����ߣ�����
	void _getInitialCoastLines(void);
	
	// ��ȡ������Ϣ
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
	
	// ����������Ϣ
	void _collectCoastLines(void);

#ifdef _SHALLOW_OCEAN_STRIP_
#ifdef _TRANSITION_OCEAN_STRIP_ 
	// ��ȡ��������߽�
	void _getTransitionBoundary(void);
#endif
#endif

	// �γɺ�������
	void _generateOceanGrid(void);

	// ��ȡ�������
	void _getDepthData(void);

	// ɾ����Ч�����Ƭ
	void _removeInvalidData(
		const std::vector<bool>& isNotOceanMesh,
		int* clFaces,
		int countFaces
	);

// private:
// 	// ����VertexData
// 	void _createVertexData(void);
// 
// 	// ����IndexData
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
