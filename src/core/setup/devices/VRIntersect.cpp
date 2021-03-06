#include "VRIntersect.h"
#include <OpenSG/OSGLineChunk.h>
#include <OpenSG/OSGIntersectAction.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGTriangleIterator.h>

#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRFunction.h"
#include "VRSignal.h"
#include "VRDevice.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

Vec2f VRIntersect_computeTexel(VRIntersection& ins, NodeRecPtr node) {
    if (!ins.hit) return Vec2f(0,0);
    if (node == 0) return Vec2f(0,0);

    GeometryRefPtr geo = dynamic_cast<Geometry*>( node->getCore() );
    if (geo == 0) return Vec2f(0,0);
    auto texcoords = geo->getTexCoords();
    if (texcoords == 0) return Vec2f(0,0);
    TriangleIterator iter = geo->beginTriangles(); iter.seek( ins.triangle );


    Matrix m = node->getToWorld();
    m.invert();
    Pnt3f local_pnt; m.mult(ins.point, local_pnt);

    Pnt3f p0 = iter.getPosition(0);
    Pnt3f p1 = iter.getPosition(1);
    Pnt3f p2 = iter.getPosition(2);
    Vec3f cr = (p1 - p0).cross(p2 - p0);
    Vec3f n = cr; n.normalize();

    float areaABC = n.dot(cr);
    float areaPBC = n.dot((p1 - local_pnt).cross(p2 - local_pnt));
    float areaPCA = n.dot((p2 - local_pnt).cross(p0 - local_pnt));
    float a = areaPBC / areaABC;
    float b = areaPCA / areaABC;
    float c = 1.0f - a - b;

    return iter.getTexCoords(0) * a + iter.getTexCoords(1) * b + iter.getTexCoords(2) * c;
}

Vec3i VRIntersect_computeVertices(VRIntersection& ins, NodeRecPtr node) {
    if (!ins.hit) return Vec3i(0,0,0);
    if (node == 0) return Vec3i(0,0,0);

    GeometryRefPtr geo = dynamic_cast<Geometry*>( node->getCore() );
    if (geo == 0) return Vec3i(0,0,0);
    TriangleIterator iter = geo->beginTriangles(); iter.seek( ins.triangle );
    return Vec3i(iter.getPositionIndex(0), iter.getPositionIndex(1), iter.getPositionIndex(2));
}

class VRIntersectAction : public IntersectAction {
    private:
        /*Action::ResultE GeoInstancer::intersectEnter(Action  *action) {
            if(_sfBaseGeometry.getValue() == NULL) return Action::Continue;
            return _sfBaseGeometry.getValue()->intersectEnter(action);
            return Action::Continue;
        }*/

    public:
        VRIntersectAction() {
            // TODO: check OSG::Geometry::intersectEnter

            //IntersectAction::registerEnterDefault( getClassType(), reinterpret_cast<Action::Callback>(&GeoInstancer::intersectEnter));
        }
};

VRIntersection VRIntersect::intersect(VRObjectWeakPtr wtree, Line ray) {
    VRIntersection ins;
    auto tree = wtree.lock();
    if (!tree) return ins;

    VRIntersectAction iAct;
    //IntersectActionRefPtr iAct = IntersectAction::create();
    iAct.setTravMask(8);
    iAct.setLine(ray);
    iAct.apply(tree->getNode());

    ins.hit = iAct.didHit();
    if (ins.hit) {
        ins.object = tree->find(iAct.getHitObject()->getParent());
        if (auto sp = ins.object.lock()) ins.name = sp->getName();
        ins.point = iAct.getHitPoint();
        ins.normal = iAct.getHitNormal();
        if (tree->getParent()) tree->getParent()->getNode()->getToWorld().mult( ins.point, ins.point );
        if (tree->getParent()) tree->getParent()->getNode()->getToWorld().mult( ins.normal, ins.normal );
        ins.triangle = iAct.getHitTriangle();
        ins.triangleVertices = VRIntersect_computeVertices(ins, iAct.getHitObject());
        ins.texel = VRIntersect_computeTexel(ins, iAct.getHitObject());
        lastIntersection = ins;
    } else {
        ins.object.reset();
        if (lastIntersection.time < ins.time) lastIntersection = ins;
    }

    intersections[tree.get()] = ins;

    if (showHit) cross->setWorldPosition(Vec3f(ins.point));
    return ins;
}

VRIntersection VRIntersect::intersect(VRObjectWeakPtr wtree) {
    VRIntersection ins;
    auto tree = wtree.lock();
    if (!tree) tree = dynTree.lock();
    if (!tree) return ins;

    VRDevice* dev = (VRDevice*)this;
    VRTransformPtr caster = dev->getBeacon();
    if (caster == 0) { cout << "Warning: VRIntersect::intersect, caster is 0!\n"; return ins; }
    if (tree == 0) return ins;

    if (intersections.count(tree.get())) ins = intersections[tree.get()];
    uint now = VRGlobals::get()->CURRENT_FRAME;
    if (ins.hit && ins.time == now) return ins; // allready found it
    ins.time = now;

    Line ray = caster->castRay(tree);
    return intersect(tree, ray);
}

VRIntersection VRIntersect::intersect() { return intersect(dynTree); }

