#include "stdafx.h"
#include "CameraController.h"

CameraController::CameraController(void)
	: camera(NULL), nodeCamera(NULL), parentNode(NULL), viewport(NULL), zorder(0)
{
}

CameraController::~CameraController(void)
{
}

void CameraController::setViewportRect(float left, float top, float width, float height, float wedge, float hedge, int zorder, Ogre::ColourValue color, bool overlayEnabled)
{
	this->left = left;
	this->top = top;
	this->width = width;
	this->height = height;
	this->edge_width = wedge;
	this->edge_height = hedge;

	this->zorder = zorder;
	this->color = color;
	this->overlayEnabled = overlayEnabled;
}

void CameraController::createCameraNode(Ogre::SceneManager* mSceneMgr, Ogre::String name, float nearDis, float farDis)
{
	//Ö÷ÉãÏñÍ·
	camera = mSceneMgr->createCamera(name);
	camera->setNearClipDistance(nearDis);
	camera->setFarClipDistance(farDis);

	nodeCamera = mSceneMgr->createSceneNode("node" + name);
	nodeCamera->attachObject(camera);
}

void CameraController::destroyCameraNode(Ogre::SceneManager* mSceneMgr)
{
	if (this->camera)
		mSceneMgr->destroyCamera(this->camera);
}

void CameraController::createViewport(Ogre::RenderWindow* mWindow)
{
	float vleft = left + edge_width;
	float vtop = top + edge_height;
	float vwidth = width - 2 * edge_width;
	float vheight = height - 2 * edge_height;

	if (mWindow->hasViewportWithZOrder(this->zorder))
		return;

	//viewport
	viewport = mWindow->addViewport(camera, zorder, vleft, vtop, vwidth, vheight);
	viewport->setBackgroundColour(color);
	viewport->setOverlaysEnabled(overlayEnabled);
	camera->setAspectRatio((Ogre::Real)viewport->getActualWidth() / (Ogre::Real)viewport->getActualHeight());
}

void CameraController::destroyViewport(Ogre::RenderWindow* mWindow)
{
	if (mWindow->hasViewportWithZOrder(this->zorder))
		mWindow->removeViewport(this->zorder);
}

Ogre::SceneNode* CameraController::getCameraNode(void)
{
	return this->nodeCamera;
}

Ogre::Camera* CameraController::getCamera(void)
{
	return this->camera;
}

void CameraController::setVisible(Ogre::RenderWindow* mWindow, bool flag)
{
	if (mWindow)
	{
		if (flag)
		{
			this->createViewport(mWindow);
			//overlayContainer->show();
		}
		else
		{
			this->destroyViewport(mWindow);
			//overlayContainer->hide();
		}
	}
}

bool CameraController::getVisible(Ogre::RenderWindow* mWindow)
{
	return mWindow->hasViewportWithZOrder(this->zorder);
}

bool CameraController::attachToParentNode(Ogre::SceneNode* node, Ogre::Vector3 offset)
{
	if (!node)
		return false;

	if (!nodeCamera)
		return false;

	nodeCamera->setPosition(offset);
	node->addChild(nodeCamera);
	parentNode = node;

	return true;
}

bool CameraController::dettachFromParentNode(void)
{
	if (!parentNode)
		return false;

	if (!nodeCamera)
		return false;

	nodeCamera->setPosition(Ogre::Vector3::ZERO);
	parentNode->removeChild(nodeCamera);
	parentNode = NULL;

	return true;
}
