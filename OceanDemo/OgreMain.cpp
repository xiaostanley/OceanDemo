#include "stdafx.h"
#include "OgreMain.h"

using namespace Ogre;

#ifdef _USE_HYDRAX_
Hydrax::Hydrax* mHydrax = NULL;
#endif // _USE_HYDRAX_

COgreMain::COgreMain(void)
	: mRoot(NULL),
	mWindow(NULL),
	mFSLayer(NULL),
	mSceneMgr(NULL),
	mOverlaySystem(NULL),
	inputManager(NULL),
	mKeyboard(NULL),
	mMouse(NULL),
	cameraForward(false),
	cameraBackward(false),
	cameraLeft(false),
	cameraRight(false),
	cameraLeftTurn(false),
	cameraRightTurn(false),
	cameraSpeed(40.0f),
	boundaryVisble(false),
	viewMode(0),
	gridShowIdx(0)
#ifdef _USE_TERRAIN_LIQUID_
	, tliquid(NULL)
#endif
#ifdef _USE_OGRE_WATER_
	, mWater(NULL)
#endif

{
}

COgreMain::~COgreMain(void)
{
}

void COgreMain::run(void)
{
	//��������
	createScene();

#ifdef _LAYERED_RENDERING_

	mWindow->setActive(true);
	mWindow->setAutoUpdated(false);

	Ogre::Viewport* mViewport = mWindow->getViewport(0);
	mViewport->setAutoUpdated(false);

	while (!mWindow->isClosed())
	{
		mWindow->_beginUpdate();

		mainCameraView.getCamera()->setNearClipDistance(20.0f);
		mainCameraView.getCamera()->setFarClipDistance(100000.f);
		mViewport->setOverlaysEnabled(false);
		mWindow->_updateViewport(mViewport);
		mViewport->setOverlaysEnabled(true);

		mViewport->setClearEveryFrame(true, Ogre::FBT_DEPTH);
		mViewport->setSkiesEnabled(false);
		mainCameraView.getCamera()->setNearClipDistance(0.1f);
		mainCameraView.getCamera()->setFarClipDistance(20.f);
		//mainCameraView.getCamera()->setPolygonMode(PM_WIREFRAME);
		mWindow->_updateViewport(mViewport);
		mainCameraView.getCamera()->setPolygonMode(PM_SOLID);
		mViewport->setSkiesEnabled(true);
		mViewport->setClearEveryFrame(true, Ogre::FBT_COLOUR | Ogre::FBT_DEPTH);

		mWindow->_updateAutoUpdatedViewports();
		mWindow->_endUpdate();

		mWindow->swapBuffers();
		mRoot->renderOneFrame();

		Ogre::WindowEventUtilities::messagePump();
	}
#else
	//������Ⱦ
	mRoot->startRendering();
#endif

	//���ٳ���
	destroyScene();
}

void COgreMain::createScene(void)
{
	mFSLayer = OGRE_NEW_T(Ogre::FileSystemLayer, Ogre::MEMCATEGORY_GENERAL)(OGRE_VERSION_NAME);

	createRoot();
	createWindow();
	createInput();
	loadResources();

	createSceneManager();
	createContent();
}

void COgreMain::destroyScene(void)
{
	destroyContent();
	destroySceneManager();

	unloadResources();
	destroyInput();
	destroyWindow();
	destroyRoot();

	OGRE_DELETE_T(mFSLayer, FileSystemLayer, Ogre::MEMCATEGORY_GENERAL);
}