VRIntersect::VRIntersect() {
    initCross();
    drop_fkt = VRFunction<VRDevice*>::create("Intersect_drop", boost::bind(&VRIntersect::drop, this, _1));
    dragged_ghost = VRTransform::create("dev_ghost");
    dragSignal = VRSignal::create((VRDevice*)this);
    dropSignal = VRSignal::create((VRDevice*)this);
}

VRIntersect::~VRIntersect() {}

void VRIntersect::dragCB(VRTransformWeakPtr caster, VRObjectWeakPtr tree, VRDevice* dev) {
    VRIntersection ins = intersect(tree);
    drag(ins.object, caster);
}

void VRIntersect::drag(VRObjectWeakPtr wobj, VRTransformWeakPtr wcaster) {
    auto obj = wobj.lock();
    auto caster = wcaster.lock();
    if (!obj || !caster) return;

    auto d = getDraggedObject();
    if (obj == 0 || d != 0 || !dnd) return;

    obj = obj->findPickableAncestor();
    if (obj == 0) return;
    if (!obj->hasAttachment("transform")) return;

    auto dobj = static_pointer_cast<VRTransform>(obj);
    dragged = dobj;
    dobj->drag(caster);

    dragged_ghost->setMatrix(dobj->getMatrix());
    dragged_ghost->switchParent(caster);

    dragSignal->trigger<VRDevice>();
}

void VRIntersect::drop(VRDevice* dev) {
    auto d = getDraggedObject();
    if (d != 0) {
        d->drop();
        dropSignal->trigger<VRDevice>();
        drop_time = VRGlobals::get()->CURRENT_FRAME;
        dragged.reset();
    }
}

VRTransformPtr VRIntersect::getDraggedObject() { return dragged.lock(); }

VRTransformPtr VRIntersect::getDraggedGhost() { return dragged_ghost; }
VRSignalPtr VRIntersect::getDragSignal() { return dragSignal; }
VRSignalPtr VRIntersect::getDropSignal() { return dropSignal; }

void VRIntersect::initCross() {
    cross = VRGeometry::create("Hit cross");
    cross->hide();
    showHit = false;

    vector<Vec3f> pos, norms;
    vector<Vec2f> texs;
    vector<int> inds;

    float d = 0.1;
    pos.push_back(Vec3f(-d,-d,-d));
    pos.push_back(Vec3f(d,d,d));
    pos.push_back(Vec3f(-d,d,-d));
    pos.push_back(Vec3f(d,-d,d));

    pos.push_back(Vec3f(-d,-d,d));
    pos.push_back(Vec3f(d,d,-d));
    pos.push_back(Vec3f(-d,d,d));
    pos.push_back(Vec3f(d,-d,-d));

    for (int i=0;i<8;i++) {
        norms.push_back(Vec3f(0,0,-1));
        inds.push_back(i);
        texs.push_back(Vec2f(0,0));
    }

    VRMaterialPtr mat = VRMaterial::create("red_cross");
    mat->setDiffuse(Color3f(1,0,0));
    cross->create(GL_LINES, pos, norms, inds, texs);
    cross->setMaterial(mat);
}

VRDeviceCb VRIntersect::addDrag(VRTransformWeakPtr caster) {
    return addDrag(caster, dynTree);
}

VRDeviceCb VRIntersect::addDrag(VRTransformWeakPtr caster, VRObjectWeakPtr tree) {
    auto sc = caster.lock();
    auto st = caster.lock();
    if (!sc || !st) return 0;

    auto key = st.get();
    if (int_fkt_map.count(key)) return int_fkt_map[key];

    VRDeviceCb fkt = VRFunction<VRDevice*>::create("Intersect_drag", boost::bind(&VRIntersect::dragCB, this, caster, tree, _1));
    dra_fkt_map[key] = fkt;
    return fkt;
}

void VRIntersect::toggleDragnDrop(bool b) { dnd = b;}
void VRIntersect::showHitPoint(bool b) {//needs to be optimized for multiple scenes
    showHit = b;
    if (b) cross->show();
    else cross->hide();
}

void VRIntersect::addDynTree(VRObjectWeakPtr o) {
    if (auto sp = o.lock()) {
        dynTrees[ sp.get() ] = o;
        dynTree = o;
    }
}

void VRIntersect::remDynTree(VRObjectWeakPtr o) {
    if (auto sp = o.lock()) {
        dynTrees.erase( sp.get() );
        dynTree.reset();
    }
}

void VRIntersect::updateDynTree(VRObjectPtr a) {
    dynTree = a;
    return; // TODO ?

    for (auto dt : dynTrees) {
        if (auto sp = dt.second.lock()) {
            if (sp->hasAncestor(a)) { dynTree = sp; return; }
        }
    }
}

VRObjectPtr VRIntersect::getCross() { return cross; }//needs to be optimized for multiple scenes
VRDeviceCb VRIntersect::getDrop() { return drop_fkt; }
//Pnt3f VRIntersect::getHitPoint() { return hitPoint; }
//Vec2f VRIntersect::getHitTexel() { return hitTexel; }
//VRObjectPtr VRIntersect::getHitObject() { return obj; }

VRIntersection VRIntersect::getLastIntersection() { return lastIntersection; }

OSG_END_NAMESPACE;
