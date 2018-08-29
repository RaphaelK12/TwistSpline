/*
MIT License

Copyright (c) 2018 Blur Studio

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <maya/M3dView.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFNPluginData.h>
#include <maya/MMatrix.h>
#include <maya/MFnPluginData.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MTypes.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MVectorArray.h>
#include <maya/MPointArray.h>
#include <maya/MBoundingBox.h>
#include <maya/MPxGeometryOverride.h>
#include <maya/MQuaternion.h>

#include <string>
#include <iostream>

#include "twistSpline.h"
#include "twistSplineData.h"
#include "twistSplineNode.h"

#define MCHECKERROR(STAT)         \
    if (MS::kSuccess != STAT) {   \
        return MS::kFailure;      \
    }


MTypeId	TwistSplineNode::id(0x001226F7);
MString	TwistSplineNode::drawDbClassification("drawdb/geometry/twistSpline");
MString	TwistSplineNode::drawRegistrantId("TwistSplineNodePlugin");

MObject TwistSplineNode::aOutputSpline;
MObject TwistSplineNode::aSplineLength;
MObject TwistSplineNode::aFirstVertex;
MObject TwistSplineNode::aFirstLock;
MObject TwistSplineNode::aVertexData;
MObject TwistSplineNode::aInTangent;
MObject TwistSplineNode::aOutTangent;
MObject TwistSplineNode::aControlVertex;
MObject TwistSplineNode::aRestLength;
MObject TwistSplineNode::aLock;
MObject TwistSplineNode::aUseTwist;
MObject TwistSplineNode::aUseOrient;
MObject TwistSplineNode::aTwist;

TwistSplineNode::TwistSplineNode() {}
TwistSplineNode::~TwistSplineNode() {}

MStatus TwistSplineNode::initialize() {
	MFnCompoundAttribute cAttr;
	MFnMatrixAttribute mAttr;
	MFnTypedAttribute tAttr;
	MFnNumericAttribute nAttr;

	//---------------- Output ------------------

	aOutputSpline = tAttr.create("outputSpline", "outputSpline", TwistSplineData::id);
	tAttr.setWritable(false);
	addAttribute(aOutputSpline);

	aSplineLength = nAttr.create("splineLength", "splineLength", MFnNumericData::kDouble);
	nAttr.setWritable(false);
	addAttribute(aSplineLength);

	//----------------- Top-level -----------------
	aFirstVertex = mAttr.create("firstVertex", "firstVertex");
	mAttr.setHidden(true);
	mAttr.setDefault(MMatrix::identity);
	addAttribute(aFirstVertex);

	aFirstLock = nAttr.create("firstLock", "firstLock", MFnNumericData::kDouble, 1.0);
	nAttr.setHidden(false);
	nAttr.setKeyable(true);
	nAttr.setChannelBox(true);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	addAttribute(aFirstLock);

	//--------------- Array -------------------

	aInTangent = mAttr.create("inTangent", "inTangent");
	mAttr.setHidden(true);
	mAttr.setDefault(MMatrix::identity);

	aOutTangent = mAttr.create("outTangent", "outTangent");
	mAttr.setHidden(true);
	mAttr.setDefault(MMatrix::identity);

	aControlVertex = mAttr.create("controlVertex", "controlVertex");
	mAttr.setHidden(true);
	mAttr.setDefault(MMatrix::identity);

	aRestLength = nAttr.create("restLength", "restLength", MFnNumericData::kDouble, 0.0);
	aLock = nAttr.create("lock", "lock", MFnNumericData::kDouble, 0.0);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	aUseTwist = nAttr.create("useTwist", "useTwist", MFnNumericData::kDouble, 0.0);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	aUseOrient = nAttr.create("useOrient", "useOrient", MFnNumericData::kDouble, 0.0);
	nAttr.setMin(0.0);
	nAttr.setMax(1.0);
	aTwist = nAttr.create("twist", "twist", MFnNumericData::kDouble, 0.0);

	aVertexData = cAttr.create("vertexData", "vertexData");
	cAttr.setArray(true);
	cAttr.addChild(aInTangent); // matrix
	cAttr.addChild(aControlVertex); // matrix
	cAttr.addChild(aOutTangent); // matrix

	cAttr.addChild(aRestLength); // float unbounded
	cAttr.addChild(aLock); // float 0-1
	cAttr.addChild(aUseTwist); // float 0-1
	cAttr.addChild(aUseOrient); // float 0-1
	cAttr.addChild(aTwist); // float unbounded

	addAttribute(aVertexData);

	attributeAffects(aFirstVertex, aOutputSpline);
	attributeAffects(aFirstLock, aOutputSpline);
	attributeAffects(aInTangent, aOutputSpline);
	attributeAffects(aControlVertex, aOutputSpline);
	attributeAffects(aOutTangent, aOutputSpline);
	attributeAffects(aRestLength, aOutputSpline);
	attributeAffects(aLock, aOutputSpline);
	attributeAffects(aUseTwist, aOutputSpline);
	attributeAffects(aTwist, aOutputSpline);
	attributeAffects(aUseOrient, aOutputSpline);

	attributeAffects(aFirstVertex, aSplineLength);
	attributeAffects(aInTangent, aSplineLength);
	attributeAffects(aControlVertex, aSplineLength);
	attributeAffects(aOutTangent, aSplineLength);
	
	return MS::kSuccess;
}

TwistSplineT* TwistSplineNode::getSplineData() const {
	MStatus stat;
	MObject thisNode = thisMObject();
	MObject output;

	// get sentinel plug, causes update
	MPlug tPlug(thisNode, aOutputSpline);
	tPlug.getValue(output);
	MFnPluginData outData(output);
	auto tsd = dynamic_cast<TwistSplineData *>(outData.data());
	return tsd->getSpline();
}

MBoundingBox TwistSplineNode::boundingBox() const {
	TwistSplineT *ts = getSplineData();

	double minx, miny, minz, maxx, maxy, maxz;
	minx = miny = minz = std::numeric_limits<double>::max();
	maxx = maxy = maxz = std::numeric_limits<double>::min();

	MPointArray pts = ts->getVerts();

	for (unsigned i = 0; i < pts.length(); ++i) {
		MPoint &pt = pts[i];
		minx = pt[0] < minx ? pt[0] : minx;
		miny = pt[1] < miny ? pt[1] : miny;
		minz = pt[2] < minz ? pt[2] : minz;
		maxx = pt[0] > maxx ? pt[0] : maxx;
		maxy = pt[1] > maxy ? pt[1] : maxy;
		maxz = pt[2] > maxz ? pt[2] : maxz;
	}
	return MBoundingBox(MPoint(minx, miny, minz), MPoint(maxx, maxy, maxz));
}

MStatus	TwistSplineNode::compute(const MPlug& plug, MDataBlock& data) {
	if (plug == aOutputSpline) {
		MStatus status;

		// Get all input Data
		MPointArray points;
		std::vector<MQuaternion> quats;
		std::vector<double> lockPositions(1, 0.0);
		//std::vector<double> lockVals(1, 1.0);
		std::vector<double> twistLock(1, 1.0);
		std::vector<double> userTwist(1, 0.0);
		std::vector<double> orientLock(1, 1.0);

		std::vector<double> lockVals;
		MDataHandle l1h = data.inputValue(aFirstLock);
		lockVals.push_back(l1h.asDouble());

		// Get the first matrix
		MDataHandle v1h = data.inputValue(aFirstVertex);
		auto v1m = MTransformationMatrix(v1h.asMatrix());
		points.append(v1m.getTranslation(MSpace::kWorld));
		quats.push_back(v1m.rotation());
		
		// loop over the input matrices
		MArrayDataHandle inputs = data.inputArrayValue(aVertexData);
		for (unsigned i = 0; i < inputs.elementCount(); ++i) {
			inputs.jumpToArrayElement(i);
			auto group = inputs.inputValue();

			lockPositions.push_back(group.child(aRestLength).asDouble());
			lockVals.push_back(group.child(aLock).asDouble());
			twistLock.push_back(group.child(aUseTwist).asDouble());
			userTwist.push_back(group.child(aTwist).asDouble() * M_PI / 180.0);
			orientLock.push_back(group.child(aUseOrient).asDouble());

			auto itMat = MTransformationMatrix(group.child(aInTangent).asMatrix());
			auto otMat = MTransformationMatrix(group.child(aOutTangent).asMatrix());
			auto cvMat = MTransformationMatrix(group.child(aControlVertex).asMatrix());

			MVector a = itMat.getTranslation(MSpace::kWorld);
			MVector b = otMat.getTranslation(MSpace::kWorld);
			MVector c = cvMat.getTranslation(MSpace::kWorld);
			points.append(a);
			points.append(b);
			points.append(c);

			// TODO: Get this stuff in the proper space
			MQuaternion qa = itMat.rotation();
			MQuaternion qb = otMat.rotation();
			MQuaternion qc = cvMat.rotation();
			quats.push_back(qa);
			quats.push_back(qb);
			quats.push_back(qc);
		}

		// Output Data Handles
		MDataHandle storageH = data.outputValue(aOutputSpline, &status);
		MCHECKERROR(status);

		// Build output data object
		TwistSplineData *outSplineData;
		MFnPluginData fnDataCreator;
		MTypeId tmpid(TwistSplineData::id);
		fnDataCreator.create(tmpid, &status);
		MCHECKERROR(status);
		outSplineData = dynamic_cast<TwistSplineData*>(fnDataCreator.data(&status));
		MCHECKERROR(status);
		TwistSplineT *outSpline = outSplineData->getSpline();

		outSpline->setVerts(points, quats, lockPositions, lockVals, userTwist, twistLock, orientLock);

        // Testing closest point
        //outSpline->buildKDTree();
        //auto cp = outSpline->getClosestPoint(MPoint());

		storageH.setMPxData(outSplineData);
		data.setClean(aOutputSpline);
	}
	else if (plug == aSplineLength) {
		// First, get the splines that need computing, and their weight.
		MDataHandle inHandle = data.inputValue(aOutputSpline);
		MDataHandle outHandle = data.outputValue(aSplineLength);

		MPxData* pd = inHandle.asPluginData();
		if (pd == nullptr) {
			outHandle.setDouble(0.0);
			return MS::kSuccess;
		}

		auto inSplineData = (TwistSplineData *)pd;
		TwistSplineT *spline = inSplineData->getSpline();

		if (spline == nullptr) {
			outHandle.setDouble(0.0);
			return MS::kSuccess;
		}
		outHandle.setDouble(spline->getTotalLength());
		data.setClean(aSplineLength);
	}
	else {
		return MS::kUnknownParameter;
	}
	return MS::kSuccess;
}

void* TwistSplineNode::creator() {
	return new TwistSplineNode();
}

void TwistSplineNode::draw(M3dView &view, const MDagPath &path, M3dView::DisplayStyle style, M3dView::DisplayStatus dstat) {
	// TODO: build the legacy viewport draw mechanism
}





//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Viewport 2.0 override implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// By setting isAlwaysDirty to false in MPxDrawOverride constructor, the
// draw override will be updated (via prepareForDraw()) only when the node
// is marked dirty via DG evaluation or dirty propagation. Additional
// callback is also added to explicitly mark the node as being dirty (via
// MRenderer::setGeometryDrawDirty()) for certain circumstances. Note that
// the draw callback in MPxDrawOverride constructor is set to NULL in order
// to achieve better performance.
TwistSplineDrawOverride::TwistSplineDrawOverride(const MObject& obj)
	: MHWRender::MPxDrawOverride(obj, NULL, false) {
	fModelEditorChangedCbId = MEventMessage::addEventCallback("modelEditorChanged", OnModelEditorChanged, this);

	MStatus status;
	MFnDependencyNode node(obj, &status);
	tsn = status ? dynamic_cast<TwistSplineNode*>(node.userNode()) : NULL;
}

TwistSplineDrawOverride::~TwistSplineDrawOverride() {
	tsn = NULL;

	if (fModelEditorChangedCbId != 0) {
		MMessage::removeCallback(fModelEditorChangedCbId);
		fModelEditorChangedCbId = 0;
	}
}

void TwistSplineDrawOverride::OnModelEditorChanged(void *clientData) {
	// Mark the node as being dirty so that it can update on display appearance
	// switch among wireframe and shaded.
	TwistSplineDrawOverride *ovr = static_cast<TwistSplineDrawOverride*>(clientData);
	if (ovr && ovr->tsn) {
		MHWRender::MRenderer::setGeometryDrawDirty(ovr->tsn->thisMObject());
	}
}

MHWRender::DrawAPI TwistSplineDrawOverride::supportedDrawAPIs() const {
	// this plugin supports both GL and DX
	return (MHWRender::kOpenGL | MHWRender::kDirectX11 | MHWRender::kOpenGLCoreProfile);
}

bool TwistSplineDrawOverride::isBounded(const MDagPath& /*objPath*/, const MDagPath& /*cameraPath*/) const {
	return true;
}

