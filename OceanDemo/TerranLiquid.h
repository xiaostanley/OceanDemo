// 2016-11-13 15:36:56
// Stanley Xiao
// ˮ��

#ifndef _TERRAIN_LIQUID_H_
#define _TERRAIN_LIQUID_H_

#include "Ogre.h"
#include "OgreSimpleRenderable.h"

#include <list>
#include <vector>

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
	void setHeight(float height);

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

// 		bool operator < (const CoastLine& cl)
// 		{
// 			if (cl.startIdx)
// 		}
	};

	std::list<TerranLiquid::CoastLine>* clList;
	float height;	// ��ƽ��߶�

private:
	// ��ȡ������
	void _getCoastLines(void);
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

};

#endif