bool COgreMain::frameRenderingQueued(const Ogre::FrameEvent & evt)
{
	//�ж��������Ƿ�ر�
	if (mWindow->isClosed())
		return false;

	//��ȡ��������
	mKeyboard->capture();
	mMouse->capture();

	if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
		return false;

	if (viewMode == 0)
	{
		//�ӵ����
		if (cameraForward || cameraBackward || cameraLeft || cameraRight)
		{
			float heightCam = mainCameraView.getCameraNode()->_getDerivedPosition().y;
			cameraSpeed = 40.f + Ogre::Math::Abs(heightCam);

			float moveX = 0.0f, moveY = 0.0f, moveZ = 0.0f;
			if (cameraForward) moveZ = -cameraSpeed;
			if (cameraBackward) moveZ = cameraSpeed;
			if (cameraLeft) moveX = -cameraSpeed;
			if (cameraRight) moveX = cameraSpeed;

			mainCameraView.getCameraNode()->translate(moveX*evt.timeSinceLastFrame, 0.0f, moveZ*evt.timeSinceLastFrame, Ogre::Node::TS_LOCAL);
		}
		if (cameraLeftTurn || cameraRightTurn)
		{
			float aglYaw = 0.f;
			if (cameraLeftTurn)		aglYaw = 1.1f;
			if (cameraRightTurn)	aglYaw = -1.1f;

			mainCameraView.getCameraNode()->yaw(Ogre::Degree(aglYaw), Ogre::Node::TS_WORLD);
		}

		const float ydiff = 0.1f;
		if (mKeyboard->isKeyDown(OIS::KC_PGUP))
			mainCameraView.getCameraNode()->translate(0.f, ydiff, 0.f);
		else if (mKeyboard->isKeyDown(OIS::KC_PGDOWN))
			mainCameraView.getCameraNode()->translate(0.f, -ydiff, 0.f);
	}

#ifdef _USE_OGRE_WATER_
	mWater->update(evt.timeSinceLastFrame);
#endif
#ifdef _USE_HYDRAX_
	mHydrax->update(evt.timeSinceLastFrame);
#endif // _USE_HYDRAX_


	// ��ʾ֡��
	Ogre::OverlayElement* textArea = OverlayManager::getSingleton().getOverlayElement("CodeText");
	if (textArea->isVisible())
		textArea->setCaption(Ogre::DisplayString(L"FPS: ") + Ogre::DisplayString(Ogre::StringConverter::toString(mWindow->getBestFPS())));

	return true;
}

bool COgreMain::frameStarted(const Ogre::FrameEvent & evt)
{
	//��ȡ����
	mKeyboard->capture();
	mMouse->capture();

	//��KC_ESCAPE���˳�
	if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
		return false;

	return true;
}

bool COgreMain::frameEnded(const Ogre::FrameEvent & evt)
{
	return true;
}

void COgreMain::createRoot(void)
{
	Ogre::String pluginsPath = "";
	pluginsPath = mFSLayer->getConfigFilePath("plugins.cfg");

	mRoot = NULL;
	mRoot = OGRE_NEW Ogre::Root(pluginsPath, "", "ogre.log");

#ifdef _GL_RENDER_SYSTEM_
	Ogre::RenderSystem* rsys = mRoot->getRenderSystemByName("OpenGL Rendering Subsystem");
#else
	Ogre::RenderSystem* rsys = mRoot->getRenderSystemByName("Direct3D9 Rendering Subsystem");
#endif
	mRoot->setRenderSystem(rsys);

	mRoot->initialise(false);
}

void COgreMain::destroyRoot(void)
{
	if (mRoot)
	{
		mRoot->saveConfig();

		OGRE_DELETE mRoot;
		mRoot = NULL;
	}
}

void COgreMain::createWindow(void)
{
	//create RenderWindow
	Ogre::NameValuePairList params;
	params["Colour Depth"] = "32";
	params["gamma"] = "Yes";
	params["FSAA"] = "4";
	params["vsync"] = "true";

	mWindow = mRoot->createRenderWindow("OgreMain", 1280, 760, false, &params);
	mRoot->addFrameListener(this);
}

