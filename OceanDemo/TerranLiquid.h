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

	// ���ú��������ܶ�
	void setGridDensity(float density);

	// ��ʼ��
	void initialize(void);


private:
	Ogre::Root* mRoot;
	Ogre::SceneManager* mSceneMgr;
	Ogre::SceneNode* nodeTerra;
	Ogre::Entity* entTerra;
	Ogre::String nameTerra;
	Ogre::Vector3 transPos;	// λ��
	Ogre::Vector3 scale;	// ����


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

	typedef std::list<TerranLiquid::CoastLine> CoastLineList;
	std::vector<CoastLineList*> clVecs;

	std::vector<Ogre::Vector3> clPoints;
	
	std::list<TerranLiquid::CoastLine>* clList;
	float heightSeaLevel;	// ��ƽ��߶�

	size_t vertex_count, index_count;
	Ogre::Vector3* vertices;
	unsigned int* indices;

	float minx, minz, maxx, maxz;
	float density;

private:
	// ��ȡ������
	void _getCoastLines(void);
	
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

	// �γɺ�������
	void _generateOceanGrid(void);

private:
	inline Ogre::Real _absValue(Ogre::Real lhs, Ogre::Real rhs)
	{
		return Ogre::Math::Abs(lhs - rhs);
	}
};

#endif
