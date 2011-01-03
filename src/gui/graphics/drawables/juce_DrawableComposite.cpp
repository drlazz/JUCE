/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-10 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "../../../core/juce_StandardHeader.h"

BEGIN_JUCE_NAMESPACE

#include "juce_DrawableComposite.h"


//==============================================================================
DrawableComposite::DrawableComposite()
    : bounds (Point<float>(), Point<float> (100.0f, 0.0f), Point<float> (0.0f, 100.0f)),
      updateBoundsReentrant (false)
{
    setContentArea (RelativeRectangle (RelativeCoordinate (0.0),
                                       RelativeCoordinate (100.0),
                                       RelativeCoordinate (0.0),
                                       RelativeCoordinate (100.0)));
}

DrawableComposite::DrawableComposite (const DrawableComposite& other)
    : bounds (other.bounds),
      markersX (other.markersX),
      markersY (other.markersY),
      updateBoundsReentrant (false)
{
    for (int i = 0; i < other.getNumChildComponents(); ++i)
    {
        const Drawable* const d = dynamic_cast <const Drawable*> (other.getChildComponent(i));

        if (d != 0)
            addAndMakeVisible (d->createCopy());
    }
}

DrawableComposite::~DrawableComposite()
{
    deleteAllChildren();
}

//==============================================================================
const Rectangle<float> DrawableComposite::getDrawableBounds() const
{
    Rectangle<float> r;

    for (int i = getNumChildComponents(); --i >= 0;)
    {
        const Drawable* const d = dynamic_cast <const Drawable*> (getChildComponent(i));

        if (d != 0)
            r = r.getUnion (d->isTransformed() ? d->getDrawableBounds().transformed (d->getTransform())
                                               : d->getDrawableBounds());
    }

    return r;
}

MarkerList* DrawableComposite::getMarkers (bool xAxis)
{
    return xAxis ? &markersX : &markersY;
}

const RelativeRectangle DrawableComposite::getContentArea() const
{
    jassert (markersX.getNumMarkers() >= 2 && markersX.getMarker (0)->name == contentLeftMarkerName && markersX.getMarker (1)->name == contentRightMarkerName);
    jassert (markersY.getNumMarkers() >= 2 && markersY.getMarker (0)->name == contentTopMarkerName && markersY.getMarker (1)->name == contentBottomMarkerName);

    return RelativeRectangle (markersX.getMarker(0)->position, markersX.getMarker(1)->position,
                              markersY.getMarker(0)->position, markersY.getMarker(1)->position);
}

void DrawableComposite::setContentArea (const RelativeRectangle& newArea)
{
    markersX.setMarker (contentLeftMarkerName, newArea.left);
    markersX.setMarker (contentRightMarkerName, newArea.right);
    markersY.setMarker (contentTopMarkerName, newArea.top);
    markersY.setMarker (contentBottomMarkerName, newArea.bottom);
    refreshTransformFromBounds();
}

void DrawableComposite::setBoundingBox (const RelativeParallelogram& newBoundingBox)
{
    bounds = newBoundingBox;
    refreshTransformFromBounds();
}

void DrawableComposite::resetBoundingBoxToContentArea()
{
    const RelativeRectangle content (getContentArea());

    setBoundingBox (RelativeParallelogram (RelativePoint (content.left, content.top),
                                           RelativePoint (content.right, content.top),
                                           RelativePoint (content.left, content.bottom)));
}

void DrawableComposite::resetContentAreaAndBoundingBoxToFitChildren()
{
    const Rectangle<float> activeArea (getDrawableBounds());

    setContentArea (RelativeRectangle (RelativeCoordinate (activeArea.getX()),
                                       RelativeCoordinate (activeArea.getRight()),
                                       RelativeCoordinate (activeArea.getY()),
                                       RelativeCoordinate (activeArea.getBottom())));
    resetBoundingBoxToContentArea();
}