MBoundingBox TwistSplineDrawOverride::boundingBox(
		const MDagPath& objPath,
		const MDagPath& cameraPath) const {

	TwistSplineT* ts = tsn->getSplineData();
	MPointArray pts = ts->getVerts();

	double minx, miny, minz, maxx, maxy, maxz;
	minx = miny = minz = std::numeric_limits<double>::max();
	maxx = maxy = maxz = std::numeric_limits<double>::min();

	for (unsigned i = 0; i < pts.length(); ++i) {
		MPoint &pt = pts[i];
		minx = pt[0] < minx ? pt[0] : minx;
		miny = pt[1] < miny ? pt[1] : miny;
		minz = pt[2] < minz ? pt[2] : minz;
		maxx = pt[0] > maxx ? pt[0] : maxx;
		maxy = pt[1] > maxy ? pt[1] : maxy;
		maxz = pt[2] > maxz ? pt[2] : maxz;
	}
	return MBoundingBox(MPoint(minx, miny, minz), MPoint(maxx, maxy, maxz));
}

// Called by Maya each time the object needs to be drawn.
MUserData* TwistSplineDrawOverride::prepareForDraw(
		const MDagPath& objPath,
		const MDagPath& cameraPath,
		const MHWRender::MFrameContext& frameContext,
		MUserData* oldData) {
	// Any data needed from the Maya dependency graph must be retrieved and cached in this stage.
	// There is one cache data for each drawable instance, if it is not desirable to allow Maya to handle data
	// caching, simply return null in this method and ignore user data parameter in draw callback method.
	// e.g. in this sample, we compute and cache the data for usage later when we create the 
	// MUIDrawManager to draw the twistspline in method addUIDrawables().
	TwistSplineDrawData* data = dynamic_cast<TwistSplineDrawData*>(oldData);
	if (!data) data = new TwistSplineDrawData();

	TwistSplineT* ts = tsn->getSplineData();

	data->splinePoints = ts->getPoints();

	MVectorArray &tans = ts->getTangents();
	data->tangents.setLength(tans.length() * 2);
	for (size_t i = 0; i < tans.length(); ++i) {
		MPoint &spi = data->splinePoints[i];
		data->tangents[2*i] = spi;
		data->tangents[(2*i) +1] = tans[i] + spi;
	}

	MVectorArray &norms = ts->getNormals();
	data->normals.setLength(norms.length() * 2);
	for (size_t i = 0; i < norms.length(); ++i) {
		MPoint &spi = data->splinePoints[i];
		MVector &nn = norms[i];
		data->normals[2 * i] = spi;
		data->normals[(2 * i) + 1] = nn + spi;
	}

	MVectorArray &binorms = ts->getBinormals();
	data->binormals.setLength(binorms.length() * 2);
	for (size_t i = 0; i < binorms.length(); ++i) {
		MPoint &spi = data->splinePoints[i];
		data->binormals[2 * i] = spi;
		data->binormals[(2 * i) + 1] = binorms[i] + spi;
	}
	
	// get correct color based on the state of object, e.g. active or dormant
	data->color = MHWRender::MGeometryUtilities::wireframeColor(objPath);
	return data;
}

// addUIDrawables() provides access to the MUIDrawManager, which can be used
// to queue up operations for drawing simple UI elements such as lines, circles and
// text. To enable addUIDrawables(), override hasUIDrawables() and make it return true.
void TwistSplineDrawOverride::addUIDrawables(
		const MDagPath& objPath,
		MHWRender::MUIDrawManager& drawManager,
		const MHWRender::MFrameContext& frameContext,
		const MUserData* userData) {
	TwistSplineDrawData* data = (TwistSplineDrawData*)userData;
	if (!data) return;

	bool draw2D = false; // ALWAYS false
	drawManager.beginDrawable();

	drawManager.setColor(data->color);

	drawManager.setDepthPriority(MHWRender::MRenderItem::sActiveLineDepthPriority);
	drawManager.lineStrip(data->splinePoints, draw2D);
# if 0
	drawManager.setColor(MColor(.5, 0, 0));
	drawManager.lineList(data->tangents, draw2D);

	drawManager.setColor(MColor(0, 0, .5));
	drawManager.lineList(data->binormals, draw2D);

	drawManager.setColor(MColor(0, .5, 0));
	drawManager.lineList(data->normals, draw2D);
# endif

	drawManager.endDrawable();
}

