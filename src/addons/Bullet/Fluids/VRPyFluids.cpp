#include "VRPyFluids.h"
#include "core/scripting/VRPyGeometry.h"
#include "core/scripting/VRPyBaseT.h"

template<> PyTypeObject VRPyBaseT<OSG::VRFluids>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Fluids",             /*tp_name*/
    sizeof(VRPyFluids),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "VRFluid binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    VRPyFluids::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New_VRObjects_unnamed_ptr,                 /* tp_new */
};

PyMethodDef VRPyFluids::methods[] = {
    {"getGeometry", (PyCFunction)VRPyFluids::getGeometry, METH_VARARGS, "Get geometry - Geometry getGeometry()" },
    {"spawnCuboid", (PyCFunction)VRPyFluids::spawnCuboid, METH_VARARGS, "spawnCuboid(x,y,z) \n\tspawnCuboid(x,y,z,distance,a,b,c) //all float \n\tdistance is space between particles"},
    {"spawnEmitter", (PyCFunction)VRPyFluids::spawnEmitter, METH_VARARGS, "int ID spawnEmitter(x,y,z) \n\tint id = spawnEmitter(x,y,z,distance,a,b,c) //all float \n\tdistance is space between particles"},
    {"stopEmitter", (PyCFunction)VRPyFluids::stopEmitter, METH_VARARGS, "stopEmitter(int id)"},

    {"setRadius", (PyCFunction)VRPyFluids::setRadius, METH_VARARGS, "setRadius(float radius, float variation) \n\tsetRadius(0.05, 0.02)"},
    {"setSimType", (PyCFunction)VRPyFluids::setSimType, METH_VARARGS, "setSimType(string type, bool forceUpdate=False) \n\tsetSimType('SPH', True) #or XSPH"},
    {"setSphRadius", (PyCFunction)VRPyFluids::setSphRadius, METH_VARARGS, "setSphRadius(float radius)"},
    {"setMass", (PyCFunction)VRPyFluids::setMass, METH_VARARGS, "setMass(float mass, float variation)"},
    {"setMassByRadius", (PyCFunction)VRPyFluids::setMassByRadius, METH_VARARGS, "setMassByRadius(float massOfOneMeterRadius) \n\tsetMass(1000.0*100)"},
    {"setMassForOneLiter", (PyCFunction)VRPyFluids::setMassForOneLiter, METH_VARARGS, "setMassForOneLiter(float massOfOneLiter) \n\tsetMass(1000.0)"},
    {"setViscosity", (PyCFunction)VRPyFluids::setViscosity, METH_VARARGS, "setViscosity(float factor) \n\tsetViscosity(0.01)"},
    {NULL}  /* Sentinel */
};

void checkObj(VRPyFluids* self) {
    if (self->objPtr == 0) self->objPtr = OSG::VRFluids::create();
}

PyObject* VRPyFluids::getGeometry(VRPyFluids* self) {
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::spawnCuboid(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    OSG::Vec3f position;
    OSG::Vec3f size;
    float x,y,z, distance=0, a=1,b=1,c=1;

    if (! PyArg_ParseTuple(args, "fff|ffff", &x,&y,&z, &distance, &a,&b,&c)) {
        // ERROR!
        Py_RETURN_FALSE;
    }
    position.setValues(x, y, z);
    size.setValues(a, b, c);

    int num = self->objPtr->spawnCuboid(position, size, distance);
    return PyInt_FromLong((long) num);
}

PyObject* VRPyFluids::spawnEmitter(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    PyObject *base, *dir;
    int from=0, to=0, interval=0;
    if (! PyArg_ParseTuple(args, "OOiii", &base, &dir, &from, &to, &interval)) { Py_RETURN_FALSE; }
    // NOTE loop is not implemented yet, so set it to false
    int id = self->objPtr->setEmitter(parseVec3fList(base), parseVec3fList(dir), from, to, interval, false);
    return PyInt_FromLong((long) id);
}

PyObject* VRPyFluids::stopEmitter(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    int id = 0;
    if (! PyArg_ParseTuple(args, "i", &id) ) { Py_RETURN_FALSE; }
    self->objPtr->disableEmitter(id);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setAmount(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    int amount = 0;
    if (! PyArg_ParseTuple(args, "i", &amount)) { Py_RETURN_FALSE; }
    self->objPtr->resetParticles<OSG::SphParticle>(amount);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setRadius(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    float radius, variation;
    radius = variation = 0.0;
    if (! PyArg_ParseTuple(args, "f|f", &radius, &variation)) { Py_RETURN_FALSE; }
    self->objPtr->setRadius(radius, variation);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setSimType(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    OSG::VRFluids::SimulationType simType = OSG::VRFluids::XSPH;
    char* sim=NULL; int length=0;
    unsigned char update = 0; bool force = false;

    if (! PyArg_ParseTuple(args, "s#|b", &sim, &length, &update)) { Py_RETURN_FALSE; }
    if (strncmp(sim, "SPH", length)==0) simType = OSG::VRFluids::SPH;
    if (update > 0) force = true;
    self->objPtr->setSimulation(simType, force);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setSphRadius(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    float radius;
    radius = 0.0;
    if (! PyArg_ParseTuple(args, "f", &radius)) { Py_RETURN_FALSE; }
    self->objPtr->setSphRadius(radius);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setMass(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    float mass, variation;
    mass = variation = 0.0;
    if (! PyArg_ParseTuple(args, "f|f", &mass, &variation)) { Py_RETURN_FALSE; }
    self->objPtr->setMass(mass, variation);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setMassByRadius(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    float massFor1mRadius = 0.0;
    if (! PyArg_ParseTuple(args, "f", &massFor1mRadius)) { Py_RETURN_FALSE; }
    self->objPtr->setMassByRadius(massFor1mRadius);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setMassForOneLiter(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    float massPerLiter = 0.0;
    if (! PyArg_ParseTuple(args, "f", &massPerLiter)) { Py_RETURN_FALSE; }
    self->objPtr->setMassForOneLiter(massPerLiter);
    Py_RETURN_TRUE;
}

PyObject* VRPyFluids::setViscosity(VRPyFluids* self, PyObject* args) {
    checkObj(self);
    float factor;
    factor = 0.01;
    if (! PyArg_ParseTuple(args, "f", &factor)) { Py_RETURN_FALSE; }
    self->objPtr->setViscosity(factor);
    Py_RETURN_TRUE;
}