void DrawableComposite::refreshTransformFromBounds()
{
    Point<float> resolved[3];
    bounds.resolveThreePoints (resolved, getParent());

    const Rectangle<float> content (getContentArea().resolve (getParent()));

    AffineTransform t (AffineTransform::fromTargetPoints (content.getX(), content.getY(), resolved[0].getX(), resolved[0].getY(),
                                                          content.getRight(), content.getY(), resolved[1].getX(), resolved[1].getY(),
                                                          content.getX(), content.getBottom(), resolved[2].getX(), resolved[2].getY()));

    if (t.isSingularity())
        t = AffineTransform::identity;

    setTransform (t);
}

void DrawableComposite::parentHierarchyChanged()
{
    DrawableComposite* parent = getParent();
    if (parent != 0)
        originRelativeToComponent = parent->originRelativeToComponent - getPosition();
}

void DrawableComposite::childBoundsChanged (Component*)
{
    updateBoundsToFitChildren();
}

void DrawableComposite::childrenChanged()
{
    updateBoundsToFitChildren();
}

struct RentrancyCheckSetter
{
    RentrancyCheckSetter (bool& b_) : b (b_)     { b_ = true; }
    ~RentrancyCheckSetter()                      { b = false; }

private:
    bool& b;

    JUCE_DECLARE_NON_COPYABLE (RentrancyCheckSetter);
};

void DrawableComposite::updateBoundsToFitChildren()
{
    if (! updateBoundsReentrant)
    {
        const RentrancyCheckSetter checkSetter (updateBoundsReentrant);

        Rectangle<int> childArea;

        for (int i = getNumChildComponents(); --i >= 0;)
            childArea = childArea.getUnion (getChildComponent(i)->getBoundsInParent());

        const Point<int> delta (childArea.getPosition());
        childArea += getPosition();

        if (childArea != getBounds())
        {
            if (! delta.isOrigin())
            {
                originRelativeToComponent -= delta;

                for (int i = getNumChildComponents(); --i >= 0;)
                {
                    Component* const c = getChildComponent(i);

                    if (c != 0)
                        c->setBounds (c->getBounds() - delta);
                }
            }

            setBounds (childArea);
        }
    }
}

//==============================================================================
const char* const DrawableComposite::contentLeftMarkerName = "left";
const char* const DrawableComposite::contentRightMarkerName = "right";
const char* const DrawableComposite::contentTopMarkerName = "top";
const char* const DrawableComposite::contentBottomMarkerName = "bottom";

//==============================================================================
const Expression DrawableComposite::getSymbolValue (const String& symbol, const String& member) const
{
    jassert (member.isEmpty()) // the only symbols available in a Drawable are markers.

    const MarkerList::Marker* m = markersX.getMarker (symbol);

    if (m == 0)
        m = markersY.getMarker (symbol);

    if (m != 0)
        return m->position.getExpression();

    throw Expression::EvaluationError (symbol, member);
}

Drawable* DrawableComposite::createCopy() const
{
    return new DrawableComposite (*this);
}

//==============================================================================
const Identifier DrawableComposite::valueTreeType ("Group");

const Identifier DrawableComposite::ValueTreeWrapper::topLeft ("topLeft");
const Identifier DrawableComposite::ValueTreeWrapper::topRight ("topRight");
const Identifier DrawableComposite::ValueTreeWrapper::bottomLeft ("bottomLeft");
const Identifier DrawableComposite::ValueTreeWrapper::childGroupTag ("Drawables");
const Identifier DrawableComposite::ValueTreeWrapper::markerGroupTagX ("MarkersX");
const Identifier DrawableComposite::ValueTreeWrapper::markerGroupTagY ("MarkersY");

//==============================================================================
DrawableComposite::ValueTreeWrapper::ValueTreeWrapper (const ValueTree& state_)
    : ValueTreeWrapperBase (state_)
{
    jassert (state.hasType (valueTreeType));
}

ValueTree DrawableComposite::ValueTreeWrapper::getChildList() const
{
    return state.getChildWithName (childGroupTag);
}

ValueTree DrawableComposite::ValueTreeWrapper::getChildListCreating (UndoManager* undoManager)
{
    return state.getOrCreateChildWithName (childGroupTag, undoManager);
}

const RelativeParallelogram DrawableComposite::ValueTreeWrapper::getBoundingBox() const
{
    return RelativeParallelogram (state.getProperty (topLeft, "0, 0"),
                                  state.getProperty (topRight, "100, 0"),
                                  state.getProperty (bottomLeft, "0, 100"));
}