void COgreMain::createInput(void)
{
	OIS::ParamList pl;
	size_t windowHnd = 0;
	std::ostringstream windowHndStr;

	//tell OIS about the Ogre window
	mWindow->getCustomAttribute("WINDOW", &windowHnd);
	windowHndStr << windowHnd;
	pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

	//setup the manager, keyboard and mouse to handle input
	inputManager = OIS::InputManager::createInputSystem(pl);
	mKeyboard = static_cast<OIS::Keyboard*>(inputManager->createInputObject(OIS::OISKeyboard, true));
	mMouse = static_cast<OIS::Mouse*>(inputManager->createInputObject(OIS::OISMouse, true));

	//tell OIS about the window's dimensions
	unsigned int width, height, depth;
	int top, left;
	mWindow->getMetrics(width, height, depth, left, top);
	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;

	//input events
	mKeyboard->setEventCallback(this);
	mMouse->setEventCallback(this);
}

void COgreMain::destroyInput(void)
{
	inputManager->destroyInputObject(mMouse); 
	mMouse = NULL;
	inputManager->destroyInputObject(mKeyboard); 
	mKeyboard = NULL;
	OIS::InputManager::destroyInputSystem(inputManager); 
	inputManager = NULL;
}

void COgreMain::destroyWindow(void)
{
	mWindow = NULL;
}

void COgreMain::loadResources(void)
{
	Ogre::String resourcePath = "";
	resourcePath = mFSLayer->getConfigFilePath("resources.cfg");

	Ogre::ConfigFile config;
	config.load(resourcePath);

	Ogre::ConfigFile::SectionIterator seci = config.getSectionIterator();
	Ogre::String secName, typeName, archName;
	while (seci.hasMoreElements())
	{
		secName = seci.peekNextKey();
		Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
		Ogre::ConfigFile::SettingsMultiMap::iterator i;
		for (i = settings->begin(); i != settings->end(); ++i)
		{
			typeName = i->first;
			archName = i->second;
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
		}
	}

	Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void COgreMain::unloadResources(void)
{
	Ogre::ResourceGroupManager::ResourceManagerIterator seci =
		Ogre::ResourceGroupManager::getSingleton().getResourceManagerIterator();
	while (seci.hasMoreElements())
	{
		seci.getNext()->unloadUnreferencedResources();
	}
}

void COgreMain::createSceneManager(void)
{
	//����������
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);

	//ƽ�������
	mOverlaySystem = new Ogre::OverlaySystem();
	mSceneMgr->addRenderQueueListener(mOverlaySystem);
}

void COgreMain::destroySceneManager(void)
{
	//ɾ��ƽ�������
	delete mOverlaySystem;
	mOverlaySystem = NULL;

	//ɾ������������
	mRoot->destroySceneManager(mSceneMgr);
	mSceneMgr = NULL;
}

