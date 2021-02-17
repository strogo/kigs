#ifndef _TEXTUREHANDLER_H_
#define _TEXTUREHANDLER_H_

#include "Drawable.h"
#include "TinyImage.h"
#include "TecLibs/2D/BBox2DI.h"
#include "Upgrador.h"
#include "Texture.h"


// ****************************************
// * TextureHandler class
// * --------------------------------------
/**
* \file	TextureHandler.h
* \class	TextureHandler
* \ingroup Renderer
* \brief handle texture drawing. A TextureHandler is used to manipulate a texture or animated texture or sprite in a texture the same way
*
*/
// ****************************************
class TextureHandler : public CoreModifiable
{
public:
	friend class AnimationUpgrador;
	DECLARE_CLASS_INFO(TextureHandler, CoreModifiable,Renderer)

	/**
	* \brief	constructor
	* \fn 		TextureHandler(const kstl::string& name,DECLARE_CLASS_NAME_TREE_ARG);
	* \param	name : instance name
	* \param	DECLARE_CLASS_NAME_TREE_ARG : list of arguments
	*/
	TextureHandler(const kstl::string& name,DECLARE_CLASS_NAME_TREE_ARG);

	// access to texture methods
	void SetRepeatUV(bool RU, bool RV) 
	{
		if (mTexture)
		{
			mTexture->SetRepeatUV(RU, RV);
		}
	}

	void	DoPreDraw(TravState* st)
	{
		if (mTexture)
		{
			mTexture->DoPreDraw(st);
		}
	}
	void	DoPostDraw(TravState* st)
	{
		if (mTexture)
		{
			mTexture->DoPostDraw(st);
		}
	}

	void GetSize(unsigned int& width, unsigned int& height)
	{
		width = mSize.x;
		height = mSize.y;
	}


	void GetSize(float& width, float& height)
	{
		width = mSize.x;
		height = mSize.y;
	}

	void GetPow2Size(unsigned int& width, unsigned int& height)
	{
		if (mTexture)
		{
			mTexture->GetPow2Size(width, height);
		}
	}
	void GetRatio(kfloat& rX, kfloat& rY) 
	{
		if (mTexture)
		{
			mTexture->GetRatio(rX, rY);
		}
	}
	int GetTransparency() 
	{ 
		if (mTexture)
		{
			return mTexture->GetTransparency();
		}
		return 0;
	}

	void SetFlag(unsigned int flag)
	{
		if (mTexture)
		{
			mTexture->SetFlag(flag);
		}
	}

	const v2f* getUVs() const
	{
		return mUV;
	}

	SP<Texture>	GetEmptyTexture(const std::string& name="");
	// use mTextureName to load texture
	void	changeTexture();

	void	refreshSizeAndUVs();
protected:

	/**
	* \brief	initialize modifiable
	* \fn		virtual	void	InitModifiable();
	*/
	void	InitModifiable() override;

	/**
	* \brief	this method is called to notify this that one of its attribute has changed.
	* \fn 		virtual void NotifyUpdate(const unsigned int);
	* \param	int : not used
	*/
	void NotifyUpdate(const unsigned int /* labelid */) override;

	/**
	* \brief	destructor
	* \fn 		~TextureHandler();
	*/
	virtual ~TextureHandler();

	SP<Texture>	mTexture = nullptr;

	maString mTextureName = BASE_ATTRIBUTE(TextureName, "");

	v2f mUV[4];

	v2f mSize;

	void	clearSpritesheetAndAnimationData()
	{
	}

	void	initFromSpriteSheet(const std::string& jsonfilename);
	void	initFromPicture(const std::string& picfilename);
	void	setCurrentFrame(const SpriteSheetFrameData* ssf);

	bool	mIsSpriteSheet = false;
};


class 	AnimationUpgrador : public Upgrador<TextureHandler>
{
protected:
	// create and init Upgrador if needed and add dynamic attributes
	virtual void	Init(CoreModifiable* toUpgrade) override;

	// destroy UpgradorData and remove dynamic attributes 
	virtual void	Destroy(CoreModifiable* toDowngrade) override;

	START_UPGRADOR(AnimationUpgrador);

	UPGRADOR_METHODS(Play, AnimationNotifyUpdate);

	void	Update(const Timer& _timer, TextureHandler* parent);
	void	NotifyUpdate(const unsigned int /* labelid */, TextureHandler* parent);

public:

	maString*			mCurrentAnimation=nullptr;
	maUInt*				mFramePerSecond = nullptr;
	maBool*				mLoop = nullptr;
	bool				mWasdAutoUpdate = false;
	unsigned int		mCurrentFrame = 0;
	double				mElpasedTime = 0.0;
	unsigned int		mFrameNumber = 0;

protected:
	
};

#endif //_TEXTUREHANDLER_H_