void DrawableComposite::ValueTreeWrapper::setBoundingBox (const RelativeParallelogram& newBounds, UndoManager* undoManager)
{
    state.setProperty (topLeft, newBounds.topLeft.toString(), undoManager);
    state.setProperty (topRight, newBounds.topRight.toString(), undoManager);
    state.setProperty (bottomLeft, newBounds.bottomLeft.toString(), undoManager);
}

void DrawableComposite::ValueTreeWrapper::resetBoundingBoxToContentArea (UndoManager* undoManager)
{
    const RelativeRectangle content (getContentArea());

    setBoundingBox (RelativeParallelogram (RelativePoint (content.left, content.top),
                                           RelativePoint (content.right, content.top),
                                           RelativePoint (content.left, content.bottom)), undoManager);
}

const RelativeRectangle DrawableComposite::ValueTreeWrapper::getContentArea() const
{
    MarkerList::ValueTreeWrapper markersX (getMarkerList (true));
    MarkerList::ValueTreeWrapper markersY (getMarkerList (false));

    return RelativeRectangle (markersX.getMarker (markersX.getMarkerState (0)).position,
                              markersX.getMarker (markersX.getMarkerState (1)).position,
                              markersY.getMarker (markersY.getMarkerState (0)).position,
                              markersY.getMarker (markersY.getMarkerState (1)).position);
}

void DrawableComposite::ValueTreeWrapper::setContentArea (const RelativeRectangle& newArea, UndoManager* undoManager)
{
    MarkerList::ValueTreeWrapper markersX (getMarkerListCreating (true, 0));
    MarkerList::ValueTreeWrapper markersY (getMarkerListCreating (false, 0));

    markersX.setMarker (MarkerList::Marker (contentLeftMarkerName, newArea.left), undoManager);
    markersX.setMarker (MarkerList::Marker (contentRightMarkerName, newArea.right), undoManager);
    markersY.setMarker (MarkerList::Marker (contentTopMarkerName, newArea.top), undoManager);
    markersY.setMarker (MarkerList::Marker (contentBottomMarkerName, newArea.bottom), undoManager);
}

MarkerList::ValueTreeWrapper DrawableComposite::ValueTreeWrapper::getMarkerList (bool xAxis) const
{
    return state.getChildWithName (xAxis ? markerGroupTagX : markerGroupTagY);
}

MarkerList::ValueTreeWrapper DrawableComposite::ValueTreeWrapper::getMarkerListCreating (bool xAxis, UndoManager* undoManager)
{
    return state.getOrCreateChildWithName (xAxis ? markerGroupTagX : markerGroupTagY, undoManager);
}

//==============================================================================
void DrawableComposite::refreshFromValueTree (const ValueTree& tree, ComponentBuilder& builder)
{
    const ValueTreeWrapper wrapper (tree);
    setComponentID (wrapper.getID());

    const RelativeParallelogram newBounds (wrapper.getBoundingBox());
    if (bounds != newBounds)
        bounds = newBounds;

    wrapper.getMarkerList (true).applyTo (markersX);
    wrapper.getMarkerList (false).applyTo (markersY);

    builder.updateChildComponents (*this, wrapper.getChildList());

    refreshTransformFromBounds();
}

const ValueTree DrawableComposite::createValueTree (ComponentBuilder::ImageProvider* imageProvider) const
{
    ValueTree tree (valueTreeType);
    ValueTreeWrapper v (tree);

    v.setID (getComponentID());
    v.setBoundingBox (bounds, 0);

    ValueTree childList (v.getChildListCreating (0));

    for (int i = getNumChildComponents(); --i >= 0;)
    {
        const Drawable* const d = dynamic_cast <const Drawable*> (getChildComponent(i));
        jassert (d != 0); // You can't save a mix of Drawables and normal components!

        childList.addChild (d->createValueTree (imageProvider), -1, 0);
    }

    v.getMarkerListCreating (true, 0).readFrom (markersX, 0);
    v.getMarkerListCreating (false, 0).readFrom (markersY, 0);

    return tree;
}


END_JUCE_NAMESPACE