void COgreMain::createContent(void)
{
	// ��ʾ������
	Ogre::Entity* axis = mSceneMgr->createEntity("axis", "Axis.mesh");
	Ogre::SceneNode* nodeAxis = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeAxis");
	nodeAxis->attachObject(axis);
	nodeAxis->setScale(50.f, 50.f, 50.f);
	nodeAxis->setVisible(false);

	// ��������ͷ
	mainCameraView.createCameraNode(mSceneMgr, "MainCamera", 10.f, 99999*6.0f);	// 10 10000

	// ������Ҫ����ͷ
	Ogre::Vector3 mainCameraPos;
	mainCameraPos.x = 0.f;
	mainCameraPos.y = 0.f;
	mainCameraPos.z = 0.f;
	mainCameraView.getCameraNode()->setPosition(mainCameraPos);

	mainCameraView.getCamera()->pitch(Ogre::Degree(-3.f));

	// �����ӿ�
	mainCameraView.setViewportRect(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1, Ogre::ColourValue(0.0f, 0.0f, 0.0f), true);
	mainCameraView.createViewport(mWindow);

	///////////////////////////////////////////////////////////////////////////////////////////////

	// ��������
	Ogre::FontPtr font = Ogre::FontManager::getSingleton().create("SimHeiFont", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	font->setSource("simhei.ttf");
	font->setType(FT_TRUETYPE);
	font->setTrueTypeSize(32);
	font->setTrueTypeResolution(96);
	font->addCodePointRange(Ogre::Font::CodePointRange(33, 166));
	font->load();

	// ����overlay
	mainOverlay = Ogre::OverlayManager::getSingleton().create("mainOverlay");
	mainOverlay->setZOrder(254);
	mainOverlay->show();

	// ����OverlayContainer
	Ogre::OverlayContainer* ovContainer = static_cast<Ogre::OverlayContainer*>(Ogre::OverlayManager::getSingleton().createOverlayElement("BorderPanel", "Container"));
	ovContainer->setMetricsMode(GMM_RELATIVE);
	ovContainer->setPosition(0.f, 0.f);
	ovContainer->setWidth(0.2f);
	ovContainer->setHeight(0.07f);
	ovContainer->setMaterialName("Core/StatsBlockCenter");
	mainOverlay->add2D(ovContainer);

	// ����������ʾ����
	Ogre::OverlayElement* textArea = OverlayManager::getSingleton().createOverlayElement("TextArea", "CodeText");
	textArea->setMetricsMode(GMM_RELATIVE);
	textArea->setPosition(0.01f, 0.01f);
	textArea->setWidth(0.2f);
	textArea->setHeight(0.1f);
	textArea->setParameter("font_name", "SimHeiFont");
	textArea->setParameter("char_height", Ogre::StringConverter::toString(Ogre::Real(0.05f)));
	textArea->setParameter("horz_align", "left");
	textArea->setColour(Ogre::ColourValue(1.f, 1.f, 1.f));
	textArea->setCaption(Ogre::DisplayString("FPS"));
	ovContainer->addChild(textArea);

	mainOverlay->hide();

	///////////////////////////////////////////////////////////////////////////////////////////////

	// ������Ч
//	mSceneMgr->setFog(FOG_LINEAR, Ogre::ColourValue(0.5f, 0.5f, 0.5f), 0.0, 10.f, 50000.f);
	mSceneMgr->setFog(FOG_EXP, Ogre::ColourValue(0.6f, 0.6f, 0.6f), 0.00003f);

	//������Ӱ��������
	mSceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);
	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.05f, 0.05f, 0.05f));

	//̫���⣨ƽ�й⣩
	Ogre::Light* sunlight = mSceneMgr->createLight("sunlight");
	sunlight->setType(Ogre::Light::LT_DIRECTIONAL);

	mSceneMgr->setSkyBox(true, "Examples/MorningSkyBox", 30000.f, true);

#ifdef _TERRAIN_SHOW_
	Ogre::Vector3 center;
	// ����
	Entity* entTerra1 = mSceneMgr->createEntity("entTerra1", "17002190PAN_2048.mesh");
	entTerra1->getSubEntity(1)->setVisible(boundaryVisble);
	entTerra1->getSubEntity(2)->setVisible(boundaryVisble);
	entTerra1->getSubEntity(3)->setVisible(boundaryVisble);
	entTerra1->getSubEntity(4)->setVisible(boundaryVisble);
	entTerra1->getSubEntity(5)->setVisible(!boundaryVisble);
	entTerra1->getSubEntity(6)->setVisible(!boundaryVisble);
	entTerra1->getSubEntity(7)->setVisible(!boundaryVisble);
	entTerra1->getSubEntity(8)->setVisible(!boundaryVisble);
	SceneNode* nodeTerraBase1 = mSceneMgr->createSceneNode("nodeTerraBase1");
	nodeTerraBase1->attachObject(entTerra1);
	nodeTerraBase1->setPosition(-entTerra1->getBoundingBox().getCenter());
	center = -entTerra1->getBoundingBox().getCenter();
	SceneNode* nodeTerra1 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeTerra1");
	nodeTerra1->addChild(nodeTerraBase1);
	nodeTerra1->setScale(0.06f, 0.14f, 0.06f);

	Entity* entTerra2 = mSceneMgr->createEntity("entTerra2", "18002190PAN_2048.mesh");
	entTerra2->getSubEntity(1)->setVisible(boundaryVisble);
	entTerra2->getSubEntity(2)->setVisible(boundaryVisble);
	entTerra2->getSubEntity(3)->setVisible(boundaryVisble);
	entTerra2->getSubEntity(4)->setVisible(boundaryVisble);
	entTerra2->getSubEntity(5)->setVisible(!boundaryVisble);
	entTerra2->getSubEntity(6)->setVisible(!boundaryVisble);
	entTerra2->getSubEntity(7)->setVisible(!boundaryVisble);
	entTerra2->getSubEntity(8)->setVisible(!boundaryVisble);
	SceneNode* nodeTerraBase2 = mSceneMgr->createSceneNode("nodeTerraBase2");
	nodeTerraBase2->attachObject(entTerra2);
	nodeTerraBase2->setPosition(center + Ogre::Vector3(0.f, 0.f, 6.05f));
	SceneNode* nodeTerra2 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeTerra2");
	nodeTerra2->addChild(nodeTerraBase2);
	nodeTerra2->setScale(0.06f, 0.14f, 0.06f);

