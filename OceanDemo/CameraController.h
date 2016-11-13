// 2016-11-12 10:53:36
// Stanley Xiao
// ÉãÏñ»ú¿ØÖÆÆ÷

#ifndef _CAMERA_CONTROLLER_H_
#define _CAMERA_CONTROLLER_H_

#include <OIS/OIS.h>

#include "Ogre.h"
#include "OgreFrameListener.h" 
#include "OgreBuildSettings.h"
#include "OgreLogManager.h"
#include "OgrePlugin.h"
#include "OgreFileSystemLayer.h"
#include "OgreOverlaySystem.h"
#include "OgreOverlayManager.h"

class CameraController
{
public:
	CameraController(void);
	~CameraController(void);

	void setViewportRect(
		float left,
		float top,
		float width,
		float height,
		float wedge,
		float hedge,
		int zorder,
		Ogre::ColourValue color,
		bool overlayEnabled
	);

	void createCameraNode(
		Ogre::SceneManager* mSceneMgr, 
		Ogre::String name, 
		float nearDis, 
		float farDis
	);
	void destroyCameraNode(Ogre::SceneManager* mSceneMgr);
	void createViewport(Ogre::RenderWindow* mWindow);
	void destroyViewport(Ogre::RenderWindow* mWindow);

	Ogre::SceneNode* getCameraNode(void);
	Ogre::Camera* getCamera(void);
	void setVisible(Ogre::RenderWindow* mWindow, bool flag);
	bool getVisible(Ogre::RenderWindow* mWindow);

	bool attachToParentNode(Ogre::SceneNode* node, Ogre::Vector3 offset);
	bool dettachFromParentNode(void);

private:
	float left;
	float top;
	float width;
	float height;
	float edge_width;
	float edge_height;

	Ogre::Camera* camera;
	Ogre::SceneNode* nodeCamera;
	Ogre::SceneNode* parentNode;
	Ogre::Viewport* viewport;

	int zorder;
	Ogre::ColourValue color;
	bool overlayEnabled;
};

#endif