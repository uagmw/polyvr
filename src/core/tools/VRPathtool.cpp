#include "VRPathtool.h"
#include "core/math/path.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRStroke.h"
#include "core/objects/material/VRMaterial.h"
#include "core/scene/VRScene.h"
#include "core/scene/VRSceneManager.h"
#include "core/setup/VRSetup.h"
#include "core/setup/VRSetupManager.h"
#include "core/setup/devices/VRDevice.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeoFunctions.h>

using namespace OSG;

VRPathtool::VRPathtool() {
    updatePtr = VRFunction<int>::create("path tool update", boost::bind(&VRPathtool::updateDevs, this) );
    VRSceneManager::getCurrent()->addUpdateFkt(updatePtr, 100);

    manip = new VRManipulator();
}

void VRPathtool::update() {
    for (auto h : handles_dict) updateHandle(h.first);
}

void VRPathtool::addPath(path* p, VRObjectPtr anchor) {
    entry* e = new entry();
    e->anchor = anchor;
    e->p = p;
    paths[e->p] = e;

    for (auto point : p->getPoints()) {
        VRGeometryPtr h = newHandle();
        handles_dict[h] = e;
        e->handles[h] = e->handles.size()-1;
        e->anchor->addChild(h);
        h->setPose(point.p, point.n, point.u);
    }
}

path* VRPathtool::newPath(VRDevice* dev, VRObjectPtr anchor, int resolution) {
    entry* e = new entry();
    e->anchor = anchor;
    e->p = new path();
    e->resolution = resolution;
    paths[e->p] = e;

    extrude(0,e->p);
    extrude(dev,e->p);
    return e->p;
}

VRGeometryPtr VRPathtool::newHandle() {
    VRGeometryPtr h;
    if (customHandle) {
        h = static_pointer_cast<VRGeometry>( customHandle->duplicate() );
    } else {
        h = VRGeometry::create("handle");
        h->setPrimitive("Torus", "0.04 0.15 3 4");
    }

    h->setPickable(true);
    h->addAttachment("handle", 0);
    h->setMaterial(VRMaterial::get("pathHandle"));
    h->getMaterial()->setDiffuse(Vec3f(0.5,0.5,0.9));
    //calcFaceNormals(h->getMesh()); // not working ??
    h->setPersistency(0);
    return h;
}

vector<path*> VRPathtool::getPaths() {
    vector<path*> res;
    for (auto p : paths) res.push_back(p.first);
    return res;
}

vector<VRGeometryPtr> VRPathtool::getHandles(path* p) {
    vector<VRGeometryPtr> res;
    if (p == 0) for (auto h : handles_dict) res.push_back(h.first);
    else {
        map<int, VRGeometryPtr> sor;
        entry* e = paths[p];
        for (auto h : e->handles) sor[h.second] = h.first;
        for (auto h : sor) res.push_back(h.second);
    }

    cout << "get handles " << p << endl;
    for (auto h : res) cout << " " << h->getName() << endl;
    return res;
}

VRStrokePtr VRPathtool::getStroke(path* p) {
    if (paths.count(p) == 0) return 0;
    return paths[p]->line;
}

VRGeometryPtr VRPathtool::extrude(VRDevice* dev, path* p) {
    if (paths.count(p) == 0) {
        cout << "Warning: VRPathtool::extrude, path " << p << " unknown\n";
        return 0;
    }

    entry* e = paths[p];
    e->p->addPoint(Vec3f(0,0,-1), Vec3f(1,0,0), Vec3f(1,1,1));

    VRGeometryPtr h = newHandle();
    handles_dict[h] = e;
    e->handles[h] = e->handles.size()-1;
    e->anchor->addChild(h);
    if (dev) {
        dev->drag(h, dev->getBeacon());
        h->setPose(Vec3f(0,0,-1), Vec3f(0,0,-1), Vec3f(0,1,0));
    }

    updateHandle(h);
    return h;
}

void VRPathtool::setHandleGeometry(VRGeometryPtr geo) {
    customHandle = static_pointer_cast<VRGeometry>( geo->duplicate() );
}

void VRPathtool::clear(path* p) {
    if (paths.count(p) == 0) return;
    entry* e = paths[p];

    for (auto h : e->handles) {
        handles_dict.erase(h.first);
        h.first->destroy();
    }
    e->handles.clear();
    if (e->line) e->line->destroy();
    e->line = 0;

    p->clear();
}

void VRPathtool::remPath(path* p) {
    if (paths.count(p) == 0) return;
    entry* e = paths[p];
    delete e->p;
    delete e;
    paths.erase(p);
}

void VRPathtool::updateHandle(VRGeometryPtr handle) {
    Matrix m = handle->getWorldMatrix();
    entry* e = handles_dict[handle];
    e->p->setPoint(e->handles[handle], Vec3f(m[3]), Vec3f(m[2]), Vec3f(1,1,1), Vec3f(m[1]));

    int hN = e->handles.size();
    if (hN <= 0) return;

    e->p->compute(e->resolution);
    int pN = e->p->getPositions().size();
    if (pN <= 2) return;

    // update path line
    if (e->line == 0) {
        e->line = VRStroke::create("path");
        e->line->setPersistency(0);
        VRMaterialPtr matl = VRMaterial::create("pline");
        matl->setLit(false);
        matl->setDiffuse(Vec3f(0.1,0.9,0.2));
        matl->setLineWidth(3);
        e->line->setMaterial(matl);
        e->anchor->addChild(e->line);
        vector<Vec3f> profile;
        profile.push_back(Vec3f());
        e->line->addPath(e->p);
        e->line->strokeProfile(profile, 0);
    }

    e->line->update();
}

void VRPathtool::updateDevs() {
    for (auto dev : VRSetupManager::getCurrent()->getDevices()) { // get dragged objects
        VRGeometryPtr obj = static_pointer_cast<VRGeometry>(dev.second->getDraggedObject());
        if (obj == 0) continue;
        if (handles_dict.count(obj) == 0) continue;
        updateHandle(obj);
    }
}

void VRPathtool::setVisible(bool handles, bool lines) {
    for (auto p : paths) if (p.second->line) p.second->line->setVisible(lines);
    for (auto h : handles_dict) h.first->setVisible(handles);
}

void VRPathtool::select(VRGeometryPtr h) {
    manip->handle(h);

    if (handles_dict.count(h) == 0) return;

    manip->manipulate(h);
}
