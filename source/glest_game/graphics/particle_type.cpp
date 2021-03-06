// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2010-2010 Titus Tscharntke
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#include "particle_type.h"
#include "unit_particle_type.h"

#include "util.h"
#include "core_data.h"
#include "xml_parser.h"
#include "model.h"
#include "config.h"
#include "game_constants.h"
#include "util.h"
#include "platform_common.h"
#include "conversion.h"
#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Shared::PlatformCommon;

namespace Glest{ namespace Game{

// =====================================================
// 	class ParticleSystemType
// =====================================================
const bool checkMemory = false;
static map<void *,int> memoryObjectList;

ParticleSystemType::ParticleSystemType() {
	if(checkMemory) {
		printf("++ Create ParticleSystemType [%p]\n",this);
		memoryObjectList[this]++;
	}

	teamcolorNoEnergy=false;
	teamcolorEnergy=false;
	alternations=0;
	particleSystemStartDelay=0;
	texture=NULL;
	model=NULL;
    minmaxEnabled=false;
    minHp=0;
    maxHp=0;
    minmaxIsPercent=false;

    modelCycle=0;
    size=0;
    sizeNoEnergy=0;
    speed=0;
	gravity=0;
	emissionRate=0;
	energyMax=0;
	energyVar=0;

}

ParticleSystemType::ParticleSystemType(const ParticleSystemType &src) {
	if(checkMemory) {
		printf("++ Create ParticleSystemType #2 [%p]\n",this);
		memoryObjectList[this]++;
	}

	copyAll(src);
}

ParticleSystemType & ParticleSystemType::operator=(const ParticleSystemType &src) {
	if(checkMemory) {
		printf("++ Create ParticleSystemType #3 [%p]\n",this);
		memoryObjectList[this]++;
	}

	copyAll(src);
	return *this;
}

ParticleSystemType::~ParticleSystemType() {
	if(checkMemory) {
		printf("-- Delete ParticleSystemType [%p] type = [%s]\n",this,type.c_str());
		memoryObjectList[this]--;
		assert(memoryObjectList[this] == 0);
	}
	for(Children::iterator it = children.begin(); it != children.end(); ++it) {
		delete *it;
	}
	children.clear();
}

void ParticleSystemType::copyAll(const ParticleSystemType &src) {
	this->type				= src.type;
	this->texture			= src.texture;
	this->model				= src.model;
	this->modelCycle		= src.modelCycle;
	this->primitive			= src.primitive;
	this->offset			= src.offset;
	this->color				= src.color;
	this->colorNoEnergy		= src.colorNoEnergy;
	this->size				= src.size;
	this->sizeNoEnergy		= src.sizeNoEnergy;
	this->speed				= src.speed;
	this->gravity			= src.gravity;
	this->emissionRate		= src.emissionRate;
	this->energyMax			= src.energyMax;
	this->energyVar			= src.energyVar;
	this->mode				= src.mode;
	this->teamcolorNoEnergy	= src.teamcolorNoEnergy;
	this->teamcolorEnergy	= src.teamcolorEnergy;
	this->alternations		= src.alternations;
	this->particleSystemStartDelay= src.particleSystemStartDelay;
	for(Children::iterator it = children.begin(); it != children.end(); ++it) {
		UnitParticleSystemType *child = *it;

		// Deep copy the child particles
		UnitParticleSystemType *newCopy = new UnitParticleSystemType();
		*newCopy = *child;
		children.push_back(newCopy);
	}

	this->minmaxEnabled		= src.minmaxEnabled;
	this->minHp				= src.minHp;
	this->maxHp				= src.maxHp;
	this->minmaxIsPercent	= src.minmaxIsPercent;
}

void ParticleSystemType::load(const XmlNode *particleSystemNode, const string &dir,
		RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader, string techtreePath) {

	//texture
	const XmlNode *textureNode= particleSystemNode->getChild("texture");
	bool textureEnabled= textureNode->getAttribute("value")->getBoolValue();

	if(textureEnabled){
		texture= renderer->newTexture2D(rsGame);
		if(texture) {
			if(textureNode->getAttribute("luminance")->getBoolValue()){
				texture->setFormat(Texture::fAlpha);
				texture->getPixmap()->init(1);
			}
			else{
				texture->getPixmap()->init(4);
			}
		}
		string currentPath = dir;
		endPathWithSlash(currentPath);
		if(texture) {
			texture->load(textureNode->getAttribute("path")->getRestrictedValue(currentPath));
		}
		loadedFileList[textureNode->getAttribute("path")->getRestrictedValue(currentPath)].push_back(make_pair(parentLoader,textureNode->getAttribute("path")->getRestrictedValue()));
	}
	else {
		texture= NULL;
	}
	
	//model
	if(particleSystemNode->hasChild("model")){
		const XmlNode *modelNode= particleSystemNode->getChild("model");
		bool modelEnabled= modelNode->getAttribute("value")->getBoolValue();
		if(modelEnabled) {
			string currentPath = dir;
			endPathWithSlash(currentPath);

			string path= modelNode->getAttribute("path")->getRestrictedValue(currentPath);
			model= renderer->newModel(rsGame,path, false, &loadedFileList, &parentLoader);
			loadedFileList[path].push_back(make_pair(parentLoader,modelNode->getAttribute("path")->getRestrictedValue()));
			
			if(modelNode->hasChild("cycles")) {
				modelCycle = modelNode->getChild("cycles")->getAttribute("value")->getFloatValue();
				if(modelCycle < 0.0)
					throw megaglest_runtime_error("negative model cycle value is bad");
			}
		}
	}
	else {
		model= NULL;
	}

	//primitive
	const XmlNode *primitiveNode= particleSystemNode->getChild("primitive");
	primitive= primitiveNode->getAttribute("value")->getRestrictedValue();

	//offset
	const XmlNode *offsetNode= particleSystemNode->getChild("offset");
	offset.x= offsetNode->getAttribute("x")->getFloatValue();
	offset.y= offsetNode->getAttribute("y")->getFloatValue();
	offset.z= offsetNode->getAttribute("z")->getFloatValue();

	//color
	const XmlNode *colorNode= particleSystemNode->getChild("color");
	color.x= colorNode->getAttribute("red")->getFloatValue(0.f, 1.0f);
	color.y= colorNode->getAttribute("green")->getFloatValue(0.f, 1.0f);
	color.z= colorNode->getAttribute("blue")->getFloatValue(0.f, 1.0f);
	color.w= colorNode->getAttribute("alpha")->getFloatValue(0.f, 1.0f);

	//color
	const XmlNode *colorNoEnergyNode= particleSystemNode->getChild("color-no-energy");
	colorNoEnergy.x= colorNoEnergyNode->getAttribute("red")->getFloatValue(0.f, 1.0f);
	colorNoEnergy.y= colorNoEnergyNode->getAttribute("green")->getFloatValue(0.f, 1.0f);
	colorNoEnergy.z= colorNoEnergyNode->getAttribute("blue")->getFloatValue(0.f, 1.0f);
	colorNoEnergy.w= colorNoEnergyNode->getAttribute("alpha")->getFloatValue(0.f, 1.0f);

	//size
	const XmlNode *sizeNode= particleSystemNode->getChild("size");
	size= sizeNode->getAttribute("value")->getFloatValue();

	//sizeNoEnergy
	const XmlNode *sizeNoEnergyNode= particleSystemNode->getChild("size-no-energy");
	sizeNoEnergy= sizeNoEnergyNode->getAttribute("value")->getFloatValue();

	//speed
	const XmlNode *speedNode= particleSystemNode->getChild("speed");
	speed= speedNode->getAttribute("value")->getFloatValue()/GameConstants::updateFps;

	//gravity
	const XmlNode *gravityNode= particleSystemNode->getChild("gravity");
	gravity= gravityNode->getAttribute("value")->getFloatValue()/GameConstants::updateFps;

	//emission rate
	const XmlNode *emissionRateNode= particleSystemNode->getChild("emission-rate");
	emissionRate= emissionRateNode->getAttribute("value")->getFloatValue();

	//energy max
	const XmlNode *energyMaxNode= particleSystemNode->getChild("energy-max");
	energyMax= energyMaxNode->getAttribute("value")->getIntValue();

	//speed
	const XmlNode *energyVarNode= particleSystemNode->getChild("energy-var");
	energyVar= energyVarNode->getAttribute("value")->getIntValue();
	
	//teamcolorNoEnergy
    if(particleSystemNode->hasChild("teamcolorNoEnergy")){
    	const XmlNode *teamcolorNoEnergyNode= particleSystemNode->getChild("teamcolorNoEnergy");
    	teamcolorNoEnergy= teamcolorNoEnergyNode->getAttribute("value")->getBoolValue();
    }
    //teamcolorEnergy
    if(particleSystemNode->hasChild("teamcolorEnergy")){
    	const XmlNode *teamcolorEnergyNode= particleSystemNode->getChild("teamcolorEnergy");
    	teamcolorEnergy= teamcolorEnergyNode->getAttribute("value")->getBoolValue();
    }
    //alternations
	if(particleSystemNode->hasChild("alternations")){
		const XmlNode *alternatingNode= particleSystemNode->getChild("alternations");
		alternations= alternatingNode->getAttribute("value")->getIntValue();
	}
    //particleSystemStartDelay
	if(particleSystemNode->hasChild("particleSystemStartDelay")){
		const XmlNode *node= particleSystemNode->getChild("particleSystemStartDelay");
		particleSystemStartDelay= node->getAttribute("value")->getIntValue();
	}
	//mode
	if(particleSystemNode->hasChild("mode")) {
		const XmlNode *modeNode= particleSystemNode->getChild("mode");
    	mode= modeNode->getAttribute("value")->getRestrictedValue();
	}
	else {
		mode="normal";
	}

	// child particles
	if(particleSystemNode->hasChild("child-particles")) {
    		const XmlNode *childrenNode= particleSystemNode->getChild("child-particles");
    		if(childrenNode->getAttribute("value")->getBoolValue()) {
			for(unsigned int i = 0; i < childrenNode->getChildCount(); ++i) {
				const XmlNode *particleFileNode= childrenNode->getChild("particle-file",i);
				string path= particleFileNode->getAttribute("path")->getRestrictedValue();
				UnitParticleSystemType *unitParticleSystemType= new UnitParticleSystemType();
				string childPath= dir;
				endPathWithSlash(childPath);
				childPath += path;
				string childDir = extractDirectoryPathFromFile(childPath);
				unitParticleSystemType->load(particleFileNode,childDir,childPath,renderer,loadedFileList,parentLoader,techtreePath);
				loadedFileList[childPath].push_back(make_pair(parentLoader,path));
				children.push_back(unitParticleSystemType);
			}
		}
	}
}

void ParticleSystemType::setValues(AttackParticleSystem *ats){
	// add instances of all children; some settings will cascade to all children
	for(Children::iterator i=children.begin(); i!=children.end(); ++i){
		UnitParticleSystem *child = new UnitParticleSystem();
		child->setParticleOwner(ats->getParticleOwner());
		(*i)->setValues(child);
		ats->addChild(child);
		child->setState(ParticleSystem::sPlay);
	}
	ats->setTexture(texture);
	ats->setPrimitive(AttackParticleSystem::strToPrimitive(primitive));
	ats->setOffset(offset);
	ats->setColor(color);
	ats->setColorNoEnergy(colorNoEnergy);
	ats->setSpeed(speed);
	ats->setGravity(gravity);
	ats->setParticleSize(size);
	ats->setSizeNoEnergy(sizeNoEnergy);
	ats->setEmissionRate(emissionRate);
	ats->setMaxParticleEnergy(energyMax);
	ats->setVarParticleEnergy(energyVar);
	ats->setModel(model);
	ats->setModelCycle(modelCycle);
	ats->setTeamcolorNoEnergy(teamcolorNoEnergy);
    ats->setTeamcolorEnergy(teamcolorEnergy);
    ats->setAlternations(alternations);
    ats->setParticleSystemStartDelay(particleSystemStartDelay);
	ats->setBlendMode(ParticleSystem::strToBlendMode(mode));
}

void ParticleSystemType::loadGame(const XmlNode *rootNode) {
	const XmlNode *particleSystemTypeNode = rootNode->getChild("ParticleSystemType");

	type = particleSystemTypeNode->getAttribute("type")->getIntValue();

	modelCycle = particleSystemTypeNode->getAttribute("modelCycle")->getFloatValue();
	primitive = particleSystemTypeNode->getAttribute("primitive")->getValue();
	offset = Vec3f::strToVec3(particleSystemTypeNode->getAttribute("offset")->getValue());
	color = Vec4f::strToVec4(particleSystemTypeNode->getAttribute("color")->getValue());
	colorNoEnergy = Vec4f::strToVec4(particleSystemTypeNode->getAttribute("colorNoEnergy")->getValue());
	size = particleSystemTypeNode->getAttribute("size")->getFloatValue();
	sizeNoEnergy = particleSystemTypeNode->getAttribute("sizeNoEnergy")->getFloatValue();
	speed = particleSystemTypeNode->getAttribute("speed")->getFloatValue();
	gravity = particleSystemTypeNode->getAttribute("gravity")->getFloatValue();
	emissionRate = particleSystemTypeNode->getAttribute("emissionRate")->getFloatValue();
	energyMax = particleSystemTypeNode->getAttribute("energyMax")->getIntValue();
	energyVar = particleSystemTypeNode->getAttribute("energyVar")->getIntValue();
	mode = particleSystemTypeNode->getAttribute("mode")->getValue();
	teamcolorNoEnergy = (particleSystemTypeNode->getAttribute("teamcolorNoEnergy")->getIntValue() != 0);
	teamcolorEnergy = (particleSystemTypeNode->getAttribute("teamcolorEnergy")->getIntValue() != 0);
	alternations = particleSystemTypeNode->getAttribute("alternations")->getIntValue();
	particleSystemStartDelay = particleSystemTypeNode->getAttribute("particleSystemStartDelay")->getIntValue();

	if(particleSystemTypeNode->hasChild("UnitParticleSystemType")) {
		vector<XmlNode *> particleSystemTypeNodeList = particleSystemTypeNode->getChildList("UnitParticleSystemType");
		for(unsigned int i = 0; i < particleSystemTypeNodeList.size(); ++i) {
			XmlNode *node = particleSystemTypeNodeList[i];

			UnitParticleSystemType *child = new UnitParticleSystemType();
			child->loadGame(node);
			children.push_back(child);
		}
	}

	minmaxEnabled = (particleSystemTypeNode->getAttribute("minmaxEnabled")->getIntValue() != 0);
	minHp = particleSystemTypeNode->getAttribute("minHp")->getIntValue();
	maxHp = particleSystemTypeNode->getAttribute("maxHp")->getIntValue();
	minmaxIsPercent = (particleSystemTypeNode->getAttribute("minmaxIsPercent")->getIntValue() != 0);
}

void ParticleSystemType::saveGame(XmlNode *rootNode) {
	std::map<string,string> mapTagReplacements;
	XmlNode *particleSystemTypeNode = rootNode->addChild("ParticleSystemType");

//	string type;
	particleSystemTypeNode->addAttribute("type",type, mapTagReplacements);
//	Texture2D *texture;
//	Model *model;
//	float modelCycle;
	particleSystemTypeNode->addAttribute("modelCycle",floatToStr(modelCycle,6), mapTagReplacements);
//	string primitive;
	particleSystemTypeNode->addAttribute("primitive",primitive, mapTagReplacements);
//	Vec3f offset;
	particleSystemTypeNode->addAttribute("offset",offset.getString(), mapTagReplacements);
//	Vec4f color;
	particleSystemTypeNode->addAttribute("color",color.getString(), mapTagReplacements);
//	Vec4f colorNoEnergy;
	particleSystemTypeNode->addAttribute("colorNoEnergy",colorNoEnergy.getString(), mapTagReplacements);
//	float size;
	particleSystemTypeNode->addAttribute("size",floatToStr(size,6), mapTagReplacements);
//	float sizeNoEnergy;
	particleSystemTypeNode->addAttribute("sizeNoEnergy",floatToStr(sizeNoEnergy,6), mapTagReplacements);
//	float speed;
	particleSystemTypeNode->addAttribute("speed",floatToStr(speed,6), mapTagReplacements);
//	float gravity;
	particleSystemTypeNode->addAttribute("gravity",floatToStr(gravity,6), mapTagReplacements);
//	float emissionRate;
	particleSystemTypeNode->addAttribute("emissionRate",floatToStr(emissionRate,6), mapTagReplacements);
//	int energyMax;
	particleSystemTypeNode->addAttribute("energyMax",intToStr(energyMax), mapTagReplacements);
//	int energyVar;
	particleSystemTypeNode->addAttribute("energyVar",intToStr(energyVar), mapTagReplacements);
//	string mode;
	particleSystemTypeNode->addAttribute("mode",mode, mapTagReplacements);
//	bool teamcolorNoEnergy;
	particleSystemTypeNode->addAttribute("teamcolorNoEnergy",intToStr(teamcolorNoEnergy), mapTagReplacements);
//    bool teamcolorEnergy;
	particleSystemTypeNode->addAttribute("teamcolorEnergy",intToStr(teamcolorEnergy), mapTagReplacements);
//    int alternations;
	particleSystemTypeNode->addAttribute("alternations",intToStr(alternations), mapTagReplacements);
//    int particleSystemStartDelay;
	particleSystemTypeNode->addAttribute("particleSystemStartDelay",intToStr(particleSystemStartDelay), mapTagReplacements);
//	typedef std::list<UnitParticleSystemType*> Children;
//	Children children;
	for(Children::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->saveGame(particleSystemTypeNode);
	}
//    bool minmaxEnabled;
	particleSystemTypeNode->addAttribute("minmaxEnabled",intToStr(minmaxEnabled), mapTagReplacements);
//    int minHp;
	particleSystemTypeNode->addAttribute("minHp",intToStr(minHp), mapTagReplacements);
//    int maxHp;
	particleSystemTypeNode->addAttribute("maxHp",intToStr(maxHp), mapTagReplacements);
//    bool minmaxIsPercent;
	particleSystemTypeNode->addAttribute("minmaxIsPercent",intToStr(minmaxIsPercent), mapTagReplacements);
}

// ===========================================================
//	class ParticleSystemTypeProjectile
// ===========================================================

ParticleSystemTypeProjectile::ParticleSystemTypeProjectile() : ParticleSystemType() {
	trajectorySpeed = 0.0f;
	trajectoryScale = 0.0f;
	trajectoryFrequency = 0.0f;
}

void ParticleSystemTypeProjectile::load(const XmlNode* particleFileNode, const string &dir, const string &path,
		RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader, string techtreePath) {

	try{
		XmlTree xmlTree;

		std::map<string,string> mapExtraTagReplacementValues;
		mapExtraTagReplacementValues["$COMMONDATAPATH"] = techtreePath + "/commondata/";
		xmlTree.load(path, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		loadedFileList[path].push_back(make_pair(parentLoader,parentLoader));

		const XmlNode *particleSystemNode= xmlTree.getRootNode();

		if(particleFileNode){
			// immediate children in the particleFileNode will override the particleSystemNode
			particleFileNode->setSuper(particleSystemNode);
			particleSystemNode= particleFileNode;
		}
		
		ParticleSystemType::load(particleSystemNode, dir, renderer, loadedFileList,parentLoader, techtreePath);

		//trajectory values
		const XmlNode *tajectoryNode= particleSystemNode->getChild("trajectory");
		trajectory= tajectoryNode->getAttribute("type")->getRestrictedValue();

		//trajectory speed
		const XmlNode *tajectorySpeedNode= tajectoryNode->getChild("speed");
		trajectorySpeed= tajectorySpeedNode->getAttribute("value")->getFloatValue() / (float)GameConstants::updateFps;
		//printf("[%s] trajectorySpeed = %f\n",path.c_str(),trajectorySpeed);

		if(trajectory=="parabolic" || trajectory=="spiral"){
			//trajectory scale
			const XmlNode *tajectoryScaleNode= tajectoryNode->getChild("scale");
			trajectoryScale= tajectoryScaleNode->getAttribute("value")->getFloatValue();
		}
		else{
			trajectoryScale= 1.0f;
		}

		if(trajectory=="spiral"){
			//trajectory frequency
			const XmlNode *tajectoryFrequencyNode= tajectoryNode->getChild("frequency");
			trajectoryFrequency= tajectoryFrequencyNode->getAttribute("value")->getFloatValue();
		}
		else{
			trajectoryFrequency= 1.0f;
		}
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

ProjectileParticleSystem *ParticleSystemTypeProjectile::create(ParticleOwner *owner) {
	ProjectileParticleSystem *ps=  new ProjectileParticleSystem();
	ps->setParticleOwner(owner);
	ParticleSystemType::setValues(ps);

	ps->setTrajectory(ProjectileParticleSystem::strToTrajectory(trajectory));

	//printf("Setting trajectorySpeed = %f\n",trajectorySpeed);
	ps->setTrajectorySpeed(trajectorySpeed);
	ps->setTrajectoryScale(trajectoryScale);
	ps->setTrajectoryFrequency(trajectoryFrequency);

	ps->initParticleSystem();

	return ps;
}

void ParticleSystemTypeProjectile::saveGame(XmlNode *rootNode) {
	ParticleSystemType::saveGame(rootNode);

	std::map<string,string> mapTagReplacements;
	XmlNode *particleSystemTypeProjectileNode = rootNode->addChild("ParticleSystemTypeProjectile");

//	string trajectory;
	particleSystemTypeProjectileNode->addAttribute("trajectory",trajectory, mapTagReplacements);
//	float trajectorySpeed;
	particleSystemTypeProjectileNode->addAttribute("trajectorySpeed",floatToStr(trajectorySpeed,6), mapTagReplacements);
//	float trajectoryScale;
	particleSystemTypeProjectileNode->addAttribute("trajectoryScale",floatToStr(trajectoryScale,6), mapTagReplacements);
//	float trajectoryFrequency;
	particleSystemTypeProjectileNode->addAttribute("trajectoryFrequency",floatToStr(trajectoryFrequency,6), mapTagReplacements);
}

// ===========================================================
//	class ParticleSystemTypeSplash
// ===========================================================

ParticleSystemTypeSplash::ParticleSystemTypeSplash() {
	emissionRateFade = 0.0f;
	verticalSpreadA = 0.0f;
	verticalSpreadB = 0.0f;
	horizontalSpreadA = 0.0f;
	horizontalSpreadB = 0.0f;
}

void ParticleSystemTypeSplash::load(const XmlNode* particleFileNode, const string &dir, const string &path,
		RendererInterface *renderer, std::map<string,vector<pair<string, string> > > &loadedFileList,
		string parentLoader, string techtreePath) {

	try{
		XmlTree xmlTree;

		std::map<string,string> mapExtraTagReplacementValues;
		mapExtraTagReplacementValues["$COMMONDATAPATH"] = techtreePath + "/commondata/";
		xmlTree.load(path, Properties::getTagReplacementValues(&mapExtraTagReplacementValues));
		loadedFileList[path].push_back(make_pair(parentLoader,parentLoader));

		const XmlNode *particleSystemNode= xmlTree.getRootNode();
		
		if(particleFileNode){
			// immediate children in the particleFileNode will override the particleSystemNode
			particleFileNode->setSuper(particleSystemNode);
			particleSystemNode= particleFileNode;
		}

		ParticleSystemType::load(particleSystemNode, dir, renderer, loadedFileList, parentLoader, techtreePath);

		//emission rate fade
		const XmlNode *emissionRateFadeNode= particleSystemNode->getChild("emission-rate-fade");
		emissionRateFade= emissionRateFadeNode->getAttribute("value")->getFloatValue();
		
		//spread values
		const XmlNode *verticalSpreadNode= particleSystemNode->getChild("vertical-spread");
		verticalSpreadA= verticalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		verticalSpreadB= verticalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);

		const XmlNode *horizontalSpreadNode= particleSystemNode->getChild("horizontal-spread");
		horizontalSpreadA= horizontalSpreadNode->getAttribute("a")->getFloatValue(0.0f, 1.0f);
		horizontalSpreadB= horizontalSpreadNode->getAttribute("b")->getFloatValue(-1.0f, 1.0f);
	}
	catch(const exception &e){
		SystemFlags::OutputDebug(SystemFlags::debugError,"In [%s::%s Line: %d] Error [%s]\n",__FILE__,__FUNCTION__,__LINE__,e.what());
		throw megaglest_runtime_error("Error loading ParticleSystem: "+ path + "\n" +e.what());
	}
}

SplashParticleSystem *ParticleSystemTypeSplash::create(ParticleOwner *owner) {
	SplashParticleSystem *ps=  new SplashParticleSystem();
	ps->setParticleOwner(owner);
	ParticleSystemType::setValues(ps);

	ps->setEmissionRateFade(emissionRateFade);
	ps->setVerticalSpreadA(verticalSpreadA);
	ps->setVerticalSpreadB(verticalSpreadB);
	ps->setHorizontalSpreadA(horizontalSpreadA);
	ps->setHorizontalSpreadB(horizontalSpreadB);

	ps->initParticleSystem();

	return ps;
}

void ParticleSystemTypeSplash::saveGame(XmlNode *rootNode) {
	ParticleSystemType::saveGame(rootNode);

	std::map<string,string> mapTagReplacements;
	XmlNode *particleSystemTypeSplashNode = rootNode->addChild("ParticleSystemTypeSplash");

//	float emissionRateFade;
	particleSystemTypeSplashNode->addAttribute("emissionRateFade",floatToStr(emissionRateFade,6), mapTagReplacements);
//	float verticalSpreadA;
	particleSystemTypeSplashNode->addAttribute("verticalSpreadA",floatToStr(verticalSpreadA,6), mapTagReplacements);
//	float verticalSpreadB;
	particleSystemTypeSplashNode->addAttribute("verticalSpreadB",floatToStr(verticalSpreadB,6), mapTagReplacements);
//	float horizontalSpreadA;
	particleSystemTypeSplashNode->addAttribute("horizontalSpreadA",floatToStr(horizontalSpreadA,6), mapTagReplacements);
//	float horizontalSpreadB;
	particleSystemTypeSplashNode->addAttribute("horizontalSpreadB",floatToStr(horizontalSpreadB,6), mapTagReplacements);
}

}}//end mamespace