// 	Entity* entTerra3 = mSceneMgr->createEntity("entTerra3", "17002200PAN_2048.mesh");
// 	entTerra3->getSubEntity(1)->setVisible(boundaryVisble);
// 	entTerra3->getSubEntity(2)->setVisible(boundaryVisble);
// 	entTerra3->getSubEntity(3)->setVisible(boundaryVisble);
// 	entTerra3->getSubEntity(4)->setVisible(boundaryVisble);
// 	entTerra3->getSubEntity(5)->setVisible(!boundaryVisble);
// 	entTerra3->getSubEntity(6)->setVisible(!boundaryVisble);
// 	entTerra3->getSubEntity(7)->setVisible(!boundaryVisble);
// 	entTerra3->getSubEntity(8)->setVisible(!boundaryVisble);
// 	SceneNode* nodeTerraBase3 = mSceneMgr->createSceneNode("nodeTerraBase3");
// 	nodeTerraBase3->attachObject(entTerra3);
// 	nodeTerraBase3->setPosition(center + Ogre::Vector3(-6.05f, 0.f, 0.f));
// 	SceneNode* nodeTerra3 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeTerra3");
// 	nodeTerra3->addChild(nodeTerraBase3);
// 	nodeTerra3->setScale(0.06f, 0.14f, 0.06f);
// 
// 	Entity* entTerra4 = mSceneMgr->createEntity("entTerra4", "18002200PAN_2048.mesh");
// 	entTerra4->getSubEntity(1)->setVisible(boundaryVisble);
// 	entTerra4->getSubEntity(2)->setVisible(boundaryVisble);
// 	entTerra4->getSubEntity(3)->setVisible(boundaryVisble);
// 	entTerra4->getSubEntity(4)->setVisible(boundaryVisble);
// 	entTerra4->getSubEntity(5)->setVisible(!boundaryVisble);
// 	entTerra4->getSubEntity(6)->setVisible(!boundaryVisble);
// 	entTerra4->getSubEntity(7)->setVisible(!boundaryVisble);
// 	entTerra4->getSubEntity(8)->setVisible(!boundaryVisble);
// 	SceneNode* nodeTerraBase4 = mSceneMgr->createSceneNode("nodeTerraBase4");
// 	nodeTerraBase4->attachObject(entTerra4);
// 	nodeTerraBase4->setPosition(center + Ogre::Vector3(-6.05f, 0.f, 6.05f));
// 	SceneNode* nodeTerra4 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeTerra4");
// 	nodeTerra4->addChild(nodeTerraBase4);
// 	nodeTerra4->setScale(0.06f, 0.14f, 0.06f);
	
	
#endif
	//nodeTerra->setVisible(false);

