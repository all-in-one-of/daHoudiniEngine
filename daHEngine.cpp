/**********************************************************************************************************************
 * THE OMEGA LIB PROJECT
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright 2010								Electronic Visualization Laboratory, University of Illinois at Chicago
 * Authors:
 *  Alessandro Febretti							febret@gmail.com
 *---------------------------------------------------------------------------------------------------------------------
 * Copyright (c) 2010, Electronic Visualization Laboratory, University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following
 * disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------------------------------------------------
 *	ohello2
 *		A slightly more complex version of ohello, implementing event handling and data sharing. ohello2 defines a
 *		new class, HelloServer, that reads mouse events and uses it to rotate the rendered object.
 *		HelloServer also implements SharedObject to synchronize rotation data across all nodes.
 *********************************************************************************************************************/
#include <omega.h>
#include <omegaGl.h>
// #include "enginesimplehostcpp.h"
#include "HAPI_CPP.h"
#include <stdlib.h>

using namespace omega;

#define ENSURE_SUCCESS(result) \
    if ((result) != HAPI_RESULT_SUCCESS) \
    { \
	ofmsg("failure at %1%:%2%", %__FILE__ %__LINE__); \
	ofmsg("%1%", %hapi::Failure::lastErrorMessage());\
	exit(1); \
    }

#define ENSURE_COOK_SUCCESS(result) \
    if ((result) != HAPI_STATE_READY) \
    { \
	ofmsg("failure at %1%:%2%", %__FILE__ %__LINE__); \
	ofmsg("%1%", %hapi::Failure::lastErrorMessage());\
	exit(1); \
    }


class HelloApplication;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HelloRenderPass: public RenderPass
{
public:
	HelloRenderPass(Renderer* client, HelloApplication* app): RenderPass(client, "HelloRenderPass"), myApplication(app) {}
	virtual void initialize();
	virtual void render(Renderer* client, const DrawContext& context);

private:
	HelloApplication* myApplication;

