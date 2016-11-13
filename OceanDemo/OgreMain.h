// 2016-11-12 10:27:09
// Stanely Xiao
// Ogre 主流程

#ifndef _OGRE_MAIN_H_
#define _OGRE_MAIN_H_

#include "OIS/OIS.h"
#include "Ogre.h"
#include "OgreFrameListener.h" 
#include "OgreBuildSettings.h"
#include "OgreLogManager.h"
#include "OgrePlugin.h"
#include "OgreFileSystemLayer.h"
#include "OgreOverlaySystem.h"
#include "OgreOverlayManager.h"

#include "CameraController.h"

class COgreMain
	: public Ogre::FrameListener,
	public OIS::KeyListener,
	public OIS::MouseListener
{
public:
	COgreMain(void);
	~COgreMain(void);

public:
	void run(void);

public:
	void createScene(void);
	void destroyScene(void);

public:
	bool frameRenderingQueued(const Ogre::FrameEvent& evt);
	bool frameStarted(const Ogre::FrameEvent& evt);
	bool frameEnded(const Ogre::FrameEvent& evt);

	virtual void createRoot(void);
	virtual void destroyRoot(void);
	virtual void createWindow(void);
	virtual void createInput(void);
	virtual void destroyInput(void);
	virtual void destroyWindow(void);
	virtual void loadResources(void);
	virtual void unloadResources(void);
	virtual void createSceneManager(void);
	virtual void destroySceneManager(void);
	virtual void createContent(void);
	virtual void destroyContent(void);

	bool keyPressed(const OIS::KeyEvent& e);
	bool keyReleased(const OIS::KeyEvent& e);
	bool mouseMoved(const OIS::MouseEvent& e);
	bool mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID id);

private:
	//Ogre
	Ogre::Root* mRoot;						// OGRE root
	Ogre::RenderWindow* mWindow;			// render window
	Ogre::FileSystemLayer* mFSLayer;		// File system abstraction layer
	Ogre::SceneManager* mSceneMgr;			// scene manager for this app
	Ogre::OverlaySystem* mOverlaySystem;
	Ogre::Overlay* mainOverlay;

	//OIS
	OIS::InputManager* inputManager;
	OIS::Keyboard* mKeyboard;
	OIS::Mouse* mMouse;

	CameraController mainCameraView;
	float cameraSpeed;

	//摄像头控制
	bool cameraForward;
	bool cameraBackward;
	bool cameraLeft;
	bool cameraRight;

	// 边界可见性
	bool visibility;
};

#endif // !