#ifdef _USE_TERRAIN_LIQUID_
// 	tliquid = new TerranLiquid;
// 	tliquid->setInputParas(mRoot, mSceneMgr, nodeTerra1, "entTerra1", -entTerra1->getBoundingBox().getCenter());
// 	tliquid->setHeight(-15.f, 3.8f * 2);
// 	tliquid->setDepthShallowOcean(-16.f);
// 	tliquid->setGridDensity(2.f);	// 2.f
// 	tliquid->generate();
// 	Ogre::Entity* entOs1 = mSceneMgr->createEntity("OceanPlane", "OceanMesh");
// 	SceneNode* nodeOs1 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanPlane");
// 	nodeOs1->attachObject(entOs1);

	Ogre::Entity* entOs1 = mSceneMgr->createEntity("OceanPlane1", "OceanGrid1.mesh");
	SceneNode* nodeOs1 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanPlane1");
	nodeOs1->attachObject(entOs1);
	Ogre::Entity* entOs2 = mSceneMgr->createEntity("OceanPlane2", "OceanGrid2.mesh");
	SceneNode* nodeOs2 = mSceneMgr->getRootSceneNode()->createChildSceneNode("nodeOceanPlane2");
	nodeOs2->attachObject(entOs2);
#endif
}

void COgreMain::destroyContent(void)
{
	mainCameraView.destroyViewport(mWindow);
	mainCameraView.destroyCameraNode(mSceneMgr);

#ifdef _USE_OGRE_WATER_
	delete mWater;
#endif
#ifdef _USE_HYDRAX_
	delete mHydrax;
#endif // _USE_HYDRAX_

#ifdef _USE_TERRAIN_LIQUID_
	delete tliquid;
#endif

	//Ogre::OverlayManager::getSingleton().destroy(mainOverlay);
	//mainOverlay = NULL;
}