	Vector3s myNormals[6];
	Vector4i myFaces[6];
	Vector3s myVertices[8];
	Color myFaceColors[6];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HelloApplication: public EngineModule
{
public:
	HelloApplication(): EngineModule("HelloApplication") { enableSharedData(); }

	~HelloApplication() {
	    try
	    {
			omsg("about to clean up HAPI");
		    ENSURE_SUCCESS(HAPI_Cleanup());
			omsg("done HAPI");
	    }
	    catch (hapi::Failure &failure)
		{
			ofmsg("%1%", %failure.lastErrorMessage());
			throw;
	    }
	}

	virtual void initializeRenderer(Renderer* r)
	{
		r->addRenderPass(new HelloRenderPass(r, this));
	}

	float getYaw() { return myYaw; }
	float getPitch() { return myPitch; }

	virtual void handleEvent(const Event& evt);
	virtual void commitSharedData(SharedOStream& out);
	virtual void updateSharedData(SharedIStream& in);

private:
	float myYaw;
	float myPitch;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloRenderPass::initialize()
{
    try
    {
// 		load_asset_and_print_info("test_asset.otl");
		omsg("about to cookOptionsCreate");
	    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
		omsg("about to HAPI");

	    ENSURE_SUCCESS(HAPI_Initialize(
		/*otl_search_path=*/NULL,
		/*dso_search_path=*/NULL,
	        &cook_options,
		/*use_cooking_thread=*/true,
		/*cooking_thread_max_size=*/-1));

		omsg("about to read otl");
	    int library_id;
	    int asset_id;
	    if (HAPI_LoadAssetLibraryFromFile(
	            "test_asset.otl",
	            &library_id) != HAPI_RESULT_SUCCESS)
	    {
	        ofmsg("Could not load %1%", %"test_asset.otl");
	        exit(1);
	    }
    }
    catch (hapi::Failure &failure)
	{
		ofmsg("%1%", %failure.lastErrorMessage());
		throw;
    }
	RenderPass::initialize();

	// Initialize cube normals.
	myNormals[0] = Vector3s(-1, 0, 0);
	myNormals[1] = Vector3s(0, 1, 0);
	myNormals[2] = Vector3s(1, 0, 0);
	myNormals[3] = Vector3s(0, -1, 0);
	myNormals[4] = Vector3s(0, 0, 1);
	myNormals[5] = Vector3s(0, 0, -1);

	// Initialize cube face indices.
	myFaces[0] = Vector4i(0, 1, 2, 3);
	myFaces[1] = Vector4i(3, 2, 6, 7);
	myFaces[2] = Vector4i(7, 6, 5, 4);
	myFaces[3] = Vector4i(4, 5, 1, 0);
	myFaces[4] = Vector4i(5, 6, 2, 1);
	myFaces[5] = Vector4i(7, 4, 0, 3);

	// Initialize cube face colors.
	myFaceColors[0] = Color::Aqua;
	myFaceColors[1] = Color::Orange;
	myFaceColors[2] = Color::Olive;
	myFaceColors[3] = Color::Navy;
	myFaceColors[4] = Color::Red;
	myFaceColors[5] = Color::Yellow;

	// Setup cube vertex data
	float size = 0.2f;
	myVertices[0][0] = myVertices[1][0] = myVertices[2][0] = myVertices[3][0] = -size;
	myVertices[4][0] = myVertices[5][0] = myVertices[6][0] = myVertices[7][0] = size;
	myVertices[0][1] = myVertices[1][1] = myVertices[4][1] = myVertices[5][1] = -size;
	myVertices[2][1] = myVertices[3][1] = myVertices[6][1] = myVertices[7][1] = size;
	myVertices[0][2] = myVertices[3][2] = myVertices[4][2] = myVertices[7][2] = size;
	myVertices[1][2] = myVertices[2][2] = myVertices[5][2] = myVertices[6][2] = -size;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloRenderPass::render(Renderer* client, const DrawContext& context)
{
	if(context.task == DrawContext::SceneDrawTask)
	{
		client->getRenderer()->beginDraw3D(context);

		// Enable depth testing and lighting.
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);

		// Setup light.
		glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, Color(1.0, 1.0, 1.0).data());
		glLightfv(GL_LIGHT0, GL_POSITION, Vector3s(0.0f, 0.0f, 1.0f).data());

		// Draw a rotating teapot.
		glTranslatef(0, 2, -2);
		glRotatef(10, 1, 0, 0);
		glRotatef(myApplication->getYaw(), 0, 1, 0);
		glRotatef(myApplication->getPitch(), 1, 0, 0);

		// Draw a box
		for (int i = 0; i < 6; i++)
		{
			glBegin(GL_QUADS);
			glColor3fv(myFaceColors[i].data());
			glNormal3fv(myNormals[i].data());
			glVertex3fv(myVertices[myFaces[i][0]].data());
			glVertex3fv(myVertices[myFaces[i][1]].data());
			glVertex3fv(myVertices[myFaces[i][2]].data());
			glVertex3fv(myVertices[myFaces[i][3]].data());
			glEnd();
		}

		client->getRenderer()->endDraw();
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::handleEvent(const Event& evt)
{
	if(evt.getServiceType() == Service::Pointer)
	{
		// Normalize the mouse position using the total display resolution,
		// then multiply to get 180 degree rotations
		DisplaySystem* ds = getEngine()->getDisplaySystem();
		Vector2i resolution = ds->getDisplayConfig().getCanvasRect().size();
		myYaw = (evt.getPosition().x() / resolution[0]) * 180;
		myPitch = (evt.getPosition().y() / resolution[1]) * 180;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::commitSharedData(SharedOStream& out)
{
	out << myYaw << myPitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HelloApplication::updateSharedData(SharedIStream& in)
{
 	in >> myYaw >> myPitch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ApplicationBase entry point
int main(int argc, char** argv)
{
	Application<HelloApplication> app("ohello2");
//     try
//     {
// // 		load_asset_and_print_info("test_asset.otl");
// 		omsg("about to cookOptionsCreate");
// 	    HAPI_CookOptions cook_options = HAPI_CookOptions_Create();
// 		omsg("about to HAPI");
//
// 	    ENSURE_SUCCESS(HAPI_Initialize(
// 		/*otl_search_path=*/NULL,
// 		/*dso_search_path=*/NULL,
// 	        &cook_options,
// 		/*use_cooking_thread=*/true,
// 		/*cooking_thread_max_size=*/-1));
//
// 		omsg("about to read otl");
// 	    int library_id;
// 	    int asset_id;
// 	    if (HAPI_LoadAssetLibraryFromFile(
// 	            "test_asset.otl",
// 	            &library_id) != HAPI_RESULT_SUCCESS)
// 	    {
// 	        ofmsg("Could not load %1%", %"test_asset.otl");
// 	        exit(1);
// 	    }
// 		omsg("about to clean up HAPI");
// 	    ENSURE_SUCCESS(HAPI_Cleanup());
// 		omsg("done HAPI");
//     }
//     catch (hapi::Failure &failure)
// 	{
// 		ofmsg("%1%", %failure.lastErrorMessage());
// 		throw;
//     }
    return omain(app, argc, argv);
}
/*
#include <omega.h>
#include <omegaGl.h>

using namespace omega;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! Contains data for a single animation frame.
struct Frame: public ReferenceType
{
	Ref<ImageUtils::LoadImageAsyncTask> imageLoader;
	int frameNumber;
	//float deadline;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! omegalib module implementing a flipbook-style animation player.
class FlipbookPlayer: public EngineModule
{
public:
	//! Convenience method to create the module, register and initialize it.
	static FlipbookPlayer* createAndInitialize();

	FlipbookPlayer();

	//! Called for each frame.
	virtual void update(const UpdateContext& context);
	//! Called when a new renderer is being initialized (i.e. for a new gl window)
	virtual void initializeRenderer(Renderer* r);
	virtual void dispose();

	//! Start loading an animation.
	void load(const String& framePath, int totalFrames, int startFrame);
	void loadMultidir(const String& framePath, int totalFrames, int startFrame, int filesPerDir, int startDirIndex);

	//! Begin playing an animation.
	void play();
	//! Pause animation
	void pause();
	// Stop and rewind animation
	void stop();
	void loop(bool value) { myLoop = value; }
	void setCurrentFrameNumber(int frameNumber);

	//! Adds the next frame to the buffering queue.
	void queueNextFrame();
	//! Returns the number of consecutive frames that finished buffering and are ready to display.
	int getNumReadyFrames();
	int getNumBufferedFrames() { return myBufferedFrames.size(); }
	int getTotalFrames() { return myTotalFrames; }

	// Return the current frame.
	Frame* getCurrentFrame() { return myCurrentFrame; }

	//! Sets / gets the number of frames to keep in the buffer
	int getFramesToBuffer() { return myFramesToBuffer; }
	void setFramesToBuffer(int value) { myFramesToBuffer = value; }

	void setFrameTime(float value) { myFrameTime = value; }
	float getFrameTime() { return myFrameTime; }

	void setPlaybackBarVisible(bool value) { myPlaybackBarVisible = value; }
	bool isPlaybackBarVisible() { return myPlaybackBarVisible; }

	void setNumGpuBuffers(int num) { myNumGpuBuffers = num; }
	int getNumGpuBuffers() { return myNumGpuBuffers; }

private:
	//! Frame file name. Should contain a printf-style placeholder
	//! that will be replaced with the frame number.
	String myFramePath;

	//! Total frames for this animation
	int myTotalFrames;
	//! Start frame index for this animation
	int myStartFrame;
	//! When true use multiple subdirectories for movie segments.
	bool myIsMultidir;
	//! Number of files per directory
	int myFramesPerDir;
	int myStartDirIndex;

	//! Number of frames that we should keep in the loading queue.
	int myFramesToBuffer;

	//! Number of frames that should be fully loaded at any time during playback.
	//! If loaded frames go under this threshold, play goes into 'buffering mode' and will
	//! resume when enough frames have loaded.
	int myFrameBufferThreshold;

	//! Time each frame should be displayed, in seconds.
	float myFrameTime;

	//! Total playback time.
	float myPlaybackTime;

	//! When set to true playback will start again once over.
	bool myLoop;
	//! True when the animation is playing (even when buffering)
	bool myPlaying;
	//! True when the animation play is suspended because not enough frames are in the queue.
	bool myBuffering;
	//! True when we are loading new frames
	bool myLoading;

	//! Current frame.
	Ref<Frame> myCurrentFrame;
	float myCurrentFrameTime;
	int myCurrentFrameIndex;

	//! Buffer of frames that are loading or ar ready to view.
	List< Ref<Frame> > myBufferedFrames;

	bool myPlaybackBarVisible;

	int myNumGpuBuffers;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! Render pass for the flipbook player
class FlipbookRenderPass: public RenderPass
{
public:
	FlipbookRenderPass(FlipbookPlayer* player, Renderer* client):
		myPlayer(player), RenderPass(client, "FlipbookRenderPass"), myActiveTextureIndex(0) {}
	virtual ~FlipbookRenderPass() { omsg("~FlipbookRenderPass"); }
	//! Drawing happens here.
	virtual void render(Renderer* client, const DrawContext& context);

private:
	//! Draw a youtube-style playback bar. Useful for debugging buffering performance.
	void drawPlaybackBar(DrawInterface* di, int x, int y, int width, int height);

private:
	Ref<PixelData> myPixelBuffer[16];
	Ref<Texture> myFrameTexture;
	int myActiveTextureIndex;
	FlipbookPlayer* myPlayer;
	Ref<Frame> myLastFrame;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Python wrapper code.
#ifdef OMEGA_USE_PYTHON
#include "omega/PythonInterpreterWrapper.h"
BOOST_PYTHON_MODULE(flipbookPlayer)
{
	// FlipbookPlayer
	PYAPI_REF_BASE_CLASS(FlipbookPlayer)
		PYAPI_STATIC_REF_GETTER(FlipbookPlayer, createAndInitialize)
		PYAPI_METHOD(FlipbookPlayer, load)
		PYAPI_METHOD(FlipbookPlayer, loadMultidir)
		PYAPI_METHOD(FlipbookPlayer, play)
		PYAPI_METHOD(FlipbookPlayer, pause)
		PYAPI_METHOD(FlipbookPlayer, stop)
		PYAPI_METHOD(FlipbookPlayer, loop)
		PYAPI_METHOD(FlipbookPlayer, getFramesToBuffer)
		PYAPI_METHOD(FlipbookPlayer, setFramesToBuffer)
		PYAPI_METHOD(FlipbookPlayer, getFrameTime)
		PYAPI_METHOD(FlipbookPlayer, setFrameTime)
		PYAPI_METHOD(FlipbookPlayer, setCurrentFrameNumber)
		PYAPI_METHOD(FlipbookPlayer, setPlaybackBarVisible)
		PYAPI_METHOD(FlipbookPlayer, isPlaybackBarVisible)
		PYAPI_METHOD(FlipbookPlayer, getNumGpuBuffers)
		PYAPI_METHOD(FlipbookPlayer, setNumGpuBuffers)
		;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
FlipbookPlayer* FlipbookPlayer::createAndInitialize()
{
	// Initialize and register the flipbook player module.
	FlipbookPlayer* instance = new FlipbookPlayer();
	ModuleServices::addModule(instance);
	instance->doInitialize(Engine::instance());
	return instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
FlipbookPlayer::FlipbookPlayer():
	EngineModule("FlipbookPlayer"),
	myPlaying(false),
	myPlaybackBarVisible(true),
	myLoop(false)
{
	myFramesToBuffer = 100;
	myFrameTime = 0.2f;
	myNumGpuBuffers = 2;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::load(const String& filePath, int totalFrames, int startFrame)
{
	myFramePath = filePath;
	myTotalFrames = totalFrames;
	myStartFrame = startFrame;
	myLoading = true;
	myIsMultidir = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::loadMultidir(const String& filePath, int totalFrames, int startFrame, int framesPerDir, int startDirIndex)
{
	myFramePath = filePath;
	myTotalFrames = totalFrames;
	myStartFrame = startFrame;
	myLoading = true;
	myIsMultidir = true;
	myFramesPerDir = framesPerDir;
	myStartDirIndex = startDirIndex;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::play()
{
	myPlaying = true;
	myBuffering = true;
	myCurrentFrame = NULL;
	myPlaybackTime = 0;

	// The initial buffering threshold is half the maximum queued frames.
	myFrameBufferThreshold = myFramesToBuffer / 2;
	myCurrentFrameTime = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::pause()
{
	myPlaying = false;
	myCurrentFrame = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::stop()
{
	myPlaying = false;
	myLoading = true;
	myBufferedFrames.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::setCurrentFrameNumber(int frameNumber)
{
	// not implemented.... would be fairly complex given way playback is currently implemented.
	// Need to rethink playback frame timing & buffering first.
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::update(const UpdateContext& context)
{
	if(myLoading)
	{
		// Do we need to queue more frames?
		while(myBufferedFrames.size() < myFramesToBuffer && myLoading) queueNextFrame();
	}
	if(myBuffering)
	{
		if(!myLoading || getNumReadyFrames() >= myFrameBufferThreshold)
		{
			// If we are out here it means that we buffered enough frames, or there are no
			// more frames we can possibly buffer.
			myBuffering = false;
			// Reduce frame buffer threshold to 75% so we don't keep switching back and forth
			// in buffering
			myFrameBufferThreshold *= 0.75;
		}
		else
		{
			// If we are in buffering mode, wait until we have enough frames buffered
			while(myLoading && getNumReadyFrames() < myFrameBufferThreshold)
			{
				osleep(100);
				// Do we need to queue more frames?
				while(myBufferedFrames.size() < myFramesToBuffer && myLoading) queueNextFrame();
			}
		}
	}
	else if(myPlaying)
	{
		// If the current frame is still valid, keep displaying it...
		if(myCurrentFrame.isNull() || myCurrentFrameTime >= myFrameTime)
		{
			if(myBufferedFrames.size() == 0)
			{
					myCurrentFrame = NULL;
			}
			else
			{
				myCurrentFrame = myBufferedFrames.front();
				myBufferedFrames.pop_front();
				myCurrentFrameTime = 0;
				myCurrentFrameIndex = myCurrentFrame->frameNumber;
			}
		}

		//myPlaybackTime += context.dt;
		//ofmsg("dt %1%", %context.dt);

		if(!myCurrentFrame.isNull())
		{
			// Increment the playback time
			myPlaybackTime += context.dt;
			myCurrentFrameTime += context.dt;
		}
		else
		{
			omsg("Finished playback");
			myPlaying = false;
			if(myLoop)
			{
				stop();
				play();
			}
		}

		// If we dont have enough frames left to continue playback, switch to buffering mode and increase
		// buffering thresholds.
		int readyFrames = getNumReadyFrames();
		if(myLoading && readyFrames < myFrameBufferThreshold)
		{
			myBuffering = true;
			myFrameBufferThreshold *= 2;
			myFramesToBuffer *= 2;
			ofmsg("BUFFERING: ready frames: %1%    frames to buffer:%2%    frames to queue:%3%", %readyFrames %myFrameBufferThreshold %myFramesToBuffer);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::queueNextFrame()
{
	// Get the last buffered frame number
	int lastBufferedFrame = -1;
	if(myBufferedFrames.size() > 0) lastBufferedFrame = myBufferedFrames.back()->frameNumber;

	// Increment it, to get the frame number for the next frame.
	lastBufferedFrame++;

	// Do we still have frames to load?
	if(lastBufferedFrame < myTotalFrames)
	{
		// Create a new frame object to hold the frame.
		Frame* nextFrame = new Frame();
		nextFrame->frameNumber = lastBufferedFrame;

		// Compute the time at which this frame should be displayed (useful for dropping frames)
		//if(nextFrame->frameNumber == 0) nextFrame->deadline = 0;
		//else nextFrame->deadline = myBufferedFrames.back()->deadline + myFrameTime;

		int nextFrameIndex = nextFrame->frameNumber + myStartFrame;

		// Get the path to the frame image file.
		String framePath = "";
		if(myIsMultidir)
		{
			int dirIndex = myStartDirIndex + nextFrameIndex / myFramesPerDir;
			framePath = ostr(myFramePath, %dirIndex %nextFrameIndex);
		}
		else
		{
			framePath = ostr(myFramePath, %nextFrameIndex);
		}

		// Load the frame image asynchronously. This call queues the image loading and
		// returns a handle to check on the image load status.
		nextFrame->imageLoader = ImageUtils::loadImageAsync(framePath);

		// Add the frame to the list of currently buffering frames.
		myBufferedFrames.push_back(nextFrame);
	}
	else
	{
		omsg("Finished loading movie frames");
		myLoading = false;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int FlipbookPlayer::getNumReadyFrames()
{
	// Count the number of consecutive frames whose image data has been loaded.
	int readyFrames = 0;
	foreach(Frame* frame, myBufferedFrames)
	{
		if(frame->imageLoader->isComplete()) readyFrames++;
		else break;
	}
	return readyFrames;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::initializeRenderer(Renderer* r)
{
	// Add our custom render pass to the renderer.
	r->addRenderPass(new FlipbookRenderPass(this, r));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookPlayer::dispose()
{
	getEngine()->removeRenderPass("FlipbookRenderPass");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookRenderPass::render(Renderer* renderer, const DrawContext& context)
{
	if(context.task == DrawContext::SceneDrawTask)
	{
		// Get the current frame data from the player
		Frame* frame = myPlayer->getCurrentFrame();
		if(frame != NULL)
		{
			PixelData* pixels = frame->imageLoader->getData().image;

			// If we did not initialize a texture for rendering, let's do it now.
			if(myFrameTexture == NULL)
			{
				myFrameTexture = renderer->createTexture();
				myFrameTexture->initialize(pixels->getWidth(), pixels->getHeight());
			}

			if(myPixelBuffer[myActiveTextureIndex] == NULL)
			{
				myPixelBuffer[myActiveTextureIndex] = new PixelData(pixels->getFormat(), pixels->getWidth(), pixels->getHeight(), NULL, PixelData::PixelBufferObject);
			}

			if(frame != myLastFrame)
			{
				int nextIndex = myActiveTextureIndex + 1;
				if(nextIndex == myPlayer->getNumGpuBuffers()) nextIndex = 0;

				myFrameTexture->writePixels(myPixelBuffer[nextIndex]);

				// Copy the frame pixels to the texture
				myPixelBuffer[myActiveTextureIndex]->copyFrom(pixels);

				myLastFrame = frame;

				// Swap textures
				myActiveTextureIndex = nextIndex;
			}
		}

		if(myFrameTexture != NULL)
		{
			// Draw the current frame.
			//if(context.task == DrawContext::OverlayDrawTask && context.eye == DrawContext::EyeCyclop)
			{
				DrawInterface* di = renderer->getRenderer();
				di->beginDraw2D(context);
				glColor4f(1, 1, 1, 1);

				const DisplayTileConfig* tile = context.tile;
				float cx = tile->offset.x();
				float cy = tile->offset.y();
				float sx = tile->pixelSize.x();
				float sy = tile->pixelSize.y();

				di->drawRectTexture(myFrameTexture, Vector2f(cx, cy), Vector2f(sx, sy));

				if(myPlayer->isPlaybackBarVisible()) drawPlaybackBar(di, cx + 5, cy + sy - 25, sx - 10, 20);

				di->endDraw();
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void FlipbookRenderPass::drawPlaybackBar(DrawInterface* di, int x, int y, int width, int height)
{
	Frame* frame = myPlayer->getCurrentFrame();

	int totalFrames = myPlayer->getTotalFrames();
	int currentFrame = (frame != NULL ? frame->frameNumber : 0);
	int bufferedFrames = myPlayer->getNumBufferedFrames() + currentFrame;
	int readyFrames = myPlayer->getNumReadyFrames() + currentFrame;

	int bufferedFramesLength = width * bufferedFrames / totalFrames;
	int readyFramesLength = width * readyFrames / totalFrames;
	int currentFrameLength = width * currentFrame / totalFrames;

	Color borderColor = Color(1, 1, 1);
	Color backColor = Color(0, 0, 0, 0.8f);
	Color currentFrameColor = Color(1, 0.2f, 0.2f);
	Color bufferedFramesColor = Color(0.3f, 0.3f, 0.3f);
	Color readyFramesColor = Color(0.5f, 0.5f, 0.5f);

	di->drawRect(Vector2f(x, y), Vector2f(width, height), backColor);
	di->drawRect(Vector2f(x, y), Vector2f(bufferedFramesLength, height), bufferedFramesColor);
	di->drawRect(Vector2f(x, y), Vector2f(readyFramesLength, height), readyFramesColor);
	di->drawRect(Vector2f(x, y), Vector2f(currentFrameLength, height), currentFrameColor);
	di->drawRectOutline(Vector2f(x, y), Vector2f(width, height), borderColor);
}
*/