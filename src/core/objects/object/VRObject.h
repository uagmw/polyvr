#ifndef VROBJECT_H_INCLUDED
#define VROBJECT_H_INCLUDED

#include <string>
#include <vector>
#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGVector.h>

#include "core/utils/VRName.h"

namespace xmlpp{ class Element; }
class VRAttachment;

OSG_BEGIN_NAMESPACE;
using namespace std;

class Node; OSG_GEN_CONTAINERPTR(Node);

/**

VRObjects are the base object from wich every other object type wich appears in the scenegraph inherits.
Here all the functions regarding the OSG scenegraph structure are wrapped.

*/

class VRGlobals {
    private:
        VRGlobals();

    public:
        unsigned int CURRENT_FRAME = 0;
        unsigned int FRAME_RATE = 0;

        static VRGlobals* get();
};

class VRObject : public VRName {
    private:
        bool specialized;
        VRObject* parent;
        NodeRecPtr node;
        int ID;
        int childIndex; // index of this object in its parent child vector
        bool pickable;
        bool visible;
        unsigned int graphChanged; //is frame number

        map<string, VRAttachment*> attachments;

        void _switchParent(NodeRecPtr parent);

        int findChild(VRObject* node);

        static void unitTest();

    protected:
        vector<VRObject*> children;
        string type;

        virtual void printInformation();
        //void printInformation();

        virtual VRObject* copy(vector<VRObject*> children);
        //VRObject* copy();

        virtual void saveContent(xmlpp::Element* e);
        virtual void loadContent(xmlpp::Element* e);

    public:

        /** initialise an object with his name **/
        VRObject(string _name = "0");
        virtual ~VRObject();

        /** Returns the Object ID **/
        int getID();

        /** Returns the object type **/
        string getType();

        VRObject* getRoot();
        string getPath();
        VRObject* getAtPath(string path);

        bool hasGraphChanged();

        template<typename T> void addAttachment(string name, T t);
        template<typename T> T getAttachment(string name);
        bool hasAttachment(string name);
        VRObject* hasAncestorWithAttachment(string name);

        /** Set the object OSG core and specify the type**/
        void setCore(NodeCoreRecPtr c, string _type);

        /** Returns the object OSG core **/
        NodeCoreRecPtr getCore();

        /** Switch the object core by another **/
        void switchCore(NodeCoreRecPtr c);

        /** Returns the object OSG node **/
        NodeRecPtr getNode();

        /** set the position in the parents child list **/
        void setSiblingPosition(int i);

        /** Add a child to this object **/
        void addChild(VRObject* child, bool osg = true);

        /** Add a OSG node as child to this object **/
        void addChild(NodeRecPtr n);

        /** Remove a child **/
        void subChild(VRObject* child, bool osg = true);

        /** Remove a OSG node child **/
        void subChild(NodeRecPtr n);

        /** Switch the parent of this object **/
        void switchParent(VRObject* new_p);

        /** Detach object from the parent**/
        void detach();

        /** Returns the child by his position **/
        VRObject* getChild(int i);

        /** Returns the parent of this object **/
        VRObject* getParent();

        /** Returns the number of children **/
        size_t getChildrenCount();

        void clearChildren();

        /** Returns all objects with a certain type wich are below this object in hirarchy **/
        vector<VRObject*> getObjectListByType( string _type );
        void getObjectListByType( string _type, vector<VRObject*>& list );

        /**
            To find an object in the scene graph was never easier, just pass an OSG node, object, ID or name to a VRObject.
            This Object will search all the hirachy below him (himself included).
        **/

        VRObject* find(NodeRecPtr n, string indent = " ");
        VRObject* find(VRObject* obj);
        VRObject* find(string Name);
        VRObject* find(int id);

        vector<VRObject*> filterByType(string Type, vector<VRObject*> res = vector<VRObject*>() );

        /** Returns the first ancestor that is pickable, or 0 if none found **/
        VRObject* findPickableAncestor();

        bool hasAncestor(VRObject* a);

        /** Returns the Boundingbox of the OSG Node */
        void getBoundingBox(Vec3f& v1, Vec3f& v2);

        /** Print to console the scene subgraph starting at this object **/
        void printTree(int indent = 0);

        static void printOSGTree(NodeRecPtr o, string indent = "");

        /** duplicate this object **/
        VRObject* duplicate(bool anchor = false);

        /** Hide this object and all his subgraph **/
        void hide();

        /** Show this object and all his subgraph **/
        void show();

        /** Returns if this object is visible or not **/
        bool isVisible();

        /** Set the visibility of this object **/
        void setVisible(bool b);

        /** toggle visibility **/
        void toggleVisible();

        /** Returns if this object is pickable or not **/
        bool isPickable();

        /** Set the object pickable or not **/
        void setPickable(bool b);

        void destroy();

        void save(xmlpp::Element* e);
        void load(xmlpp::Element* e);
};

OSG_END_NAMESPACE;

#endif // VROBJECT_H_INCLUDED