bool COgreMain::keyPressed(const OIS::KeyEvent & e)
{
	//��Ⱦģʽ�л�
	if (e.key == OIS::KC_R)
	{
		Ogre::PolygonMode pm;
		pm = mainCameraView.getCamera()->getPolygonMode();

		if (pm == Ogre::PM_SOLID)
			mainCameraView.getCamera()->setPolygonMode(Ogre::PM_WIREFRAME);
		//else if (pm == Ogre::PM_WIREFRAME)
		//	mainCameraView.getCamera()->setPolygonMode(Ogre::PM_POINTS);
		else
			mainCameraView.getCamera()->setPolygonMode(Ogre::PM_SOLID);
	}

	// �ӵ�ģʽ�л�
	if (e.key == OIS::KC_1)
	{
		if (viewMode == 0)
		{
			mainCameraView.getCamera()->pitch(Ogre::Degree(3.f));
			mainCameraView.getCameraNode()->setPosition(Ogre::Vector3(40.f, 300.f, -165.f));
			mainCameraView.getCameraNode()->resetOrientation();
			mainCameraView.getCameraNode()->setDirection(Ogre::Vector3::NEGATIVE_UNIT_Y);

			viewMode = 1;
		}
		else if (viewMode == 1)
		{
			mainCameraView.getCameraNode()->setPosition(Ogre::Vector3(40.f, 100.f, -165.f));
			mainCameraView.getCameraNode()->resetOrientation();
			mainCameraView.getCameraNode()->setDirection(Ogre::Vector3::NEGATIVE_UNIT_Y);

			viewMode = 2;
		}
		else if (viewMode == 2)
		{
			mainCameraView.getCameraNode()->setPosition(Ogre::Vector3(0.f, 0.f, 0.f));
			mainCameraView.getCameraNode()->resetOrientation();
			mainCameraView.getCamera()->pitch(Ogre::Degree(-3.f));

			viewMode = 0;
		}
	}

	//�ӵ����
	if (e.key == OIS::KC_W)
	{
		cameraForward = true;
	}
	else if (e.key == OIS::KC_S)
	{
		cameraBackward = true;
	}
	else if (e.key == OIS::KC_A)
	{
		cameraLeft = true;
	}
	else if (e.key == OIS::KC_D)
	{
		cameraRight = true;
	}
	else if (e.key == OIS::KC_LEFT)
	{
		cameraLeftTurn = true;
	}
	else if (e.key == OIS::KC_RIGHT)
	{
		cameraRightTurn = true;
	}

#ifdef _TERRAIN_SHOW_
	// �л�������ʾ
	if (e.key == OIS::KC_I)
	{
		SceneNode* nodeTerra = mSceneMgr->getSceneNode("nodeTerra1");
		nodeTerra->flipVisibility(true);
		nodeTerra = mSceneMgr->getSceneNode("nodeTerra2");
		nodeTerra->flipVisibility(true);
	}
	// �л�FPS��ʾ
	if (e.key == OIS::KC_Y)
	{
		if (mainOverlay->isVisible())
			mainOverlay->hide();
		else
			mainOverlay->show();
	}
	// �л����ֺ����������ʾ
	if (e.key == OIS::KC_0)
	{
		for (size_t i = 0; i < 2; i++)
		{
			Entity* entTerra = mSceneMgr->getEntity("OceanPlane" + Ogre::StringConverter::toString(i + 1));
			SubEntity* subEntSw = entTerra->getSubEntity("SubmeshSw");
			SubEntity* subEntTr = entTerra->getSubEntity("SubmeshTr");
			SubEntity* subEntDp = entTerra->getSubEntity("SubmeshDp");

			if (gridShowIdx == 0)
			{
				if (subEntSw != NULL)  subEntSw->setVisible(true);
				if (subEntTr != NULL)  subEntTr->setVisible(false);
				if (subEntDp != NULL)  subEntDp->setVisible(false);
			}
			else if (gridShowIdx == 1)
			{
				if (subEntSw != NULL)  subEntSw->setVisible(false);
				if (subEntTr != NULL)  subEntTr->setVisible(false);
				if (subEntDp != NULL)  subEntDp->setVisible(true);
			}
			else if (gridShowIdx == 2)
			{
				if (subEntSw != NULL)  subEntSw->setVisible(false);
				if (subEntTr != NULL)  subEntTr->setVisible(true);
				if (subEntDp != NULL)  subEntDp->setVisible(false);
			}
			else
			{
				if (subEntSw != NULL)  subEntSw->setVisible(true);
				if (subEntTr != NULL)  subEntTr->setVisible(true);
				if (subEntDp != NULL)  subEntDp->setVisible(true);
			}
		}
		if (gridShowIdx <= 2)
			gridShowIdx++;
		else
			gridShowIdx = 0;
	}
#endif

	return true;
}

bool COgreMain::keyReleased(const OIS::KeyEvent & e)
{
	//�ӵ����
	if (e.key == OIS::KC_W)
	{
		cameraForward = false;
	}
	else if (e.key == OIS::KC_S)
	{
		cameraBackward = false;
	}
	else if (e.key == OIS::KC_A)
	{
		cameraLeft = false;
	}
	else if (e.key == OIS::KC_D)
	{
		cameraRight = false;
	}
	else if (e.key == OIS::KC_LEFT)
	{
		cameraLeftTurn = false;
	}
	else if (e.key == OIS::KC_RIGHT)
	{
		cameraRightTurn = false;
	}

	return true;
}

bool COgreMain::mouseMoved(const OIS::MouseEvent & e)
{
	//�ӵ����
	//mainCameraView.getCameraNode()->yaw(Ogre::Degree(-1.0f * (float)e.state.X.rel), Ogre::Node::TS_WORLD);
	//mainCameraView.getCameraNode()->pitch(Ogre::Degree(-1.0f * (float)e.state.Y.rel), Ogre::Node::TS_LOCAL);

	return true;
}

bool COgreMain::mousePressed(const OIS::MouseEvent & e, OIS::MouseButtonID id)
{
	return true;
}

bool COgreMain::mouseReleased(const OIS::MouseEvent & e, OIS::MouseButtonID id)
{
	return true;
}

